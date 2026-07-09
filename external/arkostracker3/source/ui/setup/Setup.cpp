#include "Setup.h"

#include "../mainView/MainComponent.h"
#include "audioInterfaceSetup/AudioInterfaceSetup.h"
#include "general/GeneralSetupDialog.h"
#include "keyboardMapping/SetupKeyboardMappingDialog.h"
#include "output/OutputDialog.h"
#include "patternViewer/PatternViewerSetupDialog.h"
#include "serial/SerialSetupDialog.h"
#include "sourceProfile/SourceProfileDialog.h"
#include "themeEditor/ThemeEditor.h"

namespace arkostracker 
{

Setup::Setup(MainController& pMainController, MainComponent& pMainComponent) noexcept :
        mainController(pMainController),
        mainComponent(pMainComponent),
        setupDialog()
{
}

void Setup::openGeneralSetupDialog() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    setupDialog = std::make_unique<GeneralSetupDialog>([&] { onDialogOk(); });
}

void Setup::openSetupKeyboardMappingDialog() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    setupDialog = std::make_unique<SetupKeyboardMappingDialog>(mainController, [&] { onDialogOk(); }, [&] { onDialogCancelled(); });
}

void Setup::openThemeEditor() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    setupDialog = std::make_unique<ThemeEditor>(mainComponent, [&] { onDialogOk(); });
}

void Setup::openPatternViewerSetup() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    setupDialog = std::make_unique<PatternViewerSetupDialog>(mainController, [&] { onDialogOk(); }, [&] { onDialogCancelled(); });
}

void Setup::openAudioInterfaceSetup() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    setupDialog = std::make_unique<AudioInterfaceSetup>(mainController, [&] { onDialogOk(); });
}

void Setup::openOutputSetup() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    setupDialog = std::make_unique<OutputDialog>(mainController, [&] { onDialogOk(); }, [&] { onDialogCancelled(); });
}

void Setup::openSourceProfileSetup() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    setupDialog = std::make_unique<SourceProfileDialog>([&] { onDialogOk(); }, [&] { onDialogCancelled(); });
}

void Setup::openSerialSetup() noexcept
{
    jassert(setupDialog == nullptr);        // A dialog is already opened?

    if (setupDialog == nullptr) {           // Events can open it, we don't want double-opening.
        setupDialog = std::make_unique<SerialSetupDialog>([&] { onDialogOk(); }, [&] { onDialogCancelled(); });
    }
}

void Setup::onDialogOk() noexcept
{
    // Closes the dialog.
    setupDialog.reset();
}

void Setup::onDialogCancelled() noexcept
{
    // Closes the dialog.
    setupDialog.reset();
}


}   // namespace arkostracker

