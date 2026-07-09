#pragma once

#include "../../../controllers/observers/ExpressionChangeObserver.h"
#include "../../../controllers/observers/SelectedExpressionIndexObserver.h"
#include "../../editorWithBars/controller/EditorWithBarsControllerSpecific.h"
#include "../view/ExpressionEditorContextualMenu.h"
#include "ExpressionQuickEditController.h"

namespace arkostracker 
{

class MainController;
class SongController;

/** Base Specific controller for an Expression Table Editor. All the specific behavior of an Expression is left to the subclasses. */
class ExpressionTableEditorControllerSpecific : public EditorWithBarsControllerSpecific,
                                                public ExpressionChangeObserver,
                                                public SelectedExpressionIndexObserver
{
public:
    /**
     * Constructor.
     * @param isArpeggio true if for Arpeggio, false if Pitch.
     * @param mainController the Main Controller.
     * @param editorController the editor controller, to which all events will be sent and will know what to do with them.
     */
    ExpressionTableEditorControllerSpecific(bool isArpeggio, MainController& mainController, EditorWithBarsController& editorController) noexcept;

    /** Destructor. */
    ~ExpressionTableEditorControllerSpecific() override;


    // EditorWithBarsControllerSpecific method implementations.
    // ================================================================
    OptionalId getCurrentItemId() const noexcept override;
    std::unique_ptr<EditorWithBarsView> createView(EditorWithBarsController& editorController, int xZoomRate, const OptionalId& selectedItemId) noexcept override;
    bool shouldViewInstanceRecreated(const OptionalId& currentItemId, const Id& newItemId) const noexcept override;
    EditorWithBarsView::DisplayTopHeaderData getDisplayedTopHeaderData(const Id& itemId) const noexcept override;
    int getItemLength(const Id& currentItemId) const noexcept override;
    std::unordered_map<AreaType, BarAndCaptionData> createBarsData(const Id& expressionId, int barIndex) const noexcept override;
    void checkValueAndChangeItem(const Id& expressionId, AreaType areaType, int barIndex, int value) noexcept override;
    void onUserWantsToResetValue(const Id& itemId, AreaType areaType, int barIndex) noexcept override;
    bool isAutoSpreadAllowed() const noexcept override;
    Loop getAutoSpreadLoop(const Id& expressionId, AreaType areaType) const noexcept override;
    std::pair<int, Loop> getItemLengthAndLoop(const Id& expressionId) const noexcept override;
    void setItemMetadata(const Id& expressionId, OptionalInt newStart, OptionalInt newEnd, OptionalInt newSpeed, OptionalInt newShift, OptionalBool newIsLoop, OptionalBool newIsRetrig,
                         std::unordered_map<PsgSection, Loop> modifiedSectionToAutoSpreadLoop) noexcept override;
    bool isItemMetadataChangeFlagsInteresting(unsigned int whatChanged) noexcept override;
    void onUserWantsToToggleLoop(const Id& expressionId, OptionalValue<AreaType> areaType) noexcept override;
    void onUserWantsToToggleInstrumentRetrig(const Id& expressionId) noexcept override;
    void onUserWantsToSetAutoSpreadStartAndEnd(const Id& expressionId, AreaType areaType, OptionalInt newStart, OptionalInt newEnd) noexcept override;
    void onUserWantsToIncreaseSpeed(const Id& itemId, int step) noexcept override;
    int getItemSpeed(const Id& expressionId) noexcept override;
    int getItemShift(const Id& expressionId) noexcept override;
    int correctItemSpeed(int speed) const noexcept override;
    int correctItemShift(int shift) const noexcept override;
    bool duplicateColumn(const Id& itemId, int cellIndex) noexcept override;
    bool duplicateValue(const Id& itemId, int cellIndex, AreaType areaType) noexcept override;
    bool deleteColumn(const Id& itemId, int cellIndex) noexcept override;
    void toggleRetrig(const Id &itemId, int cellIndex) noexcept override;
    void generateIncreasingVolume(const Id &itemId, int cellIndex) noexcept override;
    void generateDecreasingVolume(const Id &itemId, int cellIndex) noexcept override;

    void openContextualMenu(const Id& itemId, AreaType areaType, int cellIndex, std::function<void()> onMenuItemClicked) noexcept override;
    void onUserWantsToEditValue(const Id& itemId, AreaType areaType, int barIndex) noexcept override;
    bool doesContainOnlyDefaultData(const Id& itemId, AreaType areaType) noexcept override;
    bool canHideRow(const Id& itemId, AreaType areaType) noexcept override;
    BarAreaSize getMinimumSizeForData(const Id& itemId, AreaType areaType, bool forceShrinkFull) noexcept override;

    // ExpressionChangeObserver method implementations.
    // ===================================================
    void onExpressionChanged(const Id& expressionId, unsigned int whatChanged) override;
    void onExpressionCellChanged(const Id& expressionId, int cellIndex, bool mustAlsoRefreshPastIndex) override;
    void onExpressionsInvalidated() override;

    // SelectedExpressionIndexObserver method implementations.
    // ==========================================================
    void onSelectedExpressionChanged(bool isArpeggio, const OptionalId& selectedExpressionId, bool forceSingleSelection) override;

    // ==========================================================

    /** @return the Editor Controller. */
    EditorWithBarsController& getEditorController() const;

    /**
     * Creates a View instance.
     * @param editorController the controller.
     * @param xZoomRate the X zoom rate.
     * @return the view.
     */
    virtual std::unique_ptr<EditorWithBarsView> createViewInstance(EditorWithBarsController& editorController, int xZoomRate) const noexcept = 0;

    /**
     * Fills a map linking the data to display, for one column, for each area, from one value. The result may be only one area, or more.
     * @param areaTypeToDataToFill an empty map to fill.
     * @param value the input value.
     * @param withinLoop true if before the end loop.
     * @param outOfBounds true if after then length.
     * @param hovered true if hovered by mouse. For now, this is per "row".
     * @param cursorXOk true if the cursor is in this row.
     * @param areaTypeForCursor only relevant if cursorXOk. The area type where the cursor is.
     */
    virtual void fillAreaTypeToData(std::unordered_map<AreaType, BarAndCaptionData>& areaTypeToDataToFill, int value, bool withinLoop, bool outOfBounds,
                                    bool hovered, bool cursorXOk, AreaType areaTypeForCursor) const noexcept = 0;

    /**
     * Asks the subclass to correct the given UI value, and indicates whether the value is worth being modified (can be the same as the current one after correction).
     * @param songController the Song Controller, as a convenience.
     * @param expressionId the ID of the expression.
     * @param barIndex the index of the bar. Always within range.
     * @param areaType the AreaType that is modified.
     * @param originalValue the value from the UI. May be out of range.
     * @return the corrected value, absent if not worth being stored.
     */
    virtual OptionalInt correctAndCheckValueFromUi(SongController& songController, const Id& expressionId, int barIndex, AreaType areaType, int originalValue) const noexcept = 0;

private:
    const bool isArpeggio;
    MainController& mainController;
    SongController& songController;

    std::unique_ptr<ExpressionEditorContextualMenu> contextualMenu;

    EditorWithBarsController& editorController;
    ExpressionQuickEditController quickEditController;
};

}   // namespace arkostracker
