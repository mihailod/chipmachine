#include "InstrumentEditorControllerImpl.h"

#include "../../../controllers/MainController.h"
#include "InstrumentEditorControllerNoOp.h"
#include "none/NoInstrumentEditorControllerImpl.h"
#include "psg/PsgInstrumentEditorControllerImpl.h"
#include "sample/SampleInstrumentEditorControllerImpl.h"

namespace arkostracker 
{

InstrumentEditorControllerImpl::InstrumentEditorControllerImpl(MainController& pMainController) noexcept :
        mainController(pMainController),
        songController(pMainController.getSongController()),
        noOpInstrumentEditorController(std::make_unique<InstrumentEditorControllerNoOp>()),
        noInstrumentEditorController(std::make_unique<NoInstrumentEditorControllerImpl>()),
        psgInstrumentEditorController(),
        sampleInstrumentEditorController(),

        currentInstrumentEditorController(noOpInstrumentEditorController.get()),        // At first, uses a no-op controller.
        parentView()
{
    // Note: the observations to events is done whenever the parent is connected/disconnected.
}


// SelectedInstrumentIndexObserver method implementations.
// ==========================================================

void InstrumentEditorControllerImpl::onSelectedInstrumentChanged(const OptionalId& selectedInstrumentIdOptional, const bool forceSingleSelection)
{
    // Warning, there are some strange behaviors taken in account. When deleting the last instr from the list box, an instr 0 is first selected, then the instr.

    if (parentView == nullptr) {
        jassertfalse;           // Should never happen!
        return;
    }

    // What is the type the Instrument?
    const auto isInstrumentSelected = selectedInstrumentIdOptional.isPresent();
    auto isFirstInstrumentSelected = false;
    auto instrumentType = InstrumentType::psgInstrument;
    if (isInstrumentSelected) {
        const auto& selectedInstrumentId = selectedInstrumentIdOptional.getValueRef();
        songController.performOnConstInstrument(selectedInstrumentId, [&](const Instrument& instrument) {
            instrumentType = instrument.getType();
        });
        isFirstInstrumentSelected = songController.isReadOnlyInstrument(selectedInstrumentId);
    }
    // The same as the current controller?
    ItemEditorController* nextInstrumentEditorController;    // NOLINT(*-init-variables)
    if (!isInstrumentSelected || isFirstInstrumentSelected) {
        // Nothing is selected, or the first read-only instrument.
        nextInstrumentEditorController = noInstrumentEditorController.get();
    } else if (instrumentType == InstrumentType::psgInstrument) {
        // Must use the PSG Controller. Does it exist?
        if (psgInstrumentEditorController == nullptr) {
            psgInstrumentEditorController = std::make_unique<PsgInstrumentEditorControllerImpl>(mainController);
        }
        nextInstrumentEditorController = psgInstrumentEditorController.get();
    } else if (instrumentType == InstrumentType::sampleInstrument) {
        // Must use the PSG Controller. Does it exist?
        if (sampleInstrumentEditorController == nullptr) {
            sampleInstrumentEditorController = std::make_unique<SampleInstrumentEditorControllerImpl>(songController);
        }
        nextInstrumentEditorController = sampleInstrumentEditorController.get();
    } else {
        // Shouldn't happen!
        nextInstrumentEditorController = noInstrumentEditorController.get();
        jassertfalse;
    }

    jassert(nextInstrumentEditorController != nullptr);

    // Is the target new Instrument Editor Controller already here? Then notifies it about the new instrument, nothing else.
    if (nextInstrumentEditorController == currentInstrumentEditorController) {
        if (selectedInstrumentIdOptional.isPresent()) {     // If not instrument present, it must be the no-op, so doesn't need to be notified.
            currentInstrumentEditorController->onNewItemSelected(selectedInstrumentIdOptional.getValueRef(), forceSingleSelection);
        }
        return;
    }

    // If not, switches.
    // But first, notifies about the removal for the previous controller.
    currentInstrumentEditorController->onParentViewDeleted();

    // Now targets the new controller.
    currentInstrumentEditorController = nextInstrumentEditorController;

    // Makes sure the views are correctly setup.
    // FIXME Now that we have IDs, remove the "forceSingleSelection"?
    // The "force" is useful when deleting an instr, the next one becomes the current, but the index is the same.
    // Maybe an "id" inside the Instrument would be better?
    if (selectedInstrumentIdOptional.isPresent()) {
        // Must be called BEFORE onParentViewCreated... a bit patchy.
        currentInstrumentEditorController->onNewItemSelected(selectedInstrumentIdOptional.getValueRef(), forceSingleSelection);
    }
    currentInstrumentEditorController->onParentViewCreated(*parentView);    // Else, the OLD instrument (possibly deleted) is tried being displayed after a Delete.
    currentInstrumentEditorController->onParentViewResized(parentView->getXInsideComponent(), parentView->getYInsideComponent(),
                                                           parentView->getAvailableWidthInComponent(), parentView->getAvailableHeightInComponent());
}


// InstrumentEditorController method implementations.
// ======================================================

void InstrumentEditorControllerImpl::onParentViewCreated(BoundedComponent& newParentView)
{
    mainController.observers().getSelectedInstrumentIndexObservers().addObserver(this);

    parentView = &newParentView;

    // Forces the selection to see an editor.
    const auto& instrumentIdOptional = mainController.getSelectedInstrumentId();
    onSelectedInstrumentChanged(instrumentIdOptional, true);
}

void InstrumentEditorControllerImpl::onParentViewResized(const int startX, const int startY, const int newWidth, const int newHeight)
{
    currentInstrumentEditorController->onParentViewResized(startX, startY, newWidth, newHeight);
}

void InstrumentEditorControllerImpl::onParentViewFirstResize(juce::Component& paramParentView)
{
    currentInstrumentEditorController->onParentViewFirstResize(paramParentView);
}

void InstrumentEditorControllerImpl::onParentViewDeleted() noexcept
{
    currentInstrumentEditorController->onParentViewDeleted();
    // Goes back to the no-op controller, so that, when coming back, everything is well reset.
    currentInstrumentEditorController = noOpInstrumentEditorController.get();

    mainController.observers().getSelectedInstrumentIndexObservers().removeObserver(this);

    parentView = nullptr;
}

void InstrumentEditorControllerImpl::getKeyboardFocus() noexcept
{
    currentInstrumentEditorController->getKeyboardFocus();
}

}   // namespace arkostracker
