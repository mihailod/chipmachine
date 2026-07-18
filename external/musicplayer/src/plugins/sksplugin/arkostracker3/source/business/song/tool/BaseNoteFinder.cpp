#include "BaseNoteFinder.h"

#include "../../../song/Song.h"
#include "../../../song/cells/CellConstants.h"
#include "browser/CellBrowser.h"

namespace arkostracker 
{

BaseNoteFinder::BaseNoteFinder(const Song& song, const Id& subsongId, int noteRange) noexcept :
        afterLastNoteIndex(CellConstants::maximumNote + 1),
        noteIndexToIteration(),
        occurrenceMap(),
        baseNote()
{
    jassert((afterLastNoteIndex > 0) && (afterLastNoteIndex < 256));

    // There as many items in the vector as there are possible notes.
    noteIndexToIteration.resize(static_cast<size_t>(afterLastNoteIndex), 0);

    // Finds all the notes by browsing the tracks.
    CellBrowser::browseConstSubsongCells(song, subsongId, true, [&](const Cell& cell) noexcept {
        onCellExplored(cell);
        return false;
    });

    // Now, finds the base note.
    baseNote = findBaseNote(noteRange);
}

void BaseNoteFinder::onCellExplored(const Cell& cell) noexcept
{
    if (cell.isNoteAndInstrument()) {
        const auto note = cell.getNote().getValueRef().getNote();

        jassert(note < afterLastNoteIndex);
        if (note < afterLastNoteIndex) {        // Security. Wouldn't want a crash just for a stupid bad format.
            noteIndexToIteration.at(static_cast<size_t>(note))++;
        }

        occurrenceMap.addItem(note);
    }
}

int BaseNoteFinder::getBaseNote() const noexcept
{
    return baseNote;
}

int BaseNoteFinder::findBaseNote(int noteRange) const noexcept
{
    // Not an efficient algorithm, who cares.
    auto bestBaseNoteIndex = -1;
    auto bestScore = -1;

    for (auto baseNoteIndex = 0; (baseNoteIndex + noteRange) <= afterLastNoteIndex; ++baseNoteIndex) {
        const auto score = calculateScore(baseNoteIndex, noteRange);
        if (score > bestScore) {
            bestScore = score;
            bestBaseNoteIndex = baseNoteIndex;
        }
    }

    return bestBaseNoteIndex;
}

int BaseNoteFinder::calculateScore(int baseNoteIndex, int noteRange) const noexcept
{
    jassert((baseNoteIndex + noteRange) <= afterLastNoteIndex);

    // Counts all the notes that are within the range.
    auto foundNoteCount = 0;
    for (auto noteIndex = baseNoteIndex; noteIndex < (baseNoteIndex + noteRange); ++noteIndex) {
        jassert(noteIndex < afterLastNoteIndex);

        foundNoteCount += noteIndexToIteration.at(static_cast<size_t>(noteIndex));
    }

    return foundNoteCount;
}

std::vector<std::pair<int, unsigned int>> BaseNoteFinder::getSortedNoteIndexToIteration() const noexcept
{
    return occurrenceMap.generateSortedMostUsedItemsAndOccurrence();
}

}   // namespace arkostracker
