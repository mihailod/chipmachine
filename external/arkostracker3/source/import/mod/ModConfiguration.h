#pragma once

#include <set>
#include <vector>

namespace arkostracker 
{

/** Holds the configuration for a MOD import. */
class ModConfiguration
{
public:
    /** Default constructor. */
    ModConfiguration() noexcept;

    /**
     * Constructor.
     * @param importInstrumentsAsPsg true to ignore samples.
     * @param sourceToDestinationTrackIndexes indexes of the tracks to mix into destination indexes.
     * @param trackIndexesToKeep indexes of the tracks to keep in the original song.
     */
    ModConfiguration(bool importInstrumentsAsPsg, std::vector<std::pair<int, int>> sourceToDestinationTrackIndexes,
                     std::set<int> trackIndexesToKeep) noexcept;

    /** @return true to convert samples to PSG instruments. */
    bool isImportInstrumentsAsPsg() const noexcept;
    /** @return the indexes of the tracks to mix into destination indexes. */
    const std::vector<std::pair<int, int>>& getTrackMixes() const noexcept;
    /** @return the indexes of the tracks to keep in the original song. */
    const std::set<int>& getTrackIndexesToKeep() const noexcept;

    /** @return how many tracks are kept. */
    int getKeptTrackCount() const noexcept;

private:
    bool importInstrumentsAsPsg;                                            // True to ignore samples.
    std::vector<std::pair<int, int>> sourceToDestinationTrackIndexes;       // Indexes of the tracks to mix into destination indexes.
    std::set<int> trackIndexesToKeep;                             // Indexes of the tracks to keep in the original song.
};



}   // namespace arkostracker

