#pragma once

#include <memory>

#include "../../../../business/instrument/SampleLoader.h"
#include "../../../../song/instrument/Instrument.h"
#include "../../../utils/backgroundTask/BackgroundTask.h"
#include "../../view/CreateNewInstrumentView.h"

namespace arkostracker 
{

/** Controller to create the new instrument. It holds the view. */
class CreateNewInstrumentController final : public CreateNewInstrumentView::Listener,
                                            public BackgroundTaskListener<std::unique_ptr<SampleLoader::Result>>
{
public:
    /** Listener to the events of this class. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user has validated the name of the expression to create.
         * @param index the index that was given when creating this object, as a convenience.
         * @param newInstrument the new instrument.
         */
        virtual void onCreateNewInstrumentValidated(int index, std::unique_ptr<Instrument> newInstrument) noexcept = 0;

        /** Called when the user cancelled the creation of the instrument. */
        virtual void onCreateNewInstrumentCancelled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events of this class.
     * @param index a convenience index that is stored and will be returned in the callback.
     */
    CreateNewInstrumentController(Listener& listener, int index) noexcept;

    // CreateNewInstrumentView::Listener method implementations.
    // ============================================================
    void onCreateNewPsgInstrumentValidated(const CreateNewInstrumentView::PsgInstrumentData& psgData) noexcept override;
    void onCreateNewSampleInstrumentValidated(const CreateNewInstrumentView::SampleInstrumentData& sampleData) noexcept override;
    void onCreateNewInstrumentCancelled() noexcept override;

    // BackgroundTaskListener method implementations.
    // ============================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<SampleLoader::Result> result) noexcept override;

private:
    /**
     * Shows an error dialog because the sample couldn't be loaded.
     * @param outcome why it failed.
     */
    void showLoadSampleFileErrorDialog(const SampleLoader::Outcome& outcome) noexcept;
    /** Closes the possible dialog. */
    void onCloseDialog() noexcept;

    Listener& listener;
    const int index;

    std::unique_ptr<CreateNewInstrumentView> view;

    std::unique_ptr<ModalDialog> dialog;
    std::unique_ptr<BackgroundTask<std::unique_ptr<SampleLoader::Result>>> loadSampleBackgroundTask;
    CreateNewInstrumentView::SampleInstrumentData pendingSampleData;               // Stored for later use.
};

}   // namespace arkostracker
