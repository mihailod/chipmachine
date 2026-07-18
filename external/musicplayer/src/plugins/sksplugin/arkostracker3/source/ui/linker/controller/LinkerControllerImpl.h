#pragma once

#include <unordered_map>
#include <unordered_set>

#include "../../../controllers/observers/LinkerObserver.h"
#include "../../../controllers/observers/SongMetadataObserver.h"
#include "../../../controllers/observers/SongPlayerObserver.h"
#include "../../../controllers/observers/SubsongMetadataObserver.h"
#include "../../utils/LinearSelection.h"
#include "../views/LinkerView.h"
#include "LinkerController.h"
#include "LinkerControllerDelegate.h"

namespace arkostracker 
{

class PlayerController;

class LinkerControllerImpl final : public LinkerController,
                                   public LinkerObserver,
                                   public SubsongMetadataObserver,
                                   public SongMetadataObserver,
                                   public SongPlayerObserver
{
public:
    friend class LinkerControllerDelegate;         // Some behaviors are put in this class.

    /**
     * Constructor.
     * @param songController to reach the Song.
     */
    explicit LinkerControllerImpl(SongController& songController) noexcept;

    /** Destructor. */
    ~LinkerControllerImpl() override;

    // LinkerController method implementations.
    // =========================================
    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;
    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) noexcept override;
    bool isOneItemSelected(bool onlyOneAuthorized) const noexcept override;
    void onWantToSetLoopStartOrEnd(OptionalInt newLoopStartPosition, OptionalInt newEndPosition) noexcept override;
    void onPatternItemClicked(int position, bool down, bool shift, bool control) noexcept override;
    void onPatternItemDoubleClicked(int position) noexcept override;
    void onWantToOpenPatternViewer() noexcept override;
    void onWantToEditPosition(int positionIndex) override;
    void onWantToEditPositions(const std::set<int>& positionIndexes) override;
    void onPatternItemRightClicked(int position) noexcept override;
    void onAddNewPositionClicked(int position) noexcept override;
    void onClonePositionClicked(int position) noexcept override;
    void onRepeatPositionClicked(int position) noexcept override;
    void onWantToMoveOrDuplicateSelectedPositions(int originalPosition, int destinationPosition, bool duplicate) override;
    void onWantToIncDecPatternItemValue(int position, int offset) override;
    void duplicateSelectedPositions() noexcept override;
    void cloneSelectedPositions() noexcept override;
    void deleteSelectedPositions() noexcept override;
    void editSelectedPosition() noexcept override;
    void createNew() noexcept override;
    void editPositions(const std::set<int>& positionIndexes) noexcept override;

    void onWantToMoveSelection(bool forward) noexcept override;
    void onWantToMoveSelectionFast(bool forward) noexcept override;
    void onWantToMoveSelectionToLimit(bool forward) noexcept override;
    void onWantToGrowSelection(bool forward) noexcept override;
    void onWantToGrowSelectionFast(bool forward) noexcept override;
    void onWantToGrowSelectionToLimit(bool forward) noexcept override;
    void markSelectionAsLoopStartAndEnd() noexcept override;
    void markLoopStart() noexcept override;
    void markEnd() noexcept override;
    void onWantToIncreasePatternIndex(int step) noexcept override;

    void copySelectionToClipboard() noexcept override;
    void cutSelectionToClipboard() noexcept override;
    void pasteClipboard() noexcept override;
    bool canCutSelection() const noexcept override;
    bool canDeleteSelection() const noexcept override;

    void getKeyboardFocus() noexcept override;

    // LinkerObserver method implementations.
    // ===============================================
    void onLinkerPositionChanged(const Id& subsongId, int index, unsigned int whatChanged) override;
    void onLinkerPositionsChanged(const Id& subsongId, const std::unordered_set<int>& indexes, unsigned int whatChanged) override;
    void onLinkerPositionsInvalidated(const Id& subsongId, const std::set<int>& highlightedItems) override;
    void onLinkerPatternInvalidated(const Id& subsongId, int patternIndex, unsigned int whatChanged) override;

    // SubsongMetadataObserver method implementations.
    // ===============================================
    void onSubsongMetadataChanged(const Id& subsongId, unsigned int what) override;

    // SongMetadataObserver method implementations.
    // ===============================================
    void onSongMetadataChanged(unsigned int what) override;

    // SongPlayerObserver method implementations.
    // ==============================================
    void onPlayerNewLocations(const Locations& locations) noexcept override;

private:
    static const int itemCountForFast;
    static const int itemCountForLimit;

    /** Simple holder of loop start/end and song length. */
    class LoopData
    {
    public:
        LoopData(const int pLoopStart, const int pEnd, const int pLength) :
                loopStart(pLoopStart),
                end(pEnd),
                length(pLength)
        {
        }
        const int loopStart;
        const int end;
        const int length;
    };

    /** @return the loop data of the current Subsong. */
    LoopData getLoopData() const noexcept;

    /**
     * Updates all the items. The locations used are the ones already given by the players.
     * @param fetchPositionData if true, the positions/patterns data is fetched. If false, cached data is used.
     */
    void refreshAllItems(bool fetchPositionData = true) noexcept;

    /** Un-selects the items. If does not notify anything. */
    void unselectItems() noexcept;
    /**
     * Selects an item. This performs a "go to".
     * @param position the position of the selected item.
     */
    void selectItem(int position) noexcept;
    /**
     * Toggles the selection of an item.
     * @param position the position of the item.
     */
    void toggleItemSelection(int position) noexcept;
    /**
     * Grows the selection to an item. Nothing happens if there was nothing already selected.
     * @param position the position of the item.
     */
    void growSelectionTo(int position) noexcept;

    /**
     * Grows the selection of a specific count. Nothing happens if there was nothing already selected, or if the selection
     * cannot grow.
     * @param count how much to grow.
     */
    void growSelectionOf(int count) noexcept;

    /**
     * Sets the currently seen location to a specific position, and selects it. If the new position is illegal, is it corrected.
     * Provokes a notification if a change is done.
     * @param position position.
     */
    void selectAndMoveCurrentLocationTo(int position) noexcept;

    /**
     * Sets the currently seen location to a specific position. If the new position is illegal, is it corrected.
     * Provokes a notification if a change is done.
     * @param position position.
     */
    void moveCurrentLocationTo(int position) noexcept;

    /**
     * Tries to move the current location by rank, with a test to limits. Provokes a notification.
     * @param rankToMove how many ranks to move from. Can be negative.
     */
    void moveCurrentLocationOf(int rankToMove) noexcept;

    /**
     * Corrects the given position according to the Subsong length limit.
     * @param inputPosition the position to correct.
     * @return the corrected position.
     */
    int correctPosition(int inputPosition) const noexcept;

    /** @return the currently shown position. */
    int getShownPosition() const noexcept;

    /** @return the end position. */
    int getEndPosition() const noexcept;

    /** @return the ID of the currently shown Subsong. */
    Id getShownSubsongId() const noexcept;

    /** @return true if the selection encompasses the whole Song. */
    bool isWholeSongSelected() const noexcept;

    /**
     * Ensures the given position is visible.
     * @param position the position.
     */
    void scrollToItem(int position) const noexcept;

    LinkerControllerDelegate linkerControllerDelegate;      // Friend class to which give the responsibility to make some specific actions.
    LinearSelection selection;                              // The Positions that are selected.

    std::unique_ptr<LinkerView> linkerView;                 // The linkerView. Maybe a no-op at first or if the Panel is destroyed!

    PlayerController& playerController;                     // The player controller.

    Location currentlyShownLocation;                        // What is shown. May be different from what is being played. Returns by the observer.
    Locations cachedPlayedLocations;    // Locations returned by the Player observer.
    std::vector<PatternItemView::DisplayedData> cachedItemsData;        // The position/patterns displayed.
    std::unordered_map<int, juce::String> cachedPositionToMarker;       // The position to marker displayed.
};

}   // namespace arkostracker
