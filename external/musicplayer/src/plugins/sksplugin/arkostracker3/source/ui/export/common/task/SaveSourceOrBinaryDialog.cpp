#include "SaveSourceOrBinaryDialog.h"

#include "../../../../utils/FileUtil.h"

namespace arkostracker 
{

SaveSourceOrBinaryDialog::SaveSourceOrBinaryDialog(juce::MemoryBlock pPrimaryMemoryBlock,
                                       std::vector<juce::MemoryBlock> pSecondaryMemoryBlocks,
                                       juce::File pOutputFile,
                                       const bool pExportAsSeveralFiles, const bool pSaveToBinary,
                                       const bool pExportPlayerConfiguration, PlayerConfiguration pPlayerConfiguration,
                                       SourceGeneratorConfiguration pSourceGeneratorConfigurationForPlayerConfiguration,
                                       juce::String pSuccessMessage, juce::String pFailureMessage,
                                       ErrorReport pErrorReport, std::function<void(bool)> pCallback) noexcept:
        saveSourceOrBinary(
                std::move(pPrimaryMemoryBlock),
                std::move(pSecondaryMemoryBlocks),
                std::move(pOutputFile),
                pExportAsSeveralFiles,
                pSaveToBinary,
                pExportPlayerConfiguration,
                std::move(pPlayerConfiguration),
                std::move(pSourceGeneratorConfigurationForPlayerConfiguration)
                ),
        successMessage(std::move(pSuccessMessage)),
        failureMessage(std::move(pFailureMessage)),
        errorReport(std::move(pErrorReport)),
        callback(std::move(pCallback)),
        resultDialog()
{
}

void SaveSourceOrBinaryDialog::perform() noexcept
{
    // Shows a dialog.
    savingOperationWithDialog = std::make_unique<BackgroundOperationWithDialog<bool>>(juce::translate("Saving"),
            juce::translate("Please wait..."), [&](const bool success) { onSaveOperationFinished(success); }, [&]() -> bool {

                // Worker thread.
                // =======================================

                return saveSourceOrBinary.perform();
            });

    savingOperationWithDialog->performOperation();
}

void SaveSourceOrBinaryDialog::onSaveOperationFinished(bool success) noexcept
{
    savingOperationWithDialog.reset();
    jassert(success);

    // Shows a pop-up with the possible Status Report.
    const auto& text = success ? successMessage : failureMessage;
    resultDialog = std::make_unique<ResultDialog>(juce::translate("Operation finished"), text, errorReport,
            [&, success] { onUserClosedResultDialog(success); });
}


// ======================================================

void SaveSourceOrBinaryDialog::onUserClosedResultDialog(const bool success) noexcept
{
    resultDialog.reset();
    callback(success);
}

}   // namespace arkostracker
