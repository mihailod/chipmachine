#pragma once

#include <memory>

#include "LinkerView.h"
#include "../../../utils/OptionalValue.h"
#include "../../components/ViewPortInterceptKeys.h"
#include "../LinkerPanelAreaCommands.h"
#include "../dialogs/EditMarkerDialog.h"
#include "ItemsHolder.h"
#include "PatternItemView.h"

namespace arkostracker 
{

class LinkerViewImpl final : public LinkerView,
                             public LoopBar::Listener,
                             public PatternItemView::Listener,
                             public juce::ApplicationCommandTarget,
                             public ItemsHolder::ScrollListener
{
public:
    /**
     * Constructor.
     * @param linkerController the Controller to which give all the events., and that decides what to do.
     */
    explicit LinkerViewImpl(LinkerController& linkerController) noexcept;

    // Component method implementations.
    // ======================================
    std::unique_ptr<juce::ComponentTraverser> createKeyboardFocusTraverser() override;

    // LinkerView method implementations.
    // ======================================
    void resized() override;
    void refreshAllItems(const std::vector<PatternItemView::DisplayedData>& itemsData, const LoopBar::DisplayData& loopData,
                         const PlayLineView::DisplayedData& playLineData, const std::unordered_map<int, juce::String>& positionToMarker) noexcept override;
    void scrollToItem(int position) noexcept override;
    void editItems(const std::set<int>& positionIndexes) noexcept override;

    // ApplicationCommandTarget method implementations.
    // ================================================
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandId, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

    // StartEndView::Listener method implementations.
    // ===============================================
    void onUserWantsToSetLoopBarStart(int id, int newStart) noexcept override;
    void onUserWantsToSetLoopBarEnd(int id, int newEnd) noexcept override;
    void onUserWantsToToggleLoop(int id) noexcept override;

    // PatternItemView::Listener method implementations.
    // ===================================================
    void onPatternItemClicked(int position, bool down, bool shift, bool control) override;
    void onPatternItemDoubleClicked(int position) override;
    void onUserWantsToEditPosition(int position) override;
    void onPatternItemRightClicked(int position) override;
    void onWantToMoveOrDuplicatePatternItemView(int originalPosition, int destinationPosition, bool duplicate) override;
    void onMouseWheelOnPatternItem(int originalPosition, int offset) override;
    void onAddNewPositionClicked(int position) noexcept override;
    void onClonePositionClicked(int position) noexcept override;
    void onRepeatPositionClicked(int position) noexcept override;

    // ItemsHolder::ScrollListener method implementations.
    // ===================================================
    void manageScrollingIfNeeded(int mouseX) override;

private:

    /**
     * The user wants to set the loop start and/or end. This performs the action, if possible.
     * @param newLoopStartPosition the new loop start position, if present.
     * @param newEndPosition the new end position, if present.
     */
    void onWantToSetLoopStartOrEnd(OptionalInt newLoopStartPosition, OptionalInt newEndPosition) const noexcept;

    LinkerPanelAreaCommands linkerPanelAreaCommands;

    ViewPortInterceptKeys itemsViewport;
    ItemsHolder itemsHolder;                    // Where the positions etc. are displayed, inside the viewport.
};

}   // namespace arkostracker
