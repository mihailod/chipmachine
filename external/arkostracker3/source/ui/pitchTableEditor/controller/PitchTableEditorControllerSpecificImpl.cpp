#include "PitchTableEditorControllerSpecificImpl.h"

#include "../../../controllers/MainController.h"
#include "../../../song/ExpressionConstants.h"
#include "../../../utils/NumberUtil.h"
#include "../view/PitchTableEditorViewImpl.h"
#include "PitchBarCreator.h"

namespace arkostracker 
{

PitchTableEditorControllerSpecificImpl::PitchTableEditorControllerSpecificImpl(MainController& pMainController, EditorWithBarsController& pEditorController) noexcept :
        ExpressionTableEditorControllerSpecific(false, pMainController, pEditorController),
        areaTypeToSlideSpeeds()             // Empty, no special need.
{
}


// EditorWithBarsControllerSpecific method implementations.
// ===========================================================

std::unordered_set<AreaType> PitchTableEditorControllerSpecificImpl::getAreaTypes() const noexcept
{
    return { AreaType::primaryPitch };
}

std::unordered_map<AreaType, BarAreaSize> PitchTableEditorControllerSpecificImpl::getAreaTypesToInitialSize() const noexcept
{
    return {
            { AreaType::primaryPitch, BarAreaSize::small },
    };
}

std::unordered_map<AreaType, BarAreaSize> PitchTableEditorControllerSpecificImpl::getAreaTypesToMaximumSize() const noexcept
{
    return { };
}

const std::unordered_map<AreaType, std::vector<int>>& PitchTableEditorControllerSpecificImpl::getSpecificAreaTypeToSlideSpeed() const noexcept
{
    return areaTypeToSlideSpeeds;
}


// ExpressionTableEditorControllerSpecific method implementations.
// ==================================================================

std::unique_ptr<EditorWithBarsView> PitchTableEditorControllerSpecificImpl::createViewInstance(EditorWithBarsController& pEditorController, int pXZoomRate) const noexcept
{
    return std::make_unique<PitchTableEditorViewImpl>(pEditorController, pXZoomRate);
}

void PitchTableEditorControllerSpecificImpl::fillAreaTypeToData(std::unordered_map<AreaType, BarAndCaptionData>& areaTypeToDataToFill, const int value, const bool withinLoop,
                                                                const bool outOfBounds, const bool hovered,
                                                                const bool cursorXOk, const AreaType areaTypeForCursor) const noexcept
{
    jassert(areaTypeToDataToFill.empty());      // Should be empty!!

    constexpr auto selected = false;        // For now... "Hovered" is also full row. This may change in the future.

    // Arpeggio note in octave.
    constexpr auto areaType = AreaType::primaryPitch;
    const auto areaSize = getEditorController().getAreaSize(areaType);
    auto pitchBarData = PitchBarCreator::createPitchBarData(areaSize, value, withinLoop, outOfBounds, hovered, selected, cursorXOk && (areaType == areaTypeForCursor));
    areaTypeToDataToFill.insert(std::make_pair(areaType, pitchBarData));
}

OptionalInt PitchTableEditorControllerSpecificImpl::correctAndCheckValueFromUi(SongController& pSongController, const Id& pExpressionId, const int pBarIndex, AreaType /*areaType*/,
                                                                                  const int pOriginalValue) const noexcept
{
    // Corrects the pitch.
    const auto newPitch = NumberUtil::correctNumber(pOriginalValue, ExpressionConstants::minimumPitch, ExpressionConstants::maximumPitch);

    // Gets the missing data to form the final arpeggio.
    auto currentPitch = 0;
    pSongController.getExpressionHandler(false).performOnConstExpression(pExpressionId, [&](const Expression& expression) {
        currentPitch = expression.getValue(pBarIndex);          // Takes care of the out-of-bound indexes.
    });

    // Is there a change?
    return (newPitch == currentPitch) ? OptionalInt() : newPitch;
}

}   // namespace arkostracker
