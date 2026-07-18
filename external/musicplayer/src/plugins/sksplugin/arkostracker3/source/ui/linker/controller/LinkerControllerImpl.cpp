#include "LinkerControllerImpl.h"

#include "../../../business/song/validation/CheckLoopStartEnd.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/PlayerController.h"
#include "../../../utils/NumberUtil.h"
#include "../../containerArranger/PanelSearcher.h"
#include "../views/LinkerViewImpl.h"
#include "../views/LinkerViewNoOp.h"

namespace arkostracker 
{

const int LinkerControllerImpl::itemCountForFast = 4;
const int LinkerControllerImpl::itemCountForLimit = 10000;            // Should be enough to simulate reaching the limits...

LinkerControllerImpl::LinkerControllerImpl(SongController& pSongController) noexcept :
        LinkerController(pSongController),
        linkerControllerDelegate(*this),
        selection(),
        linkerView(std::make_unique<LinkerViewNoOp>(*this)),
        playerController(pSongController.getMainController().getPlayerController()),
        currentlyShownLocation(pSongController.getCurrentSubsongId(), 0),
        cachedPlayedLocations(Location(currentlyShownLocation.getSubsongId(), 0),
                              Location(currentlyShownLocation.getSubsongId(), 0),
                              Location(currentlyShownLocation.getSubsongId(), 1), Location(currentlyShownLocation.getSubsongId(), 0)), // Default values.
        cachedItemsData(),
        cachedPositionToMarker()
{
    songController.getLinkerObservers().addObserver(this);                  // To know what the positions change.
    songController.getSubsongMetadataObservers().addObserver(this);         // To know when the loop changes.
    songController.getSongMetadataObservers().addObserver(this);            // To know when the subsong index changes.
    playerController.getSongPlayerObservers().addObserver(this);            // To know when the start/end/played location changes.
}

LinkerControllerImpl::~LinkerControllerImpl()
{
    songController.getLinkerObservers().removeObserver(this);
    songController.getSubsongMetadataObservers().removeObserver(this);
    songController.getSongMetadataObservers().removeObserver(this);
    playerController.getSongPlayerObservers().removeObserver(this);
}

void LinkerControllerImpl::onParentViewCreated(BoundedComponent& parentView)
{
    // We can now create a "real" LinkerView.
    linkerView = std::make_unique<LinkerViewImpl>(*this);

    parentView.addAndMakeVisible(*linkerView);
}

void LinkerControllerImpl::onParentViewFirstResize(juce::Component& parentView)
{
    jassert((parentView.getWidth() > 0) && (parentView.getHeight() > 0));   // The parent has not been resized yet... We cannot display items, so we can't scroll.
    (void)parentView;

    // Initializes the views.
    refreshAllItems();

    // Goes to where we were before.
    scrollToItem(currentlyShownLocation.getPosition());
}

void LinkerControllerImpl::onParentViewDeleted() noexcept
{
    // Goes back to a no-op LinkerView.
    linkerView = std::make_unique<LinkerViewNoOp>(*this);
}

void LinkerControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight) noexcept
{
    linkerView->setBounds(startX, startY, newWidth, newHeight);
}

bool LinkerControllerImpl::isOneItemSelected(const bool onlyOneAuthorized) const noexcept
{
    const auto count = selection.getSelectedCount();
    return onlyOneAuthorized ? (count == 1) : (count >= 1);
}


// LinkerObserver method implementations.
// ===============================================

void LinkerControllerImpl::onLinkerPositionChanged(const Id& subsongId, int /*index*/, unsigned int /*whatChanged*/)
{
    if (subsongId == getShownSubsongId()) {
        // Selection not changed, so index ignored. When shift+inc/dec the position with a selection, looks ugly.
        refreshAllItems();
    }
}

void LinkerControllerImpl::onLinkerPositionsChanged(const Id& subsongId, const std::unordered_set<int>& /*indexes*/, unsigned int /*whatChanged*/)
{
    if (subsongId == getShownSubsongId()) {
        refreshAllItems();          // Called when markers are moved, which behavior has been disabled for now.
    }
}

void LinkerControllerImpl::onLinkerPositionsInvalidated(const Id& subsongId, const std::set<int>& highlightedItems)
{
    if (subsongId == getShownSubsongId()) {
        // Goes to the first returned item, makes the selection goes to it.
        // This is the best compromise between "handy" and "not to messy on undo/redo".
        const auto position = highlightedItems.empty() ? 0 : *highlightedItems.cbegin();
        selection.selectOnly(position);
        moveCurrentLocationTo(position);
    }
}

void LinkerControllerImpl::onLinkerPatternInvalidated(const Id& subsongId, int /*patternIndex*/, unsigned int /*whatChanged*/)
{
    if (subsongId == getShownSubsongId()) {
        // A Pattern color may be used at various Position, so updates everything, simpler.
        refreshAllItems();
    }
}


// SubsongMetadataObserver method implementations.
// ===============================================

void LinkerControllerImpl::onSubsongMetadataChanged(const Id& subsongId, const unsigned int what)
{
    if ((subsongId != getShownSubsongId()) || !NumberUtil::isBitPresent(what, SubsongMetadataObserver::What::loop)) {
        return;
    }

    // Updates all the Items (simpler).
    refreshAllItems();
}


// SongMetadataObserver method implementations.
// ===============================================

void LinkerControllerImpl::onSongMetadataChanged(const unsigned int what)
{
    // Called if goto new Subsong, or click on a position.
    if (!NumberUtil::isBitPresent(what, SongMetadataObserver::What::shownLocation)) {
        return;
    }
    // No change? Then exits.
    const auto newCurrentlyViewedLocation = songController.getCurrentViewedLocation();
    const auto oldShownLocation = currentlyShownLocation;
    if (oldShownLocation == newCurrentlyViewedLocation) {
        return;
    }
    currentlyShownLocation = newCurrentlyViewedLocation;

    // New Subsong?
    const auto newSubsong = newCurrentlyViewedLocation.getSubsongId();
    const auto isNewSubsong = (newSubsong != oldShownLocation.getSubsongId());

    refreshAllItems();

    // Scrolls to the start of the Subsong, if new (there is no more selection when changing Subsong).
    if (isNewSubsong) {
        unselectItems();
        moveCurrentLocationTo(0);
    } else {
        // If moving from the PV, makes sure the new location is visible.
        scrollToItem(newCurrentlyViewedLocation.getPosition());
    }
}


// SongPlayerObserver method implementations.
// ===============================================

void LinkerControllerImpl::onPlayerNewLocations(const SongPlayerObserver::Locations& locations) noexcept
{
    cachedPlayedLocations = locations;
    const auto samePosition = currentlyShownLocation.hasSamePosition(locations.playedLocation);

    currentlyShownLocation = locations.playedLocation;      // If playing, what is played is what is shown.

    refreshAllItems(!samePosition);     // If we are still on the same position, no need to fetch the Positions.

    // Makes sure the Position is visible.
    if (!samePosition) {
        scrollToItem(locations.playedLocation.getPosition());
    }
}

void LinkerControllerImpl::onWantToSetLoopStartOrEnd(const OptionalInt optionalNewLoopStartPosition, const OptionalInt optionalNewEndPosition) noexcept
{
    jassert(optionalNewLoopStartPosition.isPresent() || optionalNewEndPosition.isPresent());  // At least one change!

    // Gets the current loop, corrects it.
    const auto loopData = getLoopData();
    const auto [loopStartPosition, endPosition] = CheckLoopStartEnd::checkLoopStartEnd(loopData.loopStart, loopData.end,
                                                                 loopData.length, optionalNewLoopStartPosition, optionalNewEndPosition);

    songController.setLoopStartAndEnd(getShownSubsongId(), loopStartPosition, endPosition);
}

void LinkerControllerImpl::onPatternItemClicked(const int position, const bool down, const bool shift, const bool control) noexcept
{
    const auto isSelected = selection.isSelected(position);
    const auto up = !down;

    if (shift) {
        growSelectionTo(position);
    } else if (control) {
        if (up) {           // A bit of a trick, but a CTRL only works if UP. This allows CTRL+drag.
            toggleItemSelection(position);
        }
    } else {
        // If not selected, the "down" doesn't affect the behavior.
        // If selected, the down does not do anything, but the up does. This allows for drag'n'drop on selected item*s*.
        if (isSelected && down) {
            return;
        }
        selectItem(position);
    }
}

void LinkerControllerImpl::onPatternItemDoubleClicked(const int /*position*/) noexcept
{
    onWantToOpenPatternViewer();
}

void LinkerControllerImpl::onWantToOpenPatternViewer() noexcept
{
    if (auto* panelSearcher = songController.getMainController().getPanelSearcher(); panelSearcher != nullptr) {
        panelSearcher->searchAndOpenPanel(PanelType::patternViewer);
    }
}

void LinkerControllerImpl::onWantToEditPosition(const int positionIndex)
{
    linkerView->editItems({ positionIndex });
}

void LinkerControllerImpl::onWantToEditPositions(const std::set<int>& positionIndexes)
{
    linkerView->editItems(positionIndexes);
}

void LinkerControllerImpl::onPatternItemRightClicked(const int position) noexcept
{
    linkerControllerDelegate.onPatternItemRightClicked(position);
}

void LinkerControllerImpl::onAddNewPositionClicked(const int position) noexcept
{
    songController.createNewPositionAndPattern(getShownSubsongId(), position);
}

void LinkerControllerImpl::onClonePositionClicked(const int position) noexcept
{
    songController.clonePositions(getShownSubsongId(), { position }, position, true);
}

void LinkerControllerImpl::onRepeatPositionClicked(const int position) noexcept
{
    songController.duplicatePositions(getShownSubsongId(), { position }, position, true, false);
}

void LinkerControllerImpl::onWantToMoveOrDuplicateSelectedPositions(const int originalPosition, const int destinationPosition, const bool duplicate)
{
    const auto shownSubsongId = getShownSubsongId();
    const auto selectedItems = selection.getSelectedItems();
    const auto insertAfter = destinationPosition > originalPosition;
    if (duplicate) {
        songController.duplicatePositions(shownSubsongId, selectedItems, destinationPosition, insertAfter, false);
    } else {
        songController.movePositions(shownSubsongId, selectedItems, destinationPosition, insertAfter);
    }
}

void LinkerControllerImpl::onWantToIncDecPatternItemValue(const int position, const int offset)
{
    linkerControllerDelegate.onWantToIncDecPatternItemValue(position, offset);
}

void LinkerControllerImpl::scrollToItem(const int position) const noexcept
{
    linkerView->scrollToItem(position);
}


// ===============================================

LinkerControllerImpl::LoopData LinkerControllerImpl::getLoopData() const noexcept
{
    int loopStartPosition;      // NOLINT(*-init-variables)
    int endPosition;            // NOLINT(*-init-variables)
    int length;                 // NOLINT(*-init-variables)
    songController.performOnConstSubsong(getShownSubsongId(), [&](const Subsong& subsong) {
        loopStartPosition = subsong.getLoopStartPosition();
        endPosition = subsong.getEndPosition();
        length = subsong.getLength();
    });

    return { loopStartPosition, endPosition, length };
}

void LinkerControllerImpl::refreshAllItems(const bool fetchPositionData) noexcept
{
    // These data comes from the player.
    const auto currentlyPlayedLine = cachedPlayedLocations.playedLocation.getLine();
    const auto currentlyPlayedPosition = cachedPlayedLocations.playedLocation.getPosition();
    const auto playStartPosition = cachedPlayedLocations.loopStartLocation.getPosition();
    const auto& pastEndLocation = cachedPlayedLocations.pastEndLocation.getPosition();
    // A bit hackish... The past end only reflects the position. If playing a block, it is the same as the play position.
    const auto playEndPosition = std::max(pastEndLocation - 1, playStartPosition);

    std::vector<PatternItemView::DisplayedData> newItemsData;
    std::unordered_map<int, juce::String> newPositionToMarker;
    // Uses the cache data, or the data to be built below.
    const auto& itemsDataToUse = fetchPositionData ? newItemsData : cachedItemsData;
    const auto& positionToMarkerToUse = fetchPositionData ? newPositionToMarker : cachedPositionToMarker;

    int loopStartPosition;      // NOLINT(*-init-variables)
    int endPosition;            // NOLINT(*-init-variables)
    auto positionCount = 0;
    auto currentPatternHeight = TrackConstants::defaultPositionHeight;
    songController.performOnConstSubsong(getShownSubsongId(), [&](const Subsong& subsong) {
        loopStartPosition = subsong.getLoopStartPosition();
        endPosition = subsong.getEndPosition();

        currentPatternHeight = subsong.getPositionHeight(currentlyPlayedPosition);
        positionCount = subsong.getLength();

        // Builds the positions only if necessary.
        if (fetchPositionData) {
            //DBG("FETCH POSITION DATA");
            const auto positions = subsong.getPositions(false);
            jassert(positionCount == static_cast<int>(positions.size()));       // Abnormal!
            auto positionIndex = 0;
            for (const auto& position : positions) {
                const auto patternIndexInPosition = position.getPatternIndex();
                const auto patternColor = juce::Colour(subsong.getPatternColor(patternIndexInPosition));
                const auto withTransposition = position.isTranspositionUsed();
                const auto selected = selection.contains(positionIndex);
                const auto current = (positionIndex == currentlyShownLocation.getPosition());
                const auto withinPlayRange = (positionIndex <= endPosition);
                const PatternItemView::DisplayedData itemData(positionIndex, patternIndexInPosition, patternColor, withinPlayRange, withTransposition, selected, current);
                newItemsData.push_back(itemData);

                // Any marker?
                const auto& markerName = position.getMarkerName();
                if (markerName.isNotEmpty()) {
                    newPositionToMarker.insert({ positionIndex, markerName  });
                }

                ++positionIndex;
            }
            // Stores the newly generated data for later.
            cachedItemsData = newItemsData;
            cachedPositionToMarker = newPositionToMarker;
        }
    });

    jassert((positionCount > 0) && !itemsDataToUse.empty());
    const PlayLineView::DisplayedData playLineData(positionCount, playStartPosition, playEndPosition,
                                             currentlyPlayedPosition, currentPatternHeight, currentlyPlayedLine);

    linkerView->refreshAllItems(itemsDataToUse, LoopBar::DisplayData(true, loopStartPosition, endPosition),
                                playLineData, positionToMarkerToUse);
}

void LinkerControllerImpl::unselectItems() noexcept
{
    if (selection.isEmpty()) {
        return;
    }
    selection.clear();
    refreshAllItems();
}

void LinkerControllerImpl::selectItem(const int position) noexcept
{
    selectAndMoveCurrentLocationTo(position);
}

void LinkerControllerImpl::toggleItemSelection(const int position) noexcept
{
    selection.toggle(position);
    refreshAllItems();
}

void LinkerControllerImpl::growSelectionTo(const int position) noexcept
{
    selection.growTo(position);
    refreshAllItems();
}

void LinkerControllerImpl::growSelectionOf(const int count) noexcept
{
    const auto endPosition = getEndPosition();
    selection.growOf(count, 0, endPosition);
    refreshAllItems();
}

void LinkerControllerImpl::duplicateSelectedPositions() noexcept
{
    const auto selectedPositions = selection.getSelectedItems();
    const auto position = selection.getHighest();         // Seems the most sensible thing to do...
    songController.duplicatePositions(getShownSubsongId(), selectedPositions, position, true, false);
}

void LinkerControllerImpl::createNew() noexcept
{
    const auto position = selection.getHighest();
    songController.createNewPositionAndPattern(getShownSubsongId(), position);
}

void LinkerControllerImpl::cloneSelectedPositions() noexcept
{
    const auto selectedPositions = selection.getSelectedItems();
    const auto position = selection.getHighest();         // Seems the most sensible thing to do...
    songController.clonePositions(getShownSubsongId(), selectedPositions, position, true);
}

void LinkerControllerImpl::deleteSelectedPositions() noexcept
{
    // Deleting the whole song is forbidden.
    if (isWholeSongSelected()) {
        return;
    }

    const auto& selectedPositions = selection.getSelectedItems();
    songController.deletePositions(getShownSubsongId(), selectedPositions, true);
}

void LinkerControllerImpl::editSelectedPosition() noexcept
{
    if (selection.isEmpty()) {
        return;
    }
    editPositions(selection.getSelectedItems());
}

void LinkerControllerImpl::editPositions(const std::set<int>& positionIndexes) noexcept
{
    linkerControllerDelegate.showEditPositionDialog(positionIndexes);
}

void LinkerControllerImpl::onWantToMoveSelection(const bool forward) noexcept
{
    moveCurrentLocationOf(forward ? 1 : -1);
}

void LinkerControllerImpl::onWantToMoveSelectionFast(const bool forward) noexcept
{
    moveCurrentLocationOf(forward ? itemCountForFast : -itemCountForFast);
}

void LinkerControllerImpl::onWantToMoveSelectionToLimit(const bool forward) noexcept
{
    moveCurrentLocationOf(forward ? itemCountForLimit : -itemCountForLimit);
}

void LinkerControllerImpl::onWantToGrowSelection(const bool forward) noexcept
{
    growSelectionOf(forward ? 1 : -1);
}

void LinkerControllerImpl::onWantToGrowSelectionFast(const bool forward) noexcept
{
    growSelectionOf(forward ? itemCountForFast : -itemCountForFast);
}

void LinkerControllerImpl::onWantToGrowSelectionToLimit(const bool forward) noexcept
{
    growSelectionOf(forward ? itemCountForLimit : -itemCountForLimit);
}

void LinkerControllerImpl::markSelectionAsLoopStartAndEnd() noexcept
{
    if (selection.isEmpty()) {
        return;
    }
    const auto lowest = selection.getLowest();
    const auto highest = selection.getHighest();
    jassert(lowest <= highest);

    onWantToSetLoopStartOrEnd(lowest, highest);
}

void LinkerControllerImpl::markLoopStart() noexcept
{
    if (selection.isEmpty()) {
        return;
    }
    const auto lowest = selection.getLowest();
    onWantToSetLoopStartOrEnd(lowest, OptionalInt());
}

void LinkerControllerImpl::markEnd() noexcept
{
    if (selection.isEmpty()) {
        return;
    }
    const auto highest = selection.getHighest();
    onWantToSetLoopStartOrEnd(OptionalInt(), highest);
}

void LinkerControllerImpl::onWantToIncreasePatternIndex(const int step) noexcept
{
    if (!selection.isSingle()) {
        return;
    }
    const auto selectedPositionIndex = selection.getLowest();
    songController.increasePatternIndex(getShownSubsongId(), selectedPositionIndex, step);
}

void LinkerControllerImpl::copySelectionToClipboard() noexcept
{
    linkerControllerDelegate.copySelectionToClipboard();
}

void LinkerControllerImpl::cutSelectionToClipboard() noexcept
{
    linkerControllerDelegate.cutSelectionToClipboard();
}

void LinkerControllerImpl::pasteClipboard() noexcept
{
    linkerControllerDelegate.pasteClipboard();
}

bool LinkerControllerImpl::canCutSelection() const noexcept
{
    return !isWholeSongSelected();
}

bool LinkerControllerImpl::canDeleteSelection() const noexcept
{
    return !isWholeSongSelected();
}

void LinkerControllerImpl::getKeyboardFocus() noexcept
{
    linkerView->grabKeyboardFocus();
}


// ========================================================

bool LinkerControllerImpl::isWholeSongSelected() const noexcept
{
    const auto& selectedPositions = selection.getSelectedItems();
    return (selectedPositions.size() >= cachedItemsData.size());
}

void LinkerControllerImpl::moveCurrentLocationOf(const int rankToMove) noexcept
{
    const auto desiredPosition = getShownPosition() + rankToMove;
    selectAndMoveCurrentLocationTo(desiredPosition);
}

void LinkerControllerImpl::selectAndMoveCurrentLocationTo(int position) noexcept
{
    position = correctPosition(position);

    selection.selectOnly(position);
    moveCurrentLocationTo(position);
}

void LinkerControllerImpl::moveCurrentLocationTo(int position) noexcept
{
    position = correctPosition(position);

    // Changes only the position. The line remains, more user-friendly (AT2-compliant).
    const auto currentLine = songController.getCurrentlyShownLine();        // Hack, because moving in the PV does not notify anything.
    songController.setCurrentLocation(Location(getShownSubsongId(), position, currentLine), false);
    scrollToItem(position);
    refreshAllItems();      // To show the selection.
}

int LinkerControllerImpl::correctPosition(const int inputPosition) const noexcept
{
    const auto endPosition = getLoopData().length - 1;
    return NumberUtil::correctNumber(inputPosition, 0, endPosition);
}

int LinkerControllerImpl::getEndPosition() const noexcept
{
    return getLoopData().length - 1;
}

int LinkerControllerImpl::getShownPosition() const noexcept
{
    return currentlyShownLocation.getPosition();
}

Id LinkerControllerImpl::getShownSubsongId() const noexcept
{
    return currentlyShownLocation.getSubsongId();
}


}   // namespace arkostracker

