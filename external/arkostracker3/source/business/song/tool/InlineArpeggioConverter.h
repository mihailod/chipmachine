#pragma once

#include <memory>
#include <unordered_map>

namespace arkostracker 
{

class Expression;
class Song;
class Cell;

/**
 * This class is useful to convert the inline (i.e. direct) Arpeggios from a Song into table Arpeggios.
 * The process may fail in case there are too many arpeggio than the format can support.
 *
 * The given Song is modified.
 *
 * All the Subsongs are parsed. Only their used Tracks are read, it stops at the loop end.
 * Arpeggios with only 0 are encoded as Arpeggio Table 0.
 *
 * Note that no optimization is performed:
 * - Generated Arpeggios are not compared with existing ones: some doubles may be generated.
 * - "Holes" or unused Arpeggios are not filled: the new Arpeggios are added at the end.
 *
 * This is up to the SongOptimizer should take care of these cases.
 */
class InlineArpeggioConverter
{
public:
    /**
     * Constructor.
     * @param song the Song to read and modify (!).
     * @param maximumIndexArpeggio the index of the maximum Arpeggio that can be created.
     */
    explicit InlineArpeggioConverter(Song& song, int maximumIndexArpeggio = 255) noexcept;

    /**
     * Converts the Arpeggios.
     * @return true if everything went fine, false if too many arpeggio were created.
     */
    bool convert() noexcept;

private:
    /**
     * Reads the given Cell, finds if there are any Table Arpeggio effects, generates the related Arpeggio,
     * return the modified Cell if a modification has been done.
     * @param the input Cell.
     * @param success true if everything went fine. False if too many Arpeggios have been generated.
     * @return nullptr if no modification is done, else the modified cell.
     */
    std::unique_ptr<Cell> readCellAndManageArpeggios(const Cell& cell, bool& success) noexcept;

    /**
     * Fills the Arpeggio which index is given. It must already exist, and should be empty.
     * @param arpeggio the Arpeggio to fill. Must be empty.
     * @param arpeggioValue the value, each nibble being a digit.
     * @param isArpeggio3Notes true if 3 nibbles are encoded, false if 4.
     */
    static void fillArpeggio(Expression& arpeggio, int arpeggioValue, bool isArpeggio3Notes) noexcept;

    Song& song;                                                             // The Song.
    int maximumIndexArpeggio;                                               // The index of the maximum Arpeggio that can be created.

    std::unordered_map<int, int> inlineArpeggio3NotesToArpeggioIndex;       // Map linking an Arpeggio 3 Notes to its generated Arpeggio.
    std::unordered_map<int, int> inlineArpeggio4NotesToArpeggioIndex;       // Map linking an Arpeggio 4 Notes to its generated Arpeggio.
};


}   // namespace arkostracker

