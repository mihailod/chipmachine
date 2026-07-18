#pragma once

#include "PsgInstrumentEditorContextualMenu.h"
#include "PsgQuickEditController.h"
#include "../../../../controllers/observers/InstrumentChangeObserver.h"
#include "../../../editorWithBars/controller/EditorWithBarsControllerSpecific.h"

namespace arkostracker 
{

class SongController;

/** Implementation to the Instrument Editor specific controller. */
class PsgInstrumentEditorControllerSpecificImpl final : public EditorWithBarsControllerSpecific,
                                                        public InstrumentChangeObserver
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param editorController the editor controller, to which all events will be sent and will know what to do with them.
     */
    PsgInstrumentEditorControllerSpecificImpl(MainController& mainController, EditorWithBarsController& editorController) noexcept;

    /** Destructor. */
    ~PsgInstrumentEditorControllerSpecificImpl() override;


    // EditorWithBarsControllerSpecific method implementations.
    // ===========================================================
    OptionalId getCurrentItemId() const noexcept override;
    std::unique_ptr<EditorWithBarsView> createView(EditorWithBarsController& editorController, int xZoomRate, const OptionalId& selectedItemId) noexcept override;
    bool shouldViewInstanceRecreated(const OptionalId& currentItemId, const Id& newItemId) const noexcept override;
    EditorWithBarsView::DisplayTopHeaderData getDisplayedTopHeaderData(const Id& itemId) const noexcept override;
    int getItemLength(const Id& itemId) const noexcept override;
    std::unordered_map<AreaType, BarAndCaptionData> createBarsData(const Id& itemId, int barIndex) const noexcept override;
    void checkValueAndChangeItem(const Id& itemId, AreaType areaType, int barIndex, int value) noexcept override;
    std::unordered_set<AreaType> getAreaTypes() const noexcept override;
    std::unordered_map<AreaType, BarAreaSize> getAreaTypesToInitialSize() const noexcept override;
    std::unordered_map<AreaType, BarAreaSize> getAreaTypesToMaximumSize() const noexcept override;
    bool isAutoSpreadAllowed() const noexcept override;
    Loop getAutoSpreadLoop(const Id& itemId, AreaType areaType) const noexcept override;
    std::pair<int, Loop> getItemLengthAndLoop(const Id& itemId) const noexcept override;
    void setItemMetadata(const Id& itemId, OptionalInt newStart, OptionalInt newEnd, OptionalInt newSpeed, OptionalInt newShift, OptionalBool newIsLoop, OptionalBool newIsRetrig,
                         std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop) noexcept override;
    bool duplicateColumn(const Id& itemId, int cellIndex) noexcept override;
    bool duplicateValue(const Id& itemId, int cellIndex, AreaType areaType) noexcept override;
    bool deleteColumn(const Id& itemId, int cellIndex) noexcept override;
    void toggleRetrig(const Id& itemId, int cellIndex) noexcept override;
    void generateIncreasingVolume(const Id& itemId, int cellIndex) noexcept override;
    void generateDecreasingVolume(const Id& itemId, int cellIndex) noexcept override;

    void openContextualMenu(const Id& itemId, AreaType areaType, int cellIndex, std::function<void()> onMenuItemClicked) noexcept override;
    void onUserWantsToEditValue(const Id& itemId, AreaType areaType, int barIndex) noexcept override;
    void onUserWantsToResetValue(const Id& itemId, AreaType areaType, int barIndex) noexcept override;

    bool isItemMetadataChangeFlagsInteresting(unsigned int whatChanged) noexcept override;
    void onUserWantsToToggleLoop(const Id& itemId, OptionalValue<AreaType> areaType) noexcept override;
    void onUserWantsToIncreaseSpeed(const Id& itemId, int step) noexcept override;
    void onUserWantsToToggleInstrumentRetrig(const Id& itemId) noexcept override;
    void onUserWantsToSetAutoSpreadStartAndEnd(const Id& itemId, AreaType areaType, OptionalInt newStart, OptionalInt newEnd) noexcept override;
    int getItemSpeed(const Id& itemId) noexcept override;
    int getItemShift(const Id& itemId) noexcept override;
    int correctItemSpeed(int speed) const noexcept override;
    int correctItemShift(int shift) const noexcept override;
    const std::unordered_map<AreaType, std::vector<int>>& getSpecificAreaTypeToSlideSpeed() const noexcept override;
    bool doesContainOnlyDefaultData(const Id& itemId, AreaType areaType) noexcept override;
    BarAreaSize getMinimumSizeForData(const Id& itemId, AreaType areaType, bool forceShrinkFull) noexcept override;
    bool canHideRow(const Id& itemId, AreaType areaType) noexcept override;

    // InstrumentChangeObserver method implementations.
    // ===================================================
    void onInstrumentChanged(const Id& itemId, unsigned int whatChanged) override;
    void onPsgInstrumentCellChanged(const Id& instrumentId, int cellIndex, bool mustRefreshAllAfterToo) override;
    void onInstrumentsInvalidated() override;


private:
    /** @return the size of the given area. */
    BarAreaSize getAreaSize(AreaType areaType) const noexcept;

    /**
     * The user wants to toggle the auto-spread loop.
     * @param instrumentId the id of the Instrument.
     * @param areaType the area to toggle.
     */
    void onUserWantsToToggleAutoSpread(const Id& instrumentId, AreaType areaType) const noexcept;

    MainController& mainController;
    SongController& songController;
    EditorWithBarsController& editorController;
    PsgQuickEditController quickEditController;

    std::unique_ptr<PsgInstrumentEditorContextualMenu> contextualMenu;
};

}   // namespace arkostracker
