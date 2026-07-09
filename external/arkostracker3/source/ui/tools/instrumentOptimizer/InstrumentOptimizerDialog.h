#pragma once

#include "../../../utils/Id.h"
#include "../../components/GroupWithViewport.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

class Song;
class SongController;

/**
 * Dialog allowing to know if Instruments are used, to delete instrument and optimize them.
 * For now, the Goto is not done, as well as Optimize the Instrument themselves.
 */
class InstrumentOptimizerDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param songController the Song controller.
     * @param listener the listener to close this Dialog.
     */
    InstrumentOptimizerDialog(SongController& songController, std::function<void()> listener) noexcept;

    // ModalDialog method implementations.
    // ===================================================
    void closeButtonPressed() override;

private:
    class DisplayedItem
    {
    public:
        DisplayedItem(Id pId, juce::String pDisplayedName, const int pUseCount, std::unique_ptr<juce::ToggleButton> pCheckBox,
            std::unique_ptr<Component> pTextLabel /*, std::unique_ptr<juce::TextButton> pGotoButton*/) :
                id(std::move(pId)),
                displayedName(std::move(pDisplayedName)),
                useCount(pUseCount),
                checkBox(std::move(pCheckBox)),
                textLabel(std::move(pTextLabel))
                //gotoButton(std::move(pGotoButton))
        {
        }

        Id getId() const noexcept
        {
            return id;
        }

        juce::String getDisplayedName() const noexcept
        {
            return displayedName;
        }

        bool isUsed() const noexcept
        {
            return (useCount > 0);
        }

        int getUseCount() const noexcept
        {
            return useCount;
        }

        bool isSelected() const noexcept
        {
            return checkBox->getToggleState();
        }

        /** Sets the selection state. This modifies the UI. */
        void setSelected(const bool selected) const noexcept
        {
            checkBox->setToggleState(selected, juce::NotificationType::dontSendNotification);
        }

    private:
        Id id;
        juce::String displayedName;
        int useCount;
        std::unique_ptr<juce::ToggleButton> checkBox;
        std::unique_ptr<Component> textLabel;
        //std::unique_ptr<juce::TextButton> gotoButton;
    };

    /** Called when OK button is clicked. Exits. */
    void onOkButtonClicked() const noexcept;

    /** Fills the instrument group with all the Instruments. Cleared first. */
    void fillInstrumentGroup() noexcept;

    /**
     * Called when a Goto is clicked. The UI has already been modified.
     * @param id the ID of the Instrument.
     */
    //void onGotoClicked(const Id& id) noexcept;

    /**
     * @return the item from its ID.
     * @param id the ID. Should exist.
     */
    DisplayedItem* searchItem(const Id& id) noexcept;

    /** Selects the unused items. */
    void onSelectUnusedButtonClicked() const noexcept;
    /** Selects all the items. */
    void onSelectAllButtonClicked() const noexcept;
    /** Selects none of the items. */
    void onSelectNoneButtonClicked() const noexcept;
    /** Deletes the selected items. */
    void onDeleteSelectedClicked() noexcept;

    /** @return the selected ids. */
    std::set<Id> getSelectedIds() const noexcept;
    /** @return the selected indexes. */
    std::set<int> getSelectedIndexes() const noexcept;

    SongController& songController;
    std::function<void()> modalCallback;

    juce::Label warningLabel;
    GroupWithViewport instrumentGroup;

    juce::Label selectLabel;
    juce::TextButton selectedUnusedButton;
    juce::TextButton selectedAllButton;
    juce::TextButton selectedNoneButton;
    juce::Label actionLabel;
    juce::TextButton deleteSelectedButton;

    std::vector<DisplayedItem> shownItems;        // The shown items, in the order they are shown.

    std::unordered_set<Id> selectedIds;
};

}   // namespace arkostracker