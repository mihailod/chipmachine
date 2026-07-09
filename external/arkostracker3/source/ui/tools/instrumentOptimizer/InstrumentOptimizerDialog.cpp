#include "InstrumentOptimizerDialog.h"

#include "../../../business/song/tool/InstrumentCounter.h"
#include "../../../controllers/SongController.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker
{

InstrumentOptimizerDialog::InstrumentOptimizerDialog(SongController& pSongController, std::function<void()> pListener) noexcept :
        ModalDialog(juce::translate("Instruments optimizer"), 460, 480,
                    [&] { onOkButtonClicked(); },
                    [&] { /* Never used. */ }),
        songController(pSongController),
        modalCallback(std::move(pListener)),
        warningLabel({ }, juce::translate("Note: remember that instruments are all shared among subsongs.")),
        instrumentGroup({ }),
        selectLabel(juce::String(), juce::translate("Select:")),
        selectedUnusedButton(juce::translate("Unused")),
        selectedAllButton(juce::translate("All")),
        selectedNoneButton(juce::translate("None")),
        actionLabel(juce::String(), juce::translate("Action:")),
        deleteSelectedButton(juce::translate("Delete selected")),
        shownItems(),
        selectedIds()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto x = bounds.getX();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    constexpr auto instrumentGroupHeight = 300;

    warningLabel.setBounds(x, top, width, labelsHeight);
    instrumentGroup.setBounds(x, warningLabel.getBottom(), width, instrumentGroupHeight);

    fillInstrumentGroup();

    constexpr auto labelsWidth = 70;
    constexpr auto buttonsX = labelsWidth + 10;
    constexpr auto selectionButtonsWidth = 90;
    selectLabel.setBounds(x, instrumentGroup.getBottom() + margins, labelsWidth, labelsHeight);
    selectedUnusedButton.setBounds(buttonsX, instrumentGroup.getBottom() + margins, selectionButtonsWidth, labelsHeight);
    selectedAllButton.setBounds(selectedUnusedButton.getRight() + margins, selectedUnusedButton.getY(), selectionButtonsWidth, labelsHeight);
    selectedNoneButton.setBounds(selectedAllButton.getRight() + margins, selectedUnusedButton.getY(), selectionButtonsWidth, labelsHeight);

    constexpr auto actionButtonsWidth = 178;
    actionLabel.setBounds(x, selectedUnusedButton.getBottom() + margins, labelsWidth, labelsHeight);
    deleteSelectedButton.setBounds(buttonsX, selectedUnusedButton.getBottom() + margins, actionButtonsWidth, labelsHeight);

    selectedUnusedButton.onClick = [&] { onSelectUnusedButtonClicked(); };
    selectedAllButton.onClick = [&] { onSelectAllButtonClicked(); };
    selectedNoneButton.onClick = [&] { onSelectNoneButtonClicked(); };
    deleteSelectedButton.onClick = [&] { onDeleteSelectedClicked(); };

    addComponentToModalDialog(warningLabel);
    addComponentToModalDialog(instrumentGroup);
    addComponentToModalDialog(selectLabel);
    addComponentToModalDialog(selectedUnusedButton);
    addComponentToModalDialog(selectedAllButton);
    addComponentToModalDialog(selectedNoneButton);
    addComponentToModalDialog(actionLabel);
    addComponentToModalDialog(deleteSelectedButton);
}

void InstrumentOptimizerDialog::onOkButtonClicked() const noexcept
{
    modalCallback();
}

void InstrumentOptimizerDialog::closeButtonPressed()
{
    modalCallback();
}

void InstrumentOptimizerDialog::fillInstrumentGroup() noexcept
{
    shownItems.clear();

    const auto song = songController.getConstSong();

    const auto countedInstruments = InstrumentCounter::countInstruments(*song);

    // Displays it.
    auto y = 0;
    const auto itemHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto width = instrumentGroup.getGroupInnerArea().getWidth();
    for (const auto& countedInstrument : countedInstruments) {
        constexpr auto x = 0;
        constexpr auto usedTextWidth = 150;
        const auto checkBoxWidth = width - usedTextWidth - x - (margins * 2);

        auto checkBox = std::make_unique<juce::ToggleButton>(countedInstrument.getName());
        // How many times used? Not very optimized because one pass could be enough. Shouldn't really matter...
        const auto useCount = countedInstrument.getCount();
        auto usedText = std::make_unique<juce::Label>(juce::String(), (useCount == 0) ?
            juce::translate("Unused") :
            juce::translate("Used ") + juce::String(useCount) + juce::translate(" times")
        );
        usedText->setJustificationType(juce::Justification::centred);

        checkBox->setBounds(x, y, checkBoxWidth + margins, itemHeight);
        usedText->setBounds(checkBox->getRight(), y, usedTextWidth, itemHeight);
        //const auto gotoButtonX = usedText->getRight();
        instrumentGroup.addComponentToGroup(*checkBox);
        instrumentGroup.addComponentToGroup(*usedText);

        // Shows the Goto button only if at least one use.
        /*std::unique_ptr<juce::TextButton> gotoButton = nullptr;
        if (useCount > 0) {
            constexpr auto gotoButtonWidth = 60;

            gotoButton = std::make_unique<juce::TextButton>(juce::translate("Goto"));      // TO DO Add lambda.
            gotoButton->setBounds(gotoButtonX, y, gotoButtonWidth, itemHeight);
            gotoButton->onClick = [&] { onGotoClicked(displayedItem.getId()); };

            instrumentGroup.addComponentToGroup(*gotoButton);
        }*/
        shownItems.emplace_back(countedInstrument.getId(), countedInstrument.getName(), countedInstrument.getCount(), std::move(checkBox), std::move(usedText));

        y += itemHeight;
    }
}

/*void InstrumentOptimizerDialog::onGotoClicked(const Id& id) noexcept
{
    const auto* item = searchItem(id);
    if (item == nullptr) {
        jassertfalse; // Not found? Abnormal.
        return;
    }

    jassertfalse; // TO DO Goto. Maybe store the first location not to have the caller to bother about it?
}*/

InstrumentOptimizerDialog::DisplayedItem* InstrumentOptimizerDialog::searchItem(const Id& id) noexcept
{
    for (auto& item : shownItems) {
        if (item.getId() == id) {
            return &item;
        }
    }

    return nullptr;
}

void InstrumentOptimizerDialog::onSelectUnusedButtonClicked() const noexcept
{
    for (const auto& item : shownItems) {
        const auto mustBeSelected = !item.isUsed();
        item.setSelected(mustBeSelected);
    }
}

void InstrumentOptimizerDialog::onSelectAllButtonClicked() const noexcept
{
    for (const auto& item : shownItems) {
        item.setSelected(true);
    }
}

void InstrumentOptimizerDialog::onSelectNoneButtonClicked() const noexcept
{
    for (const auto& item : shownItems) {
        item.setSelected(false);
    }
}

std::set<Id> InstrumentOptimizerDialog::getSelectedIds() const noexcept
{
    std::set<Id> ids;
    for (const auto& item : shownItems) {
        if (item.isSelected()) {
            ids.emplace(item.getId());
        }
    }

    return ids;
}

std::set<int> InstrumentOptimizerDialog::getSelectedIndexes() const noexcept
{
    std::set<int> indexes;
    auto index = 1;
    for (const auto& item : shownItems) {
        if (item.isSelected()) {
            indexes.emplace(index);
        }
        ++index;
    }

    return indexes;
}

void InstrumentOptimizerDialog::onDeleteSelectedClicked() noexcept
{
    const auto indexes = getSelectedIndexes();
    if (indexes.empty()) {
        return;
    }

    songController.deleteInstruments(indexes);
    fillInstrumentGroup();
}

}   // namespace arkostracker
