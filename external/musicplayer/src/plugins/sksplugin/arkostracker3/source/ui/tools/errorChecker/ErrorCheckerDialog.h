#pragma once

#include <utility>

#include "../../../song/CellLocationInPosition.h"
#include "../../components/GroupWithViewport.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

class Song;
class MainController;

/**
 * Dialog allowing to know if Instruments are used, to delete instrument and optimize them.
 * For now, the Goto is not done, as well as Optimize the Instrument themselves.
 */
class ErrorCheckerDialog final : public ModalDialog,
                                 juce::Button::Listener
{
public:
    /**
     * Constructor.
     * @param mainController the Main controller.
     * @param listener the listener to close this Dialog.
     */
    ErrorCheckerDialog(MainController& mainController, std::function<void()> listener) noexcept;

    // ModalDialog method implementations.
    // ===================================================
    void closeButtonPressed() override;

private:
    class DisplayedItem
    {
    public:
        DisplayedItem(CellLocationInPosition pCellLocation, std::unique_ptr<juce::Label> pLocationLabel,
            std::unique_ptr<juce::Label> pErrorLabel, std::unique_ptr<juce::TextButton> pGotoButton,
            std::unique_ptr<Component> pSeparator) :
                cellLocation(std::move(pCellLocation)),
                locationLabel(std::move(pLocationLabel)),
                errorLabel(std::move(pErrorLabel)),
                gotoButton(std::move(pGotoButton)),
                separator(std::move(pSeparator))
        {
        }

        const juce::TextButton& getGotoButton() const noexcept
        {
            return *gotoButton;
        }

        CellLocationInPosition getCellLocation() const noexcept
        {
            return cellLocation;
        }

    private:
        CellLocationInPosition cellLocation;
        std::unique_ptr<juce::Label> locationLabel;
        std::unique_ptr<juce::Label> errorLabel;
        std::unique_ptr<juce::TextButton> gotoButton;
        std::unique_ptr<Component> separator;
    };

    // juce::Button::Listener method implementation
    // =================================================
    void buttonClicked(juce::Button* button) override;

    // =================================================

    /** Called when OK button is clicked. Exits. */
    void onOkButtonClicked() const noexcept;

    /** Fills the instrument group with all the Instruments. Cleared first. */
    void fillErrorGroup() noexcept;

    /**
     * Goes to the given location. This also closes this dialog.
     * @param location where to go.
     */
    void gotoLocation(const CellLocationInPosition& location) const noexcept;

    /**
     * Called when a Goto is clicked. The UI has already been modified.
     * @param id the ID of the Instrument.
     */
    //void onGotoClicked(const Id& id) noexcept;

    MainController& mainController;
    std::function<void()> modalCallback;

    GroupWithViewport errorGroup;

    std::vector<DisplayedItem> shownItems;        // The shown items, in the order they are shown.
};


}   // namespace arkostracker
