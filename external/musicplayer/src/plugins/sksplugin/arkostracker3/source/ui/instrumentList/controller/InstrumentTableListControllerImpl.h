#pragma once

#include "../../../controllers/observers/InstrumentChangeObserver.h"
#include "../../../controllers/observers/SelectedInstrumentIndexObserver.h"
#include "../../lists/controller/ListControllerImpl.h"
#include "InstrumentDataController.h"
#include "operation/CreateNewInstrumentController.h"
#include "operation/LoadInstrument.h"
#include "operation/SaveInstrument.h"

namespace arkostracker 
{

class MainController;
class SongController;

/** Implementation of the Instrument Table List Controller. */
class InstrumentTableListControllerImpl final : public ListControllerImpl,
                                                public SelectedInstrumentIndexObserver,
                                                public InstrumentChangeObserver,
                                                public CreateNewInstrumentController::Listener,
                                                public SaveInstrument::Listener,
                                                public LoadInstrument::Listener,
                                                public InstrumentDataController
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit InstrumentTableListControllerImpl(MainController& mainController) noexcept;

    /** Destructor. */
    ~InstrumentTableListControllerImpl() override;

    // SelectedInstrumentIndexObserver method implementations.
    // =========================================================
    void onSelectedInstrumentChanged(const OptionalId& selectedInstrumentId, bool forceSingleSelection) override;

    // InstrumentChangeObserver method implementations.
    // ==================================================
    void onInstrumentChanged(const Id& id, unsigned int whatChanged) override;
    void onInstrumentsInvalidated() override;
    void onPsgInstrumentCellChanged(const Id& id, int cellIndex, bool mustRefreshAllAfterToo) override;

    // DataProvider method implementations.
    // ======================================
    int getItemCount() const noexcept override;
    juce::String getItemTitle(int rowNumber) const noexcept override;
    juce::String getNumberPrefix() const noexcept override;
    OptionalInt getSelectedIndex() const noexcept override;

    // InstrumentDataController method implementations.
    // ===================================================
    void setInstrumentColor(int instrumentIndex, juce::Colour color) noexcept override;
    InstrumentType getInstrumentType(int instrumentIndex) const noexcept override;
    juce::Colour getInstrumentColor(int instrumentIndex) const noexcept override;
    void onInstrumentSymbolClicked(int instrumentIndex) noexcept override;

    // CreateNewInstrumentController::Listener method implementations.
    // ==================================================================
    void onCreateNewInstrumentValidated(int index, std::unique_ptr<Instrument> newInstrument) noexcept override;
    void onCreateNewInstrumentCancelled() noexcept override;

    // SaveInstrument::Listener method implementations.
    // ===================================================
    void onSaveInstrumentFinished(bool success) noexcept override;
    void onSaveInstrumentCancelled() noexcept override;

    // LoadInstrument::Listener method implementations.
    // ===================================================
    void onLoadInstrumentFinished(std::unique_ptr<Instrument> loadedInstrument) noexcept override;
    void onLoadInstrumentCancelled() noexcept override;

    // ListControllerImpl method implementations.
    // ============================================
    std::unique_ptr<ListView> createListView() noexcept override;
    OptionalId getIdFromIndex(int index) const noexcept override;
    void moveItems(const std::set<int>& indexesToMove, int targetIndex) noexcept override;
    void deleteItems(const std::set<int>& indexesToDelete) noexcept override;
    void renameItem(int itemIndex, const juce::String& newName) noexcept override;
    void promptToAddNewItemAndPerform(int itemIndex) noexcept override;
    void notifyNewSelectedId(Id newSelectedItemId) noexcept override;
    void notifySelection(const std::vector<int>& selectedIndexes) noexcept override;
    std::unique_ptr<juce::XmlElement> getItemsToCopy(const std::set<int>& indexesToCopy) const noexcept override;
    std::vector<Id> deserializeXmlItemsAndPaste(const juce::XmlElement& serializedItems, int targetIndex) noexcept override;
    void putAdditionalPopUpMenuItems(juce::PopupMenu& popupMenu, juce::ApplicationCommandManager& commandManager) noexcept override;
    bool performSubCommand(const juce::ApplicationCommandTarget::InvocationInfo& info) noexcept override;
    ListCommands& getListCommands() noexcept override;
    void enterSelectedItem() noexcept override;
    PanelType getRelatedPanelToOpen() const noexcept override;

private:
    /** Starts the process of loading an instrument (prompt, etc.). */
    void loadInstrument() noexcept;
    /** Starts the process of saving an instrument (prompt, etc.). */
    void saveInstrument() noexcept;

    SongController& songController;
    ListCommands listCommands;

    std::unique_ptr<CreateNewInstrumentController> createNewInstrumentController;

    std::unique_ptr<SaveInstrument> saveInstrumentProcess;
    std::unique_ptr<LoadInstrument> loadInstrumentProcess;
};

}   // namespace arkostracker
