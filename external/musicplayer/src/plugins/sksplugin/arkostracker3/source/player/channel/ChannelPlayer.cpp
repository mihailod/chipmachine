#include "ChannelPlayer.h"

#include <algorithm>

#include "../../business/song/cells/CellEffectsChecker.h"
#include "../../song/ExpressionBuilder.h"
#include "../../song/psg/PsgFrequency.h"
#include "../../song/sid/SidPlayerCapability.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../CellToPlay.h"
#include "../PsgPeriod.h"
#include "../TemperedScaleUtil.h"

namespace arkostracker 
{

ChannelPlayer::ChannelPlayer(SongDataProvider& pSongDataProvider, const int pChannelIndex) noexcept :
        songDataProvider(pSongDataProvider),
        channelIndex(pChannelIndex),
        postedCellMutex(),
        postedMustStopUiThread(false),
        postedCellsUiThread(),
        newPlayedNote(),
        newCellEffects(),
        newPostedCells(),

        currentSample(nullptr),
        isSampleRestarted(false),
        currentSampleAmplification(1.0F),
        currentSampleLoop(),
        currentSampleFrequencyHz(PsgFrequency::defaultSampleFrequencyHz),
        currentSampleVolume4Bits(15),
        currentSamplePitchHz(0.0F),
        currentSampleReferenceFrequency(PsgFrequency::defaultReferenceFrequencyHz),

        baseNote(0),
        trackNote(0),
        noise(0),
        soundOpen(false),
        trackPitch(),
        trackPitchSlide(),
        trackVolume(15U),
        trackVolumeSlide(),
        trackGlideActivated(false),
        trackGlideGoalPeriod(0U),
        trackGlideFinalPitchValue(0U),
        trackGlideInitialPeriod(0U),
        isTrackGlidePeriodIncreasing(false),
        pitchFromPitchTable(0),
        currentInstrumentId(),
        instrumentCellVolumeOrHardwareVolume(0),
        instrumentCurrentLine(0),
        instrumentCurrentTick(0),
        forcedInstrumentSpeed(),
        hardwareEnvelope(PsgValues::minimumHardwareEnvelope),
        hardwareRetrig(false),
        hardwarePeriod(0),
        softwarePeriod(0),

        currentInlineArpeggio(true, "inline", true),
        useInlineArpeggio(false),
        currentTableArpeggioId(),
        currentArpeggioStepIndex(0),
        currentArpeggioTick(0),
        currentArpeggioSpeed(0),
        currentArpeggioEndIndex(0),
        currentArpeggioLoopStartIndex(0),
        forcedArpeggioSpeed(-1),            // Not forced.

        currentPitchId(),
        currentPitchStepIndex(0),
        currentPitchTick(0),
        currentPitchSpeed(0),
        currentPitchEndIndex(0),
        currentPitchLoopStartIndex(0),
        forcedPitchSpeed(-1),               // Not forced.

        currentIsSidActivated(false),
        currentSidHighShelfPeriod(0),
        currentSidLowShelfPeriod(0),
        currentSidLowShelfVolume(PsgValues::defaultSidLowShelfVolume)
{
}

void ChannelPlayer::postCell(const CellToPlay& cellToPlay) noexcept
{
    // Useless to post empty Cells. Empty Cells with Effect Context are allowed.
    if (cellToPlay.getCell().isEmpty() && cellToPlay.getLocation().isAbsent()) {
        return;
    }

    // Enqueues the Cell.
    const std::scoped_lock lock(postedCellMutex);                    // Lock!
    postedCellsUiThread.push_back(cellToPlay);
}

void ChannelPlayer::postStopSound() noexcept
{
    const std::scoped_lock lock(postedCellMutex);                    // Lock!
    postedMustStopUiThread = true;
}

std::unique_ptr<ChannelPlayerResults> ChannelPlayer::playStream(bool /*isFirstTick*/, const bool stillWithinLine) noexcept
{
    // This is called from a method called from the Audio Thread!
    // ----------------------------------------------------------

    //DBG("PlayStream for PSG " + juce::String(psgIndex) + ", channel " + juce::String(channelIndexInPsg));

    // Copies the possible queued Cells from UI thread and clears it.
    auto postedCells = std::vector<CellToPlay>();
    {
        const std::scoped_lock lock(postedCellMutex);                    // Lock!

        // Any "stop" flag? If yes, clears all the sounds. We don't stop the process, because a Stop is used when starting playing,
        // so pending notes can be present too.
        if (postedMustStopUiThread) {
            postedMustStopUiThread = false;

            currentSample = nullptr;
            currentInstrumentId = {};
            currentTableArpeggioId = {};
            currentPitchId = {};
            manageEffectSetVolume(PsgValues::maximumVolumeNoHard);
            resetInlineArpeggio();
        }

        postedCells = postedCellsUiThread;
        postedCellsUiThread.clear();
    }

    newPlayedNote = OptionalInt();
    newCellEffects = { };
    newPostedCells = std::vector<Cell>();
    newPostedCells.reserve(postedCells.size());

    // Reads all the Cells of the pattern, in order.
    for (const auto& cellToPlay : postedCells) {
        const auto& cell = cellToPlay.getCell();
        newPostedCells.push_back(cell);

        // First, manages the Track data (base note, arpeggios, current pitch), then, reads the instrument.
        readCellAndSetStates(cell);

        // A location is set, so we can determine whether an effect context must apply.
        if (songDataProvider.isEffectContextEnabled()) {
            if (const auto& location = cellToPlay.getLocation(); location.isPresent()) {
                //DBG("Effect Context in ChannelPlayer: Must be determined for channel index: " + juce::String(location.getValueRef().getChannelIndex()) + ", " + juce::String(juce::Time::getCurrentTime().getMilliseconds()));
                const auto effectContext = songDataProvider.determineEffectContextFromAudioThread(location.getValueRef());
                applyEffectContext(effectContext);
            }
        }
    }

    // The pitch/arpeggio or inline arpeggio should be determined by now. If not, finds it. Useful when playing a new sequence.
    if (songDataProvider.isEffectContextEnabled()) {
        if (currentPitchId.isAbsent()
            || (!useInlineArpeggio && currentTableArpeggioId.isAbsent())
            || (useInlineArpeggio && currentInlineArpeggio.getLength() <= 1)        // If smaller than a 3-step arp, then it's the inline is not well set.
        ) {
            //DBG("Effect Context in ChannelPlayer: Must be determined.");
            const auto effectContext = songDataProvider.determineEffectContextFromAudioThread(channelIndex);
            applyEffectContext(effectContext);
        }
    }

    // Manages the continuous effects (arpeggio, pitches, glide, volume).
    manageTrailingEffects(stillWithinLine);

    // Set before playing the instrument frame, because it will change them.
    const auto playedIndexInInstrument = instrumentCurrentLine;
    const auto playedInstrumentId = currentInstrumentId;

    // Plays the instrument.
    playInstrumentFrame();

    // Returns the PSG registers/sample data.
    auto channelOutputRegisters = std::make_unique<ChannelOutputRegisters>(instrumentCellVolumeOrHardwareVolume, noise,
                                                                           soundOpen, softwarePeriod, hardwarePeriod, hardwareEnvelope, hardwareRetrig,
                                                                           buildSpecialEffect());
    auto results = std::make_unique<ChannelPlayerResults>(newPlayedNote, newCellEffects, playedInstrumentId, std::move(channelOutputRegisters),
                                                  generateSamplePlayInfo(), playedIndexInInstrument, newPostedCells);
    isSampleRestarted = false;      // Resets as soon as published.

    return results;
}

void ChannelPlayer::applyEffectContext(const LineContext& lineContext) noexcept
{
    if (const auto newPitchId = lineContext.getPitchId(); newPitchId.isPresent()) {
        currentPitchId = newPitchId.getValueRef();
    }

    if (const auto newArpeggioId = lineContext.getArpeggioId(); newArpeggioId.isPresent()) {
        resetInlineArpeggio();
        currentTableArpeggioId = newArpeggioId.getValueRef();
        useInlineArpeggio = false;
    }

    if (const auto newArpeggioInline = lineContext.getArpeggioInline(); newArpeggioInline.isPresent()) {
        resetInlineArpeggio();
        currentInlineArpeggio = newArpeggioInline.getValueRef();
        useInlineArpeggio = true;
    }

    trackVolume = lineContext.getVolume();
}

SamplePlayInfo ChannelPlayer::generateSamplePlayInfo() const noexcept
{
    return (currentSample != nullptr)
        ? SamplePlayInfo(currentSample, isSampleRestarted, currentSampleAmplification, currentSampleLoop, currentSampleFrequencyHz,
                         currentSamplePitchHz, currentSampleReferenceFrequency, currentSampleVolume4Bits, false)
                                                   : SamplePlayInfo();  // This means we don't cut anything that is already playing.
}

void ChannelPlayer::readCellAndSetStates(const Cell& cell) noexcept
{
    // Corrects the cell to remove the possible errors.
    const auto currentEffects = CellEffectsChecker::correctEffects(cell);

    // One effect could be a glide. If yes, we must NOT read the note normally, we only want to get the "final" period.
    // If there is no note (tested after), only the Glide Speed is modified.
    const auto isGlide = browseEffectsForGlideAndSetGlideSpeed(currentEffects);

    // Is there a note?
    if (cell.isNote()) {
        // There is a new note. Adds the transposition.
        const auto readNote = cell.getNote().getValue().getNote() + songDataProvider.getTranspositionFromAudioThread(channelIndex);
        newPlayedNote = readNote;

        // In case of a Glide effect, this means the Glide goal is set from the note.
        // This is also all that we do! The Instrument is thus ignored.
        if (isGlide) {
            const auto psgAndReferenceFrequencies = songDataProvider.getPsgFrequencyFromChannelFromAudioThread(channelIndex);
            const auto psgFrequency = psgAndReferenceFrequencies.first;
            const auto referenceFrequency = psgAndReferenceFrequencies.second;

            int periodGoal;         // NOLINT(*-init-variables)
            int periodBaseNote;     // NOLINT(*-init-variables)
            if (currentSample == nullptr) {
                // Pitch for PSG.
                periodGoal = PsgPeriod::getPeriod(psgFrequency, referenceFrequency, readNote);
                periodBaseNote = PsgPeriod::getPeriod(psgFrequency, referenceFrequency, baseNote);
            } else {
                // Pitch for Sample. It works exactly like the PSG, except that the period calculation is using *frequencies*(!!),
                // it works exactly the same! The base/goal are inverted, because periods and frequencies are going in opposite direction.
                // Only when sending the frequency to the sample, it is inverted.
                // Fortunately, the frequency min/max in Hz is included in an unsigned short!
                periodBaseNote = static_cast<int>(TemperedScaleUtil::getSampleFrequency(readNote, referenceFrequency));
                periodGoal = static_cast<int>(TemperedScaleUtil::getSampleFrequency(baseNote, referenceFrequency));
            }

            trackGlideFinalPitchValue = static_cast<uint16_t>(periodGoal - periodBaseNote);
            trackGlideGoalPeriod = static_cast<uint16_t>(periodGoal);
            const auto periodBaseNotePitched = static_cast<uint16_t>(static_cast<unsigned int>(periodBaseNote) + trackPitch.getIntegerPart());

            trackGlideInitialPeriod = static_cast<uint16_t>(periodBaseNote);

            isTrackGlidePeriodIncreasing = (periodGoal >= static_cast<int>(periodBaseNotePitched));       // Must take in account the current pitch.
        } else {
            // No glide. We can process the note as normal.

            // Resets some effects, including the slides and glide.
            // Note that the position on Arpeggio and Pitches are NOT reset here, because it can be a legato!
            baseNote = readNote;

            trackPitch.reset();
            trackPitchSlide.reset();

            trackGlideActivated = false;

            // Instrument? It is read ONLY if there is a note. An instrument alone makes no sense.
            if (cell.isNoteAndInstrument()) {
                trackVolumeSlide.reset();       // Volume slide is stopped only if there is an instrument.

                // The instrument is set.
                const auto readInstrumentIndex = cell.getInstrument().getValue();

                updateArpeggioMetadata(true);   // Reads the Arpeggio metadata, especially useful for the speed that is put back.
                resetPositionOnArpeggio();      // Restarts the arpeggio.

                updatePitchMetadata(true);      // Reads the Pitch metadata, especially useful for the speed that is put back.
                resetPositionOnPitchTable();    // The same with the Pitch Table.

                instrumentCurrentLine = 0;
                instrumentCurrentTick = 0;

                currentInstrumentId = songDataProvider.getInstrumentIdFromAudioThread(readInstrumentIndex);

                // Performs some initializations according to the Instrument type.
                const auto instrumentType = songDataProvider.getInstrumentTypeFromAudioThread(currentInstrumentId);
                switch (instrumentType) {
                    default:
                        jassertfalse;           // Should never happen!!
                    case InstrumentType::psgInstrument:
                        // On new sound, the speed is reset and not forced.
                        forcedInstrumentSpeed = OptionalInt();
                        break;
                    case InstrumentType::sampleInstrument:
                        isSampleRestarted = true;
                        // Stops the PSG sound.
                        noise = 0;
                        soundOpen = false;
                        instrumentCellVolumeOrHardwareVolume = 0;
                        break;
                }
            }
        }
    }

    // Reads the effects and sets them up.
    manageNewCellEffects(currentEffects);
}

void ChannelPlayer::manageNewCellEffects(const CellEffects& originalEffects) noexcept
{
    // Manages the effects.
    const auto cellEffects = originalEffects.getExistingEffects();
    for (const auto& entry : cellEffects) {                 // NoEffect are discarded.
        const auto cellEffect = entry.second;
        const auto effectNumber = cellEffect.getEffect();
        const auto effectValue = cellEffect.getEffectLogicalValue();

        newCellEffects = originalEffects;

        switch (effectNumber) {
            case Effect::volume:
                manageEffectSetVolume(effectValue);
                break;
            case Effect::volumeIn:
                manageEffectVolumeIn(effectValue);
                break;
            case Effect::volumeOut:
                manageEffectVolumeOut(effectValue);
                break;
            case Effect::pitchUp:
                manageEffectPitchUp(effectValue);
                break;
            case Effect::fastPitchUp:
                manageEffectFastPitchUp(effectValue);
                break;
            case Effect::pitchDown:
                manageEffectPitchDown(effectValue);
                break;
            case Effect::fastPitchDown:
                manageEffectFastPitchDown(effectValue);
                break;
            case Effect::pitchTable:
                manageEffectPitchTable(effectValue);
                break;
            case Effect::reset:
                manageEffectReset(effectValue);
                break;
            case Effect::forceInstrumentSpeed:
                manageEffectForceInstrumentSpeed(effectValue);
                break;
            case Effect::forceArpeggioSpeed:
                manageEffectForceArpeggioSpeed(effectValue);
                break;
            case Effect::forcePitchTableSpeed:
                manageEffectForcePitchTableSpeed(effectValue);
                break;
            case Effect::arpeggio3Notes:
                manageEffectArpeggio3Notes(effectValue);
                break;
            case Effect::arpeggio4Notes:
                manageEffectArpeggio4Notes(effectValue);
                break;
            case Effect::arpeggioTable:
                manageEffectArpeggioTable(effectValue);
                break;
            case Effect::noEffect:              // Nothing to do when no effect!
                jassertfalse;           // The "no effect" is not supposed to be returned!
                break;
            case Effect::pitchGlide:            // The Glide effect is managed before, on reading the Cell, as it is a bit special.
                [[fallthrough]];
            case Effect::legato:
                // Nothing to do, normal.
                break;
            default:
                jassertfalse;            // Unknown effect?? Shouldn't happen.
                break;
        }
    }
}

void ChannelPlayer::manageEffectSetVolume(const int volume) noexcept
{
    jassert(volume >= 0);
    trackVolume.setValues(static_cast<uint16_t>(volume));
    // The volume stays as it is.
    trackVolumeSlide.reset();
}

void ChannelPlayer::manageEffectVolumeIn(const int effectValue) noexcept
{
    trackVolumeSlide.setFromDigits(effectValue);
}

void ChannelPlayer::manageEffectVolumeOut(const int effectValue) noexcept
{
    trackVolumeSlide.setFromDigits(effectValue);
    trackVolumeSlide.negate();      // Simpler than calculating the value here...
}

void ChannelPlayer::manageEffectPitchUp(const int effectValue) noexcept
{
    trackPitchSlide.setFromDigits(effectValue);
    trackPitchSlide.negate();       // Negative because for the pitch to go up, the period must go down!

    trackGlideActivated = false;    // A pitch cancels a previous Glide.
}

void ChannelPlayer::manageEffectFastPitchUp(const int effectValue) noexcept
{
    manageEffectPitchUp(static_cast<int>(static_cast<unsigned int>(effectValue) << 4U));              // Artificially creates a digit.
}

void ChannelPlayer::manageEffectPitchDown(const int effectValue) noexcept
{
    trackPitchSlide.setFromDigits(effectValue);

    trackGlideActivated = false;    // A pitch cancels a previous Glide.
}

void ChannelPlayer::manageEffectFastPitchDown(const int effectValue) noexcept
{
    manageEffectPitchDown(static_cast<int>(static_cast<unsigned int>(effectValue) << 4U));            // Artificially creates a digit.
}

void ChannelPlayer::manageEffectPitchTable(const int pitchTableIndex) noexcept
{
    currentPitchId = songDataProvider.getExpressionIdFromAudioThread(false, pitchTableIndex);

    // The Pitch Table is restarted. Useful when using this effect without putting a note.
    resetPositionOnPitchTable();
    // The Pitch data are requested, but this is really done only to put the Speed back, as it may have been forced.
    updatePitchMetadata(true);
}

void ChannelPlayer::manageEffectReset(const int effectValue) noexcept
{
    resetEffects(effectValue);
}

void ChannelPlayer::manageEffectForceInstrumentSpeed(const int effectValue) noexcept
{
    forcedInstrumentSpeed = effectValue;
}

void ChannelPlayer::manageEffectForceArpeggioSpeed(const int effectValue) noexcept
{
    forcedArpeggioSpeed = effectValue;
}

void ChannelPlayer::manageEffectForcePitchTableSpeed(const int effectValue) noexcept
{
    forcedPitchSpeed = effectValue;
}

void ChannelPlayer::manageEffectArpeggio3Notes(const int effectValue) noexcept
{
    resetInlineArpeggio();
    currentInlineArpeggio = ExpressionBuilder::buildArpeggio3Notes(effectValue);

    useInlineArpeggio = true;
    forcedArpeggioSpeed = -1;           // Use the speed of the new Arpeggio.
}

void ChannelPlayer::manageEffectArpeggio4Notes(const int effectValue) noexcept
{
    resetInlineArpeggio();
    currentInlineArpeggio = ExpressionBuilder::buildArpeggio4Notes(effectValue);

    useInlineArpeggio = true;
    forcedArpeggioSpeed = -1;           // Use the speed of the new Arpeggio.
}

void ChannelPlayer::manageEffectArpeggioTable(const int arpeggioNumber) noexcept
{
    useInlineArpeggio = false;
    currentTableArpeggioId = songDataProvider.getExpressionIdFromAudioThread(true, arpeggioNumber);

    // The Arpeggio is restarted. Useful when using this effect without putting a note.
    resetPositionOnArpeggio();
    // The Arpeggio data are requested, but this is really done only to put the Speed back, as it may have been forced.
    updateArpeggioMetadata(true);
}

void ChannelPlayer::updateArpeggioMetadata(const bool resetSpeed) noexcept
{
    // Must the (possible) forced speed must be reset?
    if (resetSpeed) {
        forcedArpeggioSpeed = -1;
    }

    if (useInlineArpeggio) {
        // The data can be directly extracted from the stored Direct Arpeggio.
        currentArpeggioLoopStartIndex = currentInlineArpeggio.getLoopStart(true);
        currentArpeggioEndIndex = currentInlineArpeggio.getEnd(true);
        currentArpeggioSpeed = currentInlineArpeggio.getSpeed();
    } else {
        // We must ask the Song about the Arpeggio data.
        const auto expressionMetadata = songDataProvider.getExpressionMetadataFromAudioThread(true, currentTableArpeggioId);
        currentArpeggioLoopStartIndex = expressionMetadata.loopStartIndex;
        currentArpeggioEndIndex = expressionMetadata.endIndex;
        currentArpeggioSpeed = expressionMetadata.speed;
    }

    // Is the Arpeggio Speed overridden (by the Force Speed effect)?
    if (forcedArpeggioSpeed >= 0) {
        currentArpeggioSpeed = forcedArpeggioSpeed;
    }
}

void ChannelPlayer::updatePitchMetadata(const bool resetSpeed) noexcept
{
    // Must the (possible) forced speed must be reset?
    if (resetSpeed) {
        forcedPitchSpeed = -1;
    }

    // We must ask the Song about the Pitch data.
    const auto expressionMetadata = songDataProvider.getExpressionMetadataFromAudioThread(false, currentPitchId);
    currentPitchLoopStartIndex = expressionMetadata.loopStartIndex;
    currentPitchEndIndex = expressionMetadata.endIndex;
    currentPitchSpeed = expressionMetadata.speed;

    // Is the Pitch Speed overridden (by the Force Speed effect)?
    if (forcedPitchSpeed >= 0) {
        currentPitchSpeed = forcedPitchSpeed;
    }
}

void ChannelPlayer::manageTrailingEffects(const bool stillWithinLine) noexcept
{
    // Reads the arpeggio first, as it defines the track note, on which everything else is based.
    const auto arpeggioNote = readArpeggioAndAdvance();

    trackNote = Cell::correctNote(baseNote + arpeggioNote);     // Protects from out of boundaries notes.

    // Manages the "trailing effects", such as pitch, glide and volume.
    // If we are past the "speed" tick, the pitch and volume slide are stopped. This is useful for the "read one line" feature.
    if (stillWithinLine) {
        manageTrailingGlide();
        manageTrailingPitch();
        manageTrailingVolume();
    }

    // Reads the Pitch Table.
    pitchFromPitchTable = readPitchTableAndAdvance();
}

void ChannelPlayer::manageTrailingVolume() noexcept
{
    // Manages the Track Volume. A limit is tested and applied (no looping).
    trackVolume += trackVolumeSlide;
    trackVolume.correctFrom0To(PsgValues::maximumVolumeNoHard);
}

void ChannelPlayer::manageTrailingPitch() noexcept
{
    // Track Pitch is not managed in case of a Glide.
    if (trackGlideActivated) {
        return;
    }

    // Track Pitch.
    trackPitch = trackPitch + trackPitchSlide;      // No limit to test, the value cycles (like on real CPC).
}

void ChannelPlayer::manageTrailingGlide() noexcept
{
    if (!trackGlideActivated) {
        return;
    }

    // Starts by putting the glide pitch.
    if (isTrackGlidePeriodIncreasing) {
        trackPitch += trackPitchSlide;
    } else {
        trackPitch -= trackPitchSlide;
    }

    // Are we there yet?
    const auto currentPeriod = static_cast<uint16_t>((trackGlideInitialPeriod + trackPitch.getIntegerPart()));

    if (currentPeriod == trackGlideGoalPeriod) {
        setGlideToComplete();
    } else {
        // Not finished, it seems. However, we could have gone too far!

        // Is the goal superior to the current period?
        if (trackGlideGoalPeriod > currentPeriod) {
            if (!isTrackGlidePeriodIncreasing) {
                // The glide should not go down! This means we have gone too far. Consider the glide over.
                setGlideToComplete();
            }
        } else {
            // The goal is inferior to the current period.
            // The current pitch must be decreased.
            // However, the glide direction is a strong indication about whether we have been too far (in the previous iteration).
            if (isTrackGlidePeriodIncreasing) {
                // The glide should not go up! This means we have gone too far. Considers the glide is over.
                setGlideToComplete();
            }
        }
    }
}

void ChannelPlayer::setGlideToComplete() noexcept
{
    trackPitch = trackGlideFinalPitchValue;
    trackGlideActivated = false;
    trackPitchSlide = 0U;
}

void ChannelPlayer::playInstrumentFrame() noexcept
{
    // What to do? Depends on the Instrument type.
    // Note that there is no transaction. It is unlikely a change occurs between the get type and perform, but if it does,
    // it doesn't really matter, as the calls are all safe. Only one frame will be "default".
    switch (songDataProvider.getInstrumentTypeFromAudioThread(currentInstrumentId)) {
        default:
            jassertfalse;           // Should never happen!!
        case InstrumentType::psgInstrument:
            playPsgInstrumentFrame();
            break;
        case InstrumentType::sampleInstrument:
            playSampleInstrumentFrame();
            break;
    }
}

void ChannelPlayer::playSampleInstrumentFrame() noexcept
{
    // This is called only when sample notes are played (every frame till another sound is triggered). Event samples are not managed here.
    const auto sampleFrameData = songDataProvider.getSampleInstrumentFrameDataFromAudioThread(currentInstrumentId);

    currentSample = sampleFrameData.sample;
    currentSampleAmplification = sampleFrameData.amplificationRatio;
    currentSampleLoop = sampleFrameData.loop;
    currentSampleFrequencyHz = sampleFrameData.frequencyHz;
    currentSampleVolume4Bits = std::max(0, static_cast<int>(trackVolume.getIntegerPart()));

    const auto [psgFrequency, referenceFrequency] = songDataProvider.getPsgFrequencyFromChannelFromAudioThread(channelIndex);
    currentSampleReferenceFrequency = referenceFrequency;
    // Adds the pitch!
    auto samplePitchHz = TemperedScaleUtil::getSampleFrequency(trackNote, currentSampleReferenceFrequency);
    const auto signedPitch = static_cast<int16_t>(trackPitch.getIntegerPart());       // Gets a signed value!
    samplePitchHz -= static_cast<float>(signedPitch);

    currentSamplePitchHz = samplePitchHz;       // FIXME should be ranged (0-22050 for ex), but can't find a nice way to do it (fmod only manages >0).
}

void ChannelPlayer::playPsgInstrumentFrame() noexcept
{
    // No more sample.
    currentSample = nullptr;

    // Gets a frame of the Instrument.
    const auto psgFrameData = songDataProvider.getPsgInstrumentFrameDataFromAudioThread(currentInstrumentId, instrumentCurrentLine);

    // The retrig is first populated with the "isRetrig" state from the Instrument itself. It may be updated just below by the Instrument Cell.
    // The "isRetrig" must be checked only at the beginning of the Instrument, only when it is first played!
    hardwareRetrig = (newPlayedNote.isPresent() && (instrumentCurrentTick == 0) && psgFrameData.isInstrumentRetrig);

    // Gets the current Cell.
    const auto& instrumentCell = psgFrameData.cell;

    // Gets the noise.
    noise = instrumentCell.getNoise();

    const auto link = instrumentCell.getLink();
    currentIsSidActivated = instrumentCell.isSidActivated();

    // Sets the sound on/off. As long as a software is generated, the sound channel is open.
    soundOpen = ((link != PsgInstrumentCellLink::hardOnly) && (link != PsgInstrumentCellLink::noSoftNoHard));

    // Sets the software volume, if the conditions are right.
    if ((link == PsgInstrumentCellLink::noSoftNoHard) || (link == PsgInstrumentCellLink::softOnly)) {
        instrumentCellVolumeOrHardwareVolume = instrumentCell.getVolume();
    }

    // Sets the possible hardware parameters (volume, envelope, retrig), if the conditions are right.
    if ((link == PsgInstrumentCellLink::softAndHard) || (link == PsgInstrumentCellLink::softToHard) || (link == PsgInstrumentCellLink::hardOnly)
            || (link == PsgInstrumentCellLink::hardToSoft)) {
        instrumentCellVolumeOrHardwareVolume = 16;
        hardwareEnvelope = instrumentCell.getHardwareEnvelope();
        // The instrument metadata may have set the hardware retrig to true, in that case, no need to check the cell retrig state.
        if (!hardwareRetrig) {
            hardwareRetrig = (instrumentCell.isRetrig() && (instrumentCurrentTick == 0));  // The retrig may be set to true, but only on the first tick!
        }
    }

    // Not used all the time, oh well.
    const auto [psgFrequency, referenceFrequency] = songDataProvider.getPsgFrequencyFromChannelFromAudioThread(channelIndex);

    // Calculates the software part, if any.
    // --------------------------------------
    if ((link == PsgInstrumentCellLink::softOnly) || (link == PsgInstrumentCellLink::softAndHard) || (link == PsgInstrumentCellLink::softToHard)
        // If NoSoftNoHard, manages the period anyway for SID (only), because SID needs to know the period to generate a signal if NoSoftNoHard.
        || ((link == PsgInstrumentCellLink::noSoftNoHard) && currentIsSidActivated)) {
        // Is there a forced period?
        if (instrumentCell.doesForceSoftwarePeriod()) {
            // Forced period. Forget about the note from the track, uses the period given in the instrument.
            // Also don't add the possible instrument and track pitch!
            softwarePeriod = instrumentCell.getSoftwarePeriod();
        } else {
            // Automatic period. Finds the period from the note from the track.
            auto softwareNote = trackNote;
            softwareNote += instrumentCell.getSoftwareArpeggio();
            softwareNote = Cell::correctNote(softwareNote);
            softwarePeriod = PsgPeriod::getPeriod(psgFrequency, referenceFrequency, softwareNote);

            // Adds the pitches (from the track slide/glide, the pitch table, and from the instrument).
            softwarePeriod += static_cast<int16_t>(trackPitch.getIntegerPart());        // Converted to signed, else the period will be decreased of 65xxx!
            softwarePeriod -= pitchFromPitchTable;
            softwarePeriod -= instrumentCell.getSoftwarePitch();                // More logical: increasing the pitch will make the sound higher.
        }
    }

    // Calculates the hardware part, if any.
    // --------------------------------------
    // If SoftToHard, calculates the hardware period from the software period.
    // Also, a Hardware Pitch can still be used to allow de-sync!
    if (link == PsgInstrumentCellLink::softToHard) {
        const auto ratio = instrumentCell.getRatio();
        hardwarePeriod = (softwarePeriod >> ratio);
        // If the last removed bit is true, the result is added one.
        if ((ratio > 0) && (((softwarePeriod >> (ratio - 1)) & 1) != 0)) {
            ++hardwarePeriod;
        }
        // Possible hardware desync.
        hardwarePeriod -= instrumentCell.getHardwarePitch();        // More logical: increasing the pitch will make the sound higher.
    } else if ((link == PsgInstrumentCellLink::hardOnly) || (link == PsgInstrumentCellLink::hardToSoft) || (link == PsgInstrumentCellLink::softAndHard)) {
        // If HardOnly, HardToSoft or SoftAndHard, calculates the hardware period from scratch, exactly like the software period was calculated above.
        // Is there a forced period?
        if (instrumentCell.doesForceHardwarePeriod()) {
            // Forced period. Forget about the note from the track, uses the period given in the instrument.
            // Also don't add the possible instrument and track pitch!
            hardwarePeriod = instrumentCell.getHardwarePeriod();
        } else {
            // Automatic period. Finds the period from the note from the track.
            auto hardwareNote = trackNote;
            hardwareNote += instrumentCell.getHardwareArpeggio();
            hardwarePeriod = PsgPeriod::getPeriod(psgFrequency, referenceFrequency, hardwareNote);

            // Adds the pitches (from the track slide/glide, the pitch table, and from the instrument).
            hardwarePeriod += static_cast<juce::int16>(trackPitch.getIntegerPart());        // Converted to signed, else the period will be decreased of 65xxx!
            hardwarePeriod -= pitchFromPitchTable;
            hardwarePeriod -= instrumentCell.getHardwarePitch();        // More logical: increasing the pitch will make the sound higher.
        }
    }

    // If HardToSoft, calculates the software period from the hardware period.
    // Also, a Software Pitch can still be used to allow desync.
    if (link == PsgInstrumentCellLink::hardToSoft) {
        const auto ratio = instrumentCell.getRatio();
        softwarePeriod = (hardwarePeriod << ratio);
        // Possible software desync.
        softwarePeriod -= instrumentCell.getSoftwarePitch();            // More logical: increasing the pitch will make the sound higher.
    }

    // Finally, corrects the periods in case of overflow, with loop, as it would on Z80.
    restrictSoftwarePeriod(softwarePeriod);
    restrictHardwarePeriod(hardwarePeriod);

    // If the volume is not hardware, the volume should be decreased according to the Track volume.
    if (instrumentCellVolumeOrHardwareVolume <= PsgValues::maximumVolumeNoHard) {
        instrumentCellVolumeOrHardwareVolume -= (PsgValues::maximumVolumeNoHard - trackVolume.getIntegerPart());
        instrumentCellVolumeOrHardwareVolume = std::max(instrumentCellVolumeOrHardwareVolume, 0);
    }

    // Manages SID.
    if (currentIsSidActivated) {
        auto periodToUse = softwarePeriod;
        switch (link) {
            case PsgInstrumentCellLink::noSoftNoHard:   // Still better than hard, works very fine.
            case PsgInstrumentCellLink::softOnly:
            case PsgInstrumentCellLink::softToHard:
            case PsgInstrumentCellLink::softAndHard:    // Have to choose...
                break;
            case PsgInstrumentCellLink::hardOnly:
            case PsgInstrumentCellLink::hardToSoft:
                periodToUse = hardwarePeriod;
                break;
        }

        const auto sidPlayerCapability = songDataProvider.getSidPlayerCapability();
        const auto [highSidPeriod, lowSidPeriod] = calculateSidPeriods(instrumentCell, periodToUse, psgFrequency, sidPlayerCapability);
        currentSidHighShelfPeriod = highSidPeriod;
        currentSidLowShelfPeriod = lowSidPeriod;

        currentSidLowShelfVolume = instrumentCell.getSidLowShelfVolume();
    }

    // Finally, advances in the Instrument.
    // Uses the Instrument speed or the forced one?
    const auto instrumentCurrentSpeed = forcedInstrumentSpeed.isPresent() ? forcedInstrumentSpeed.getValue() : psgFrameData.speed;
    if (++instrumentCurrentTick > instrumentCurrentSpeed) {
        instrumentCurrentTick = 0;
        // Next line. Is it the end of the instrument?
        const auto& loop = psgFrameData.loop;
        if (++instrumentCurrentLine > loop.getEndIndex()) {
            // End of the instrument. Loops?
            if (loop.isLooping()) {
                // Loops.
                instrumentCurrentLine = loop.getStartIndex();
            } else {
                // The instrument does not loop! The current instrument becomes the "empty" instrument.
                instrumentCurrentLine = 0;
                currentInstrumentId = {};
            }
        }
    }
}

void ChannelPlayer::resetEffects(const int invertedVolume) noexcept
{
    jassert((invertedVolume >= 0) && (invertedVolume <= PsgValues::maximumVolumeNoHard));

    trackVolume.setValues(static_cast<uint16_t>(PsgValues::maximumVolumeNoHard - (static_cast<unsigned int>(invertedVolume) & 0xfU)));     // The reset sets the volume.
    trackVolumeSlide.reset();
    trackPitchSlide.reset();
    resetInlineArpeggio();
    // Forces the arpeggio to the empty one, cleaner for the EffectContext.
    currentTableArpeggioId = songDataProvider.getExpressionIdFromAudioThread(true, 0);
    useInlineArpeggio = false;

    resetPitchTable();
}

void ChannelPlayer::resetInlineArpeggio() noexcept
{
    currentInlineArpeggio.clear(true);
    currentInlineArpeggio.setSpeed(0);
    currentArpeggioSpeed = 0;
    // Makes sure we start reading it from the beginning.
    resetPositionOnArpeggio();
}

void ChannelPlayer::resetPositionOnArpeggio() noexcept
{
    currentArpeggioStepIndex = 0;
    currentArpeggioTick = 0;
}

void ChannelPlayer::resetPitchTable() noexcept
{
    // Forces the pitch to the empty one, cleaner for the EffectContext.
    currentPitchId = songDataProvider.getExpressionIdFromAudioThread(false, 0);
    // Not really useful, but cleaner.
    resetPositionOnPitchTable();
    currentPitchSpeed = 0;
    currentPitchEndIndex = 0;
    currentPitchLoopStartIndex = 0;
}

void ChannelPlayer::resetPositionOnPitchTable() noexcept
{
    currentPitchStepIndex = 0;
    currentPitchTick = 0;
}

int ChannelPlayer::readArpeggioAndAdvance() noexcept
{
    updateArpeggioMetadata();

    // Makes sure the index is still within boundaries.
    if (currentArpeggioStepIndex > currentArpeggioEndIndex) {        // Security in case the Arpeggio has changed.
        currentArpeggioStepIndex = 0;
    }

    // Reads the value.
    const auto arpeggioValue = readArpeggioValue();

    // Advances in the Arpeggio.
    if (++currentArpeggioTick > currentArpeggioSpeed) {
        // Reads next line. End of the Arpeggio?
        currentArpeggioTick = 0;
        if (++currentArpeggioStepIndex > currentArpeggioEndIndex) {
            currentArpeggioStepIndex = currentArpeggioLoopStartIndex;
        }
    }

    return arpeggioValue;
}

int ChannelPlayer::readArpeggioValue() const noexcept
{
    if (useInlineArpeggio) {
        // Direct access to the (direct) Arpeggio.
        return currentInlineArpeggio.getValue(currentArpeggioStepIndex, true);
    }

    // We must ask the Song about the Arpeggio value.
    return songDataProvider.getExpressionValueFromAudioThread(true, currentTableArpeggioId, currentArpeggioStepIndex);
}

int ChannelPlayer::readPitchTableAndAdvance() noexcept
{
    updatePitchMetadata();

    // Makes sure the index is still within boundaries.
    if (currentPitchStepIndex > currentPitchEndIndex) {      // Security in case the Pitch has changed.
        currentPitchStepIndex = 0;
    }

    // Reads the value.
    const auto pitchValue = readPitchValue();

    // Advances in the Pitch.
    if (++currentPitchTick > currentPitchSpeed) {
        // Reads next line. End of the Pitch?
        currentPitchTick = 0;
        if (++currentPitchStepIndex > currentPitchEndIndex) {
            currentPitchStepIndex = currentPitchLoopStartIndex;
        }
    }

    return pitchValue;
}

int ChannelPlayer::readPitchValue() const noexcept
{
    // We must ask the Song about the Pitch value.
    return songDataProvider.getExpressionValueFromAudioThread(false, currentPitchId, currentPitchStepIndex);
}

bool ChannelPlayer::browseEffectsForGlideAndSetGlideSpeed(const CellEffects& effects) noexcept
{
    // Any Glide effect?
    const auto& optionalEffect = effects.find(Effect::pitchGlide);
    if (optionalEffect.isAbsent()) {
        return false;
    }

    // A Glide is found.
    const auto rawGlideSpeed = optionalEffect.getValue().getEffectLogicalValue();
    // Converts the raw value to a Float.
    trackPitchSlide.setFromDigits(rawGlideSpeed);
    trackGlideActivated = true;

    return true;
}

void ChannelPlayer::restrictSoftwarePeriod(int& period) noexcept
{
    period = NumberUtil::restrictWithLoop(period, PsgValues::maximumSoftwarePeriod);
}

void ChannelPlayer::restrictHardwarePeriod(int& period) noexcept
{
    period = NumberUtil::restrictWithLoop(period, PsgValues::maximumHardwarePeriod);
}

std::unique_ptr<ChannelPlayerResults> ChannelPlayer::getResults() const noexcept
{
    // Agglomerates the channel data into the Channel Registers, stored inside the ChannelPlayerResults.
    auto channelOutputRegisters = std::make_unique<ChannelOutputRegisters>(instrumentCellVolumeOrHardwareVolume, noise, soundOpen, softwarePeriod,
                                                                           hardwarePeriod, hardwareEnvelope, hardwareRetrig,
                                                                           buildSpecialEffect());
    return std::make_unique<ChannelPlayerResults>(newPlayedNote, newCellEffects, currentInstrumentId, std::move(channelOutputRegisters), generateSamplePlayInfo(),
        instrumentCurrentLine, newPostedCells);
}

SpecialEffect ChannelPlayer::buildSpecialEffect() const noexcept
{
    return {
        currentIsSidActivated,
        currentSidHighShelfPeriod,
        currentSidLowShelfPeriod,
        currentSidLowShelfVolume
    };
}

std::pair<int, int> ChannelPlayer::calculateSidPeriods(const LowLevelPsgInstrumentCell& cell, const int period, const int psgFrequency,
    const SidPlayerCapability& sidPlayerCapability) noexcept
{
    const auto sidBalancePercent = cell.getSidBalancePercent();
    const auto sidRatio = cell.getSidRatio();

    // Divides first, with the before-last division by 2 holding a carry.
    const auto isCarryA = (sidRatio == 0) ? false : ((static_cast<unsigned int>(period) & (1U << (static_cast<unsigned int>(sidRatio) - 1U))) != 0U);
    auto periodDivided = static_cast<int>((static_cast<unsigned int>(period) / (1U << static_cast<unsigned int>(sidRatio))) + (isCarryA ? 1 : 0));

    // Applies semi-tones. This is a trick, done by converting the period back to frequencies, adding the semi-tones,
    // then converting back to period.
    auto softwareFrequency = PsgPeriod::getFrequency(psgFrequency, periodDivided);
    if (const auto semiTonesInOctave = cell.getSidArpeggio(); semiTonesInOctave != 0) {
        softwareFrequency *= std::pow(2, semiTonesInOctave / 12.0);
        periodDivided = static_cast<int>((psgFrequency / 16.0) / softwareFrequency);
    }

    // Applies the pitch (inverted from UI).
    periodDivided = NumberUtil::correctNumber(periodDivided - cell.getSidPitch(), 0, 65535);     // Arbitrary limit.

    // Ratio +1 (AT3 = 3).
    //const auto isCarryA = (static_cast<unsigned int>(periodA) & 3U) != 0U;
    //const auto periodDivided = (periodA / 8) + (isCarryA ? 1 : 0);

    // Ratio 0 (AT3 = 2).
    //const auto isCarryA = (static_cast<unsigned int>(periodA) & 2U) != 0U;
    //const auto periodDivided = (periodA / 4) + (isCarryA ? 1 : 0);

    // Ratio -1 (AT3 = 1).
    //const auto isCarryA = (static_cast<unsigned int>(periodA) & 1U) != 0U;
    //const auto periodDivided = (periodA / 2) + (isCarryA ? 1 : 0);

    // Ratio -2 (AT3 = 0).
    //const auto periodADivided = periodA;

    // Separates the high/low shelves, putting the carry on the low.
    const auto basePeriodAHighShelf = periodDivided / 2;
    const auto isCarryPeriodA = (static_cast<unsigned int>(periodDivided) & 1U) != 0U;
    const auto basePeriodALowShelf = basePeriodAHighShelf + (isCarryPeriodA ? 1 : 0);

    // Applies a balance. Note the min/max applied here, which is important according to the platform.
    return NumberUtil::calculateBalance(basePeriodAHighShelf, basePeriodALowShelf, sidBalancePercent,
        sidPlayerCapability.getMinimumPeriod(), sidPlayerCapability.getMaximumPeriod());
}

}   // namespace arkostracker
