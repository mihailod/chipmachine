#pragma once

#include "../../expressionTableEditor/controller/ExpressionTableEditorControllerSpecific.h"

namespace arkostracker 
{

class MainController;
class EditorWithBarsController;

/** Implementation to the Arpeggio specific controller. Most code are from the Expression controller parent. */
class ArpeggioTableEditorControllerSpecificImpl final : public ExpressionTableEditorControllerSpecific
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     * @param editorController the editor controller, to which all events will be sent and will know what to do with them.
     */
    ArpeggioTableEditorControllerSpecificImpl(MainController& mainController, EditorWithBarsController& editorController) noexcept;

    // EditorWithBarsControllerSpecific method implementations.
    // ===========================================================
    std::unordered_set<AreaType> getAreaTypes() const noexcept override;
    std::unordered_map<AreaType, BarAreaSize> getAreaTypesToInitialSize() const noexcept override;
    std::unordered_map<AreaType, BarAreaSize> getAreaTypesToMaximumSize() const noexcept override;
    const std::unordered_map<AreaType, std::vector<int>>& getSpecificAreaTypeToSlideSpeed() const noexcept override;

    // ExpressionTableEditorControllerSpecific method implementations.
    // ==================================================================
    std::unique_ptr<EditorWithBarsView> createViewInstance(EditorWithBarsController& editorController, int xZoomRate) const noexcept override;
    void fillAreaTypeToData(std::unordered_map<AreaType, BarAndCaptionData>& areaTypeToDataToFill, int value, bool withinLoop, bool outOfBounds,
                            bool hovered, bool cursorXOk, AreaType areaTypeForCursor) const noexcept override;
    OptionalInt correctAndCheckValueFromUi(SongController& songController, const Id& expressionId, int barIndex, AreaType areaType, int originalValue) const noexcept override;

private:
    std::unordered_map<AreaType, std::vector<int>> areaTypeToSlideSpeeds;
};

}   // namespace arkostracker
