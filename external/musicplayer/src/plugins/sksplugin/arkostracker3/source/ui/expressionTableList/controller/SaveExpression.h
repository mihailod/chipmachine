#pragma once

#include <memory>
#include <functional>

#include "../../../song/Expression.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundOperation/BackgroundOperationWithDialog.h"

namespace arkostracker 
{

/** Saves an Expression, showing the dialog, asynchronously saving it, etc. */
class SaveExpression
{
public:
    /**
     * Constructor.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expression the Expression to save.
     * @param compressXml true to compress the XML.
     * @param onFinished called when finished, with true if successful.
     * @param onCanceled called if the user canceled.
     */
    SaveExpression(bool isArpeggio, Expression expression, bool compressXml, std::function<void(bool)> onFinished, std::function<void()> onCanceled) noexcept;

    /** Performs saving the instrument. */
    void save() noexcept;

private:
    /**
     * Called when the operation finishes.
     * @param success true if success.
     */
    void onBackgroundOperationFinished(bool success) noexcept;

    /** Shows an error dialog and notifies the client. */
    void showErrorDialogAndNotify() noexcept;

    const bool isArpeggio;
    const Expression expression;
    const bool compressXml;
    const std::function<void(bool)> onFinished;
    const std::function<void()> onCanceled;

    std::unique_ptr<ModalDialog> modalDialog;

    std::unique_ptr<BackgroundOperationWithDialog<bool>> backgroundOperation;
};


}   // namespace arkostracker

