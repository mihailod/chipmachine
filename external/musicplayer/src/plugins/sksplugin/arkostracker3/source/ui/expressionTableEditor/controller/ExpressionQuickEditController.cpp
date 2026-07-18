#include "ExpressionQuickEditController.h"

#include "../../../controllers/SongController.h"
#include "../../../song/ExpressionConstants.h"
#include "../../../song/cells/CellConstants.h"
#include "../../../utils/NoteUtil.h"

namespace arkostracker
{

ExpressionQuickEditController::ExpressionQuickEditController(SongController& pSongController) noexcept :
        songController(pSongController)
{
}

int ExpressionQuickEditController::getItemLength(const Id& itemId) const noexcept
{
    const auto isArpeggio = songController.isExpressionArpeggio(itemId);
    const auto& expressionHandler = songController.getExpressionHandler(isArpeggio);
    auto length = 0;
    expressionHandler.performOnConstExpression(itemId, [&] (const Expression& expression) {
        length = expression.getLength();
    });

    return length;
}

juce::String ExpressionQuickEditController::validateNoise(const Id& /*itemId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validateEnvelope(const Id& /*itemId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validateSoundType(const Id& /*itemId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validateSidIsActivatedAndRatio(const Id& /*itemId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validatePeriod(bool /*isPrimaryPeriod*/, const Id& /*itemId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validateSidLowShelfVolume(const Id& /*instrumentId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validateSidBalancePercent(const Id& /*instrumentId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validateSidArpeggio(const Id& /*instrumentId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validateSidPitch(const Id& /*instrumentId*/, int /*barIndex*/, const juce::String& /*values*/) const noexcept
{
    jassertfalse;           // Not supposed to be called.
    return { };
}

juce::String ExpressionQuickEditController::validatePitch(const bool /*isPrimaryPitch*/, const Id& expressionId, const int barIndex, const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values, ExpressionConstants::minimumPitch, ExpressionConstants::maximumPitch);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnExpressionCells(expressionId, false, barIndex, static_cast<int>(numbers.size()),
        juce::translate("Expression pitch quick edit"),
        [numbersRef] (const int iterationIndex, const int /*inputValue*/) noexcept {
            return numbersRef.at(static_cast<size_t>(iterationIndex));
    });

    return { };
}

juce::String ExpressionQuickEditController::validateArpeggioNoteInOctave(const bool /*isPrimaryArpeggioInNote*/, const Id& expressionId, const int barIndex,
    const juce::String& values) const noexcept
{
    const auto& [errorMessage, numbers] = validateValues(values,  0, CellConstants::lastNoteInOctave);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnExpressionCells(expressionId, true, barIndex, static_cast<int>(numbers.size()),
            juce::translate("Expression arpeggio octave quick edit"),
            [numbersRef] (const int iterationIndex, const int inputValue) noexcept {
                const auto newNoteInOctave = numbersRef.at(static_cast<size_t>(iterationIndex));
                // Rebuilds the value, as only the note in octave changed.
                return NoteUtil::getNote(newNoteInOctave, NoteUtil::getOctaveFromNote(inputValue));
        });

    return { };
}

juce::String ExpressionQuickEditController::validateArpeggioOctave(const bool /*isPrimaryArpeggioOctave*/, const Id& expressionId, const int barIndex,
    const juce::String& values) const noexcept
{
    // TO IMPROVE: the max/min is the same for PsgQuickEditController so it works, but it is not the following min/max constants that is displayed.
    const auto& [errorMessage, numbers] = validateValues(values, ExpressionConstants::minimumArpeggioOctave,
        ExpressionConstants::maximumArpeggioOctave);
    if (errorMessage.isNotEmpty()) {
        return errorMessage;
    }

    // Valid.
    const auto& numbersRef = numbers;      // Clang cannot capture from structured bindings, GCC can. WTH.
    songController.applyOnExpressionCells(expressionId, true, barIndex, static_cast<int>(numbers.size()),
            juce::translate("Expression arpeggio note quick edit"),
            [numbersRef] (const int iterationIndex, const int inputValue) noexcept {
                const auto newOctave = numbersRef.at(static_cast<size_t>(iterationIndex));
                // Rebuilds the value, as only the note in octave changed.
                return NoteUtil::getNote(NoteUtil::getNoteInOctave(inputValue), newOctave);
        });

    return { };
}

}   // namespace arkostracker
