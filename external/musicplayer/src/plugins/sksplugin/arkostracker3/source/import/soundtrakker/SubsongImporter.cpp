#include "SubsongImporter.h"

#include <unordered_set>

#include "../../business/song/tool/builder/SongBuilder.h"
#include "../../business/song/tool/builder/TrailingEffectContext.h"
#include "../../song/subsong/SubsongConstants.h"
#include "../../player/PsgPeriod.h"
#include "../../song/cells/CellConstants.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/NoteUtil.h"
#include "../../utils/NumberUtil.h"
#include "Stk128Constants.h"

namespace arkostracker 
{

SubsongImporter::SubsongImporter(SongBuilder& pSongBuilder, SubsongBuilder& pSubsongBuilder, const juce::MemoryBlock& pMusicData,
                                 std::unordered_set<int> pSoundtrakkerInstrumentWithSFlagIndexes) noexcept :
        songBuilder(pSongBuilder),
        builder(pSubsongBuilder),
        musicData(pMusicData),
        positionsHeight(),
        songEndIndex(),
        hasSpeedBeenFoundInPattern(),
        nextSpeedTrackIndex(1),        // The 0 is empty and created from the start.
        hardwareRatioToInstrumentIndexWithHard8(),
        hardwareRatioToInstrumentIndexWithHardA(),
        hardwarePeriodToInstrumentIndexWithHard8(),
        hardwarePeriodToInstrumentIndexWithHardA(),
        soundtrakkerInstrumentWithSFlagIndexes(std::move(pSoundtrakkerInstrumentWithSFlagIndexes))
{
}

void SubsongImporter::parse() noexcept
{
    // Sets the general data of the Subsong.
    builder.addPsg(Psg());

    // Reads the Subsong attributes (unique PatternHeight, song speed, length and loop).
    bool tempSuccess;    // NOLINT(*)
    positionsHeight = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(musicData, offsetPatternLength, tempSuccess));
    auto success = tempSuccess;
    songEndIndex = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(musicData, offsetSongLength, tempSuccess));
    success = success && tempSuccess;
    const auto songLoopStartIndex = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(musicData, offsetPatternLoop, success));
    success = success && tempSuccess;
    const auto initialSpeed = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(musicData, offsetInitialSpeed, success));
    success = success && tempSuccess;

    if (!success) {
        builder.addError("Error while reading the song metadata.");
        return;
    }

    builder.setMetadata(SubsongConstants::defaultFirstSubsongName, initialSpeed, PsgFrequency::defaultReplayFrequencyHz);
    builder.setLoop(songLoopStartIndex, songEndIndex);

    // Creates one empty Speed and Event Track.
    createEmptySpecialTracks();

    // Reads each Pattern.
    readAndCreatePatternsAndTracks();
}

void SubsongImporter::readAndCreatePatternsAndTracks() noexcept
{
    // A Set is created to hold the Pattern index we already found. This allows us to know if the Tracks must be created or not.
    std::unordered_set<int> treatedPatternIndexes;
    // Stores the height of the Patterns.
    std::unordered_map<int, int> patternIndexToPatternHeight;

    // Reads up the EndIndex, else there is no way to tell what is junk or unused patterns (else, would have to use the maximumPosition constant).
    auto success = true;
    for (auto position = 0; position <= songEndIndex; ++position) {
        const auto patternIndex = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(musicData, offsetPatternsList + position, success));
        if (!success) {
            builder.addError("Unable to read the pattern index.");
            return;
        }

        hasSpeedBeenFoundInPattern = false;

        // The Tracks of the Pattern don't exist in the Soundtrakker, so we consider them proportional according to the pattern index.
        const auto trackIndexChannel0 = patternIndex * channelCount + 0;
        const auto trackIndexChannel1 = patternIndex * channelCount + 1;
        const auto trackIndexChannel2 = patternIndex * channelCount + 2;
        // Extracts the transpositions.
        const auto trackTransposition = extractPatternTransposition(position);

        // Encodes the Tracks only if this Pattern is not known.
        auto positionHeight = positionsHeight;     // Default value.
        if (treatedPatternIndexes.find(patternIndex) == treatedPatternIndexes.end()) {
            // New Pattern. Encodes the Tracks. As they are read, a Special Track may be built.
            // Also, may modify the pattern height according to the 9 effect.
            success = success && readAndCreateTrack(patternIndex, 0, trackIndexChannel0, positionHeight);
            success = success && readAndCreateTrack(patternIndex, 1, trackIndexChannel1, positionHeight);
            success = success && readAndCreateTrack(patternIndex, 2, trackIndexChannel2, positionHeight);

            // Marks the PatternIndex as "treated".
            treatedPatternIndexes.insert(patternIndex);
            patternIndexToPatternHeight[patternIndex] = positionHeight;

            // Is there a new speed Track?
            const auto speedTrackIndex = hasSpeedBeenFoundInPattern ? nextSpeedTrackIndex : 0;

            // Creates the Pattern.
            builder.startNewPattern(patternIndex);
            builder.setCurrentPatternTrackIndex(0, trackIndexChannel0);
            builder.setCurrentPatternTrackIndex(1, trackIndexChannel1);
            builder.setCurrentPatternTrackIndex(2, trackIndexChannel2);
            builder.setCurrentPatternSpeedTrackIndex(speedTrackIndex);
            builder.setCurrentPatternEventTrackIndex(0);
            builder.finishCurrentPattern();

            // Closes the possible Speed Track.
            if (hasSpeedBeenFoundInPattern) {
                builder.finishCurrentSpeedTrack();
                ++nextSpeedTrackIndex;
            }
        } else {
            // The Pattern is not new. However, we still want to get its (maybe) specific height. Fortunately, we have saved it.
            positionHeight = patternIndexToPatternHeight.at(patternIndex);
        }

        // Creates the Position.
        builder.startNewPosition();
        builder.setCurrentPositionPatternIndex(patternIndex);

        // Now only are sure on the Pattern height.
        builder.setCurrentPositionHeight(positionHeight);
        // Sets the transpositions.
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            builder.setCurrentPositionTransposition(channelIndex, trackTransposition);
        }
        builder.finishCurrentPosition();

        if (!success) {
            builder.addError("Error while reading the song tracks.");
            return;
        }
    }
}

int SubsongImporter::extractPatternTransposition(const int position) const noexcept
{
    bool success;    // NOLINT(*)

    auto trackTransposition = MemoryBlockUtil::extractSignedByte(musicData, offsetPatternTranspositions + position, success);
    if (!success) {
        builder.addWarning("Unable to read a transposition for position " + juce::String(position));
    } else {
        // Corrects the transposition, if needed.
        trackTransposition = NumberUtil::correctNumber(trackTransposition, Position::minimumTransposition, Position::maximumTransposition);
    }

    return trackTransposition;
}

bool SubsongImporter::readAndCreateTrack(const int patternIndex, const int channelIndex, const int trackIndex, int& patternHeight) noexcept
{
    jassert(patternIndex >= 0);
    jassert((channelIndex >= 0) && (channelIndex <= 2));
    jassert(trackIndex >= 0);

    TrailingEffectContext trailingEffectContext;

    // Gets the Track offset.
    const auto trackOffset = Stk128Constants::offsetPatternsData
                       + positionsHeight * patternLineSize * patternIndex       // Reaches the right Pattern.
                      + 3 * channelIndex;                                       // Reaches the channel data inside the line.

    // Declares the Track.
    builder.startNewTrack(trackIndex);

    // Reads the cell for one Track only.
    auto success = true;
    auto currentNoteAndOctave = 12 * 4;             // Arbitrary.
    auto wasPitchPresent = false;
    auto wasHardSound = false;
    for (auto cellIndex = 0; cellIndex < positionsHeight; ++cellIndex) {
        const auto lineOffset = cellIndex * patternLineSize;
        const auto cellOffset = lineOffset + trackOffset;
        // Three bytes to read.
        const auto rawNote = static_cast<unsigned int>(MemoryBlockUtil::extractUnsignedChar(musicData, cellOffset + 0, success));
        auto localSuccess = success;
        const auto rawInstrumentAndEffectCommand = static_cast<unsigned int>(MemoryBlockUtil::extractUnsignedChar(musicData, cellOffset + 1, success));
        localSuccess = localSuccess && success;
        const auto effectParameter = static_cast<unsigned int>(MemoryBlockUtil::extractUnsignedChar(musicData, cellOffset + 2, success));
        localSuccess = localSuccess && success;
        if (!localSuccess) {
            builder.addError("Unable to read the cell for line " + juce::String(cellIndex) + " of track " + juce::String(trackIndex));
            return false;
        }

        const auto effectCommand = rawInstrumentAndEffectCommand & 0xfU;
        // Nothing to do if all is empty, we don't need to encode empty cells. Still the special case of the "was Pitch present" to make it go on.
        if ((rawNote == 0U) && (rawInstrumentAndEffectCommand == 0U) && (effectParameter == 0U) && !wasPitchPresent) {
            continue;
        }

        // Declares the Cell of the Track.
        builder.startNewCell(cellIndex);

        // Converts the note.
        const auto isNotePresent = (rawNote != 0U);        // 0 means "no note".
        auto isRst = false;
        auto soundtrakkerInstrument = -1;
        if (isNotePresent) {
            trailingEffectContext.declareNote();

            // Converts the note in the octave. If error, only raises a warning and corrects the number.
            const auto encodedNoteInOctave = ((rawNote >> 4U) & 0xfU) - 1U;  // Four bits (7-4) for the note. - 1 Because notes are shifted in the Soundtrakker.
            const auto encodedOctave = (rawNote & 0xfU) - 1U;                // Four bits (3-0) for the octave. -1 because the Soundtrakker octave starts at 1.
            isRst = (encodedNoteInOctave == 12U);                      // Is it a RST?
            int noteAndOctave;    // NOLINT(*-init-variables)
            int instrument;       // NOLINT(*-init-variables)
            if (!isRst) {
                // Normal note.
                noteAndOctave = NoteUtil::getNote(static_cast<int>(encodedNoteInOctave), static_cast<int>(encodedOctave));
                noteAndOctave = NumberUtil::correctNumber(noteAndOctave, CellConstants::minimumNote, CellConstants::maximumNote);
                soundtrakkerInstrument = static_cast<int>((rawInstrumentAndEffectCommand >> 4U) & 0xfU);      // Four bits (7-4) for the instrument.
                instrument = soundtrakkerInstrument + 1;                                    // + 1 because in AT2, the instrument 0 is reserved!
            } else {
                // RST.
                noteAndOctave = CellConstants::rstNote;
                instrument = 0;
            }

            currentNoteAndOctave = noteAndOctave;       // Stores the latest note and octave, useful for effects.

            // Encodes the note + instrument.
            builder.setCurrentCellNote(noteAndOctave);
            builder.setCurrentCellInstrument(instrument);
        }

        // Manages the effects. RST notes are considered "effect free" (this is needed specially because of hardware effect: the RST would be replaced by a new instrument).
        if (!isRst) {
            // A "pre-pass" is done to manage a special case: if the instrument has "S" flag, its volume is forced to the maximum,
            // thus an effect is added (unless a volume is also set here).
            auto hasVolumeBeenForcedOut = false;
            manageForcedInstrumentVolumeEffect(currentNoteAndOctave, soundtrakkerInstrument, static_cast<int>(effectCommand),
                                               trailingEffectContext, hasVolumeBeenForcedOut);
            // Now reads the real effects.
            manageReadEffect(isNotePresent ? currentNoteAndOctave : -1, static_cast<int>(effectCommand), static_cast<int>(effectParameter), // -1 if no note.
                             trackIndex, cellIndex, wasPitchPresent, wasHardSound, hasVolumeBeenForcedOut, patternHeight, trailingEffectContext);
        }

        builder.finishCurrentCell();
    }

    builder.finishCurrentTrack();

    return builder.isOk();
}

void SubsongImporter::manageForcedInstrumentVolumeEffect(const int noteAndOctave, const int instrumentIndex, const int effectCommand, TrailingEffectContext& trailingEffectContext,
                                                         bool& hasVolumeBeenForcedOut) const noexcept
{
    const auto currentVolume = trailingEffectContext.getCurrentVolume();

    // Nothing is needed if the current volume is already at 15!
    if (currentVolume == 15) {
        return;
    }

    // Both the note and the Instrument must be here.
    if ((noteAndOctave < 0) || (instrumentIndex < 0)) {
        return;
    }

    // Is the Instrument a "S" (forces volume)? If no, nothing to do.
    if (!doesInstrumentForcesVolume(instrumentIndex)) {
        return;        // Not a "S". We can stop here.
    }

    // It's a "S" Instrument. Is there a volume effect?
    bool isVolumeCommand;    // NOLINT(*-init-variables)
    switch (effectCommand) {
        case 0x1:
            [[fallthrough]];
        case 0x6:
            [[fallthrough]];
        case 0xb:
            [[fallthrough]];
        case 0x8:       // Hardware effects also considered as "volume": no need to set the volume!
            [[fallthrough]];
        case 0xa:
            isVolumeCommand = true;
            break;
        default:
            isVolumeCommand = false;
            break;
    }

    if (isVolumeCommand) {
        return;
    }

    hasVolumeBeenForcedOut = true;

    // There is no volume command. Let's add ours...
    // Note that the current volume is NOT modified (the S Instruments don't modify the volume of the NEXT notes).
    if (currentVolume.isPresent()) {
        manageVolumeEffect(currentVolume.getValue(), trailingEffectContext);
    }
}

bool SubsongImporter::doesInstrumentForcesVolume(const int instrumentIndex) const noexcept
{
    return (soundtrakkerInstrumentWithSFlagIndexes.find(instrumentIndex) != soundtrakkerInstrumentWithSFlagIndexes.cend());
}

void SubsongImporter::addEffect(const Effect effect, const int logicalValue, const bool dontEncodeIfZero) const noexcept
{
    // Don't encode the value if zero (and if wanted!).
    if ((dontEncodeIfZero) && (logicalValue == 0)) {
        return;
    }
    builder.addCurrentCellEffect(effect, logicalValue);
}

void SubsongImporter::manageReadEffect(const int currentNoteAndOctave, const int effectCommand, const int effectParameter, const int trackIndex, const int lineIndex, bool& wasPitchPresent,
                                       bool& wasHardSound, bool /*hasVolumeBeenForced*/, int& patternHeight, TrailingEffectContext& trailingEffectContext) noexcept
{
    // First, manages the effects.
    auto isNewPitch = false;
    auto newHardSoundState = false;
    //bool isVolumeEncoded = false;     --> More pollution than result, so it is discarded.

    switch (effectCommand) {
        default:
            jassertfalse;                           // Not managed?
        case 0x0:                                   // Nothing.
            break;
        case 0x1:                                   // Resets all effects, plus volume decrease.
            manageResetEffect(effectParameter);
            break;
        case 0x2:           // Portamento down.
            managePitchDownEffect(effectParameter);
            isNewPitch = true;
            break;
        case 0x3:           // Portamento up.
            managePitchUpEffect(effectParameter);
            isNewPitch = true;
            break;
        case 0x4:           // Volume down.
            trailingEffectContext.invalidateEffect(TrailingEffect::volume);
            manageVolumeDownEffect(effectParameter);
            break;
        case 0x5:           // Volume up.
            trailingEffectContext.invalidateEffect(TrailingEffect::volume);
            manageVolumeUpEffect(effectParameter);
            break;
        case 0x6:           // Arpeggio with volume.
            //resetTrailingEffects(channelIndex);
            manageArpeggioWithVolumeEffect(effectParameter, trailingEffectContext);
            //isVolumeEncoded = true;
            break;
        case 0x7:           // Tone vibrato. Not managed.
            break;
        case 0x8:
            [[fallthrough]];
        case 0xa:
            manageHardEffect(currentNoteAndOctave, (effectCommand == 0x8), effectParameter, trackIndex, lineIndex, wasHardSound);
            newHardSoundState = true;
            break;
        case 0x9:           // Break/Ins. delay.
            manageBreakOrInsDelayEffect(effectParameter, lineIndex, patternHeight);
            break;
        case 0xb:           // Volume.
            manageVolumeEffect(effectParameter, trailingEffectContext);
            //currentVolume = (effectParameter & 0xf);                // Needed? Seems like an oversight.
            //isVolumeEncoded = true;
            break;
        case 0xc:           // Switch noise+tone. Not managed.
            builder.addWarning("Effect C not supported in track " + juce::String(trackIndex) + ", line " + juce::String(lineIndex));
            break;
        case 0xd:           // Speed.
            manageSpeedEffect(effectParameter, lineIndex);
            break;
        case 0xe:           // Direct Arpeggio.
            // resetTrailingEffects(channelIndex);
            manageDirectArpeggio(effectParameter);
            break;
        case 0xf:           // Arpeggio.
            // resetTrailingEffects(channelIndex);
            manageArpeggioEffect(effectParameter);
            break;
    }

    // Special case: if there is no note, and if there is no pitch now but there WAS a pitch on the previous note, a pitch with a value of 0 is encoded.
    // This "trick" is needed before AT Pitch effects continue through time, not the Soundtrakker ones, which must be stopped, else it will not sound right at all.
    if ((currentNoteAndOctave < 0) && !isNewPitch && wasPitchPresent) {
        addEffect(Effect::pitchUp, 0, false);     // Up or down makes no difference.
    }
    wasPitchPresent = isNewPitch;

    wasHardSound = newHardSoundState;

    // If the volume has been forced before (because of a "S" Instrument) and that a "normal" one was not encoded this time, we set a volume so that
    // things go to normal.
    // --> More pollution than result, so it is discarded.
    /*if (hasVolumeBeenForced && !isVolumeEncoded) {
        success &= manageVolumeEffect(currentVolume);
    }*/
}

void SubsongImporter::manageVolumeEffect(const int newVolume, TrailingEffectContext& trailingEffectContext) const noexcept
{
    const auto effectAndValue = trailingEffectContext.declareEffect(TrailingEffect::volume, newVolume);
    if (effectAndValue.first != TrailingEffect::noEffect) {
        addEffect(Effect::volume, effectAndValue.second);
    }
}

void SubsongImporter::manageResetEffect(const int invertedVolume) const noexcept
{
    // The "reset" effect is linked to an inverted volume. In AT2 too, so there is nothing to change!
    addEffect(Effect::reset, invertedVolume);
}

void SubsongImporter::managePitchDownEffect(const int effectParameter) const noexcept
{
    const auto pitchToEncode = static_cast<int>((static_cast<unsigned int>(effectParameter) & 0xffU)) * pitchSlideMultiplier;
    addEffect(Effect::pitchDown, pitchToEncode, true);
}

void SubsongImporter::managePitchUpEffect(const int effectParameter) const noexcept
{
    const auto pitchToEncode = static_cast<int>((static_cast<unsigned int>(effectParameter) & 0xffU)) * pitchSlideMultiplier;
    addEffect(Effect::pitchUp, pitchToEncode, true);
}

void SubsongImporter::manageVolumeDownEffect(const int effectParameter) const noexcept
{
    const auto volumeToEncode = static_cast<int>((static_cast<unsigned int>(effectParameter) & 0xfU)) * volumeSlideMultiplier;
    addEffect(Effect::volumeOut, volumeToEncode, true);
}

void SubsongImporter::manageVolumeUpEffect(const int effectParameter) const noexcept
{
    const auto volumeToEncode = static_cast<int>((static_cast<unsigned int>(effectParameter) & 0xfU)) * volumeSlideMultiplier;
    addEffect(Effect::volumeIn, volumeToEncode, true);
}

void SubsongImporter::manageArpeggioWithVolumeEffect(const int effectParameter, TrailingEffectContext& trailingEffectContext) const noexcept
{
    // Manages the Arpeggio.
    const auto arpeggioNumber = (static_cast<unsigned int>(effectParameter) >> 4U) & 0xfU;      // The Arpeggio is the second digit.
    encodeArpeggio(static_cast<int>(arpeggioNumber));

    // Manages the volume. It fits the volume effect perfectly, so we can transmit the parameter directly.
    manageVolumeEffect(effectParameter, trailingEffectContext);
}

void SubsongImporter::manageArpeggioEffect(const int effectParameter) const noexcept
{
    // The Arpeggio also performs a reset (at least the volume is reset).
    addEffect(Effect::reset, 0);
    // Now, the Arpeggio.
    encodeArpeggio(static_cast<int>(static_cast<unsigned int>(effectParameter) & 0xfU));       // The Arpeggio is the first digit.
}

void SubsongImporter::encodeArpeggio(int arpeggioNumber) const noexcept
{
    // + 1 because the Arpeggio 0 is reserved in AT2!
    arpeggioNumber = static_cast<int>((static_cast<unsigned int>(arpeggioNumber) & 0xfU)) + 1;
    addEffect(Effect::arpeggioTable, arpeggioNumber, false);
}

void SubsongImporter::manageDirectArpeggio(const int effectParameter) const noexcept
{
    const auto arpeggioValueToEncode = static_cast<int>((static_cast<unsigned int>(effectParameter) & 0xffU));
    addEffect(Effect::arpeggio3Notes, arpeggioValueToEncode, false);
}

void SubsongImporter::manageBreakOrInsDelayEffect(const int effectParameter, const int lineIndex, int& patternHeight) const noexcept
{
    // If the effect is parameter is 0, is means "pattern break".
    if (effectParameter == 0) {
        // Only overrides this height if the new one is smaller! Else, several Break in a row (or in several Tracks) are present, the last one
        // will overwrite the others, yet it is the first one that should be used.
        const auto newPatternHeight = lineIndex + 1;
        if (newPatternHeight < patternHeight) {
            patternHeight = newPatternHeight;
        }
        return;
    }

    // Else, it means "instrument delay". However, the second digit MUST be "1", else it does nothing.
    if ((static_cast<unsigned int>(effectParameter) >> 4U) != 1U) {
        return;
    }
    // -1 because the speed in AT is expressed from 0 to 255.
    addEffect(Effect::forceInstrumentSpeed, static_cast<int>((static_cast<unsigned int>(effectParameter) & 0xfU)) - 1);
}

void SubsongImporter::manageSpeedEffect(const int effectParameter, const int lineIndex) noexcept
{
    // The speed is from 2 to 0x3f, included.
    const auto speed = NumberUtil::correctNumber(effectParameter, 2, 0x3f);

    // Opens a SpeedTrack if none is opened.
    if (!hasSpeedBeenFoundInPattern) {
        hasSpeedBeenFoundInPattern = true;
        builder.startNewSpeedTrack(nextSpeedTrackIndex);
    }

    // Encodes the event.
    builder.setCurrentSpeedTrackValue(lineIndex, speed);
}

void SubsongImporter::manageHardEffect(const int noteAndOctave, const bool isCurve8, int effectHardwarePeriod, int /*trackIndex*/, int /*lineIndex*/, const bool wasHardSound) noexcept
{
    // If there is no note, there is nothing to do. Indeed, we can't change the hardware period on the fly in AT2.
    if (noteAndOctave < 0) {
        return;
    }

    // In Soundtrakker, the Hardware effects stops the arpeggios. To simulate that simply, we put a reset effect. It's raw but it's simple and works.
    // However, to avoid pollution of the Track, the effect is only added if we weren't already on an Hardware effect before.
    if (!wasHardSound) {
        addEffect(Effect::reset, 0);
    }

    // A hardware period to 0 is not possible with AT2 (considered as "auto"), so we use 1 instead. Sonically, it doesn't change anything.
    if (effectHardwarePeriod == 0) {
        effectHardwarePeriod = 1;
    }

    const auto hardwareCurve = isCurve8 ? 8 : 0xa;     // In Soundtrakker, only 2 curves are possible.

    // The hardware effect is only managed by specific instruments.
    // A first attempt is made to find an "automatic" conversion. In this case, a hardware instrument is created with the right ratio (if it doesn't already exist).
    // However, in the case the hardware period is really low, we consider it a "special effect", and forces the instrument as a "forced hardware period".
    const auto notePeriod = PsgPeriod::getPeriod(PsgFrequency::psgFrequencyCPC, PsgFrequency::defaultReferenceFrequencyHz, noteAndOctave);
    auto isRatioFound = false;
    auto ratio = 0;
    if (effectHardwarePeriod >= minimumHardwarePeriodToPretendToBeARatio) {
        while (!isRatioFound && (ratio < hardwareRatioCount)) {
            const auto noteWithRatio = static_cast<int>((static_cast<unsigned int>(notePeriod) >> static_cast<unsigned int>(ratio)));
            if ((noteWithRatio == effectHardwarePeriod) || (abs(noteWithRatio - effectHardwarePeriod) == 1)) {      // Keeps a tolerance.
                isRatioFound = true;
            } else {
                ++ratio;
            }
        }
    }

    int instrumentIndexToUse;    // NOLINT(*-init-variables)

    if (isRatioFound) {
        // A ratio is found! Does the instrument already exist? Gets the right table according to the effect command (8/A).
        auto& hardwareRatioToInstrumentIndexWithHard = isCurve8 ? hardwareRatioToInstrumentIndexWithHard8 : hardwareRatioToInstrumentIndexWithHardA;
        instrumentIndexToUse = hardwareRatioToInstrumentIndexWithHard[static_cast<size_t>(ratio)]; // NOLINT(*-pro-bounds-constant-array-index)
        if (instrumentIndexToUse == 0) {
            // The instrument does not exist! We must create it.
            //DBG("Must create instrument for ratio " + String(ratio) + " for hardware curve 8?: " + String(isCurve8));

            const auto newInstrumentIndex = songBuilder.findNextInstrumentIndex();          // What will be the index of our new Instrument?
            songBuilder.startNewPsgInstrument(newInstrumentIndex);
            songBuilder.setCurrentInstrumentName("Generated");
            songBuilder.setCurrentPsgInstrumentMainLoop(0, 0, true);
            // Creates the only Instrument Cell.
            songBuilder.startNewPsgInstrumentCell();
            songBuilder.setCurrentPsgInstrumentCellLink(PsgInstrumentCellLink::softToHard);
            songBuilder.setCurrentPsgInstrumentCellPrimaryPeriod(0);
            songBuilder.setCurrentPsgInstrumentCellRatio(ratio);
            songBuilder.setCurrentPsgInstrumentCellHardwareEnvelope(hardwareCurve);
            songBuilder.finishCurrentPsgInstrumentCell();

            songBuilder.finishCurrentInstrument();
            instrumentIndexToUse = newInstrumentIndex;

            hardwareRatioToInstrumentIndexWithHard[static_cast<size_t>(ratio)] = newInstrumentIndex;     // This ratio is now linked to this new Instrument. NOLINT(*-pro-bounds-constant-array-index)
        }
    } else {
        // No valid ratio was found. So a specific Instrument is or has been created.

        // Does it exist? Checks in the right table according to the effect command (8/A).
        auto& hardwarePeriodToInstrumentIndexWithHard = isCurve8 ? hardwarePeriodToInstrumentIndexWithHard8 : hardwarePeriodToInstrumentIndexWithHardA;
        if (const auto it = hardwarePeriodToInstrumentIndexWithHard.find(effectHardwarePeriod); it != hardwarePeriodToInstrumentIndexWithHard.cend()) {
            // The instrument exists. Simply uses it.
            instrumentIndexToUse = it->second;
        } else {
            // The instrument does not exist. Creates it.
            //DBG("Must create instrument for forced period " + String(effectHardwarePeriod) + " for hardware curve 8?: " + String(isCurve8));

            const auto newInstrumentIndex = songBuilder.findNextInstrumentIndex();          // What will be the index of our new Instrument?
            songBuilder.startNewPsgInstrument(newInstrumentIndex);
            songBuilder.setCurrentInstrumentName("Generated");
            songBuilder.setCurrentPsgInstrumentMainLoop(0, 0, true);

            // Creates the only Instrument Cell.
            songBuilder.startNewPsgInstrumentCell();
            songBuilder.setCurrentPsgInstrumentCellLink(PsgInstrumentCellLink::softAndHard);        // The Hard period is forced, thus independent.
            songBuilder.setCurrentPsgInstrumentCellPrimaryPeriod(0);
            songBuilder.setCurrentPsgInstrumentCellSecondaryPeriod(effectHardwarePeriod);
            songBuilder.setCurrentPsgInstrumentCellHardwareEnvelope(hardwareCurve);
            songBuilder.finishCurrentPsgInstrumentCell();

            songBuilder.finishCurrentInstrument();
            instrumentIndexToUse = newInstrumentIndex;

            hardwarePeriodToInstrumentIndexWithHard.insert(std::pair(effectHardwarePeriod, newInstrumentIndex));      // Links the Hardware Period to this Instrument.
        }
    }

    // Encodes (overwrites) the new instrument.
    builder.setCurrentCellInstrument(instrumentIndexToUse);
}

void SubsongImporter::createEmptySpecialTracks() const noexcept
{
    // Creates the Speed Track.
    builder.startNewSpeedTrack(0);
    builder.finishCurrentSpeedTrack();

    // Creates the Event Track.
    builder.startNewEventTrack(0);
    builder.finishCurrentEventTrack();
}

}   // namespace arkostracker
