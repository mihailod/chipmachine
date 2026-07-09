#include "Toolbox.h"

#include "../../../../utils/PsgValues.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

Toolbox::Toolbox(ToolboxController& pToolboxController) noexcept :
        displayedData(ToolboxController::ToolType::remapInstrument, juce::String(), juce::String(), false,
            ToolboxController::Target::wholeSubsong, 1, PsgValues::channelCountPerPsg, 0, 1),
        margins(LookAndFeelConstants::margins),
        left(LookAndFeelConstants::margins),
        toolboxController(pToolboxController),
        hasFocus(false),
        toolTypeComboBox(),
        selectedInstrumentsLabel(),
        selectionGroup(juce::translate("Target"), 4, false, true, 140,
                       [&](const int index) {
                           return getSelectionTargetToggleName(index);
                       },
                       [&](const int index) {
                           return isSelectionTargetSelected(index);
                       }
        ),
        performButton(),
        transposePlusMinus(*this, juce::translate("Amount"), 2, 1, 2, SliderIncDec::Filter::decimal),
        transposePlus1("+1"),
        transposeMinus1("-1"),
        transposePlus12("+12"),
        transposeMinus12("-12"),
        firstChannelToSwapComboBox(),
        secondChannelToSwapComboBox()
{
    setOpaque(true);
    setWantsKeyboardFocus(true);

    toolTypeComboBox.addItem(juce::translate("Transpose"), static_cast<int>(ToolboxController::ToolType::transpose) + offsetComboBoxId);
    toolTypeComboBox.addItem(juce::translate("Swap instruments"), static_cast<int>(ToolboxController::ToolType::swapInstrument) + offsetComboBoxId);
    toolTypeComboBox.addItem(juce::translate("Remap instruments"), static_cast<int>(ToolboxController::ToolType::remapInstrument) + offsetComboBoxId);
    toolTypeComboBox.addItem(juce::translate("Delete note"), static_cast<int>(ToolboxController::ToolType::deleteNote) + offsetComboBoxId);
    toolTypeComboBox.addItem(juce::translate("Swap cells"), static_cast<int>(ToolboxController::ToolType::swapCells) + offsetComboBoxId);
    toolTypeComboBox.onChange = [&] { onToolTypeComboBoxChanged(); };

    selectionGroup.onSelectionChanged = [&] { onSelectionTargetChanged(); };
    performButton.onClick = [&] { onPerformClicked(); };

    transposeMinus1.onClick = [&] { toolboxController.onUserWantsToChangeTranspositionRate(-1); };
    transposeMinus12.onClick = [&] { toolboxController.onUserWantsToChangeTranspositionRate(-12); };
    transposePlus1.onClick = [&] { toolboxController.onUserWantsToChangeTranspositionRate(1); };
    transposePlus12.onClick = [&] { toolboxController.onUserWantsToChangeTranspositionRate(12); };

    firstChannelToSwapComboBox.onChange = [&] { onFirstChannelToSwapChanged(); };
    secondChannelToSwapComboBox.onChange = [&] { onSecondChannelToSwapChanged(); };

    addAndMakeVisible(toolTypeComboBox);
    addAndMakeVisible(selectedInstrumentsLabel);
    addAndMakeVisible(performButton);
    addAndMakeVisible(selectionGroup);
    addChildComponent(transposePlusMinus);
    addChildComponent(transposeMinus1);
    addChildComponent(transposeMinus12);
    addChildComponent(transposePlus1);
    addChildComponent(transposePlus12);
    addChildComponent(firstChannelToSwapComboBox);
    addChildComponent(secondChannelToSwapComboBox);
}


// Component method implementations
// ==================================

void Toolbox::paint(juce::Graphics& g)
{
    const auto backgroundColor = findColour(static_cast<int>(
            hasFocus ? LookAndFeelConstants::Colors::panelBackgroundFocused : LookAndFeelConstants::Colors::panelBackgroundUnfocused));
    g.fillAll(backgroundColor);

    const auto borderColor = findColour(static_cast<int>(
                hasFocus ? LookAndFeelConstants::Colors::panelBorderFocused : LookAndFeelConstants::Colors::panelBorderUnfocused));
    g.setColour(borderColor);
    g.drawRect(0, 0, getWidth(), getHeight());
}

void Toolbox::focusOfChildComponentChanged(FocusChangeType /*cause*/)
{
    if (const auto newFocus = hasKeyboardFocus(true); hasFocus != newFocus) {
        hasFocus = newFocus;
        repaint();
    }
}

void Toolbox::resized()
{
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto top = margins;

    const auto availableWidth = getAvailableWidth();
    toolTypeComboBox.setBounds(left, top, availableWidth, labelsHeight);
    selectedInstrumentsLabel.setBounds(left, toolTypeComboBox.getBottom() + margins, availableWidth, labelsHeight);
}


// ==================================

void Toolbox::updateUi(const ToolboxController::DisplayedData& newDisplayedData, const bool forceViewCreation) noexcept
{
    const auto oldDisplayedData = displayedData;
    if (oldDisplayedData == newDisplayedData) {
        return;
    }
    displayedData = newDisplayedData;

    // Creates new views if needed.
    if (forceViewCreation || (oldDisplayedData.getToolType() != newDisplayedData.getToolType())) {
        createViewsFromToolType(newDisplayedData.getToolType());
    }

    // Updates the various UI elements.
    toolTypeComboBox.setSelectedId(static_cast<int>(newDisplayedData.getToolType()) + offsetComboBoxId, juce::NotificationType::dontSendNotification);

    const auto selectedTargetIndex = newDisplayedData.getSelectionTarget();
    static_assert(static_cast<int>(ToolboxController::Target::first) == 0, "The id must start at 0.");
    selectionGroup.setSelected(std::set { static_cast<int>(selectedTargetIndex)});

    selectedInstrumentsLabel.setText(displayedData.getSelectedInstrumentsText(), juce::NotificationType::dontSendNotification);
    performButton.setButtonText(displayedData.getButtonText());
    performButton.setEnabled(displayedData.isButtonEnabled());

    transposePlusMinus.setShownValue(displayedData.getTranspositionRate());

    // A different channel count/selection to show?
    buildSwapChannelComboBoxIfNeeded(newDisplayedData.getCurrentSubsongChannelCount(), newDisplayedData.getFirstChannelIndex(), newDisplayedData.getSecondChannelIndex());
}

void Toolbox::onToolTypeComboBoxChanged() const noexcept
{
    const auto id = toolTypeComboBox.getSelectedId() - offsetComboBoxId;
    const auto newToolType = static_cast<ToolboxController::ToolType>(id);
    toolboxController.onUserChangedToolType(newToolType);
}

void Toolbox::createViewsFromToolType(const ToolboxController::ToolType toolType) noexcept
{
    switch (toolType) {
        case ToolboxController::ToolType::transpose:
            createViewsForTranspose();
            break;
        case ToolboxController::ToolType::deleteNote: [[fallthrough]];
        case ToolboxController::ToolType::remapInstrument: [[fallthrough]];
        case ToolboxController::ToolType::swapInstrument:
            createViewsForSimpleAction();
            break;
        case ToolboxController::ToolType::swapCells:
            createViewsForSwapCells();
            break;
    }
}

void Toolbox::createViewsForTranspose() noexcept
{
    const auto availableWidth = getAvailableWidth();
    const auto buttonsHeight = LookAndFeelConstants::buttonsHeight;
    transposePlusMinus.setBounds(left, selectedInstrumentsLabel.getBottom() + margins, availableWidth, buttonsHeight);
    const auto transposeY = transposePlusMinus.getBottom() + (margins / 2);
    constexpr auto transposeButtonsSpacing = 4;
    constexpr auto transposeLongButtonsWidth = 44;
    constexpr auto transposeShortButtonsWidth = 39;
    transposeMinus1.setBounds(left, transposeY, transposeShortButtonsWidth, buttonsHeight);
    transposeMinus12.setBounds(transposeMinus1.getRight() + transposeButtonsSpacing, transposeY, transposeLongButtonsWidth, buttonsHeight);
    transposePlus1.setBounds(transposeMinus12.getRight() + transposeButtonsSpacing, transposeY, transposeShortButtonsWidth, buttonsHeight);
    transposePlus12.setBounds(transposePlus1.getRight() + transposeButtonsSpacing, transposeY, transposeLongButtonsWidth, buttonsHeight);

    showViews(true);

    locateSelectionAndPerformButton(transposePlus1);

    setHeight(326);
}

void Toolbox::createViewsForSimpleAction() noexcept
{
    showViews(false);
    locateSelectionAndPerformButton(selectedInstrumentsLabel);

    setHeight(260);
}

void Toolbox::createViewsForSwapCells() noexcept
{
    const auto availableWidth = getAvailableWidth();
    const auto buttonsHeight = LookAndFeelConstants::buttonsHeight;

    showViews(false, false, true);

    firstChannelToSwapComboBox.setBounds(left, selectedInstrumentsLabel.getY() + margins, availableWidth, buttonsHeight);
    secondChannelToSwapComboBox.setBounds(firstChannelToSwapComboBox.getX(), firstChannelToSwapComboBox.getBottom() + (margins / 2),
        firstChannelToSwapComboBox.getWidth(), firstChannelToSwapComboBox.getHeight());

    locateSelectionAndPerformButton(secondChannelToSwapComboBox);

    setHeight(300);
}

void Toolbox::onPerformClicked() const noexcept
{
    toolboxController.perform();
}

juce::String Toolbox::getSelectionTargetToggleName(const int index) noexcept
{
    const auto target = static_cast<ToolboxController::Target>(index);

    juce::String label;
    switch (target) {
        case ToolboxController::Target::selection:
            label = juce::translate("Selection");
            break;
        case ToolboxController::Target::wholePattern:
            label = juce::translate("Current pattern");
            break;
        case ToolboxController::Target::wholeSubsong:
            label = juce::translate("Current subsong");
            break;
        case ToolboxController::Target::wholeSong:
            label = juce::translate("Whole song");
            break;
        default:
            jassertfalse;       // Should never happen!
            break;
    }

    return label;
}

bool Toolbox::isSelectionTargetSelected(const int index) const noexcept
{
    const auto currentTarget = displayedData.getSelectionTarget();
    const auto testedTarget = static_cast<ToolboxController::Target>(index);

    return (currentTarget == testedTarget);
}

void Toolbox::onSelectionTargetChanged() const noexcept
{
    // What is the clicked item?
    const auto indexes = selectionGroup.getToggledIndexes();
    if (indexes.size() != 1) {
        return;
    }

    const auto index = *indexes.begin();
    const auto newTarget = static_cast<ToolboxController::Target>(index);
    toolboxController.onUserSelectedTarget(newTarget);
}

void Toolbox::onFirstChannelToSwapChanged() const noexcept
{
    const auto index = firstChannelToSwapComboBox.getSelectedItemIndex();
    toolboxController.onUserWantsToChangeFirstChannelToSwap(index);
}

void Toolbox::onSecondChannelToSwapChanged() const noexcept
{
    const auto index = secondChannelToSwapComboBox.getSelectedItemIndex();
    toolboxController.onUserWantsToChangeSecondChannelToSwap(index);
}

void Toolbox::setHeight(const int height) noexcept
{
    setSize(getWidth(), height);
}

int Toolbox::getAvailableWidth() const noexcept
{
    return getWidth() - (margins * 2);
}

void Toolbox::locateSelectionAndPerformButton(const Component& afterComponent) noexcept
{
    const auto availableWidth = getAvailableWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    selectionGroup.setBounds(left, afterComponent.getBottom() + margins, availableWidth, 128);
    performButton.setBounds(left, static_cast<int>(selectionGroup.getBottom() + (margins * 1.5)), availableWidth, labelsHeight);
}

void Toolbox::showViews(const bool showTranspositionsViews, const bool showInstrumentSelectedLabel, const bool showSwapChannelsComboBoxes) noexcept
{
    transposePlusMinus.setVisible(showTranspositionsViews);
    transposeMinus1.setVisible(showTranspositionsViews);
    transposeMinus12.setVisible(showTranspositionsViews);
    transposePlus1.setVisible(showTranspositionsViews);
    transposePlus12.setVisible(showTranspositionsViews);

    selectedInstrumentsLabel.setVisible(showInstrumentSelectedLabel);

    firstChannelToSwapComboBox.setVisible(showSwapChannelsComboBoxes);
    secondChannelToSwapComboBox.setVisible(showSwapChannelsComboBoxes);
}

void Toolbox::buildSwapChannelComboBoxIfNeeded(const int channelCount, const int firstChannelIndex, const int secondChannelIndex) noexcept
{
    // New items to rebuild?
    if (channelCount != firstChannelToSwapComboBox.getNumItems()) {
        firstChannelToSwapComboBox.clear(juce::NotificationType::dontSendNotification);
        secondChannelToSwapComboBox.clear(juce::NotificationType::dontSendNotification);
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            firstChannelToSwapComboBox.addItem("Swap channel " + juce::String(channelIndex + 1), channelIndex + offsetComboBoxId);
            secondChannelToSwapComboBox.addItem("and channel " + juce::String(channelIndex + 1), channelIndex + offsetComboBoxId);
        }
    }

    firstChannelToSwapComboBox.setSelectedItemIndex(firstChannelIndex, juce::NotificationType::dontSendNotification);
    secondChannelToSwapComboBox.setSelectedItemIndex(secondChannelIndex, juce::NotificationType::dontSendNotification);
}


// SliderIncDec::Listener method implementations.
// ===============================================

void Toolbox::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    jassert(&slider == &transposePlusMinus);
    (void)slider;

    toolboxController.onUserWantsToChangeTranspositionRate(static_cast<int>(value));
}

} // namespace arkostracker
