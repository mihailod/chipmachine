#pragma once

#include <functional>
#include <memory>

#include "../../../song/Expression.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundOperation/BackgroundOperationWithDialog.h"

namespace arkostracker 
{

/** Manages the loading of an expression, including all the modals and the asynchronous loading. */
class LoadExpression
{
public:
    /**
     * Constructor.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param onFinished called when finished, with the Expression if successful.
     * @param onCanceled called if the user canceled.
     */
    explicit LoadExpression(bool isArpeggio, std::function<void(std::unique_ptr<Expression>)> onFinished, std::function<void()> onCanceled) noexcept;

    /** Performs the loading of the instrument. */
    void load() noexcept;

private:
    /**
     * Called when the operation finishes.
     * @param result the result, if successful.
     */
    void onBackgroundOperationFinished(std::unique_ptr<Expression> result) noexcept;

    /** Shows an error dialog and notifies the client. */
    void showErrorDialogAndNotify() noexcept;

    const bool isArpeggio;
    const std::function<void(std::unique_ptr<Expression>)> onFinished;
    const std::function<void()> onCanceled;

    std::unique_ptr<ModalDialog> modalDialog;

    std::unique_ptr<BackgroundOperationWithDialog<std::unique_ptr<Expression>>> backgroundOperation;
};

}   // namespace arkostracker
