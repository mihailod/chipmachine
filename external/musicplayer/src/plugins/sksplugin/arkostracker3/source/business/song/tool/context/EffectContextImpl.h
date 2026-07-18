#pragma once

#include <memory>

#include "../../../../controllers/observers/ExpressionChangeObserver.h"
#include "../../../../controllers/observers/LinkerObserver.h"
#include "../../../../controllers/observers/SongPlayerObserver.h"
#include "../../../../controllers/observers/SubsongMetadataObserver.h"
#include "../../../../controllers/observers/TrackChangeObserver.h"
#include "../../../../song/cells/Effect.h"
#include "../../../../song/tracks/SpecialTrack.h"
#include "../../../../song/tracks/Track.h"
#include "../../../../ui/utils/backgroundOperation/BackgroundOperation.h"
#include "EffectContext.h"
#include "model/TrackContext.h"

namespace arkostracker
{

class ExpressionHandler;
class Cell;
class CellEffects;
class CellLocationInPosition;
class Song;
class SongController;

/** Implementation of the Effect Context. */
class EffectContextImpl final : public EffectContext,
                                public TrackChangeObserver,
                                public LinkerObserver,
                                public ExpressionChangeObserver,
                                public SubsongMetadataObserver,
                                public SongPlayerObserver
{
public:
    friend class EffectContextTest;

    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param observe true to start the observation on construction. As a convenience, use false in test units.
     */
    explicit EffectContextImpl(SongController& songController, bool observe = true) noexcept;

    /** Destructor. */
    ~EffectContextImpl() override;

    // EffectContext method implementations.
    // ====================================================
    LineContext determineContext(const CellLocationInPosition& location, juce::int64 now) const noexcept override;

    // LinkerObserver method implementations.
    // ====================================================
    void onLinkerPositionChanged(const Id& subsongId, int index, unsigned whatChanged) override;
    void onLinkerPositionsChanged(const Id& subsongId, const std::unordered_set<int>& indexes, unsigned whatChanged) override;
    void onLinkerPositionsInvalidated(const Id& subsongId, const std::set<int>& highlightedItems) override;
    void onLinkerPatternInvalidated(const Id& subsongId, int patternIndex, unsigned whatChanged) override;

    // TrackChangeObserver method implementations.
    // ====================================================
    void onTrackDataChanged(const Id& subsongId) override;
    void onTrackMetaDataChanged(const Id& subsongId) override;

    // ExpressionChangeObserver method implementations.
    // ====================================================
    void onExpressionChanged(const Id& expressionId, unsigned whatChanged) override;
    void onExpressionCellChanged(const Id& expressionId, int cellIndex, bool mustAlsoRefreshPastIndex) override;
    void onExpressionsInvalidated() override;

    // SubsongMetadataObserver method implementations.
    // ====================================================
    void onSubsongMetadataChanged(const Id& subsongId, unsigned what) override;

    // SongPlayer method implementations.
    // ====================================================
    void onPlayerNewLocations(const Locations& locations) noexcept override;

private:
    static constexpr auto timeDifferenceMsToConsiderModification = 100;       // Arbitrary...

    /** List of positions (a channel index linked to a Track Context). */
    using PositionsContext = std::vector<std::map<int, TrackContext>>;

    /**
     * Calls when a change has been detected. If the given subsong matches the target one, the queueBuild is called.
     * @param subsongId the possible SubsongId given by the change callback.
     */
    void onChange(const OptionalId& subsongId) noexcept;

    /** Call this from the main thread when a potential change happens (new subsong, or change in the subsong). This will (re)trigger the context building. */
    void queueBuild() noexcept;

    /**
     * Builds the context. CALLED FROM WORKER THREAD.
     * @return the context, or nullptr if could not, or the task was stopped.
     */
    std::unique_ptr<PositionsContext> buildContext() const noexcept;

    /**
     * Searches for the expression table in the given cell and returns the possible ID of the expression if a value was found.
     * @param expressionHandler the arpeggio/pitch handler.
     * @param cellEffects the cell effects. It should be normalized!
     * @param effectToFind the effect to find (arpeggio or pitch table. NOT inline arpeggio!).
     * @return the possible ID of the expression.
     */
    static OptionalId searchForExpressionTable(const ExpressionHandler& expressionHandler, const CellEffects& cellEffects, Effect effectToFind) noexcept;

    /**
     * Searches for the Arpeggio table in the given cell and returns the possible ID of the expression if a value was found.
     * @param arpeggioHandler the arpeggio handler.
     * @param cellEffects the cell effects. It should be normalized!
     * @return the possible ID of the expression.
     */
    static OptionalId searchForArpeggioTable(const ExpressionHandler& arpeggioHandler, const CellEffects& cellEffects) noexcept;

    /**
     * Searches for the Pitch table in the given cell and returns the possible ID of the expression if a value was found.
     * @param pitchHandler the pitch handler.
     * @param cellEffects the cell effects. It should be normalized!
     * @return the possible ID of the expression.
     */
    static OptionalId searchForPitchTable(const ExpressionHandler& pitchHandler, const CellEffects& cellEffects) noexcept;

    /**
     * Searches for an inline arpeggio, and returns it if found.
     * @param cellEffects the cell effects. It should be normalized!
     * @param effectToFind the effect to find (arpeggio 3/4 notes).
     */
    static OptionalValue<Expression> searchForInlineArpeggio(const CellEffects& cellEffects, Effect effectToFind) noexcept;

    /**
     * Searches for an inline arpeggio (both 3/4 notes), and returns it if found.
     * @param cellEffects the cell effects. It should be normalized!
     */
    static OptionalValue<Expression> searchForInlineArpeggio(const CellEffects& cellEffects) noexcept;

    /**
     * Fills the given LineContext from the possible effects from the given Cell.
     * @param cell the Cell. It will be normalized, no need to do it beforehand.
     * @param lineContextToFill the line context to fill.
     * @param arpeggioHandler the Expression handler for Arpeggio.
     * @param pitchHandler the Expression handler for Pitch.
     * @param volumeSlide the volume slide. May be modified in case of Reset or volume effect.
     */
    static void fillLineContextFromCell(const Cell& cell, LineContext& lineContextToFill, const ExpressionHandler& arpeggioHandler, const ExpressionHandler& pitchHandler,
        FpFloat& volumeSlide) noexcept;

    /**
     * @return a LineContext with default value (first Pitch and Arpeggio, default volume).
     * @param arpeggioHandler the Expression handler for Arpeggio.
     * @param pitchHandler the Expression handler for Pitch.
     */
    static LineContext buildDefaultLineContext(const ExpressionHandler& arpeggioHandler, const ExpressionHandler& pitchHandler) noexcept;

    /** Parses the Track and fills the given Track Context from it. */
    static void parseTrackAndFillTrackContext(int height, int& speed, FpFloat& volumeSlide, LineContext& currentLineContext,
        const Track& track, const SpecialTrack& speedTrack, TrackContext& trackContextToFill,
        const ExpressionHandler& arpeggioHandler, const ExpressionHandler& pitchHandler) noexcept;

    SongController& songController;
    bool observe;

    Id targetSubsongId;                                                             // The Subsong for which the context is built.
    std::unique_ptr<BackgroundOperation<std::unique_ptr<PositionsContext>>> buildContextOperation;

    // ==========================================
    mutable std::mutex mutexPositionsContext;
    std::unique_ptr<PositionsContext> positionsContext_protectedByMutex;            // Protected to avoid it being replaced when it is needed.
    // ==========================================

    mutable std::atomic_bool isDirty;
    std::atomic_bool mustStop;                                                      // Only a security for destruction.

    juce::int64 modificationTimestamp;
};

}   // namespace arkostracker
