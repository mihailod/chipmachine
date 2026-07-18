#pragma once

#include <memory>

#include "../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

class MainController;
class MainComponent;

/**
 * Manages the setup (general, keyboard, etc.) of the application.
 * We consider only one window can be opened at the same time.
 */
class Setup
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param mainComponent the Main Component.
     */
    Setup(MainController& mainController, MainComponent& mainComponent) noexcept;

    /** Opens the general setup. */
    void openGeneralSetupDialog() noexcept;
    /** Opens the setup for the Keyboard Mapping. */
    void openSetupKeyboardMappingDialog() noexcept;
    /** Opens the setup for the Theme editor. */
    void openThemeEditor() noexcept;
    /** Opens the setup for the Pattern Viewer. */
    void openPatternViewerSetup() noexcept;
    /** Opens the setup for the Audio interfaces Setup. */
    void openAudioInterfaceSetup() noexcept;
    /** Opens the setup for the Output (mix) Setup. */
    void openOutputSetup() noexcept;
    /** Opens the setup for the Source Profile Setup. */
    void openSourceProfileSetup() noexcept;
    /** Opens the setup for the Serial. */
    void openSerialSetup() noexcept;

private:
    void onDialogOk() noexcept;
    void onDialogCancelled() noexcept;

    MainController& mainController;
    MainComponent& mainComponent;
    std::unique_ptr<ModalDialog> setupDialog;
};

}   // namespace arkostracker

