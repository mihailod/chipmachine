#include "InstrumentTableListControllerImpl.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../business/serialization/instrument/InstrumentSerializer.h"
#include "../../../business/song/validation/CorrectInstrument.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/PlayerController.h"
#include "../../../utils/NumberUtil.h"
#include "../../containerArranger/PanelSearcher.h"
#include "../../keyboard/CommandIds.h"
#include "../view/InstrumentTableListViewImpl.h"

namespace arkostracker 
{

InstrumentTableListControllerImpl::InstrumentTableListControllerImpl(MainController& pMainController) noexcept :
        ListControllerImpl(pMainController),
        songController(pMainController.getSongController()),
        listCommands(*this),
        createNewInstrumentController(),
        saveInstrumentProcess(),
        loadInstrumentProcess()
{
    // Registers to be aware of changes.
    songController.getInstrumentObservers().addObserver(this);
    getMainController().observers().getSelectedInstrumentIndexObservers().addObserver(this);
}

InstrumentTableListControllerImpl::~InstrumentTableListControllerImpl()
{
    songController.getInstrumentObservers().removeObserver(this);
    getMainController().observers().getSelectedInstrumentIndexObservers().removeObserver(this);
}


// DataProvider method implementations.
// ======================================

int InstrumentTableListControllerImpl::getItemCount() const noexcept
{
    return std::max(songController.getInstrumentCount(), 0);
}

juce::String InstrumentTableListControllerImpl::getItemTitle(const int rowNumber) const noexcept
{
    return (rowNumber == 0) ? juce::translate("Silence (read-only)") : songController.getInstrumentNameFromIndex(rowNumber);
}

juce::String InstrumentTableListControllerImpl::getNumberPrefix() const noexcept
{
    return juce::translate("Instr ");
}

OptionalInt InstrumentTableListControllerImpl::getSelectedIndex() const noexcept
{
    const auto instrumentId = getMainController().getSelectedInstrumentId();
    if (instrumentId.isAbsent()) {
        return { };
    }

    return songController.getInstrumentIndex(instrumentId.getValueRef());
}


// SelectedInstrumentIndexObserver method implementations.
// =========================================================

void InstrumentTableListControllerImpl::onSelectedInstrumentChanged(const OptionalId& selectedInstrumentId, const bool forceSingleSelection)
{
    if (selectedInstrumentId.isAbsent()) {
        jassertfalse;       // Nothing to show?
        return;
    }

    const auto instrumentIndex = songController.getInstrumentIndex(selectedInstrumentId.getValueRef());
    if (instrumentIndex.isAbsent()) {
        jassertfalse;       // Instrument does not exist?
        return;
    }

    onSelectedItemChanged(instrumentIndex.getValueRef(), forceSingleSelection);
}


// InstrumentChangeObserver method implementations.
// ==================================================

void InstrumentTableListControllerImpl::onInstrumentChanged(const Id& id, const unsigned int whatChanged)
{
    // Only the name and colors are important.
    if (!NumberUtil::isBitPresent(whatChanged, name) && !NumberUtil::isBitPresent(whatChanged, color)) {
        return;
    }

    const auto instrumentIndex = songController.getInstrumentIndex(id);
    if (instrumentIndex.isAbsent()) {
        jassertfalse;       // Instrument does not exist?
        return;
    }
    jassert(instrumentIndex.getValueRef() > 0);

    updateItem(instrumentIndex.getValueRef());
}

void InstrumentTableListControllerImpl::onInstrumentsInvalidated()
{
    updateList();
}

void InstrumentTableListControllerImpl::onPsgInstrumentCellChanged(const Id& /*id*/, int /*cellIndex*/, bool /*mustRefreshAllAfterToo*/)
{
    // We don't care.
}


// ListControllerImpl method implementations.
// ============================================

std::unique_ptr<ListView> InstrumentTableListControllerImpl::createListView() noexcept
{
    return std::make_unique<InstrumentTableListViewImpl>(*this, *this, *this, *this);
}

OptionalId InstrumentTableListControllerImpl::getIdFromIndex(const int index) const noexcept
{
    return songController.getInstrumentId(index);
}

void InstrumentTableListControllerImpl::moveItems(const std::set<int>& indexesToMove, const int targetIndex) noexcept
{
    songController.moveInstruments(indexesToMove, targetIndex);
}

void InstrumentTableListControllerImpl::deleteItems(const std::set<int>& indexesToDelete) noexcept
{
    songController.deleteInstruments(indexesToDelete);
}

void InstrumentTableListControllerImpl::renameItem(const int itemIndex, const juce::String& newName) noexcept
{
    const auto instrumentId = songController.getInstrumentId(itemIndex);
    if (instrumentId.isAbsent()) {
        jassertfalse;
        return;
    }
    songController.setInstrumentName(instrumentId.getValueRef(), newName);
}

void InstrumentTableListControllerImpl::enterSelectedItem() noexcept
{
    getMainController().searchAndOpenPanel(PanelType::instrumentEditor);
}

void InstrumentTableListControllerImpl::promptToAddNewItemAndPerform(int itemIndex) noexcept
{
    jassert(createNewInstrumentController == nullptr);      // Controller already present?

    createNewInstrumentController = std::make_unique<CreateNewInstrumentController>(*this, itemIndex);
}

void InstrumentTableListControllerImpl::notifyNewSelectedId(const Id newSelectedItemId) noexcept
{
    getMainController().setSelectedInstrumentId(newSelectedItemId, false);
}

void InstrumentTableListControllerImpl::notifySelection(const std::vector<int>& newSelectedIndexes) noexcept
{
    getMainController().onSelectedInstrumentsChanged(newSelectedIndexes);
}

std::unique_ptr<juce::XmlElement> InstrumentTableListControllerImpl::getItemsToCopy(const std::set<int>& indexesToCopy) const noexcept
{
    std::vector<const Instrument*> instruments;

    // Reads all the Instruments.
    for (const auto index : indexesToCopy) {
        jassert(index > 0);         // Not touching the 0th!

        const auto instrumentId = songController.getInstrumentId(index);
        if (instrumentId.isAbsent()) {
            jassertfalse;
            continue;
        }

        songController.performOnConstInstrument(instrumentId.getValueRef(), [&](const Instrument& instrument) {
            instruments.push_back(&instrument);
        });
    }

    // Serializes them to XML.
    return InstrumentSerializer::serialize(instruments);
}

std::vector<Id> InstrumentTableListControllerImpl::deserializeXmlItemsAndPaste(const juce::XmlElement& serializedItems, const int targetIndex) noexcept
{
    // Deserializes the XML.
    auto instruments = InstrumentSerializer::deserializeInstruments(serializedItems);
    if (instruments.empty()) {
        jassertfalse;           // Unable to deserialize.
        return { };
    }

    // Corrects the XML. Who knows what a sneaky user modified in the XML!
    std::vector<Id> ids;
    for (auto& instrument : instruments) {
        CorrectInstrument::correctInstrument(*instrument);
        ids.push_back(instrument->getId());
    }

    // Performs the paste.
    songController.insertInstruments(targetIndex, std::move(instruments));

    return ids;
}

void InstrumentTableListControllerImpl::putAdditionalPopUpMenuItems(juce::PopupMenu& popupMenu, juce::ApplicationCommandManager& commandManager) noexcept
{
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, listLoadItem, juce::translate("Load instrument after"));
    popupMenu.addCommandItem(&commandManager, listSaveItem, juce::translate("Save instrument"));
}

bool InstrumentTableListControllerImpl::performSubCommand(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept
{
    auto success = true;

    switch (info.commandID) {
        case listLoadItem:
            loadInstrument();
            break;
        case listSaveItem:
            saveInstrument();
            break;
        default:
            success = false;
            jassertfalse;       // Command not managed?
            break;
    }

    return success;
}

ListCommands& InstrumentTableListControllerImpl::getListCommands() noexcept
{
    return listCommands;
}


// CreateNewInstrumentController::Listener method implementations.
// ==================================================================

void InstrumentTableListControllerImpl::onCreateNewInstrumentValidated(const int index, std::unique_ptr<Instrument> newInstrument) noexcept
{
    createNewInstrumentController.reset();

    auto instruments = std::vector<std::unique_ptr<Instrument>>();
    instruments.push_back(std::move(newInstrument));

    songController.insertInstruments(index, std::move(instruments));

    // Opens the instrument editor.
    getMainController().searchAndOpenPanel(PanelType::instrumentEditor);
}

void InstrumentTableListControllerImpl::onCreateNewInstrumentCancelled() noexcept
{
    createNewInstrumentController.reset();
}


// ==================================================================

void InstrumentTableListControllerImpl::loadInstrument() noexcept
{
    jassert(loadInstrumentProcess == nullptr);          // Already instantiated?

    // No need to check the index, it will be done after loading, and it has been checked before (and we cannot anywhere anyway).
    loadInstrumentProcess = std::make_unique<LoadInstrument>(*this);
    loadInstrumentProcess->load();
}

void InstrumentTableListControllerImpl::saveInstrument() noexcept
{
    jassert(saveInstrumentProcess == nullptr);          // Already instantiated?

    // Index of the instrument to save.
    const auto indexOptional = getSelectedIndex();
    if (indexOptional.isAbsent()) {
        jassertfalse;       // Should have been detected before.
        return;
    }
    const auto instrumentId = songController.getInstrumentId(indexOptional.getValue());
    if (instrumentId.isAbsent()) {
        jassertfalse;
        return;
    }

    // Copies the instrument.
    std::unique_ptr<Instrument> localInstrument;
    songController.performOnConstInstrument(instrumentId.getValueRef(), [&](const Instrument& instrument) {
        localInstrument = std::make_unique<Instrument>(instrument);
    });

    // Performs the save.
    const auto compressXml = PreferencesManager::getInstance().areSavedFilesCompressed();
    saveInstrumentProcess = std::make_unique<SaveInstrument>(*this, std::move(localInstrument), compressXml);
    saveInstrumentProcess->save();
}


// SaveInstrument::Listener method implementations.
// ===================================================

void InstrumentTableListControllerImpl::onSaveInstrumentFinished(bool /*success*/) noexcept
{
    saveInstrumentProcess.reset();
}

void InstrumentTableListControllerImpl::onSaveInstrumentCancelled() noexcept
{
    saveInstrumentProcess.reset();
}

// LoadInstrument::Listener method implementations.
// ===================================================

void InstrumentTableListControllerImpl::onLoadInstrumentFinished(std::unique_ptr<Instrument> loadedInstrument) noexcept
{
    loadInstrumentProcess.reset();

    if (loadedInstrument == nullptr) {
        return;         // Error dialog already shown, we can leave.
    }
    // Where to insert? Should be valid, checked before.
    const auto indexOptional = getSelectedIndex();
    if (indexOptional.isAbsent()) {
        jassertfalse;       // Should have been detected before.
        return;
    }
    const auto index = indexOptional.getValue() + 1;        // Inserts after.

    std::vector<std::unique_ptr<Instrument>> instruments;
    instruments.push_back(std::move(loadedInstrument));
    songController.insertInstruments(index, std::move(instruments));
}

void InstrumentTableListControllerImpl::onLoadInstrumentCancelled() noexcept
{
    loadInstrumentProcess.reset();
}


// InstrumentDataController method implementations.
// ===================================================

void InstrumentTableListControllerImpl::setInstrumentColor(const int instrumentIndex, const juce::Colour pColor) noexcept
{
    const auto instrumentId = songController.getInstrumentId(instrumentIndex);
    if (instrumentId.isAbsent()) {
        jassertfalse;
        return;
    }
    songController.setInstrumentColor(instrumentId.getValueRef(), pColor);
}

InstrumentType InstrumentTableListControllerImpl::getInstrumentType(const int instrumentIndex) const noexcept
{
    auto instrumentType = InstrumentType::psgInstrument;

    const auto instrumentId = songController.getInstrumentId(instrumentIndex);
    if (instrumentId.isAbsent()) {
        jassertfalse;
        return instrumentType;
    }

    songController.performOnConstInstrument(instrumentId.getValueRef(), [&](const Instrument& instrument) {
        instrumentType = instrument.getType();
    });

    return instrumentType;
}

juce::Colour InstrumentTableListControllerImpl::getInstrumentColor(const int instrumentIndex) const noexcept
{
    juce::Colour pColor;
    const auto instrumentId = songController.getInstrumentId(instrumentIndex);
    if (instrumentId.isAbsent()) {
        jassertfalse;
        return pColor;
    }

    songController.performOnConstInstrument(instrumentId.getValueRef(), [&](const Instrument& instrument) {
        pColor = juce::Colour(instrument.getArgbColor());
    });

    return pColor;
}

void InstrumentTableListControllerImpl::onInstrumentSymbolClicked(const int instrumentIndex) noexcept
{
    const auto instrumentId = songController.getInstrumentId(instrumentIndex);
    if (instrumentId.isAbsent()) {
        jassertfalse;
        return;
    }

    getMainController().getPlayerController().play({ }, false, instrumentId.getValueRef());
}

PanelType InstrumentTableListControllerImpl::getRelatedPanelToOpen() const noexcept
{
    return PanelType::instrumentEditor;
}

}   // namespace arkostracker
