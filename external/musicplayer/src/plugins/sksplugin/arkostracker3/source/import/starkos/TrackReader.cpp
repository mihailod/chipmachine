#include "TrackReader.h"

#include "../../business/song/tool/builder/SubsongBuilder.h"
#include "../../business/song/tool/builder/TrailingEffectContext.h"
#include "../../utils/MemoryBlockUtil.h"

namespace arkostracker 
{

void TrackReader::readTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& offset) noexcept
{
    bool readSuccess; // NOLINT(*-init-variables)
    auto trackIndex = MemoryBlockUtil::extractUnsignedWord(musicData, offset, readSuccess);
    offset += 2;

    while (trackIndex < 0xffff) {           // 0xffff marks the end of the Track section. Only non-empty Tracks are encoded.
        TrailingEffectContext context;

        auto cellIndex = 0;
        auto isTrackOver = false;

        // Declares the Track.
        subsongBuilder.startNewTrack(trackIndex);

        // Skips the size.
        offset += 2;

        auto currentInstrument = 0;

        // Browses the Track data.
        while (!isTrackOver && (cellIndex < trackMaximumHeight)) {
            const auto readByte = MemoryBlockUtil::extractUnsignedChar(musicData, offset++, readSuccess);
            // Bit 7 to 1: wait for (b6-b0) lines. #FF means end of Track.
            if (readByte == 0xff) {
                isTrackOver = true;
            } else if ((readByte & 0x80) != 0) {
                // Empty line(s).
                // The end of a Pitch may be needed to be encoded.
                const auto effectResult = context.declareEffect(TrailingEffect::noEffect, 0);
                if ((effectResult.first == TrailingEffect::pitchUp) || (effectResult.first == TrailingEffect::pitchDown)) {
                    subsongBuilder.startNewCell(cellIndex);
                    addStopPitchToCurrentCell(subsongBuilder, (effectResult.first == TrailingEffect::pitchUp));
                    subsongBuilder.finishCurrentCell();
                }
                cellIndex += ((readByte & 0x7f) + 1);
            } else {
                // Notes and effects.
                auto mustReadInstrument = false;
                auto mustReadVolume = false;
                auto mustReadPitch = false;
                auto mustReadNote = false;
                auto mustReadFlags = true;

                subsongBuilder.startNewCell(cellIndex);

                switch (readByte) {
                    case sksReset:
                        subsongBuilder.setCurrentCellRst();
                        break;
                    case sksDigidrum:
                        // Ignored.
                        ++offset;       // Skips the drum number byte.
                        break;
                    case sksVolumeOnly:
                        mustReadVolume = true;
                        break;
                    case sksPitchOnly:
                        mustReadPitch = true;
                        mustReadFlags = false;
                        break;
                    case sksVolumeAndPitchOnly:
                        mustReadVolume = true;
                        mustReadPitch = true;
                        break;
                    default:
                        mustReadNote = true;
                        mustReadInstrument = true;
                        break;
                }

                // What to read/encode?
                if (mustReadNote || mustReadPitch || /*mustReadInstrument ||*/ mustReadVolume) {
                    // Note?
                    if (mustReadNote) {
                        context.declareNote();
                        subsongBuilder.setCurrentCellNote(readByte);
                    }

                    // Reads the flags?
                    unsigned char flagsByte = 0;
                    if (mustReadFlags) {
                        flagsByte = MemoryBlockUtil::extractUnsignedChar(musicData, offset++, readSuccess);
                        mustReadVolume |= ((flagsByte & 0x40) == 0);
                        mustReadPitch |= ((flagsByte & 0x10) != 0);
                    }

                    if (mustReadVolume) {
                        // Bit 6 to 0: inverted volume. Adds a volume effect, if different from a possible previous one.
                        const auto volume = (0xf - (flagsByte & 0xf));
                        auto effectAndValue = context.declareEffect(TrailingEffect::volume, volume);
                        if (effectAndValue.first == TrailingEffect::volume) {
                            subsongBuilder.addCurrentCellEffect(Effect::volume, effectAndValue.second);
                        }
                    }

                    if (mustReadInstrument) {
                        // Same instrument or new one?
                        if ((flagsByte & 0x20) == 0) {          // Bit 5 to 0: new instrument.
                            currentInstrument = MemoryBlockUtil::extractUnsignedChar(musicData, offset++, readSuccess);
                        }
                        subsongBuilder.setCurrentCellInstrument(currentInstrument);
                    }

                    // Pitch? Signed inverted byte.
                    if (mustReadPitch) {                        // Bit 4 to 1: pitch present.
                        auto pitchByte = static_cast<signed char>(MemoryBlockUtil::extractUnsignedChar(musicData, offset++, readSuccess));
                        auto pitch = pitchByte * 8;             // Adds a ratio to adapt the pitch from SKS to AT3.
                        Effect effect;  // NOLINT(*-init-variables)
                        // Keeps only a positive Pitch. Inverts the effect though, as the SKS pitch is inverted!
                        // Note that a Fast pitch is used. A "slow" could be used, but then we would have to manage a possible
                        // overflow, which would require the need of a fastPitch anyway. So...
                        TrailingEffect trailingEffect;  // NOLINT(*-init-variables)
                        if (pitch >= 0) {
                            effect = Effect::fastPitchDown;
                            trailingEffect = TrailingEffect::pitchDown;
                        } else {
                            effect = Effect::fastPitchUp;
                            trailingEffect = TrailingEffect::pitchUp;
                            pitch = -pitch;
                        }
                        const auto value = (pitch & 0xfff);

                        // Encodes it?
                        auto effectResult = context.declareEffect(trailingEffect, value);
                        if ((effectResult.first == TrailingEffect::pitchUp) || (effectResult.first == TrailingEffect::pitchDown)) {
                            subsongBuilder.addCurrentCellEffect(effect, effectResult.second);
                        }
                    } else if (!mustReadNote && !mustReadInstrument) {
                        // No pitch, no note, no instrument. A possible must be stopped?
                        auto effectResult = context.declareEffect(TrailingEffect::noEffect, 0);
                        if ((effectResult.first == TrailingEffect::pitchUp) || (effectResult.first == TrailingEffect::pitchDown)) {
                            addStopPitchToCurrentCell(subsongBuilder, (effectResult.first == TrailingEffect::pitchUp));
                        }
                    }
                }

                subsongBuilder.finishCurrentCell();

                ++cellIndex;
            }
        }

        subsongBuilder.finishCurrentTrack();

        // Reads the next Track.
        trackIndex = MemoryBlockUtil::extractUnsignedWord(musicData, offset, readSuccess);
        offset += 2;
    }
}

void TrackReader::addStopPitchToCurrentCell(SubsongBuilder& subsongBuilder, bool up) noexcept
{
    subsongBuilder.addCurrentCellEffect(up ? Effect::fastPitchUp : Effect::fastPitchDown, 0);
}

void TrackReader::readSpecialTracks(SubsongBuilder& subsongBuilder, const juce::MemoryBlock& musicData, juce::int64& offset) noexcept
{
    bool success;  // NOLINT(*-init-variables)
    auto specialTrackIndex = MemoryBlockUtil::extractUnsignedWord(musicData, offset, success);
    offset += 2;

    while (specialTrackIndex < 0xffff) {            //0xffff marks the end of the Special Tracks section.

        // Builds BOTH an Event and Speed Tracks. Not optimized, but who cares?
        subsongBuilder.startNewSpeedTrack(specialTrackIndex);
        subsongBuilder.startNewEventTrack(specialTrackIndex);

        auto currentLine = 0;
        auto isSpecialTrackOver = false;

        // Skips the size (one byte), useless.
        ++offset;

        while (!isSpecialTrackOver && (currentLine < trackMaximumHeight)) {
            const auto readByte = MemoryBlockUtil::extractUnsignedChar(musicData, offset++, success);
            if (readByte == 0xff) {         // Bit 7 to 1 = wait for (b6-b0) lines. #ff means end of track.
                isSpecialTrackOver = true;
            } else if ((readByte & 0x80) != 0) {
                // Skips lines.
                currentLine += ((readByte & 0x7f) + 1);
            } else {
                // Bit 7 to 0: bit 6 = effect (0=speed 1=digidrum)  (b4-b0 = Value).
                const auto value = (readByte & 0x1f);
                if ((readByte & 0x40) != 0) {
                    // Digidrum.
                    subsongBuilder.setCurrentEventTrackValue(currentLine, value);
                } else {
                    // Speed.
                    subsongBuilder.setCurrentSpeedTrackValue(currentLine, value);
                }
                ++currentLine;
            }
        }

        subsongBuilder.finishCurrentSpeedTrack();
        subsongBuilder.finishCurrentEventTrack();

        // Reads the next Special Track.
        specialTrackIndex = MemoryBlockUtil::extractUnsignedWord(musicData, offset, success);
        offset += 2;
    }
}

}   // namespace arkostracker
