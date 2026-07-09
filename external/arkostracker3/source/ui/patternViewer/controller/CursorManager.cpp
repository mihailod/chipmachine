#include "CursorManager.h"

#include "EffectErrorHelper.h"
#include "../../../business/song/tool/context/EffectContext.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/PlayerController.h"
#include "../../../song/cells/CellConstants.h"
#include "../../../utils/CollectionUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../utils/KeysUtil.h"
#include "PatternViewerControllerImpl.h"

namespace arkostracker 
{

CursorManager::CursorManager(PatternViewerControllerImpl& pController) noexcept :
        controller(pController)
{
}

void CursorManager::manageNoteReceived(const int note, const OptionalInt forcedInstrument) const noexcept
{
    // Nothing is done if the note is illegal.
    if ((note < CellConstants::minimumNote) || (note > CellConstants::maximumNote)) {
        return;
    }

    const auto& cursorLocation = controller.cursorLocation;
    const auto& mainController = controller.mainController;

    const auto channelIndex = cursorLocation.getChannelIndex();
    auto instrumentIndex = forcedInstrument;
    // The instrument may not be written.
    if (forcedInstrument.isAbsent() && controller.writeInstrumentOnWriteNote) {
        const auto instrumentId = mainController.getSelectedInstrumentId();
        if (instrumentId.isPresent()) {
            instrumentIndex = mainController.getInstrumentIndex(instrumentId.getValueRef());
        }
    }

    // Simple case first: plays the note if not record, or if recording, we are on a special track.
    const auto trackType = cursorLocation.getTrackType();
    if (!controller.recording || (trackType != TrackType::normal)) {
        const auto shownLocation = controller.getShownLocation();
        controller.playerController.playFromEditor(note, instrumentIndex, CellLocationInPosition(
            shownLocation.getSubsongId(), shownLocation.getPosition(), shownLocation.getLine(), channelIndex));
        return;
    }

    // We are recording, and are on a Normal Track. The note can be only recorded if on a Note column.
    // Also, don't record if the Track is read only.
    if ((cursorLocation.getRankAsCellCursorRank() == CellCursorRank::cursorOnNote) && !controller.isReadOnlyTrack(channelIndex)) {
        manageItemsToWrite(channelIndex, Note::buildNote(note), instrumentIndex);
    }
}

bool CursorManager::onKeyPressed(const juce::KeyPress& key) const noexcept
{
    // If the cursor is on a read-only Track, don't do anything.
    if (const auto channelIndex = controller.cursorLocation.getChannelIndex(); controller.isReadOnlyTrack(channelIndex)) {
        return false;
    }

    // Checks whether the key is a non-command digit (1-9, A-F).
    const auto numberOptional = KeysUtil::getHexNumberFromKey(key);
    if (numberOptional.isPresent() && manageTypedDigitIfPossible(numberOptional.getValue())) {
        return true;
    }
    // Checks whether the key is an effect.
    if (const auto effectOptional = determineEffectFromKey(key); effectOptional.isPresent() && manageTypedEffectIfPossible(effectOptional.getValue())) {
        return true;
    }

    // The key is unknown. Let the caller decides what to do (keyboard notes are determined this way via JUCE commands).
    return false;
}

OptionalValue<Effect> CursorManager::determineEffectFromKey(const juce::KeyPress& key) const noexcept
{
    const auto foundEffectOptional = CollectionUtil::findKeyFromValue(controller.effectToChar, key.getTextCharacter());
    if (foundEffectOptional.isAbsent()) {
        return { };
    }

    return foundEffectOptional.getValue();
}

bool CursorManager::manageTypedDigitIfPossible(const int digit) const noexcept
{
    jassert((digit >= 0) && (digit <= 15));

    // If not recording, nothing to do.
    if (!controller.recording) {
        return false;
    }

    // Digits are useful on Instrument, and effect values.
    switch (controller.cursorLocation.getTrackType()) {
        case TrackType::normal:
            return manageTypedDigitIfPossibleOnTrack(digit);
        case TrackType::speed:
            return manageTypedDigitIfPossibleOnSpecialTrack(true, digit);
        case TrackType::event:
            return manageTypedDigitIfPossibleOnSpecialTrack(false, digit);
        case TrackType::line:
        default:
            jassertfalse;
            return false;
    }
}

bool CursorManager::manageTypedDigitIfPossibleOnTrack(const int digit) const noexcept
{
    const auto& cursorLocation = controller.cursorLocation;
    jassert(cursorLocation.getTrackType() == TrackType::normal);

    const auto rank = cursorLocation.getRankAsCellCursorRank();

    // Digits are managed only on an instrument or an effect column.
    // Instrument?
    if ((rank == CellCursorRank::cursorOnInstrumentDigit1) || (rank == CellCursorRank::cursorOnInstrumentDigit2)) {
        // If there is no note, or if it is an RST, don't do anything (consume anyway).
        const auto cell = controller.getCell(cursorLocation.getChannelIndex(), controller.shownLocation.getLine());
        if (!cell.isNote() || cell.isRst()) {
            return true;
        }

        const auto digitIndex = (rank == CellCursorRank::cursorOnInstrumentDigit1) ? 0 : 1;
        const auto instrumentDigit = OptionalValue(Digit(digitIndex, digit));

        manageItemsToWrite(cursorLocation.getChannelIndex(), OptionalValue<Note>(), OptionalInt(), OptionalValue<EffectDigit>(),
                           OptionalValue<EffectNumber>(), instrumentDigit);
        return true;
    }

    // Effects?
    // Map linking a rank to the effect index and digit index.
    static const std::unordered_map<CellCursorRank, std::pair<int, int>> rankToEffectIndexAndDigitIndex = {
            { CellCursorRank::cursorOnEffect1Digit1, { 0, 0 } },
            { CellCursorRank::cursorOnEffect1Digit2, { 0, 1 } },
            { CellCursorRank::cursorOnEffect1Digit3, { 0, 2 } },

            { CellCursorRank::cursorOnEffect2Digit1, { 1, 0 } },
            { CellCursorRank::cursorOnEffect2Digit2, { 1, 1 } },
            { CellCursorRank::cursorOnEffect2Digit3, { 1, 2 } },

            { CellCursorRank::cursorOnEffect3Digit1, { 2, 0 } },
            { CellCursorRank::cursorOnEffect3Digit2, { 2, 1 } },
            { CellCursorRank::cursorOnEffect3Digit3, { 2, 2 } },

            { CellCursorRank::cursorOnEffect4Digit1, { 3, 0 } },
            { CellCursorRank::cursorOnEffect4Digit2, { 3, 1 } },
            { CellCursorRank::cursorOnEffect4Digit3, { 3, 2 } },
            };

    // Finds the rank. If not found, it means we don't care about this digit in this rank.
    const auto iterator = rankToEffectIndexAndDigitIndex.find(rank);
    if (iterator == rankToEffectIndexAndDigitIndex.cend()) {
        return false;
    }

    const auto effectIndex = iterator->second.first;
    const auto digitIndex = iterator->second.second;

    // We try to write an effect. But if none is "locked", there must be one where we want to write, else the digits will be written but nothing
    // will be visible.
    const auto lockedEffect = controller.defaultEffect;
    const auto lock = controller.overwriteDefaultEffect;
    const auto cell = controller.getCell(cursorLocation.getChannelIndex(), controller.shownLocation.getLine());
    const auto isEffectPresent = cell.isEffectPresent(effectIndex);
    const auto noEffectSelected = (lockedEffect == Effect::noEffect);

    // Nothing can be done if there is no effect, but no effect is selected.
    if (!isEffectPresent && noEffectSelected) {
        return false;
    }

    const auto newEffectNumberOptional =
            // Overwrites the effect if:
            // - Lock && effect is selected.
            // - Not locked && effect is selected && no effect is present.
            (lock && !noEffectSelected) ||
            (!lock && !noEffectSelected && !isEffectPresent)
            ? EffectNumber(effectIndex, controller.defaultEffect)
            : OptionalValue<EffectNumber>();

    manageItemsToWrite(cursorLocation.getChannelIndex(), OptionalValue<Note>(), OptionalInt(),
                       EffectDigit(effectIndex, digitIndex, digit), newEffectNumberOptional);
    return true;
}

bool CursorManager::manageTypedDigitIfPossibleOnSpecialTrack(const bool isSpeedTrack, const int digit) const noexcept
{
    const auto& cursorLocation = controller.cursorLocation;
    jassert((isSpeedTrack && (cursorLocation.getTrackType() == TrackType::speed)) || (!isSpeedTrack && (cursorLocation.getTrackType() == TrackType::event)));

    // On what digit?
    const auto digitIndex = (cursorLocation.getRankAsSpecialCursorRank() == SpecialCellCursorRank::cursorOnDigit1) ? 0 : 1;

    manageSpecialItemsToWrite(isSpeedTrack, Digit(digitIndex, digit));

    return true;
}

bool CursorManager::manageTypedEffectIfPossible(const Effect effect) const noexcept
{
    // If not recording, nothing to do.
    if (!controller.recording) {
        return false;
    }
    // Must be on a Track.
    const auto& cursorLocation = controller.cursorLocation;
    if (cursorLocation.getTrackType() != TrackType::normal) {
        return false;
    }

    const auto rank = cursorLocation.getRankAsCellCursorRank();

    // Map linking a rank to the effect index and digit index.
    static const std::unordered_map<CellCursorRank, int> rankToEffectIndex = {
            { CellCursorRank::cursorOnEffect1Number, 0 },
            { CellCursorRank::cursorOnEffect2Number, 1 },
            { CellCursorRank::cursorOnEffect3Number, 2 },
            { CellCursorRank::cursorOnEffect4Number, 3 },
            };

    // Finds the rank. If not found, it means we don't care about this digit in this rank.
    const auto iterator = rankToEffectIndex.find(rank);
    if (iterator == rankToEffectIndex.cend()) {
        return false;
    }

    const auto effectIndex = iterator->second;

    manageItemsToWrite(cursorLocation.getChannelIndex(), OptionalValue<Note>(), OptionalInt(),
                       OptionalValue<EffectDigit>(), EffectNumber(effectIndex, effect));
    return true;
}

void CursorManager::manageItemsToWrite(const int channelIndex, const OptionalValue<Note> newNote, const OptionalInt newInstrument, const OptionalValue<EffectDigit> newEffectDigit,
                                       const OptionalValue<EffectNumber> newEffectNumber, const OptionalValue<Digit> newInstrumentDigit) const noexcept
{
    manageWrite([&](const Location& whereToWrite) {
                    controller.songController.setCell(whereToWrite, channelIndex, newNote, newInstrument, newEffectDigit, newEffectNumber, newInstrumentDigit);
                }, [&](const Location& whereToWrite) {
                    if (newNote.isPresent()) {
                        const CellLocationInPosition locationInPosition(whereToWrite.getSubsongId(), whereToWrite.getPosition(), whereToWrite.getLine(), channelIndex);

                        controller.playerController.playFromEditor(newNote.getValueRef().getNote(), newInstrument, locationInPosition);
                    }
                }
            );
}

void CursorManager::manageItemsToDelete(const SelectedData &selectedData) const noexcept
{
    if (!controller.recording) {        // Ideally, should have been managed in the Action Manager...
        controller.showWarningBecauseRecordingOff();
        return;
    }

    manageWrite([&](const Location&) {
                    controller.songController.clearSelection(selectedData);
                }, [](const Location&) {
                    // No note to play.
                }
            );
}

void CursorManager::manageWrite(const std::function<void(const Location& whereToWrite)>& modifyCellAction,
                                const std::function<void(const Location& whereToWrite)>& playCellIfNote) const noexcept
{
    if (!controller.recording) {     // Only a security.
        return;
    }

    // Recording. Sends the note to be written where the cursor is.
    const auto locationWhereToWrite = controller.shownLocation;
    if (controller.playerController.isPlaying()) {
        // If playing, writes the note.
        modifyCellAction(locationWhereToWrite);
        // If Following, plays ONLY the added note. PlayLine doesn't work because it would trigger the other channel's notes as well and stop the playing.
        if (controller.isFollowing()) {
            playCellIfNote(locationWhereToWrite);
        } else {
            controller.tryToMoveForward();
        }
    } else {
        // If not playing, plays the line, and go further according to step.
        // Not sure if it is more optimized, but setting the Cell will provoke a refresh. So first moves to the next location, then sets the cell. Probably fewer Cells to refresh.
        controller.tryToMoveForward();
        modifyCellAction(locationWhereToWrite);
        controller.playerController.playLine(locationWhereToWrite);
    }
}

void CursorManager::manageSpecialItemsToWrite(const bool isSpeedTrack, const Digit& digit) const noexcept
{
    manageWrite([&](const Location& whereToWrite) {
        controller.songController.setSpecialCell(isSpeedTrack, whereToWrite, digit);
            }, [](const Location&) {
                // No note to play.
            }
        );
}

std::pair<bool, juce::String> CursorManager::getInformationText() const noexcept
{
    // Don't display if there is scrolling, causes flickering text.
    if (controller.isPlaying() && controller.isFollowing()) {
        return { false, juce::String() };
    }

    const auto cursorLocation = controller.cursorLocation;

    switch (cursorLocation.getTrackType()) {
        default:
        case TrackType::line:
            return { };
        case TrackType::speed:
            return { false, juce::translate("Speed change (01 is fastest, FF slowest. 00 = no change)") };
        case TrackType::event:
            return { false, juce::translate("Event declaration (00 = no event)") };
        case TrackType::normal:
            // Continue.
            break;
    }

    // Normal Track.
    const auto rank = cursorLocation.getRankAsCellCursorRank();
    const auto channelIndex = cursorLocation.getChannelIndex();
    const auto cellIndex = controller.shownLocation.getLine();
    const auto cell = controller.getCell(channelIndex, cellIndex);
    switch (rank) {
        case CellCursorRank::cursorOnNote:
        {
            // If RST, shows it.
            if (cell.isRst()) {
                return { false, juce::translate("RST (silence)") };
            }
            return { false, juce::translate("Column for the note and octave") };
        }
        case CellCursorRank::cursorOnInstrumentDigit1: [[fallthrough]];
        case CellCursorRank::cursorOnInstrumentDigit2:
        {
            // Is there an instrument? There must be a note too.
            const auto instrumentOptional = cell.getInstrument();
            if (instrumentOptional.isAbsent() || !cell.isNote()) {
                return { false, juce::translate("Column for an instrument number") };
            }
            const auto instrument = instrumentOptional.getValue();
            // Does it exist?
            const auto& songController = controller.mainController.getSongController();
            const auto instrumentCount = songController.getInstrumentCount();
            if (instrument >= instrumentCount) {
                return { true, juce::translate("Unknown instrument") };
            }
            // The instrument exists.
            if (instrument == CellConstants::rstInstrument) {
                return { false, juce::translate("RST (silence)") };
            }
            const auto instrumentId = songController.getInstrumentId(instrument);
            if (instrumentId.isAbsent()) {
                jassertfalse;
                return { false, juce::String() };
            }
            const auto instrumentName = songController.getInstrumentName(instrumentId.getValueRef());
            auto text = juce::translate("Instrument ") + NumberUtil::toHexByte(instrument);
            if (!instrumentName.isEmpty()) {
                text = text + ": " + instrumentName;
            }
            return { false, text };
        }
        case CellCursorRank::cursorOnEffect1Number: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Number: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Number: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Number: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit1: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit3:
            return determineInformationTextOnEffect(rank, cell);
        case CellCursorRank::count: [[fallthrough]];
        default:
            jassertfalse;       // Shouldn't happen.
            return { false, juce::String() };
    }
}

std::pair<bool, juce::String> CursorManager::determineInformationTextOnEffect(const CellCursorRank rank, const Cell& cell) const noexcept
{
    // On what index effect is the cursor?
    int effectIndex;            // NOLINT(*-init-variables)
    auto isOnEffectRank = true;
    switch (rank) {
        case CellCursorRank::cursorOnEffect1Number:
            effectIndex = 0;
            break;
        case CellCursorRank::cursorOnEffect2Number:
            effectIndex = 1;
            break;
        case CellCursorRank::cursorOnEffect3Number:
            effectIndex = 2;
            break;
        case CellCursorRank::cursorOnEffect4Number:
            effectIndex = 3;
            break;
        case CellCursorRank::cursorOnEffect1Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit1:
            effectIndex = 0;
            isOnEffectRank = false;
            break;
        case CellCursorRank::cursorOnEffect2Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit1:
            effectIndex = 1;
            isOnEffectRank = false;
            break;
        case CellCursorRank::cursorOnEffect3Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit1:
            effectIndex = 2;
            isOnEffectRank = false;
            break;
        case CellCursorRank::cursorOnEffect4Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit1:
            effectIndex = 3;
            isOnEffectRank = false;
            break;
        // Fallbacks.
        case CellCursorRank::first: [[fallthrough]];
        case CellCursorRank::cursorOnInstrumentDigit2: [[fallthrough]];
        case CellCursorRank::cursorOnInstrumentDigit1: [[fallthrough]];
        case CellCursorRank::count: [[fallthrough]];
        default:
            jassertfalse;               // Not supposed to happen!
            return { false, juce::String() };
    }

    // Any effect?
    const auto& cellEffects = cell.getEffects();
    const auto cellEffect = cellEffects.getEffect(effectIndex);
    const auto effect = cellEffect.getEffect();
    if (effect == Effect::noEffect) {
        return { false, juce::translate("Column for an effect number") };
    }
    // There is an effect. Any generic error?
    const auto errorOptional = cellEffects.getError(effectIndex, cell.isNote(), cell.isInstrument());
    if (errorOptional.isPresent()) {
        return determineInformationTextOnEffectError(errorOptional.getValue());
    }
    // Special display for arpeggio/pitch table.
    if (effect == Effect::arpeggioTable) {
        return determineInformationTextForExpression(cellEffect, true);
    }
    if (effect == Effect::pitchTable) {
        return determineInformationTextForExpression(cellEffect, false);
    }

    // No error.
    // Effect without special behavior. If in column of the value, stops here.
    if (!isOnEffectRank) {
        return { false, juce::translate("Column for an effect value") };
    }

    // Displays the effect.
    static const std::map<Effect, juce::String> effectToString = {                // Links an Effect to a String for display.
            { Effect::arpeggio3Notes, juce::translate("Arpeggio on 3 notes (xx-)") },
            { Effect::arpeggio4Notes, juce::translate("Arpeggio on 4 notes (xxx)") },
            //{ Effect::arpeggioTable, juce::translate("Arpeggio from a table (xx-)") },    // Handled above.
            { Effect::pitchUp, juce::translate("Pitch up with speed (xxx)") },
            { Effect::pitchDown, juce::translate("Pitch down with speed (xxx)") },
            { Effect::fastPitchUp, juce::translate("Fast pitch up with speed (xxx)") },
            { Effect::fastPitchDown, juce::translate("Fast pitch down with speed (xxx)") },
            //{ Effect::pitchTable, juce::translate("Pitch from a table (xx-)") },          // Handled above.
            { Effect::pitchGlide, juce::translate("Glide to note, with speed (xxx)") },
            { Effect::reset, juce::translate("Reset effects, inverted volume (x--) (0 = full, f = no volume)") },
            { Effect::volume, juce::translate("Volume (x--) (f = full volume, 0 = no volume)") },
            { Effect::volumeIn, juce::translate("Volume increasing with speed (xxx)") },
            { Effect::volumeOut, juce::translate("Volume decreasing with speed (xxx)") },
            { Effect::forceInstrumentSpeed, juce::translate("Force the instrument speed (xx-)") },
            { Effect::forceArpeggioSpeed, juce::translate("Force the arpeggio table speed (xx-)") },
            { Effect::forcePitchTableSpeed, juce::translate("Force the pitch table speed (xx-)") },
    };

    if (const auto iterator = effectToString.find(effect); iterator != effectToString.cend()) {
        return { false, iterator->second };
    }

    jassertfalse;           // Effect not found, shouldn't be possible!
    return { true, juce::translate("Unknown effect.") };
}

std::pair<bool, juce::String> CursorManager::determineInformationTextOnEffectError(const EffectError effectError) noexcept
{
    return { true, EffectErrorHelper::effectErrorToDisplayableString(effectError) };
}

std::pair<bool, juce::String> CursorManager::determineInformationTextForExpression(const CellEffect& cellEffect, const bool isArpeggio) const noexcept
{
    // Does the expression exist? If not, error.
    const auto expressionIndex = cellEffect.getEffectLogicalValue();
    const auto& songController = controller.mainController.getSongController();
    const auto expressionCount = songController.getExpressionCount(isArpeggio);
    if (expressionIndex >= expressionCount) {
        return { true, isArpeggio ? juce::translate("Unknown arpeggio") : juce::translate("Unknown pitch") };
    }
    // It exists.
    const auto expressionId = songController.getExpressionId(isArpeggio, expressionIndex);
    const auto expressionName = songController.getExpressionName(isArpeggio, expressionId.getValueRef());
    auto text = isArpeggio ? juce::translate("Arpeggio from a table (xx-). ") : juce::translate("Pitch from a table (xx-). ");
    if (!expressionName.isEmpty()) {
        text += NumberUtil::toHexByte(expressionIndex) + ": " + expressionName;
    }
    return { false, text };
}

std::vector<CursorLocation> CursorManager::buildCursorLocations() const noexcept
{
    std::vector<CursorLocation> locations;
    const auto channelCount = controller.getChannelCount();
    for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
        locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnNote));
        locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnInstrumentDigit2));
        locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnInstrumentDigit1));

        // Effects not minimized?
        const auto& minimizedChannelIndexes = controller.minimizedChannelIndexes;
        const auto minimized = (minimizedChannelIndexes.find(channelIndex) != minimizedChannelIndexes.cend());

        locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect1Number));
        if (!minimized) {
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect1Digit3));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect1Digit2));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect1Digit1));
        }
        locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect2Number));
        if (!minimized) {
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect2Digit3));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect2Digit2));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect2Digit1));
        }
        locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect3Number));
        if (!minimized) {
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect3Digit3));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect3Digit2));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect3Digit1));
        }
        locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect4Number));
        if (!minimized) {
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect4Digit3));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect4Digit2));
            locations.push_back(CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnEffect4Digit1));
        }
    }
    // Special tracks.
    locations.push_back(CursorLocation::onSpeedTrack(SpecialCellCursorRank::cursorOnDigit2));
    locations.push_back(CursorLocation::onSpeedTrack(SpecialCellCursorRank::cursorOnDigit1));

    locations.push_back(CursorLocation::onEventTrack(SpecialCellCursorRank::cursorOnDigit2));
    locations.push_back(CursorLocation::onEventTrack(SpecialCellCursorRank::cursorOnDigit1));

    return locations;
}

std::pair<int, juce::String> CursorManager::buildEffectContextText() const noexcept
{
    const auto cursorLocation = controller.cursorLocation;
    if (!cursorLocation.isMusicTrack()) {
        return { 0, juce::String() };
    }

    const auto channelIndex = cursorLocation.getChannelIndex();

    // Don't display if playing.
    if (controller.isPlaying()) {
        return { channelIndex, juce::String() };
    }

    const auto shownLocation = controller.shownLocation;

    // Gets the context for the line.
    const auto* effectContext = controller.getEffectContext();
    if (effectContext == nullptr) {
        return { 0, juce::String() };
    }
    const CellLocationInPosition location(shownLocation.getSubsongId(), shownLocation.getPosition(), shownLocation.getLine(), channelIndex);
    const auto lineContext = effectContext->determineContext(location);

    const auto& songController = controller.mainController.getSongController();

    // Displays it.
    const auto volume = lineContext.getVolume().getIntegerPart();
    const auto pitchIdOptional = lineContext.getPitchId();
    const auto arpeggioIdOptional = lineContext.getArpeggioId();
    const auto arpeggioInlineOptional = lineContext.getArpeggioInline();

    const auto pitchIndex = pitchIdOptional.isAbsent() ? 0 : songController.getExpressionIndex(false, pitchIdOptional.getValueRef()).getValueRef();
    const auto arpeggioIndex = arpeggioIdOptional.isAbsent() ? 0 : songController.getExpressionIndex(true, arpeggioIdOptional.getValueRef()).getValueRef();

    // Builds the possible inline arpeggio. Don't display arps with only 0.
    juce::String arpeggioInline;
    if (arpeggioInlineOptional.isPresent() && !arpeggioInlineOptional.getValueRef().hasOnlyZeros()) {
        const auto expression = arpeggioInlineOptional.getValue();
        const auto expressionLength = expression.getLength();
        for (auto index = 0; index < expressionLength; ++index) {
            arpeggioInline += NumberUtil::toHexDigit(expression.getValue(index));
        }
    }

    juce::String text;
    if (arpeggioIndex > 0) {
        text += "a:" + NumberUtil::toHexByte(arpeggioIndex);
    } else if (arpeggioInline.isNotEmpty()) {
        text += "a:" + arpeggioInline;
    }

    if (pitchIndex > 0) {
        if (text.isNotEmpty()) {
            text += " ";
        }
        text += "p:" + NumberUtil::toHexByte(pitchIndex);
    }

    if (volume != PsgValues::maximumVolumeNoHard) {
        if (text.isNotEmpty()) {
            text += " ";
        }
        text += "v:" + NumberUtil::toHexDigit(volume);
    }

    return { channelIndex, text };
}

}   // namespace arkostracker
