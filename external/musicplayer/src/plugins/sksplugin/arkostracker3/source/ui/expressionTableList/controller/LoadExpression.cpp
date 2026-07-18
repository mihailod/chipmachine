#include "LoadExpression.h"

#include "../../../app/preferences/FolderContext.h"
#include "../../../business/serialization/song/ExpressionSerializer.h"
#include "../../../business/song/validation/CorrectExpression.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/FileUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker 
{

LoadExpression::LoadExpression(const bool pIsArpeggio, std::function<void(std::unique_ptr<Expression>)> pOnFinished, std::function<void()> pOnCanceled) noexcept :
        isArpeggio(pIsArpeggio),
        onFinished(std::move(pOnFinished)),
        onCanceled(std::move(pOnCanceled)),
        modalDialog(),
        backgroundOperation()
{
}

void LoadExpression::load() noexcept
{
    const auto pickerText = isArpeggio ? juce::translate("Load an arpeggio") : juce::translate("Load a pitch");
    const auto extensionFilter = isArpeggio ? FileExtensions::arpeggioExtensionWithWildcard : FileExtensions::pitchExtensionWithWildcard;
    const auto folderContext = isArpeggio ? FolderContext::loadOrSaveArpeggio : FolderContext::loadOrSavePitch;

    // Opens a FilePicker.
    FileChooserCustom fileChooser(pickerText, folderContext, extensionFilter);
    auto success = fileChooser.browseForFileToOpen(nullptr);
    if (!success) {
        onCanceled();
        return;
    }

    const auto fileToLoad = fileChooser.getResultWithExtensionIfNone(extensionFilter);

    // Performs the operation asynchronously.
    jassert(backgroundOperation == nullptr);
    backgroundOperation = std::make_unique<BackgroundOperationWithDialog<std::unique_ptr<Expression>>>(juce::translate("Loading"),
            juce::translate("Please wait..."),
            [&](std::unique_ptr<Expression> expression) { onBackgroundOperationFinished(std::move(expression)); }, [=]() -> std::unique_ptr<Expression> {
                // Worker thread.

                // Loads the file. It may be zipped!
                auto inputStream = FileUtil::openFileOrZip(fileToLoad);
                if (inputStream == nullptr) {
                    jassertfalse;           // Unable to read the file.
                    return nullptr;
                }

                // Creates an XML from the stream.
                auto string = inputStream->readEntireStreamAsString();
                juce::XmlDocument xmlDocument(string);
                auto xmlElement = xmlDocument.getDocumentElement();
                if (xmlElement == nullptr) {
                    jassertfalse;           // Unable to extract XML.
                    return nullptr;
                }

                // Deserializes the XML.
                auto expression = ExpressionSerializer::deserializeExpression(*xmlElement);

                if (expression == nullptr) {
                    jassertfalse;           // Unable to deserialize the XML.
                    return nullptr;
                }

                // Corrects the instrument.
                CorrectExpression correctExpression;
                correctExpression.correctExpression(*expression);

                return expression;          // When done, the onBackgroundOperationFinished is called.
            });
    backgroundOperation->performOperation();
}

void LoadExpression::onBackgroundOperationFinished(std::unique_ptr<Expression> result) noexcept
{
    // Main thread.

    backgroundOperation.reset();

    if (result == nullptr) {
        showErrorDialogAndNotify();
    } else {
        onFinished(std::move(result));
    }
}


// ============================================================================

void LoadExpression::showErrorDialogAndNotify() noexcept
{
    const auto text = juce::translate("The file is could not be loaded or may be corrupted.");
    jassert(modalDialog == nullptr);     // Already present?
    modalDialog = SuccessOrErrorDialog::buildForError(text, [&] {
        modalDialog.reset();
        onFinished(nullptr);
    });
}

}   // namespace arkostracker
