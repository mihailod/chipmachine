#include "ArpeggioTableEditorControllerSpecificImpl.h"

#include "../../../controllers/MainController.h"
#include "../../../song/ExpressionConstants.h"
#include "../../../song/cells/CellConstants.h"
#include "../../../utils/NoteUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../view/ArpeggioTableEditorViewImpl.h"
#include "ArpeggioBarCreator.h"

namespace arkostracker 
{

ArpeggioTableEditorControllerSpecificImpl::ArpeggioTableEditorControllerSpecificImpl(MainController& pMainController, EditorWithBarsController& pEditorController) noexcept :
        ExpressionTableEditorControllerSpecific(true, pMainController, pEditorController),
        areaTypeToSlideSpeeds()             // Empty, no special need.
{
}


// EditorWithBarsControllerSpecific method implementations.
// ===========================================================

std::unordered_set<AreaType> ArpeggioTableEditorControllerSpecificImpl::getAreaTypes() const noexcept
{
    return { AreaType::primaryArpeggioNoteInOctave, AreaType::primaryArpeggioOctave };
}

std::unordered_map<AreaType, BarAreaSize> ArpeggioTableEditorControllerSpecificImpl::getAreaTypesToInitialSize() const noexcept
{
    return {
            { AreaType::primaryArpeggioOctave, BarAreaSize::small },
            { AreaType::primaryArpeggioNoteInOctave, BarAreaSize::normal },
    };
}

std::unordered_map<AreaType, BarAreaSize> ArpeggioTableEditorControllerSpecificImpl::getAreaTypesToMaximumSize() const noexcept
{
    return {
            { AreaType::primaryArpeggioNoteInOctave, BarAreaSize::normal },
            { AreaType::primaryArpeggioOctave, BarAreaSize::normal },
    };
}

const std::unordered_map<AreaType, std::vector<int>>& ArpeggioTableEditorControllerSpecificImpl::getSpecificAreaTypeToSlideSpeed() const noexcept
{
    return areaTypeToSlideSpeeds;
}


// ExpressionTableEditorControllerSpecific method implementations.
// ==================================================================

std::unique_ptr<EditorWithBarsView> ArpeggioTableEditorControllerSpecificImpl::createViewInstance(EditorWithBarsController& pEditorController, int pXZoomRate) const noexcept
{
    return std::make_unique<ArpeggioTableEditorViewImpl>(pEditorController, pXZoomRate);
}

void ArpeggioTableEditorControllerSpecificImpl::fillAreaTypeToData(std::unordered_map<AreaType, BarAndCaptionData>& areaTypeToDataToFill, const int value, const bool withinLoop,
                                                                   const bool outOfBounds, const bool hovered,
                                                                   const bool cursorXOk, const AreaType areaTypeForCursor) const noexcept
{
    jassert(areaTypeToDataToFill.empty());      // Should be empty!!

    constexpr auto selected = false;        // For now... "Hovered" is also full row. This may change in the future.

    // Arpeggio note in octave.
    auto primaryArpeggioNoteBarData = ArpeggioBarCreator::createArpeggioNoteInOctaveBarData(value, withinLoop, outOfBounds, hovered, selected,
        cursorXOk && (areaTypeForCursor == AreaType::primaryArpeggioNoteInOctave));
    areaTypeToDataToFill.insert(std::make_pair(AreaType::primaryArpeggioNoteInOctave, primaryArpeggioNoteBarData));

    // Arpeggio octave.
    const auto barSize = getEditorController().getAreaSize(AreaType::primaryArpeggioOctave);
    auto primaryArpeggioOctaveBarData = ArpeggioBarCreator::createArpeggioOctaveBarData(barSize, value, withinLoop, outOfBounds, hovered, selected,
        cursorXOk && (areaTypeForCursor == AreaType::primaryArpeggioOctave));
    areaTypeToDataToFill.insert(std::make_pair(AreaType::primaryArpeggioOctave, primaryArpeggioOctaveBarData));
}

OptionalInt ArpeggioTableEditorControllerSpecificImpl::correctAndCheckValueFromUi(SongController& pSongController, const Id& expressionId,
                                                                                  const int barIndex, const AreaType areaType, const int originalValue) const noexcept
{
    // Is the given value an octave or an note? This depends on the area.
    OptionalInt noteInOctave;
    OptionalInt octave;
    switch (areaType) {
        case AreaType::primaryArpeggioNoteInOctave:
            noteInOctave = NumberUtil::correctNumber(originalValue, CellConstants::firstNoteInOctave, CellConstants::lastNoteInOctave);
            break;
        case AreaType::primaryArpeggioOctave:
            octave = NumberUtil::correctNumber(originalValue, ExpressionConstants::minimumArpeggioOctave, ExpressionConstants::maximumArpeggioOctave);
            break;
        case AreaType::soundType: [[fallthrough]];
        case AreaType::envelope: [[fallthrough]];
        case AreaType::noise: [[fallthrough]];
        case AreaType::primaryPeriod: [[fallthrough]];
        case AreaType::primaryPitch: [[fallthrough]];
        case AreaType::secondaryPeriod: [[fallthrough]];
        case AreaType::secondaryArpeggioNoteInOctave: [[fallthrough]];
        case AreaType::secondaryArpeggioOctave: [[fallthrough]];
        case AreaType::secondaryPitch:
        case AreaType::sidIsActivatedAndRatio:
        case AreaType::sidBalancePercent:
        case AreaType::sidLowShelfVolume:
        case AreaType::sidArpeggio:
        case AreaType::sidPitch:
            jassertfalse;               // Shouldn't happen!
            return { };
    }

    // Gets the missing data to form the final arpeggio.
    auto currentArpeggio = 0;
    pSongController.getExpressionHandler(true).performOnConstExpression(expressionId, [&] (const Expression& expression) {
        currentArpeggio = expression.getValue(barIndex);          // Takes care of the out-of-bound indexes.

        if (noteInOctave.isAbsent()) {
            noteInOctave = NoteUtil::getNoteInOctave(currentArpeggio);
        } else if (octave.isAbsent()) {
            octave = NoteUtil::getOctaveFromNote(currentArpeggio);
        }
    });
    jassert(noteInOctave.isPresent() && octave.isPresent());          // Both are determined now!

    const auto newArpeggio = NoteUtil::getNote(noteInOctave.getValue(), octave.getValue());

    // Is there a change?
    return (newArpeggio == currentArpeggio) ? OptionalInt() : newArpeggio;
}

}   // namespace arkostracker
