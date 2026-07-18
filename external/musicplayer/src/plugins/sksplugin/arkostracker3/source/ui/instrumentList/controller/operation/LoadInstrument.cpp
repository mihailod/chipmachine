#include "LoadInstrument.h"

#include "../../../../business/serialization/instrument/InstrumentDeserializerAt2.h"
#include "../../../../business/serialization/instrument/InstrumentSerializer.h"
#include "../../../../business/song/validation/CorrectInstrument.h"
#include "../../../../utils/FileExtensions.h"
#include "../../../../utils/FileUtil.h"
#include "../../../components/FileChooserCustom.h"
#include "../../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

LoadInstrument::LoadInstrument(LoadInstrument::Listener& pListener) noexcept :
        listener(pListener),
        modalDialog(),
        backgroundOperation()
{
}

void LoadInstrument::load() noexcept
{
    // Opens a FilePicker.
    const auto extensionFilter = FileExtensions::instrumentExtensionWithWildcard;
    FileChooserCustom fileChooser(juce::translate("Load an instrument"),
                                  FolderContext::loadOrSaveInstrument, extensionFilter);
    const auto success = fileChooser.browseForFileToOpen(nullptr);            // A preview for later.
    if (!success) {
        listener.onLoadInstrumentCancelled();
        return;
    }

    const auto fileToLoad = fileChooser.getResultWithExtensionIfNone(extensionFilter);

    // Asynchronously loads the Instrument.
    jassert(backgroundOperation == nullptr);
    backgroundOperation = std::make_unique<BackgroundOperationWithDialog<std::unique_ptr<Instrument>>>(juce::translate("Loading"),
            juce::translate("Please wait..."),
            [&](std::unique_ptr<Instrument> instrument) { onBackgroundOperationFinished(std::move(instrument)); },
            [=]() -> std::unique_ptr<Instrument> {
                // Loads the file.
                return load(fileToLoad);
    });
    backgroundOperation->performOperation();
}

std::unique_ptr<Instrument> LoadInstrument::load(const juce::File& fileToLoad) noexcept
{
    // Loads the file. It may be zipped!
    const auto inputStream = FileUtil::openFileOrZip(fileToLoad);
    if (inputStream == nullptr) {
        jassertfalse;           // Unable to read the file.
        return nullptr;
    }

    // Creates an XML from the stream.
    const auto string = inputStream->readEntireStreamAsString();
    juce::XmlDocument xmlDocument(string);
    const auto xmlElement = xmlDocument.getDocumentElement();
    if (xmlElement == nullptr) {
        jassertfalse;           // Unable to extract XML.
        return nullptr;
    }

    // Deserializes the XML (AT3).
    auto instrument = InstrumentSerializer::deserializeInstrument(*xmlElement);
    // Or maybe AT2 instrument?
    if (instrument == nullptr) {
        instrument = InstrumentDeserializerAt2::deserializeInstrument(*xmlElement);
    }

    if (instrument == nullptr) {
        jassertfalse;           // Unable to deserialize the XML.
        return nullptr;
    }

    // Corrects the instrument.
    CorrectInstrument::correctInstrument(*instrument);

    // Notifies.
    return instrument;
}

void LoadInstrument::onBackgroundOperationFinished(std::unique_ptr<Instrument> instrument) noexcept
{
    // Main thread.

    backgroundOperation.reset();

    if (instrument == nullptr) {
        showErrorDialogAndNotify();
        return;
    }

    listener.onLoadInstrumentFinished(std::move(instrument));
}


// ============================================================================

void LoadInstrument::showErrorDialogAndNotify() noexcept
{
    jassert(modalDialog == nullptr);        // Dialog not already removed?

    modalDialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while loading the instrument."), [&] {
        closeErrorDialogAndNotify();
    });
}

void LoadInstrument::closeErrorDialogAndNotify() noexcept
{
    modalDialog.reset();

    listener.onLoadInstrumentFinished(nullptr);
}

}   // namespace arkostracker
