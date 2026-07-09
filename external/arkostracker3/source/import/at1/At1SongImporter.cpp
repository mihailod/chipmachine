#include "At1SongImporter.h"

#include "../../business/song/tool/builder/TrailingEffectContext.h"
#include "../../business/song/tool/optimizers/SongOptimizer.h"
#include "../../song/cells/CellConstants.h"
#include "../../song/subsong/SubsongConstants.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../../utils/StreamUtil.h"
#include "../../utils/XmlHelper.h"

namespace arkostracker 
{

At1SongImporter::At1SongImporter() noexcept:
        songBuilder()
{
}

// SongImporter method implementations.
// =======================================

bool At1SongImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Simply looks for the tag "<Version>Arkos Tracker 1.0</Version>" at the beginning. No need to go further!
    // However, on some old AT1 song, another tag is used, we have to check them both.
    constexpr auto sizeToRead = 256;
    return StreamUtil::findStringInStream(inputStream, sizeToRead, "<Version>Arkos Tracker 1.0</Version>", true) ||
            StreamUtil::findStringInStream(inputStream, sizeToRead, "<Version>ArkosTrackerSong 1.0a</Version>", true);
}

ImportedFormat At1SongImporter::getFormat() noexcept
{
    return ImportedFormat::at1;
}

std::unique_ptr<SongImporter::Result> At1SongImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*configuration*/) noexcept
{
    songBuilder = std::make_unique<SongBuilder>();

    const auto documentText = inputStream.readEntireStreamAsString();
    const auto songNode = juce::parseXML(documentText);
    if (songNode != nullptr) {
        parse(*songNode);
    } else {
        jassertfalse;           // Abnormal, this method is called only when the format matches!
    }

    // Gets the result from the Builder and returns it.
    auto songAndReport = songBuilder->buildSong();

    return std::make_unique<Result>(std::move(songAndReport.first), std::move(songAndReport.second));
}

void At1SongImporter::parse(const juce::XmlElement& songNode) const noexcept
{
    // Extracts the metadata of the Song.
    const auto title = XmlHelper::readString(songNode, "Name");
    const auto author = XmlHelper::readString(songNode, "Author");
    const auto comments = XmlHelper::readString(songNode, "Comments");

    songBuilder->setSongMetaData(title, author, author, comments);

    const auto loopStartIndex = XmlHelper::readInt(songNode, "LoopStart");
    const auto endIndex = XmlHelper::readInt(songNode, "LoopEnd");
    const auto replayFrequencyHz = extractReplayFrequency(songNode);
    const auto psgFrequencyHz = XmlHelper::readInt(songNode, "MasterFrequency", PsgFrequency::psgFrequencyCPC);
    const auto digiChannel = XmlHelper::readInt(songNode, "SamplesChannel");
    const auto initialSpeed = XmlHelper::readInt(songNode, "BeginningSpeed", 6);
    const auto primaryHighlight = XmlHelper::readInt(songNode, "Highlight", 8);

    auto& subsongBuilder = songBuilder->startNewSubsong(0);
    subsongBuilder.setLoop(loopStartIndex, endIndex);
    subsongBuilder.addPsg(Psg(PsgType::ay, psgFrequencyHz, PsgFrequency::defaultReferenceFrequencyHz, PsgFrequency::defaultSamplePlayerFrequencyHz,
                              PsgMixingOutput::ABC));
    subsongBuilder.setMetadata(SubsongConstants::defaultFirstSubsongName, initialSpeed, static_cast<float>(replayFrequencyHz), digiChannel, primaryHighlight);

    // Parses the Tracks.
    const auto* tracksNode = songNode.getChildByName("TrackArray");
    parseTracks(tracksNode, subsongBuilder);
    // Parses the SpecialTracks.
    const auto* specialTracksNode = songNode.getChildByName("SpecialTracksArray");
    parseSpecialTracks(specialTracksNode, subsongBuilder);

    // Parses the Patterns.
    const auto* patternsNode = songNode.getChildByName("PatternsList");
    parsePatterns(patternsNode, subsongBuilder);

    // Parses the Instruments.
    const auto* instrumentsNode = songNode.getChildByName("InstrumentsList");
    parseInstruments(instrumentsNode);

    songBuilder->finishCurrentSubsong();
}

int At1SongImporter::extractReplayFrequency(const juce::XmlElement& songNode) const noexcept
{
    auto foundFrequency = 0;

    // Extracts the frequency from "freqXXhz".
    static const juce::String freqString = "freq";
    const auto text = XmlHelper::readString(songNode, "Frequency");
    if (text.startsWith(freqString)) {
        const auto index = text.indexOf("hz");

        if (index >= (freqString.length() + 1)) {
            const auto valueString = text.substring(freqString.length(), index);
            foundFrequency = valueString.getIntValue();
        }
    }

    if (foundFrequency <= 0) {
        songBuilder->addWarning("The replay frequency could not be parsed from: " + text);
        foundFrequency = static_cast<int>(PsgFrequency::defaultReplayFrequencyHz);
    }

    return foundFrequency;
}

void At1SongImporter::parseTracks(const juce::XmlElement* tracksNode, SubsongBuilder& subsongBuilder) const
{
    if (tracksNode == nullptr) {
        songBuilder->addError("There are no tracks in this song, abnormal.");
        return;
    }

    const auto tracksWrapper = XmlHelper::getChildrenList(tracksNode, "Track");
    auto trackIndex = 0;
    for (const auto* trackWrapper : tracksWrapper) {
        if (trackWrapper->getNumChildElements() == 0) {
            ++trackIndex;
            continue;               // Skips empty tracks (there are many).
        }

        subsongBuilder.startNewTrack(trackIndex);

        TrailingEffectContext context;

        const auto* track = trackWrapper->getChildByName("track");
        const auto cells = XmlHelper::getChildrenList(track, "Cell");

        auto cellIndex = 0;
        for (const auto* cell : cells) {
            auto effectIndex = 0;
            const auto readNote = XmlHelper::readInt(*cell, "Note", at1EmptyNote);
            const auto readInstrument = XmlHelper::readInt(*cell, "Instrument");
            const auto readVolume = XmlHelper::readInt(*cell, "Volume");
            const auto readPitch = XmlHelper::readInt(*cell, "Pitch");

            subsongBuilder.startNewCell(cellIndex);

            if (readNote != at1EmptyNote) {
                context.declareNote();
                // Rst?
                if (readNote == at1RstNote) {
                    subsongBuilder.setCurrentCellNote(CellConstants::rstNote);
                    subsongBuilder.setCurrentCellInstrument(CellConstants::rstInstrument);
                } else {
                    subsongBuilder.setCurrentCellNote(readNote);
                    subsongBuilder.setCurrentCellInstrument(readInstrument);
                }
            }
            if (readVolume != at1EmptyVolume) {
                const auto[effect, value] = context.declareEffect(TrailingEffect::volume, readVolume);
                if (effect == TrailingEffect::volume) {
                    subsongBuilder.setCurrentCellEffectRawValue(effectIndex, Effect::volume, static_cast<int>(static_cast<unsigned int>(value) << 8U));
                    ++effectIndex;
                }
            }
            if (readPitch != at1EmptyPitch) {
                const auto trailingEffect = (readPitch > 0) ? TrailingEffect::pitchUp : TrailingEffect::pitchDown;

                const auto newPitch = std::abs(readPitch) * 0x20;    // The value must remain positive. Arbitrary pitch multiplier.
                const auto[effect, value] = context.declareEffect(trailingEffect, newPitch);
                if (effect == TrailingEffect::noEffect) {
                    // Nothing to do, normal.
                } else if (effect == TrailingEffect::pitchUp) {
                    subsongBuilder.setCurrentCellEffectRawValue(effectIndex, Effect::pitchUp, value);
                    ++effectIndex;
                } else if (effect == TrailingEffect::pitchDown) {
                    subsongBuilder.setCurrentCellEffectRawValue(effectIndex, Effect::pitchDown, value);
                    ++effectIndex;
                } else {
                    jassertfalse;       // Effect not handled?
                }
            }

            subsongBuilder.finishCurrentCell();
            ++cellIndex;
        }

        subsongBuilder.finishCurrentTrack();

        ++trackIndex;
    }
}

void At1SongImporter::parseSpecialTracks(const juce::XmlElement* specialTracksNode, SubsongBuilder& subsongBuilder) const
{
    if (specialTracksNode == nullptr) {
        songBuilder->addError("There are no special tracks in this song, abnormal.");
        return;
    }

    // Reads one special Track, but generates two (speed/event) in parallel.
    // To manage this easily, use the special short-cut method.
    const auto specialTracksWrapper = XmlHelper::getChildrenList(specialTracksNode, "SpecialTrack");
    auto specialTrackIndex = 0;
    for (const auto* specialTrackWrapper : specialTracksWrapper) {
        if (specialTrackWrapper->getNumChildElements() == 0) {
            ++specialTrackIndex;
            continue;               // Skips empty special tracks (there are many).
        }

        const auto* specialCellsWrapper = specialTrackWrapper->getChildByName("SpecialCells");
        const auto specialCells = XmlHelper::getChildrenList(specialCellsWrapper, "SpecialCell");

        auto specialCellIndex = 0;
        for (const auto* specialCell : specialCells) {
            const auto readEffect = XmlHelper::readInt(*specialCell, "Effect", at1SpecialCellNoEffect);
            const auto readValue = XmlHelper::readInt(*specialCell, "Value", 0);

            if (readValue > 0) {
                if (readEffect == at1SpecialCellNoEffect) {
                    // Nothing to do.
                } else if (readEffect == at1SpecialCellEffectSpeed) {
                    subsongBuilder.setSpecialCellOnTrack(true, specialTrackIndex, specialCellIndex, SpecialCell::buildSpecialCell(readValue));
                } else if (readEffect == at1SpecialCellEffectEvent) {
                    subsongBuilder.setSpecialCellOnTrack(false, specialTrackIndex, specialCellIndex, SpecialCell::buildSpecialCell(readValue));
                } else {
                    jassertfalse;   // Unhandled effect?
                }
            }

            ++specialCellIndex;
        }

        ++specialTrackIndex;
    }
}

void At1SongImporter::parsePatterns(const juce::XmlElement* patternsNode, SubsongBuilder& subsongBuilder) const
{
    if (patternsNode == nullptr) {
        songBuilder->addError("There are no patterns in this song, abnormal.");
        return;
    }

    // Creates as many Positions as they are Patterns. We will optimize them later (then unoptimize :)).
    const auto patterns = XmlHelper::getChildrenList(patternsNode, "Pattern");
    for (const auto* pattern : patterns) {
        const auto channel0TrackIndex = XmlHelper::readInt(*pattern, "Track1Number");
        const auto channel1TrackIndex = XmlHelper::readInt(*pattern, "Track2Number");
        const auto channel2TrackIndex = XmlHelper::readInt(*pattern, "Track3Number");
        const auto channel0Transposition = XmlHelper::readInt(*pattern, "Transposition1");
        const auto channel1Transposition = XmlHelper::readInt(*pattern, "Transposition2");
        const auto channel2Transposition = XmlHelper::readInt(*pattern, "Transposition3");
        const auto height = XmlHelper::readInt(*pattern, "Height");
        const auto specialTrackIndex = XmlHelper::readInt(*pattern, "SpecialTrackNumber");

        // Builds the Pattern. The Tracks will be "unoptimized" (no duplicates) by the SongBuilder itself,
        // but the duplicates if patterns must be optimized by this class. The SongBuilder has fortunately a helper...
        const Pattern fakePattern({ channel0TrackIndex, channel1TrackIndex, channel2TrackIndex }, specialTrackIndex, specialTrackIndex);
        const auto patternResult = subsongBuilder.checkRawOriginalPatternAndStoreIfDifferent(fakePattern);
        const auto patternIndex = patternResult.second;
        if (patternResult.first) {
            // Encodes the Pattern.
            subsongBuilder.startNewPattern(patternIndex);
            subsongBuilder.setCurrentPatternTrackIndex(0, channel0TrackIndex);
            subsongBuilder.setCurrentPatternTrackIndex(1, channel1TrackIndex);
            subsongBuilder.setCurrentPatternTrackIndex(2, channel2TrackIndex);
            subsongBuilder.setCurrentPatternSpeedTrackIndex(specialTrackIndex);
            subsongBuilder.setCurrentPatternEventTrackIndex(specialTrackIndex);
            subsongBuilder.finishCurrentPattern();
        }

        // Creates the Position too.
        subsongBuilder.startNewPosition();
        subsongBuilder.setCurrentPositionHeight(height);
        subsongBuilder.setCurrentPositionPatternIndex(patternIndex);
        subsongBuilder.setCurrentPositionTransposition(0, channel0Transposition);
        subsongBuilder.setCurrentPositionTransposition(1, channel1Transposition);
        subsongBuilder.setCurrentPositionTransposition(2, channel2Transposition);
        subsongBuilder.finishCurrentPosition();
    }
}

void At1SongImporter::parseInstruments(const juce::XmlElement* instrumentsNode) const
{
    if (instrumentsNode == nullptr) {
        songBuilder->addError("There are no instruments in this song, abnormal.");
        return;
    }

    const auto instruments = XmlHelper::getChildrenList(instrumentsNode, "Instrument");
    auto instrumentIndex = 0;
    for (const auto* instrument : instruments) {
        // Instrument 0 must not be encoded.
        if (instrumentIndex == 0) {
            ++instrumentIndex;
            continue;
        }

        songBuilder->startNewPsgInstrument(instrumentIndex);

        const auto name = XmlHelper::readString(*instrument, "Name");
        const auto speed = XmlHelper::readInt(*instrument, "Speed");
        const auto isLooping = XmlHelper::readBool(*instrument, "Loop");
        const auto loopStartIndex = XmlHelper::readInt(*instrument, "LoopStart");
        const auto endIndex = XmlHelper::readInt(*instrument, "LoopEnd");
        const auto instrumentRetrig = XmlHelper::readBool(*instrument, "Retrig");

        songBuilder->setCurrentInstrumentName(name);
        songBuilder->setCurrentPsgInstrumentSpeed(speed);
        songBuilder->setCurrentPsgInstrumentRetrig(instrumentRetrig);
        songBuilder->setCurrentPsgInstrumentMainLoop(loopStartIndex, endIndex, isLooping);

        // Parses each Instrument cell. Keeps only the used ones.
        const auto* instrumentItems = instrument->getChildByName("InstrumentItems");
        jassert(instrumentItems != nullptr);        // Abnormal.
        if (instrumentItems != nullptr) {
            auto cellIndex = 0;
            for (const auto* cell : XmlHelper::getChildrenList(*instrumentItems, "InstrumentItem")) {
                if (cell == nullptr) {
                    jassertfalse;   // Should never happen.
                } else if (cellIndex <= endIndex) {
                    declareInstrumentCell(*cell);
                }
                ++cellIndex;
            }
        }

        songBuilder->finishCurrentInstrument();

        ++instrumentIndex;
    }
}

void At1SongImporter::declareInstrumentCell(const juce::XmlElement& cell) const
{
    songBuilder->startNewPsgInstrumentCell();

    const auto isRetrig = XmlHelper::readBool(cell, "IsRetrig");
    const auto isSound = XmlHelper::readBool(cell, "IsSound");
    const auto isHard = XmlHelper::readBool(cell, "IsHard");
    const auto linkValue = XmlHelper::readInt(cell, "LinkValue");
    const auto volume = XmlHelper::readInt(cell, "Volume");
    const auto noise = XmlHelper::readInt(cell, "Noise");
    //const auto pitch = XmlHelper::readInt(cell, "Pitch");   // ???
    const auto readHardwareEnvelope = XmlHelper::readInt(cell, "HardwareEnvelope");
    const auto shift = XmlHelper::readInt(cell, "Shift");
    const auto softwareFrequency = XmlHelper::readInt(cell, "SoundFrequency");
    const auto softwareArpeggio = XmlHelper::readInt(cell, "SoundArpeggio");
    const auto softwarePitch = XmlHelper::readInt(cell, "SoundPitch");
    const auto hardwareFrequency = XmlHelper::readInt(cell, "HardwareFrequency");
    const auto hardwareArpeggio = XmlHelper::readInt(cell, "HardwareArpeggio");
    const auto hardwarePitch = XmlHelper::readInt(cell, "HardwarePitch");

    PsgInstrumentCellLink link;    // NOLINT(*-init-variables)

    if (!isHard) {
        // Not a hardware sound.
        link = isSound ? PsgInstrumentCellLink::softOnly : PsgInstrumentCellLink::noSoftNoHard;
    } else {
        // Hardware sound.
        if (linkValue == at1LinkStateHardwareDependentToSoftware) {
            // Is there sound?
            link = isSound ? PsgInstrumentCellLink::softToHard : PsgInstrumentCellLink::softOnly;
        } else if (linkValue == at1LinkStateSoftwareDependentToHardware) {
            // Is there sound?
            link = isSound ? PsgInstrumentCellLink::hardToSoft : PsgInstrumentCellLink::hardOnly;
        } else {
            // Independent. Is there sound?
            link = isSound ? PsgInstrumentCellLink::softAndHard : PsgInstrumentCellLink::hardOnly;
        }
    }

    songBuilder->setCurrentPsgInstrumentCellNoise(noise);
    songBuilder->setCurrentPsgInstrumentCellLink(link);
    songBuilder->setCurrentPsgInstrumentCellVolume(volume);
    songBuilder->setCurrentPsgInstrumentCellRetrig(isRetrig);
    songBuilder->setCurrentPsgInstrumentCellRatio(shift);
    songBuilder->setCurrentPsgInstrumentCellHardwareEnvelope(NumberUtil::correctNumber(readHardwareEnvelope,
                                                                                       PsgValues::minimumHardwareEnvelope, PsgValues::maximumHardwareEnvelope));

    switch (link) {
        case PsgInstrumentCellLink::noSoftNoHard:
            [[fallthrough]];
        case PsgInstrumentCellLink::softOnly:
            [[fallthrough]];
        case PsgInstrumentCellLink::softToHard:
            [[fallthrough]];
        case PsgInstrumentCellLink::softAndHard:
            songBuilder->setCurrentPsgInstrumentCellPrimaryPeriod(softwareFrequency);
            songBuilder->setCurrentPsgInstrumentCellPrimaryArpeggio(softwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellPrimaryPitch(softwarePitch);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPeriod(hardwareFrequency);
            songBuilder->setCurrentPsgInstrumentCellSecondaryArpeggio(hardwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPitch(hardwarePitch);
            break;
        case PsgInstrumentCellLink::hardOnly:
            [[fallthrough]];
        case PsgInstrumentCellLink::hardToSoft:
            songBuilder->setCurrentPsgInstrumentCellPrimaryPeriod(hardwareFrequency);
            songBuilder->setCurrentPsgInstrumentCellPrimaryArpeggio(hardwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellPrimaryPitch(hardwarePitch);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPeriod(softwareFrequency);
            songBuilder->setCurrentPsgInstrumentCellSecondaryArpeggio(softwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPitch(softwarePitch);
            break;
    }

    songBuilder->finishCurrentPsgInstrumentCell();
}

}   // namespace arkostracker
