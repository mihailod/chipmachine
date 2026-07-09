#pragma once

#include <memory>

#include "../../../../song/instrument/Instrument.h"
#include "../../../components/dialogs/ModalDialog.h"
#include "../../../utils/backgroundOperation/BackgroundOperationWithDialog.h"

namespace arkostracker 
{

/**
 * Saves an Instrument. Opens a dialog to store it, as a ZIP XML file.
 * In case of failure, opens a dialog.
 */
class SaveInstrument
{
public:
    /** Listener to the events of this class. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the operation has finished, successfully or not.
         * @param success true if the saving has been done successfully.
         */
        virtual void onSaveInstrumentFinished(bool success) noexcept = 0;

        /** Called when the operation has been cancelled. */
        virtual void onSaveInstrumentCancelled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener of the events of this class.
     * @param instrument the Instrument to save.
     * @param compressXml true to compress to ZIP.
     */
    SaveInstrument(Listener& listener, std::unique_ptr<Instrument> instrument, bool compressXml) noexcept;

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

    Listener& listener;
    std::unique_ptr<Instrument> instrument;
    bool compressXml;

    std::unique_ptr<ModalDialog> modalDialog;
    std::unique_ptr<BackgroundOperationWithDialog<bool>> backgroundOperation;
};



}   // namespace arkostracker

