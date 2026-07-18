#include "EffectContextImpl.h"

#include "../../../../controllers/MainController.h"
#include "../../../../controllers/SongController.h"
#include "../../../../player/SongPlayer.h"
#include "../../../../song/CellLocationInPosition.h"
#include "../../../../song/ExpressionBuilder.h"
#include "../../../../song/Song.h"
#include "../../../../song/subsong/SubsongConstants.h"
#include "../../../../utils/NumberUtil.h"
#include "../../cells/CellEffectsChecker.h"
#include "model/LineContext.h"

//#define USE_EFFECT_CONTEXT_FULL_LOGS 1         // Else, too much noise.         NOLINT(*-macro-usage)

namespace arkostracker
{

EffectContextImpl::EffectContextImpl(SongController& pSongController, const bool pObserve) noexcept :
        songController(pSongController),
        observe(pObserve),
        targetSubsongId(),              // This is going to be an unknown ID, forcing the context.
        buildContextOperation(),
        mutexPositionsContext(),
        positionsContext_protectedByMutex(),
        isDirty(false),
        mustStop(false),
        modificationTimestamp()
{
    if (observe) {
        songController.getTrackObservers().addObserver(this);
        songController.getLinkerObservers().addObserver(this);
        songController.getExpressionObservers(true).addObserver(this);
        songController.getExpressionObservers(false).addObserver(this);
        songController.getSubsongMetadataObservers().addObserver(this);
        songController.getMainController().getSongPlayer().getSongPlayerObservers().addObserver(this);
    }
}

EffectContextImpl::~EffectContextImpl()
{
    if (observe) {
        songController.getTrackObservers().removeObserver(this);
        songController.getLinkerObservers().removeObserver(this);
        songController.getExpressionObservers(true).removeObserver(this);
        songController.getExpressionObservers(false).removeObserver(this);
        songController.getSubsongMetadataObservers().removeObserver(this);
        songController.getMainController().getSongPlayer().getSongPlayerObservers().removeObserver(this);
    }
    mustStop = true;
}


// TrackChangeObserver method implementations.
// ====================================================

void EffectContextImpl::onTrackDataChanged(const Id& subsongId)
{
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
        DBG("      EffectContext: onTrackDataChanged.");
#endif
    onChange(subsongId);
}

void EffectContextImpl::onTrackMetaDataChanged(const Id& /*subsongId*/)
{
    // Nothing to do.
}


// LinkerObserver method implementations.
// ====================================================

void EffectContextImpl::onLinkerPositionChanged(const Id& subsongId, int /*index*/, unsigned /*whatChanged*/)
{
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
        DBG("      EffectContext: onLinkerPositionChanged.");
#endif

    onChange(subsongId);
}

void EffectContextImpl::onLinkerPositionsChanged(const Id& subsongId, const std::unordered_set<int>& /*indexes*/, unsigned /*whatChanged*/)
{
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
        DBG("      EffectContext: onLinkerPositionsChanged.");
#endif
    onChange(subsongId);
}

void EffectContextImpl::onLinkerPositionsInvalidated(const Id& subsongId, const std::set<int>& /*highlightedItems*/)
{
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
        DBG("      EffectContext: onLinkerPositionsInvalidated.");
#endif
    onChange(subsongId);
}

void EffectContextImpl::onLinkerPatternInvalidated(const Id& subsongId, int /*patternIndex*/, unsigned /*whatChanged*/)
{
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
        DBG("      EffectContext: onLinkerPatternInvalidated.");
#endif
    onChange(subsongId);
}

// ExpressionChangeObserver method implementations.
// ====================================================

void EffectContextImpl::onExpressionChanged(const Id& /*expressionId*/, unsigned /*whatChanged*/)
{
    // Nothing to do.
}

void EffectContextImpl::onExpressionCellChanged(const Id& /*expressionId*/, int /*cellIndex*/, bool /*mustAlsoRefreshPastIndex*/)
{
    // Nothing to do.
}

void EffectContextImpl::onExpressionsInvalidated()
{
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
        DBG("      EffectContext: onExpressionsInvalidated.");
#endif

    onChange({ });
}

// SubsongMetadataObserver method implementations.
// ====================================================

void EffectContextImpl::onSubsongMetadataChanged(const Id& subsongId, const unsigned what)
{
    // Change in the PSG count? Metadata used in case the speed also changes.
    if (NumberUtil::isBitPresent(what, psgsData) || NumberUtil::isBitPresent(what, subsongMetadata)) {
        onChange(subsongId);
    }
}

// SongPlayer method implementations.
// ====================================================

void EffectContextImpl::onPlayerNewLocations(const Locations& locations) noexcept
{
    // This only interests us if the subsong HAS changed, so that we can rebuild the context on the newly selected subsong.
    if (locations.playStartLocation.getSubsongId() != targetSubsongId) {
        DBG("      EffectContext: Going to a new subsong. Building new queue.");
        queueBuild();
    }
}

// ====================================================

void EffectContextImpl::onChange(const OptionalId& subsongId) noexcept
{
    if (subsongId.isPresent() && (targetSubsongId != subsongId.getValueRef())) {
        DBG("      EffectContext: the Subsong ID is not the same, ignoring.");
        return;
    }

    modificationTimestamp = juce::Time::currentTimeMillis();

    queueBuild();
}


// ====================================================

void EffectContextImpl::queueBuild() noexcept
{
    targetSubsongId = songController.getCurrentSubsongId();

    if (buildContextOperation != nullptr) {
        isDirty = true;     // The currently processed operation must be abandoned and restarted.
        return;
    }

    buildContextOperation = std::make_unique<BackgroundOperation<std::unique_ptr<PositionsContext>>>([&] (std::unique_ptr<PositionsContext> result) {
        // Callback when all is done, on main thread.

        // Replaces the previous result.
        if (!mustStop) {
            const std::lock_guard lock(mutexPositionsContext);        // Lock!
            positionsContext_protectedByMutex = std::move(result);
            DBG("      EffectContext: DONE.");

#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
                if (positionsContext_protectedByMutex != nullptr) {
                    DBG("      EffectContext: PositionsContext size = " + juce::String(positionsContext_protectedByMutex->size()));
                } else {
                    DBG("      EffectContext: PositionsContext is null!!!");
                }
#endif
        }

        isDirty = false;
        buildContextOperation.reset();      // Must be performed at the end, else crash on Windows (mustStop access violation, strange).

    }, [&] {
        // WORKER THREAD.
        DBG("      EffectContext: Starting work.");

        auto tries = 20;            // Only a security, shouldn't be necessary.
        std::unique_ptr<PositionsContext> result;
        while ((result == nullptr) && (--tries > 0) && !mustStop) {
            result = buildContext();

            if (result == nullptr) {
                isDirty = false;        // Will start again, so the dirty flag is cleared.
                DBG("      EffectContext: Not a success (probably killed). Work restart. Remaining tries = " + juce::String(tries));
            }
        }

        jassert(tries > 0);     // Too many tries! Strange, shouldn't happen.

        return mustStop ? nullptr : std::move(result);
    });

    buildContextOperation->performOperation();
}


// ====================================================

LineContext EffectContextImpl::determineContext(const CellLocationInPosition& location, const juce::int64 now) const noexcept
{
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
        DBG("      EffectContext: determineContext.");
#endif

    const std::lock_guard lock(mutexPositionsContext);        // Lock!

    // If no data, returns a default context.
    if (positionsContext_protectedByMutex == nullptr) {
        // Shouldn't happen in normal conditions, but still called a few times on app start, while the process is first finished.
        return { };
    }

    // As a security, checks the subsong ID.
    if (targetSubsongId != location.getSubsongId()) {
        jassertfalse;               // The subsong is not the same as the context! Shouldn't happen, it should have been recalculated.
        return { };
    }

    // Gets the position data.
    const auto positionIndex = static_cast<size_t>(location.getPositionIndex());
    if (positionIndex >= positionsContext_protectedByMutex->size()) {
        jassertfalse;           // The position is out of bounds??
        return { };
    }
    const auto& channelToTrackContext = positionsContext_protectedByMutex->at(positionIndex);

    // Gets the channel data.
    const auto channelIndex = location.getChannelIndex();
    const auto it = channelToTrackContext.find(channelIndex);
    if (it == channelToTrackContext.cend()) {
        jassertfalse;           // No channel data? Happens when adding a PSG, but then the change should have been notified and a rebuild queued!
        return { };
    }

    // Gets the line data. This is the "main algorithm", but it is not enough in some case, which makes things more complex, sadly. See below.
    const auto& trackContext = it->second;
    const auto lineIndex = location.getLineIndex();
    auto initialResult = trackContext.getContext(lineIndex);

    // WARNING, special case:
    // ===============================
    // If an arp/pitch/volume has just been entered manually, the background process is started, but we need a LineContext NOW,
    // so what we get does not take in account the modification!
    // Solution:
    // =================
    // If the timestamp of modification is recent, rebuild a whole temporary Track, but uses the first item of the trackContext to have the previous
    // context and have a quick reference.

    const auto timeDifferenceMs = now - modificationTimestamp;
    if (timeDifferenceMs > timeDifferenceMsToConsiderModification) {
//#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
//        DBG("      EffectContext: latest modification is old: keep the determined context.");
//#endif
        return initialResult;
    }
#ifdef USE_EFFECT_CONTEXT_FULL_LOGS
    DBG("      EffectContext: Diff Modification: " + juce::String(timeDifferenceMs));
    DBG("      EffectContext: latest modification is very recent: rebuild and use a temp track.");
#endif

    // The data is being recalculated by a background operation, but we need a result now. So, we rebuild the context of
    // the current position, it is a fast operation.

    TrackContext newTrackContext;
    const auto firstLineContext = trackContext.getContext(0);
    if (!firstLineContext.isFull()) {
        jassertfalse;       // Context not known on the first line? Shouldn't happen.
        return initialResult;
    }

    auto volumeSlide = trackContext.getInitialVolumeSlide();
    auto speed = trackContext.getInitialSpeed();

    const auto subsongId = location.getSubsongId();

    const auto& arpeggioHandler = songController.getExpressionHandler(true);
    const auto& pitchHandler = songController.getExpressionHandler(false);

    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto position = subsong.getPosition(static_cast<int>(positionIndex));
        const auto height = position.getHeight();
        const auto& pattern = subsong.getPatternRef(static_cast<int>(positionIndex));
        const auto trackIndex = pattern.getCurrentTrackIndex(channelIndex);
        const auto& track = subsong.getTrackRefFromIndex(trackIndex);

        const auto speedTrackIndex = pattern.getCurrentSpecialTrackIndex(true);
        const auto& speedTrack = subsong.getSpecialTrackRefFromIndex(speedTrackIndex, true);

        auto currentLineContext = firstLineContext;

        parseTrackAndFillTrackContext(height, speed, volumeSlide, currentLineContext, track, speedTrack, newTrackContext, arpeggioHandler, pitchHandler);
    });

    return newTrackContext.getContext(lineIndex);
}

std::unique_ptr<EffectContextImpl::PositionsContext> EffectContextImpl::buildContext() const noexcept
{
    // WORKER THREAD.
    // The action will be stopped as soon as the "dirty" flag is set!

    const auto subsongId = songController.getCurrentSubsongId();

    // In order to keep the lock as short as possible, the operation is split into several passes.
    // First, gets the positions.
    std::vector<Position> positions;
    auto channelCount = 0;
    auto speed = SubsongConstants::defaultSpeed;
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        positions = subsong.getPositions(true);
        channelCount = subsong.getChannelCount();
        speed = subsong.getMetadata().getInitialSpeed();
    });

    // Security in case the Subsong doesn't exist anymore.
    if (positions.empty() || (channelCount <= 0) || isDirty || mustStop) {
        jassertfalse;
        return nullptr;
    }

    const auto& pitchHandler = songController.getExpressionHandler(false);
    const auto& arpeggioHandler = songController.getExpressionHandler(true);

    // Forces the default context to no pitch/arp and a default volume, as it is what we hear when starting a Song.
    const auto defaultLineContext = buildDefaultLineContext(arpeggioHandler, pitchHandler);

    // Builds a default context for each channel. Each will evolve as the song is browsed.
    std::map<int, LineContext> channelToCurrentLineContext;
    std::map<int, FpFloat> channelToVolumeSlide;
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        channelToCurrentLineContext.insert({ channelIndex, defaultLineContext });
        channelToVolumeSlide.insert({ channelIndex, FpFloat() });       // For now, 0 because no volume slide.
    }
    auto positionIndexToChannelToTrackContext = std::make_unique<PositionsContext>();

    // Reads every position.
    for (const auto& position : positions) {
        if (isDirty || mustStop) {
            return nullptr;
        }

        const auto height = position.getHeight();

        // Resets the Tracks for each position.
        std::map<int, TrackContext> channelToTrackContext;
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            channelToTrackContext.insert({ channelIndex, TrackContext() });
        }

        SpecialTrack speedTrack;
        std::vector<Track> tracks;
        auto channelCountToUse = 0;

        // Gets a copy of Tracks as fast as possible and exits the lock.
        songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            const auto localPatternIndex = position.getPatternIndex();
            const auto& pattern = subsong.getPatternFromIndex(localPatternIndex);
            const auto channelCountInPattern = pattern.getChannelCount();
            jassert(channelCount == channelCountInPattern);
            channelCountToUse = std::min(channelCount, channelCountInPattern);   // Just a security.

            const auto speedTrackIndex = pattern.getCurrentSpecialTrackIndex(true);
            speedTrack = subsong.getSpecialTrackRefFromIndex(speedTrackIndex, true);

            for (auto channelIndex = 0; channelIndex < channelCountToUse; ++channelIndex) {
                const auto trackIndex = pattern.getCurrentTrackIndex(channelIndex);
                const auto& track = subsong.getTrackRefFromIndex(trackIndex);
                tracks.push_back(track);
            }
        });

        jassert(tracks.size() == static_cast<size_t>(channelCountToUse));
        for (auto channelIndex = 0; channelIndex < channelCountToUse; ++channelIndex) {
            auto& trackContext = channelToTrackContext.at(channelIndex);
            auto& currentLineContext = channelToCurrentLineContext.at(channelIndex);
            auto& volumeSlide = channelToVolumeSlide.at(channelIndex);

            // Keeps the volume slide and speed at the beginning of the track, for a faster look-up later (for the "special case" algorithm).
            trackContext.setInitialVolumeSlide(volumeSlide);
            trackContext.setInitialSpeed(speed);

            // Parses the song track and fills the Track Context.
            const auto& track = tracks.at(static_cast<size_t>(channelIndex));
            parseTrackAndFillTrackContext(height, speed, volumeSlide, currentLineContext, track, speedTrack,
                trackContext, arpeggioHandler, pitchHandler);
        }

        // Stores the data for this position.
        positionIndexToChannelToTrackContext->push_back(channelToTrackContext);
    }

    return positionIndexToChannelToTrackContext;
}

void EffectContextImpl::parseTrackAndFillTrackContext(const int height, int& speed, FpFloat& volumeSlide, LineContext& currentLineContext,
                                                      const Track& track, const SpecialTrack& speedTrack, TrackContext& trackContextToFill,
                                                      const ExpressionHandler& arpeggioHandler, const ExpressionHandler& pitchHandler) noexcept
{
    auto lineContextToFill = currentLineContext;

    for (auto lineIndex = 0; lineIndex < height; ++lineIndex) {
        // New speed?
        if (const auto newSpeed = speedTrack.getCellRefConst(lineIndex); !newSpeed.isEmpty()) {
            speed = newSpeed.getValue();
            jassert(speed > 0);
        }

        const auto& baseCell = track.getCellRefConst(lineIndex);

        // If there is a note, the volume slide stops (even if legato).
        if (baseCell.isNote()) {
            volumeSlide = 0;
        }

        // "Stacks up" the pitch/arp/inline in the line. This is important, so that a context is always
        // present in each encoded line. No need to go far when looking up.
        fillLineContextFromCell(baseCell, lineContextToFill, arpeggioHandler, pitchHandler, volumeSlide);

        // As an optimization, encodes the line context only if different.
        // Also, if first line of position, always encodes it. This will allow to always find a result when wanting a context in a position.
        if ((lineIndex == 0) || (lineContextToFill != currentLineContext)) {
            currentLineContext = lineContextToFill;
            trackContextToFill.setContext(lineIndex, lineContextToFill);
        }

        // Manages the volume slide. Must be managed AFTER the context is stored, to simulate the "play note" behavior.
        auto currentVolume = lineContextToFill.getVolume();
        const auto volumeSlideOnLine = volumeSlide * speed;
        currentVolume += volumeSlideOnLine;

        currentVolume.correctFrom0To(PsgValues::maximumVolumeNoHard);
        lineContextToFill.setVolume(currentVolume);
    }

    // After the track is parsed, the line context must be updated with the volume slide, else the next track volume
    // won't take it in account.
    currentLineContext = lineContextToFill;
}

LineContext EffectContextImpl::buildDefaultLineContext(const ExpressionHandler& arpeggioHandler, const ExpressionHandler& pitchHandler) noexcept
{
    constexpr auto defaultVolume = PsgValues::maximumVolumeNoHard;
    const auto pitchNoneId = pitchHandler.getExpressionCopy(0).getId();
    const auto arpeggioNoneId = arpeggioHandler.getExpressionCopy(0).getId();
    return { pitchNoneId, arpeggioNoneId, defaultVolume };
}

void EffectContextImpl::fillLineContextFromCell(const Cell& baseCell, LineContext& lineContextToFill, const ExpressionHandler& arpeggioHandler,
                                                const ExpressionHandler& pitchHandler, FpFloat& volumeSlide) noexcept
{
    // Normalizes the effects.
    const auto normalizedCellEffects = CellEffectsChecker::normalize(baseCell);

    // Reset? If yes, resets the whole line with default values. However, do NOT stop here, because more effects may be present afterward!
    if (const auto cellEffect = normalizedCellEffects.find(Effect::reset); cellEffect.isPresent()) {
        lineContextToFill = buildDefaultLineContext(arpeggioHandler, pitchHandler);
        // Fills the volume from the inverted volume.
        const auto volume = PsgValues::maximumVolumeNoHard - cellEffect.getValueRef().getEffectLogicalValue();
        jassert((volume >= PsgValues::minimumVolume) && (volume <= PsgValues::maximumVolumeNoHard));
        lineContextToFill.setVolume(FpFloat(static_cast<uint16_t>(volume)));

        volumeSlide.reset();
    }

    // Finds the Pitch table.
    if (const auto foundPitchId = searchForPitchTable(pitchHandler, normalizedCellEffects); foundPitchId.isPresent()) {
        lineContextToFill.setPitchId(foundPitchId.getValueRef());
    }
    // Finds the Arpeggio table (not inline!)
    const auto foundArpeggioId = searchForArpeggioTable(arpeggioHandler, normalizedCellEffects);
    if (foundArpeggioId.isPresent()) {
        lineContextToFill.setArpeggioIdAndResetInline(foundArpeggioId.getValueRef());
    }

    // Finds the inline arpeggio. Only possible if no arpeggio table was not found (they are exclusive, as the cell is normalized).
    if (foundArpeggioId.isAbsent()) {
        if (const auto foundArpeggioNotes = searchForInlineArpeggio(normalizedCellEffects); foundArpeggioNotes.isPresent()) {
            const auto& arpeggioNotes = foundArpeggioNotes.getValueRef();
            // If the arpeggio has no data, it means the user wants to stop it. In this case, switch to the Arpeggio table 0.
            if (!arpeggioNotes.hasOnlyZeros()) {
                lineContextToFill.setInlineArpeggioAndResetArpeggioTable(foundArpeggioNotes.getValueRef());
            } else {
                const auto arpeggio0IdOptional = arpeggioHandler.getId(0);
                if (arpeggio0IdOptional.isPresent()) {
                    lineContextToFill.setArpeggioIdAndResetInline(arpeggio0IdOptional.getValueRef());
                } else {
                    jassertfalse;       // Should never happen! 0 is always present!
                }
            }
        }
    }

    // Set volume.
    if (const auto cellEffect = normalizedCellEffects.find(Effect::volume); cellEffect.isPresent()) {
        lineContextToFill.setVolume(FpFloat(static_cast<uint16_t>(cellEffect.getValue().getEffectLogicalValue())));
        volumeSlide.reset();
    }
    // Volume out/in. The logic is the same as ChannelPlayer.
    if (const auto cellEffect = normalizedCellEffects.find(Effect::volumeOut); cellEffect.isPresent()) {
        const auto value = cellEffect.getValue().getEffectLogicalValue();
        volumeSlide.setFromDigits(value);
        volumeSlide.negate();      // Simpler than calculating the value here...
    }
    if (const auto cellEffect = normalizedCellEffects.find(Effect::volumeIn); cellEffect.isPresent()) {
        const auto value = cellEffect.getValue().getEffectLogicalValue();
        volumeSlide.setFromDigits(value);
    }
}

OptionalId EffectContextImpl::searchForExpressionTable(const ExpressionHandler& expressionHandler, const CellEffects& cellEffects, const Effect effectToFind) noexcept
{
    jassert((effectToFind == Effect::arpeggioTable) || (effectToFind == Effect::pitchTable));

    if (const auto cellEffect = cellEffects.find(effectToFind); cellEffect.isPresent()) {
        const auto foundPitchIndex = cellEffect.getValueRef().getEffectLogicalValue();

        // Does the pitch exist? If not, forces to pitch 0.
        auto foundPitchId = expressionHandler.find(foundPitchIndex);
        if (foundPitchId.isAbsent()) {
            foundPitchId = expressionHandler.find(0);
        }

        jassert(foundPitchId.isPresent());

        return foundPitchId;
    }

    return { };
}

OptionalId EffectContextImpl::searchForArpeggioTable(const ExpressionHandler& arpeggioHandler, const CellEffects& cellEffects) noexcept
{
    return searchForExpressionTable(arpeggioHandler, cellEffects, Effect::arpeggioTable);
}

OptionalId EffectContextImpl::searchForPitchTable(const ExpressionHandler& pitchHandler, const CellEffects& cellEffects) noexcept
{
    return searchForExpressionTable(pitchHandler, cellEffects, Effect::pitchTable);
}

OptionalValue<Expression> EffectContextImpl::searchForInlineArpeggio(const CellEffects& cellEffects) noexcept
{
    if (const auto foundArpeggio3Notes = searchForInlineArpeggio(cellEffects, Effect::arpeggio3Notes);
        foundArpeggio3Notes.isPresent()) {
        return foundArpeggio3Notes.getValueRef();
    }

    if (const auto foundArpeggio4Notes = searchForInlineArpeggio(cellEffects, Effect::arpeggio4Notes);
        foundArpeggio4Notes.isPresent()) {
        return foundArpeggio4Notes.getValueRef();
    }

    return { };
}

OptionalValue<Expression> EffectContextImpl::searchForInlineArpeggio(const CellEffects& cellEffects, const Effect effectToFind) noexcept
{
    jassert((effectToFind == Effect::arpeggio3Notes) || (effectToFind == Effect::arpeggio4Notes));

    if (const auto cellEffect = cellEffects.find(effectToFind); cellEffect.isPresent()) {
        const auto arpeggioDigits = cellEffect.getValueRef().getEffectLogicalValue();

        if (effectToFind == Effect::arpeggio3Notes) {
            return ExpressionBuilder::buildArpeggio3Notes(arpeggioDigits);
        }

        jassert(effectToFind == Effect::arpeggio4Notes);
        return ExpressionBuilder::buildArpeggio4Notes(arpeggioDigits);
    }

    return { };
}

}   // namespace arkostracker
