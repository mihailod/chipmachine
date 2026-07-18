#include "CreateNewInstrumentController.h"

#include "../../../../utils/NumberUtil.h"
#include "../../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../../import/sample/LoadSampleTask.h"

namespace arkostracker 
{

CreateNewInstrumentController::CreateNewInstrumentController(Listener& pListener, const int pIndex) noexcept :
        listener(pListener),
        index(pIndex),
        view(std::make_unique<CreateNewInstrumentView>(*this,
                juce::Colour(ColorConstants::colors.at(static_cast<size_t>(juce::Random::getSystemRandom().nextInt(
                        static_cast<int>(ColorConstants::colors.size()))))))       // Finds a random color.
        ),
        dialog(),
        loadSampleBackgroundTask(),
        pendingSampleData(juce::String(), juce::Colours::red, juce::String())       // Boilerplate.
{
}


// CreateNewInstrumentView::Listener method implementations.
// ============================================================

void CreateNewInstrumentController::onCreateNewPsgInstrumentValidated(const CreateNewInstrumentView::PsgInstrumentData& psgData) noexcept
{
    auto instrument = GenericInstrumentGenerator::buildFromTemplateId(psgData.getTemplateId());
    instrument->setName(psgData.getName());
    instrument->setColor(psgData.getColor().getARGB());

    listener.onCreateNewInstrumentValidated(index, std::move(instrument));
}

void CreateNewInstrumentController::onCreateNewInstrumentCancelled() noexcept
{
    listener.onCreateNewInstrumentCancelled();
}

void CreateNewInstrumentController::onCreateNewSampleInstrumentValidated(const CreateNewInstrumentView::SampleInstrumentData& pSampleData) noexcept
{
    const auto path = pSampleData.getSamplePath();
    pendingSampleData = pSampleData;            // Stored for later, when the task finishes.

    // Loads the sample on a background thread.
    auto loadSampleTask = std::make_unique<LoadSampleTask>(path);
    loadSampleBackgroundTask = std::make_unique<BackgroundTask<std::unique_ptr<SampleLoader::Result>>>(*this, std::move(loadSampleTask));
    loadSampleBackgroundTask->performTask();
}

void CreateNewInstrumentController::showLoadSampleFileErrorDialog(const SampleLoader::Outcome& outcome) noexcept
{
    jassert(dialog == nullptr);         // Dialog already there?

    const auto errorMessage = LoadSampleTask::getErrorMessage(outcome);
    dialog = SuccessOrErrorDialog::buildForError(errorMessage, [&] { onCloseDialog(); });
}

void CreateNewInstrumentController::onCloseDialog() noexcept
{
    dialog.reset();
}


// BackgroundTaskListener method implementations.
// ============================================================

void CreateNewInstrumentController::onBackgroundTaskFinished(TaskOutputState /*taskOutputState*/, const std::unique_ptr<SampleLoader::Result> result) noexcept
{
    // UI thread.

    jassert(result != nullptr);         // Not supposed to happen.
    // Any error?
    if (const auto outCome = result->getOutcome(); outCome != SampleLoader::Outcome::success) {
        showLoadSampleFileErrorDialog(outCome);
        return;
    }

    // Creates the Instrument.
    const auto sampleMemoryBlock = result->getData();
    const auto sample = std::make_shared<Sample>(*sampleMemoryBlock);
    const SamplePart samplePart(sample, Loop(0, sample->getLength() - 1),
        SamplePart::defaultAmplificationRatio, result->getSampleFrequencyHz(), PsgValues::digidrumNote,
        result->getOriginalFileName());
    auto instrument = Instrument::buildSampleInstrument(pendingSampleData.getName(), samplePart, pendingSampleData.getColor().getARGB());

    listener.onCreateNewInstrumentValidated(index, std::move(instrument));
}

}   // namespace arkostracker
