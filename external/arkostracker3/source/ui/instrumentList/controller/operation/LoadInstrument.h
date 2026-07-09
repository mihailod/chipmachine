#pragma once

#include <memory>

#include "../../../../song/instrument/Instrument.h"
#include "../../../components/dialogs/ModalDialog.h"
#include "../../../utils/backgroundOperation/BackgroundOperationWithDialog.h"

namespace arkostracker 
{

/**
 * Loads an Instrument. The static load method can be used to load without any UI.
 * The normal load opens a dialog to load it. In case of failure, opens a dialog.
 */
class LoadInstrument
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
         * @param loadedInstrument the instrument if successful, or nullptr.
         */
        virtual void onLoadInstrumentFinished(std::unique_ptr<Instrument> loadedInstrument) noexcept = 0;

        /** Called when the operation has been cancelled. */
        virtual void onLoadInstrumentCancelled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener of the events of this class.
     */
    explicit LoadInstrument(Listener& listener) noexcept;

    /** Performs the loading of the instrument. */
    void load() noexcept;

    /**
     * Attempts to load the Instrument from its file. It will be corrected.
     * This method does not do any UI work.
     * Since the operation may take a small while, it is advisable to run it on a worker thread.
     * @param fileToLoad the file to load. It may be zipped.
     * @return the instrument, or nullptr if it couldn't be loaded/parsed.
     */
    static std::unique_ptr<Instrument> load(const juce::File& fileToLoad) noexcept;

private:
/**
     * Called when the operation finishes.
     * @param result the instrument, if present.
     */
    void onBackgroundOperationFinished(std::unique_ptr<Instrument> result) noexcept;

    /** Shows an error dialog and notifies the client. */
    void showErrorDialogAndNotify() noexcept;
    /** Closes the error dialog and notifies the client. */
    void closeErrorDialogAndNotify() noexcept;

    Listener& listener;
    std::unique_ptr<ModalDialog> modalDialog;

    std::unique_ptr<BackgroundOperationWithDialog<std::unique_ptr<Instrument>>> backgroundOperation;
};

}   // namespace arkostracker
