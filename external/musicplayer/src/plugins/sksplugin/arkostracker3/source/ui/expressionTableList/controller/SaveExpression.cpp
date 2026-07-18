#include "SaveExpression.h"

#include "../../../business/serialization/song/ExpressionSerializer.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/FileUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker 
{

SaveExpression::SaveExpression(const bool pIsArpeggio, Expression pExpression, const bool pCompressXml, std::function<void(bool)> pOnFinished,
                               std::function<void()> pOnCanceled) noexcept :
        isArpeggio(pIsArpeggio),
        expression(std::move(pExpression)),
        compressXml(pCompressXml),
        onFinished(std::move(pOnFinished)),
        onCanceled(std::move(pOnCanceled)),
        modalDialog(),
        backgroundOperation()
{
}

void SaveExpression::save() noexcept
{
    const auto pickerText = isArpeggio ? juce::translate("Save an arpeggio") : juce::translate("Save a pitch");
    const auto extensionFilter = isArpeggio ? FileExtensions::arpeggioExtensionWithWildcard : FileExtensions::pitchExtensionWithWildcard;
    const auto folderContext = isArpeggio ? FolderContext::loadOrSaveArpeggio : FolderContext::loadOrSavePitch;

    // Opens a FilePicker.
    FileChooserCustom fileChooser(pickerText, folderContext, extensionFilter, expression.getName());
    const auto success = fileChooser.browseForFileToSave(true);
    if (!success) {
        onCanceled();
        return;
    }

    const auto fileToSaveTo = fileChooser.getResultWithExtensionIfNone(extensionFilter);

    // Performs the operation asynchronously.
    backgroundOperation = std::make_unique<BackgroundOperationWithDialog<bool>>(juce::translate("Loading"),
                                                                      juce::translate("Please wait..."),
                                                                      [&](const bool pSuccess) { onBackgroundOperationFinished(pSuccess); }, [=]() -> bool {
                // Worker thread.

                // Serializes the Expression.
                const auto xmlElement = ExpressionSerializer::serialize(expression);     // Copied!
                auto iSuccess = (xmlElement != nullptr);
                jassert(iSuccess);

                // Writes it, as a zip file.
                if (iSuccess) {
                    iSuccess = FileUtil::writeXmlToFile(*xmlElement, fileToSaveTo, compressXml);
                }
                return iSuccess;          // When done, the onBackgroundOperationFinished is called.
            });
    backgroundOperation->performOperation();
}

void SaveExpression::onBackgroundOperationFinished(const bool success) noexcept
{
    // Exits if success, else shows a pop-up.
    if (success) {
        onFinished(true);
    } else {
        showErrorDialogAndNotify();
    }
}


// ==============================================

void SaveExpression::showErrorDialogAndNotify() noexcept
{
    jassert(modalDialog == nullptr);        // Dialog not already removed?

    modalDialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while saving the expression."), [&] {
        modalDialog.reset();

        onFinished(false);
    });
}


}   // namespace arkostracker

