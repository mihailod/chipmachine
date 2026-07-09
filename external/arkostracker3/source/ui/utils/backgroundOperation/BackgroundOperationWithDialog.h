#pragma once

#include "../../components/dialogs/LoadingDialog.h"
#include "BackgroundOperation.h"

namespace arkostracker 
{

/**
 * Manages a background operation, returning a result. It shows a dialog.
 * Note that the dialog is deleted only when this object is deleted too.
 *
 * @deprecated Use BackgroundTaskWithProgress, the API is safer.
 */
template<typename RESULT>
class BackgroundOperationWithDialog
{
public:
    /**
     * Constructor.
     * @param pTitle the title of the Dialog.
     * @param pText the text of the Dialog.
     * @param pCallback the callback, called on the UI thread, with the result (it has to also indicate if successful).
     * @param pWork the work to perform on the background thread.
     */
    BackgroundOperationWithDialog(juce::String pTitle, juce::String pText, std::function<void(RESULT)> pCallback, std::function<RESULT()> pWork) noexcept :
            title(std::move(pTitle)),
            text(std::move(pText)),
            loadingDialog(),
            backgroundOperation(std::move(pCallback), std::move(pWork))
    {
    }

    /** Starts the operation. Shows a pop-up. */
    void performOperation() noexcept
    {
        // This is called on the UI thread.

        // Opens the Progress Dialog.
        jassert(loadingDialog == nullptr);
        loadingDialog = std::make_unique<LoadingDialog>(title, text);

        // Let the inner object do to job.
        backgroundOperation.performOperation();
    }

private:
    juce::String title;
    juce::String text;

    std::unique_ptr<ModalDialog> loadingDialog;

    BackgroundOperation<RESULT> backgroundOperation;
};

}   // namespace arkostracker

