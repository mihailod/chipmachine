#include "SaveInstrument.h"
#include "../../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../../components/FileChooserCustom.h"
#include "../../../../business/serialization/instrument/InstrumentSerializer.h"
#include "../../../../utils/FileUtil.h"
#include "../../../../utils/FileExtensions.h"

namespace arkostracker 
{

SaveInstrument::SaveInstrument(SaveInstrument::Listener& pListener, std::unique_ptr<Instrument> pInstrumentToSave, bool pCompressXml) noexcept :
        listener(pListener),
        instrument(std::move(pInstrumentToSave)),
        compressXml(pCompressXml),
        modalDialog(),
        backgroundOperation()
{
}

void SaveInstrument::save() noexcept
{
    // Opens a FilePicker.
    const auto extensionFilter = FileExtensions::instrumentExtensionWithWildcard;
    FileChooserCustom fileChooser(juce::translate("Save an instrument"),
                                  FolderContext::loadOrSaveInstrument, extensionFilter, instrument->getName());
    auto success = fileChooser.browseForFileToSave(true);
    if (!success) {
        listener.onSaveInstrumentCancelled();
        return;
    }

    const auto fileToSaveTo = fileChooser.getResultWithExtensionIfNone(extensionFilter);

    // Asynchronously saves the Instrument.
    jassert(backgroundOperation == nullptr);
    backgroundOperation = std::make_unique<BackgroundOperationWithDialog<bool>>(juce::translate("Loading"),
            juce::translate("Please wait..."),
            [&](bool pSuccess) { onBackgroundOperationFinished(pSuccess); }, [=]() -> bool {
                // Worker thread.

                // Serializes the instrument.
                auto xmlElement = InstrumentSerializer::serialize(*instrument);
                if (xmlElement == nullptr) {
                    jassertfalse;           // Unable to serialize?
                    return false;
                }

                // Writes it, as a zip file.
                return FileUtil::writeXmlToFile(*xmlElement, fileToSaveTo, compressXml);
            });
    backgroundOperation->performOperation();
}

void SaveInstrument::onBackgroundOperationFinished(bool success) noexcept
{
    // Main thread.

    backgroundOperation.reset();

    if (success) {
        listener.onSaveInstrumentFinished(true);
    } else {
        showErrorDialogAndNotify();
    }
}


// ============================================================================

void SaveInstrument::showErrorDialogAndNotify() noexcept
{
    jassert(modalDialog == nullptr);        // Dialog not already removed?

    modalDialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while saving the instrument."), [&] {
        modalDialog.reset();

        listener.onSaveInstrumentFinished(false);
    });
}


}   // namespace arkostracker

