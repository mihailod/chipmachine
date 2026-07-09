#pragma once

#include <vector>

#include "../../../../utils/OptionalValue.h"

namespace arkostracker
{

class Song;

/** Tool to find the first note used for each instrument. Non-exported Instruments are also found. Samples are ignored. */
class FirstNotePerInstrumentFinder
{
public:
    /** Prevents instantiation. */
    FirstNotePerInstrumentFinder() = delete;

    class ResultItem
    {
    public:
        ResultItem(OptionalInt foundNote, bool exported) noexcept;

        OptionalInt getFoundNote() const noexcept;
        /** @return may be true even if there is no note! For example, the user wanted a sfx, but removed it from the song tracks. */
        bool isExported() const noexcept;
        /** @return true if the sfx is exported and the note is found. */
        bool canUseUsed() const noexcept;

    private:
        OptionalInt foundNote;                      // The note, if found.
        bool exported;                              // True if the instrument is set as "exported".
    };

    /**
     * Finds the first note of each Instruments (excluding 0 and the samples).
     * @param song the Song. All Subsongs will be browsed.
     * @return the instrument indexes, in order, linked to the related sound effects. There is may be holes, as samples are excluded.
     */
    static std::map<int, ResultItem> find(const Song& song) noexcept;

private:
    /**
     * Finds an Instrument in the Song and fills the given Instrument Index structure.
     * @param song the Song.
     * @param instrumentIndex the Instrument index to find in a Cell of the Song.
     * @param isSfxInstrumentExported true if the Instrument sfx is exported.
     * @param instrumentIndexToSfxResultToFill will be fill with the Sound effect of the Instrument if found, or default data if not.
     */
    static void findInstrument(const Song& song, int instrumentIndex, bool isSfxInstrumentExported,
                               std::map<int, FirstNotePerInstrumentFinder::ResultItem>& instrumentIndexToSfxResultToFill) noexcept;

};

}   // namespace arkostracker
