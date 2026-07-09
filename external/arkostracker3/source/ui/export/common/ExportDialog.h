#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "../../../controllers/MainController.h"
#include "../../components/ThemedColoredImage.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

class MainController;
class SongController;

/**
 * A generic dialog for Exports.
 * It contains:
 * - the settings of the OK/Cancel texts, resize of the OK button.
 * - Displays of the possible error besides the OK in case there are errors in the song.
 */
class ExportDialog : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param listener the listener to close this Dialog.
     * @param title the title of the Dialog.
     * @param width the width.
     * @param height the height.
     * @param showOkButton true to show the Ok Button.
     * @param showCancelButton true to show the Cancel Button.
     * @param resizable true if the Dialog can be resized.
     */
    ExportDialog(const MainController& mainController, std::function<void()> listener,
        const juce::String& title, int width, int height,
        bool showOkButton = true, bool showCancelButton = true, bool resizable = false) noexcept;

protected:
    /** Called when the Export button is clicked. */
    virtual void onExportButtonClicked() noexcept = 0;
    /** Called when the Cancel button is clicked. */
    virtual void onCancelButtonClicked() const noexcept = 0;

    const MainController& mainController;
    SongController& songController;
    std::function<void()> listener;

    ThemedColoredImage warningImage;
    std::unique_ptr<juce::BubbleMessageComponent> warningBubble;
};

}   // namespace arkostracker
