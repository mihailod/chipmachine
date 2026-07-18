#pragma once

#include <juce_core/juce_core.h>

#include "../../../../export/playerConfiguration/PlayerConfiguration.h"
#include "../../../../export/sourceGenerator/SourceGeneratorConfiguration.h"
#include "../../../../utils/ErrorReport.h"
#include "../../../components/dialogs/ResultDialog.h"
#include "../../../utils/backgroundOperation/BackgroundOperationWithDialog.h"
#include "SaveSourceOrBinary.h"

namespace arkostracker 
{

/**
 * Takes care of saving asynchronously, and with UI, the given source. It might also be compiled into Z80 (the source must be Z80!).
 * As a bonus, the configuration player can also be exported, as source.
 * Shows a success message when done, or an error message. In both cases, the possible ErrorReport can be shown.
 */
class SaveSourceOrBinaryDialog
{
public:
    /**
     * Constructor.
     * @param primaryMemoryBlock the source to save/compile. It must already have the ORG if wanted (should be present if binary).
     * @param secondaryMemoryBlocks the possible other (subsong?) files, if relevant.
     * @param outputFile the base file to save. May already exist, will be overwritten without prompt. Player configuration and subsong files derive from it.
     * @param exportAsSeveralFiles true to save to different files.
     * @param saveToBinary true to save to binary, false to save as source. Always false if export as several files.
     * @param exportPlayerConfiguration true to export the player configuration.
     * @param playerConfiguration the player configuration.
     * @param sourceGeneratorConfigurationForPlayerConfiguration the source configuration, only used for exporting the player configuration.
     * @param successMessage the message to show when the saving succeeds.
     * @param failureMessage the message to show if the saving fails.
     * @param errorReport the possible error report to show once the save is done (and if the save was successful).
     * @param callback the callback once the window is closed, with a boolean indicating whether the save was successful.
     */
    SaveSourceOrBinaryDialog(juce::MemoryBlock primaryMemoryBlock,
                       std::vector<juce::MemoryBlock> secondaryMemoryBlocks,
                       juce::File outputFile,
                       bool exportAsSeveralFiles,
                       bool saveToBinary,
                       bool exportPlayerConfiguration, PlayerConfiguration playerConfiguration,
                       SourceGeneratorConfiguration sourceGeneratorConfigurationForPlayerConfiguration,
                       juce::String successMessage, juce::String failureMessage,
                       ErrorReport errorReport, std::function<void(bool)> callback) noexcept;

    /** Performs the save. */
    void perform() noexcept;

private:
    /**
     * Called when the user wants to close the Result Dialog.
     * @param success convenient flag indicating whether the export has been successful.
     */
    void onUserClosedResultDialog(bool success) noexcept;

    /**
     * Called when the save background operation is finished.
     * @param success true if success.
     */
    void onSaveOperationFinished(bool success) noexcept;

    SaveSourceOrBinary saveSourceOrBinary;         // In charge of doing all the job.

    juce::String successMessage;
    juce::String failureMessage;
    const ErrorReport errorReport;
    const std::function<void(bool)> callback;

    std::unique_ptr<BackgroundOperationWithDialog<bool>> savingOperationWithDialog;
    std::unique_ptr<ResultDialog> resultDialog;
};

}   // namespace arkostracker
