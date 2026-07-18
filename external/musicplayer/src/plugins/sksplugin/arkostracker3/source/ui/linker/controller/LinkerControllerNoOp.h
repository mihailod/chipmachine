#pragma once

#include "LinkerController.h"

namespace arkostracker 
{

/** A LinkerController that does nothing. */
class LinkerControllerNoOp final : public LinkerController
{
public:
    explicit LinkerControllerNoOp(SongController& songController);

    /** Destructor. */
    ~LinkerControllerNoOp() override = default;

    void onParentViewCreated(BoundedComponent& parentView) override;
    void onParentViewFirstResize(juce::Component& parentView) override;
    void onParentViewDeleted() noexcept override;

    void onParentViewResized(int startX, int startY, int newWidth, int newHeight) noexcept override;
    void onWantToSetLoopStartOrEnd(OptionalInt newLoopStartPosition, OptionalInt newEndPosition) noexcept override;
    void onPatternItemClicked(int position, bool down, bool shift, bool control) noexcept override;
    void onPatternItemDoubleClicked(int position) noexcept override;
    void onWantToEditPosition(int position) override;
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
    void createNew() noexcept override;
    void editSelectedPosition() noexcept override;
    void editPositions(const std::set<int>& positionIndexes) noexcept override;
    void onWantToMoveSelection(bool forward) noexcept override;
    void onWantToMoveSelectionFast(bool forward) noexcept override;
    void onWantToMoveSelectionToLimit(bool forward) noexcept override;
    void markSelectionAsLoopStartAndEnd() noexcept override;
    void markLoopStart() noexcept override;
    void markEnd() noexcept override;
    void onWantToOpenPatternViewer() noexcept override;
    void onWantToIncreasePatternIndex(int step) noexcept override;

    void onWantToGrowSelection(bool forward) noexcept override;
    void onWantToGrowSelectionFast(bool forward) noexcept override;
    void onWantToGrowSelectionToLimit(bool forward) noexcept override;
    void copySelectionToClipboard() noexcept override;
    void cutSelectionToClipboard() noexcept override;
    void pasteClipboard() noexcept override;
    bool canCutSelection() const noexcept override;
    bool canDeleteSelection() const noexcept override;

    void getKeyboardFocus() noexcept override;

    bool isOneItemSelected(bool onlyOneAuthorized) const noexcept override;
};

}   // namespace arkostracker

