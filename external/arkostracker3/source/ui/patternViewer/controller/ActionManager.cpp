#include "ActionManager.h"

#include "../../../business/instrument/GenerateInstrument.h"
#include "../../../business/serialization/patternViewer/TrackSerializer.h"
#include "../../../controllers/SongController.h"
#include "../../keyboard/CommandIds.h"
#include "../../utils/Clipboard.h"
#include "../view/dialog/GenerateArpeggioDialog.h"
#include "../view/dialog/GenerateInstrumentDialog.h"
#include "CaptureFlags.h"
#include "Selection.h"

namespace arkostracker 
{

ActionManager::ActionManager(Controller& pController) noexcept :
        controller(pController),
        dialog()
{
}

void ActionManager::openContextualMenu() const noexcept
{
    auto& commandManager = controller.getCommandManager();

    juce::PopupMenu popupMenu;
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::cut, juce::translate("Cut"));
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::copy, juce::translate("Copy"));
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::paste, juce::translate("Paste"));
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, patternViewerPasteAndMoveBelow, juce::translate("Paste and move below"));
    popupMenu.addCommandItem(&commandManager, patternViewerPasteContinuously, juce::translate("Paste continuously"));
    popupMenu.addCommandItem(&commandManager, patternViewerPasteMix, juce::translate("Paste mix"));
    popupMenu.addCommandItem(&commandManager, patternViewerClearSelection, juce::translate("Clear selection"));
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, patternViewerTransposePlusOne, juce::translate("Transpose +1"));
    popupMenu.addCommandItem(&commandManager, patternViewerTransposeMinusOne, juce::translate("Transpose -1"));
    popupMenu.addCommandItem(&commandManager, patternViewerTransposePlusOneOctave, juce::translate("Transpose +12"));
    popupMenu.addCommandItem(&commandManager, patternViewerTransposeMinusOneOctave, juce::translate("Transpose -12"));
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, patternViewerGenerateArpeggio, juce::translate("Generate arpeggio from selection"));
    popupMenu.addCommandItem(&commandManager, patternViewerGenerateInstrument, juce::translate("Generate instrument from selection"));
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, patternViewerSetTrackNameAndColor, juce::translate("Edit track name"));
    popupMenu.addCommandItem(&commandManager, patternViewerLinkTrack, juce::translate("Edit track link"));
    popupMenu.addCommandItem(&commandManager, patternViewerToggleReadOnly, juce::translate("Toggle read-only"));

    popupMenu.show();
}

void ActionManager::onUserWantsToCopy() const noexcept
{
    copySelectionToClipboard();
}

void ActionManager::onUserWantsToCut() const noexcept
{
    if (!checkIfRecordingAndNotify()) {
        return;
    }
    copySelectionToClipboard();
    clearSelection();
}

void ActionManager::onUserWantsToPaste(const bool moveBelowAfter, const bool pasteContinuously) const noexcept
{
    paste(false, moveBelowAfter, pasteContinuously);
}

void ActionManager::onUserWantsToPasteMix() const noexcept
{
    paste(true, false, false);
}

void ActionManager::onUserWantsToClearSelection() const noexcept
{
    clearSelection();
}

void ActionManager::onUserWantsToTransposeSelection(const TransposeRate transposeRate) const noexcept
{
    if (!checkIfRecordingAndNotify()) {
        return;
    }
    const auto selectedData = buildSelectedData();
    controller.getSongController().transpose(transposeRate, selectedData);
}

void ActionManager::onUserWantsToToggleReadOnly() const noexcept
{
    const auto selectedData = buildSelectedData();
    const auto channelIndexes = selectedData.getSelectedChannelIndexes();

    const auto& location = selectedData.getStartLocation();
    const auto positionIndex = location.getPosition();
    const auto subsongId = location.getSubsongId();
    std::set<int> trackIndexes;
    OptionalInt speedTrackIndex;
    OptionalInt eventTrackIndex;
    // Gets the indexes of the selected Tracks/Special Tracks.
    controller.getSongController().getSong()->performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto& pattern = subsong.getPatternRef(positionIndex);
        for (const auto channelIndex : channelIndexes) {
            trackIndexes.insert(pattern.getCurrentTrackIndex(channelIndex));
        }
        if (selectedData.getIncludeSpeedTrack()) {
            speedTrackIndex = pattern.getCurrentSpecialTrackIndex(true);
        }
        if (selectedData.getIncludeEventTrack()) {
            eventTrackIndex = pattern.getCurrentSpecialTrackIndex(false);
        }
    });

    controller.getSongController().toggleTracksReadOnly(subsongId, trackIndexes, speedTrackIndex, eventTrackIndex);
}

SelectedData ActionManager::buildSelectedData() const noexcept
{
    const auto selection = getSelectionOrCursor();
    return buildSelectedData(selection);
}

SelectedData ActionManager::buildSelectedDataAsPattern() const noexcept
{
    const auto selection = Selection::buildForWholePattern();
    return buildSelectedData(selection);
}

SelectedData ActionManager::buildSelectedDataAsCurrentSubsong() const noexcept
{
    const auto subsongId = controller.getShownLocation().getSubsongId();
    return buildSelectedDataAsWholeSubsong(subsongId);
}

std::vector<SelectedData> ActionManager::buildSelectedDataWithAllSubsongs() const noexcept
{
    std::vector<SelectedData> outputSelectedData;

    auto& songController = controller.getSongController();
    const auto subsongIds = songController.getSong()->getSubsongIds();

    for (const auto& subsongId : subsongIds) {
        const auto selectedData = buildSelectedDataAsWholeSubsong(subsongId);
        outputSelectedData.push_back(selectedData);
    }

    return outputSelectedData;
}

SelectedData ActionManager::buildSelectedDataAsWholeSubsong(const Id& subsongId) const noexcept
{
    const auto selection = Selection::buildForWholePattern();

    const auto channelCount = controller.getChannelCount(subsongId);
    const auto trackToCaptureFlag = buildCaptureFlags(selection, channelCount);

    const auto endPosition = controller.getSubsongLength(subsongId);

    const Location startLocation(subsongId, 0, 0);
    const Location endLocation(subsongId, endPosition - 1, TrackConstants::lastPossibleIndex);

    return { trackToCaptureFlag, startLocation, endLocation, 0, true, true };
}

SelectedData ActionManager::buildSelectedData(const Selection& selection) const noexcept
{
    const auto shownLocation = controller.getShownLocation();
    const auto subsongIndex = shownLocation.getSubsongId();
    const auto currentPosition = shownLocation.getPosition();
    const auto channelCount = controller.getChannelCount();
    const auto startChannelIndexOptional = selection.getLeftStartChannelIndexIfOnTrack();
    const auto startChannelIndex = startChannelIndexOptional.isPresent() ? startChannelIndexOptional.getValue() : 0;

    const auto trackToCaptureFlag = buildCaptureFlags(selection, channelCount);
    const auto startLocation = Location(subsongIndex, currentPosition, selection.getTopLine());
    const auto endLocation = startLocation.withLine(selection.getBottomLine());

    const auto includeSpeedTrack = selection.containsSpecialTrack(true);
    const auto includeEventTrack = selection.containsSpecialTrack(false);

    return { trackToCaptureFlag, startLocation, endLocation, startChannelIndex, includeSpeedTrack, includeEventTrack };
}

std::vector<CaptureFlags> ActionManager::buildCaptureFlags(const Selection& selection, const int channelCount) noexcept
{
    const auto channelIndexes = selection.getChannelIndexes(channelCount);

    std::vector<CaptureFlags> captureFlags;
    captureFlags.reserve(static_cast<size_t>(channelCount));
    for (const auto channelIndex : channelIndexes) {
        jassert(channelIndex < channelCount);

        // Grabs what part of the cells was actually captured. We trick by creating a selection specific to this channel.
        const auto selectionForChannel = selection.extractForTrack(channelIndex);
        jassert(selectionForChannel.isPresent());           // Nothing inside? Abnormal.
        const auto leftRank = selectionForChannel.getLeftRankAsCellCursorRank();
        const auto rightRank = selectionForChannel.getRightRankAsCellCursorRank();

        const auto isNoteCaptured = (leftRank == CellCursorRank::cursorOnNote);
        const auto isInstrumentCaptured = ((leftRank <= CellCursorRank::cursorOnInstrumentDigit2) && (rightRank >= CellCursorRank::cursorOnInstrumentDigit1));
        const auto isEffect1Captured = ((leftRank <= CellCursorRank::cursorOnEffect1Number) && (rightRank >= CellCursorRank::cursorOnEffect1Digit1));
        const auto isEffect2Captured = ((leftRank <= CellCursorRank::cursorOnEffect2Number) && (rightRank >= CellCursorRank::cursorOnEffect2Digit1));
        const auto isEffect3Captured = ((leftRank <= CellCursorRank::cursorOnEffect3Number) && (rightRank >= CellCursorRank::cursorOnEffect3Digit1));
        const auto isEffect4Captured = ((leftRank <= CellCursorRank::cursorOnEffect4Number) && (rightRank >= CellCursorRank::cursorOnEffect4Digit1));

        captureFlags.emplace_back(isNoteCaptured, isInstrumentCaptured, isEffect1Captured, isEffect2Captured, isEffect3Captured, isEffect4Captured);
    }

    return captureFlags;
}

void ActionManager::copySelectionToClipboard() const noexcept
{
    const auto selection = getSelectionOrCursor();
    const auto selectionTop = selection.getTopLine();
    const auto selectionHeight = selection.getHeight();
    jassert(selectionHeight > 0);

    // Gets all the cells from the Tracks, if any. What channels are involved?
    const auto channelCount = controller.getChannelCount();
    const auto channelIndexes = selection.getChannelIndexes(channelCount);

    const auto captureFlags = buildCaptureFlags(selection, channelCount);

    std::vector<std::vector<Cell>> tracksOfCells;
    for (const auto channelIndex : channelIndexes) {
        jassert(channelIndex < channelCount);

        std::vector<Cell> cells;
        for (auto cellIndex = selectionTop, pastLastCellIndex = selectionTop + selectionHeight; cellIndex < pastLastCellIndex; ++cellIndex) {
            const auto cell = controller.getCell(channelIndex, cellIndex);
            cells.push_back(cell);
        }
        tracksOfCells.push_back(cells);
    }
    jassert(tracksOfCells.size() == captureFlags.size());           // There must be as much capture as tracks!

    // Gets all the cells from the Special Tracks, if any.
    const auto speedCells = extractSpecialCells(true, selection);
    const auto eventCells = extractSpecialCells(false, selection);

    // Serializes the whole.
    const auto xmlElement = TrackSerializer::serialize(tracksOfCells, captureFlags, speedCells, eventCells);
    if (xmlElement == nullptr) {
        jassertfalse;           // Shouldn't happen!
        return;
    }

    // Puts it in the clipboard.
    Clipboard::storeXmlNode(xmlElement.get());
}

std::vector<SpecialCell> ActionManager::extractSpecialCells(const bool isSpeedTrack, const Selection& selection) const noexcept
{
    std::vector<SpecialCell> specialCells;

    // Does the selection encompass the special track?
    if (!selection.containsSpecialTrack(isSpeedTrack)) {
        return specialCells;
    }

    const auto selectionTop = selection.getTopLine();
    const auto selectionHeight = selection.getHeight();
    jassert(selectionHeight > 0);

    for (auto cellIndex = selectionTop, pastLastCellIndex = selectionTop + selectionHeight; cellIndex < pastLastCellIndex; ++cellIndex) {
        const auto specialCell = controller.getSpecialCell(isSpeedTrack, cellIndex);
        specialCells.push_back(specialCell);
    }

    return specialCells;
}

PasteData ActionManager::getPasteDataFromClipboard() noexcept
{
    const auto xmlElement = Clipboard::retrieveXmlNode();
    if (xmlElement == nullptr) {
        jassertfalse;       // Nothing valid in the clipboard.
        return { };
    }

    // Deserializes the clipboard.
    return TrackSerializer::deserialize(*xmlElement);
}

void ActionManager::paste(const bool pasteMix, const bool moveBelowAfter, const bool pasteContinuously) const noexcept
{
    if (!checkIfRecordingAndNotify()) {
        return;
    }

    const auto pasteData = getPasteDataFromClipboard();

    if (!pasteData.isValid()) {
        jassertfalse;       // Invalid extracted data.
        return;
    }

    const auto cursorLocation = controller.getCursorLocation();
    const auto shownLocation = controller.getShownLocation();
    controller.getSongController().pasteCells(shownLocation, cursorLocation, pasteData, pasteMix, pasteContinuously);

    if (moveBelowAfter) {
        controller.tryToMoveVertically(pasteData.height);
    }
}

Selection ActionManager::getSelectionOrCursor() const noexcept
{
    // Gets the selection. If empty, uses the cursor.
    auto selection = controller.getCurrentSelection();
    if (selection.isEmpty()) {
        selection = getCursorAsSelection(false);
    }
    return selection;
}

Selection ActionManager::getCursorAsSelection(const bool growToWholeCell /*, const bool selectInstrumentIfWasOnNote*/) const noexcept
{
    const auto cursorLocation = controller.getCursorLocation();
    const auto cellIndex = controller.getShownLocation().getLine();

    const auto trackType = cursorLocation.getTrackType();

    if (!growToWholeCell) {
        // Grows to the Instrument too, if on a normal track, and we are on a Note.
        if (/*selectInstrumentIfWasOnNote &&*/ (trackType == TrackType::normal) && (cursorLocation.getRankAsCellCursorRank() == CellCursorRank::cursorOnNote)) {
            const auto channelIndex = cursorLocation.getChannelIndex();
            return { cellIndex, CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnNote),
                     cellIndex, CursorLocation::onTrack(channelIndex, CellCursorRank::cursorOnInstrumentDigit2), false, false };
        }

        return { cellIndex, cursorLocation };
    }

    // Grows the selection.
    switch (trackType) {
        default:
            [[fallthrough]];
        case TrackType::line:
            return { cellIndex, cursorLocation };
        case TrackType::normal: {
            const auto channelIndex = cursorLocation.getChannelIndex();
            return { cellIndex, CursorLocation::onTrack(channelIndex, CellCursorRank::first),
                     cellIndex, CursorLocation::onTrack(channelIndex, CellCursorRank::last),
                     false, false };
        }
        case TrackType::speed:
            return { cellIndex, CursorLocation::onSpecialTrack(true, SpecialCellCursorRank::first),
                     cellIndex, CursorLocation::onSpecialTrack(true, SpecialCellCursorRank::last),
                     false, false };
        case TrackType::event:
            return { cellIndex, CursorLocation::onSpecialTrack(false, SpecialCellCursorRank::first),
                     cellIndex, CursorLocation::onSpecialTrack(false, SpecialCellCursorRank::last),
                     false, false };
    }
}

void ActionManager::onUserWantsToInsert() const noexcept
{
    if (!checkIfRecordingAndNotify()) {
        return;
    }
    controller.getSongController().insertCellAt(controller.getShownLocation(), controller.getCursorLocation());
}

void ActionManager::onUserWantsToRemove() const noexcept
{
    if (!checkIfRecordingAndNotify()) {
        return;
    }
    controller.getSongController().removeCellAt(controller.getShownLocation(), controller.getCursorLocation());
}

void ActionManager::clearSelection() const noexcept
{
    if (!checkIfRecordingAndNotify()) {
        return;
    }
    const auto selectedData = buildSelectedData();
    controller.getSongController().clearSelection(selectedData);
}

void ActionManager::onUserWantsToCapture() const noexcept
{
    // If on Effect column, captures the effect.
    const auto cell = getCell();
    if (const auto [effectIndex, _] = controller.getCursorLocation().getEffectIndexIfRankOnEffect(); effectIndex.isPresent()) {
        const auto cellEffect = cell.getEffects().getEffect(effectIndex.getValue());
        const auto effect = cellEffect.getEffect();
        // If no effect, passes to the instrument.
        if (effect != Effect::noEffect) {
            // Always captures the effect.
            controller.setSelectedEffect(effect);
            // Also selects the pitch/arpeggio value, if we are on it.
            if ((effect == Effect::pitchTable) || (effect == Effect::arpeggioTable)) {
                controller.setSelectedExpression((effect == Effect::arpeggioTable), cellEffect.getEffectLogicalValue());
            }
            return;
        }
    }

    // Finally, the instrument.
    if (const auto instrumentId = tryToCaptureInstrument(); instrumentId.isPresent()) {
        controller.setSelectedInstrument(instrumentId.getValue());
    }
}

OptionalInt ActionManager::tryToCaptureInstrument() const noexcept
{
    OptionalInt instrumentIndex;

    // Only the normal tracks are valid.
    const auto cursorLocation = controller.getCursorLocation();
    if (cursorLocation.getTrackType() != TrackType::normal) {
        return instrumentIndex;
    }

    const auto positionHeight = controller.getPositionHeight();
    const auto channelIndex = cursorLocation.getChannelIndex();

    auto topCellIndex = getCursorAsSelection(false).getTopLine();
    auto bottomCellIndex = topCellIndex;    // Same value, two comparisons on first iteration, who cares.

    // Goes up and down to get the nearest, as long as no instrument is found and both indexes are within bounds.
    auto isTopCellValid = true;
    auto isBottomCellValid = true;
    while (instrumentIndex.isAbsent() && (isTopCellValid || isBottomCellValid)) {
        if (isBottomCellValid) {
            instrumentIndex = getInstrumentIfPossible(channelIndex, bottomCellIndex);
        }
        if (instrumentIndex.isAbsent() && isTopCellValid) {
            instrumentIndex = getInstrumentIfPossible(channelIndex, topCellIndex);
        }

        --topCellIndex;
        ++bottomCellIndex;
        isTopCellValid = (topCellIndex >= 0);
        isBottomCellValid = (bottomCellIndex < positionHeight);
    }

    return instrumentIndex;
}

Cell ActionManager::getCell() const noexcept
{
    const auto cursorLocation = controller.getCursorLocation();
    const auto cellIndex = controller.getShownLocation().getLine();
    if (!cursorLocation.isMusicTrack()) {
        return { };
    }

    return controller.getCell(cursorLocation.getChannelIndex(), cellIndex);
}

OptionalInt ActionManager::getInstrumentIfPossible(const int channelIndex, const int cellIndex) const noexcept
{
    const auto& cell = controller.getCell(channelIndex, cellIndex);
    return cell.isNoteAndInstrument() ? cell.getInstrument() : OptionalInt();
}

std::vector<int> ActionManager::getNotesForGeneration() const noexcept
{
    const auto& selection = controller.getCurrentSelection();
    if ((selection.getHeight() <= 1) || selection.containsSpecialTrack() || !selection.isSingleChannel()) {
        return { };
    }

    std::vector<int> notes;

    // Any notes?
    const auto firstLine = selection.getTopLine();
    const auto lastLine = selection.getBottomLine();
    const auto channelIndex = selection.getChannelIndexes(controller.getChannelCount()).front();
    for (auto lineIndex = firstLine; lineIndex <= lastLine; ++lineIndex) {
        const auto& cell = controller.getCell(channelIndex, lineIndex);
        if (const auto& note = cell.getNote(); note.isPresent()) {
            notes.push_back(note.getValue().getNote());
        }
    }

    return notes;
}

void ActionManager::onUserWantsToGenerateArpeggio() noexcept
{
    const auto notes = getNotesForGeneration();
    if (notes.size() < 2U) {
        jassertfalse;           // Should have been tested earlier.
        return;
    }

    jassert(dialog == nullptr);

    dialog = std::make_unique<GenerateArpeggioDialog>(
        [&, notes] (const juce::String& arpeggioName, const int baseNote) { onGeneratedArpeggioReceived(notes, arpeggioName, baseNote); },
        [&] { dialog.reset(); },
        notes);
}

void ActionManager::onGeneratedArpeggioReceived(const std::vector<int>& notes, const juce::String& arpeggioName, const int baseNote) noexcept
{
    jassert(!notes.empty());

    // Generates the Arpeggio. This is simply sticking the notes, with a shift from the base note.
    std::vector<std::unique_ptr<Expression>> arpeggios;
    auto arpeggio = std::make_unique<Expression>(true, arpeggioName, false);
    for (const auto originalNote : notes) {
        arpeggio->addValue(originalNote - baseNote);
    }
    arpeggio->setEnd(static_cast<int>(notes.size()) - 1);
    arpeggios.push_back(std::move(arpeggio));

    controller.getSongController().insertExpressions(true, -1, { std::move(arpeggios) } );

    dialog.reset();         // Make sure the arpeggio name is already used before deleting the dialog!
}

void ActionManager::onUserWantsToGenerateInstrument() noexcept
{
    dialog = std::make_unique<GenerateInstrumentDialog>(
        [&] (const juce::String& instrumentName, const bool usePeriods) { onGeneratedInstrumentReceived(instrumentName, usePeriods); },
        [&] { dialog.reset(); }
    );
}

void ActionManager::onGeneratedInstrumentReceived(const juce::String& instrumentName, const bool usePeriods) noexcept
{
    const auto localInstrumentName = instrumentName;    // NOLINT(*-unnecessary-copy-initialization)
    dialog.reset();         // Make sure the data is already used before deleting the dialog!

    const auto& selection = controller.getCurrentSelection();
    if ((selection.getHeight() <= 1) || selection.containsSpecialTrack() || !selection.isSingleChannel()) {
        jassertfalse;       // Should have been tested earlier.
        return;
    }

    const auto firstLine = selection.getTopLine();
    const auto lastLine = selection.getBottomLine();
    const auto channelIndex = selection.getChannelIndexes(controller.getChannelCount()).front();
    const auto location = controller.getShownLocation();
    const auto subsongId = location.getSubsongId();
    const auto positionIndex = location.getPosition();
    auto instrument = GenerateInstrument::generateInstrument(controller.getSongController(), subsongId, positionIndex,
        firstLine, lastLine, channelIndex, localInstrumentName, usePeriods);
    if (instrument == nullptr) {
        jassertfalse;       // Shouldn't happen.
        return;
    }

    controller.getSongController().addInstrument(std::move(instrument));
}

std::unordered_set<int> ActionManager::toggleMinimize(const std::unordered_set<int>& minimizedChannelIndexes) const noexcept
{
    // What are the channels to toggle?
    const auto selection = getSelectionOrCursor();
    const auto channelIndexes = selection.getChannelIndexes(controller.getChannelCount());
    if (channelIndexes.empty()) {       // Happens if selection is on Line or special tracks only.
        return minimizedChannelIndexes;
    }
    // The first channel indicates the toggle to apply everywhere. It is implicitly inverted here.
    const auto isMinimized = minimizedChannelIndexes.find(channelIndexes.at(0)) == minimizedChannelIndexes.cend();

    auto outputMinimizedChannelIndexes = minimizedChannelIndexes;
    for (auto channelIndex : channelIndexes) {
        if (isMinimized) {
            outputMinimizedChannelIndexes.insert(channelIndex);
        } else {
            // Removes the possibly stores minimize for this channel.
            if (const auto iterator = outputMinimizedChannelIndexes.find(channelIndex); iterator != outputMinimizedChannelIndexes.cend()) {
                outputMinimizedChannelIndexes.erase(iterator);
            }
        }
    }

    return outputMinimizedChannelIndexes;
}

bool ActionManager::checkIfRecordingAndNotify() const noexcept
{
    const auto isRecording = controller.isRecording();
    if (!isRecording) {
        controller.notifyActionFailedBecauseRecordingOff();
    }

    return isRecording;
}

}   // namespace arkostracker
