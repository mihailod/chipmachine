#include "ModConfiguration.h"

namespace arkostracker 
{

ModConfiguration::ModConfiguration() noexcept :
        importInstrumentsAsPsg(false),
        sourceToDestinationTrackIndexes(),
        trackIndexesToKeep({ 0, 1, 2 })     // Default. Must be valid, else the first pass will fail, preventing to determine the track count for the import panel.
{
}

ModConfiguration::ModConfiguration(bool pImportInstrumentsAsPsg, std::vector<std::pair<int, int>> pSourceToDestinationTrackIndexes,
                                   std::set<int> pTrackIndexesToKeep) noexcept :
        importInstrumentsAsPsg(pImportInstrumentsAsPsg),
        sourceToDestinationTrackIndexes(std::move(pSourceToDestinationTrackIndexes)),
        trackIndexesToKeep(std::move(pTrackIndexesToKeep))
{
}

bool ModConfiguration::isImportInstrumentsAsPsg() const noexcept
{
    return importInstrumentsAsPsg;
}

const std::vector<std::pair<int, int>>& ModConfiguration::getTrackMixes() const noexcept
{
    return sourceToDestinationTrackIndexes;
}

const std::set<int>& ModConfiguration::getTrackIndexesToKeep() const noexcept
{
    return trackIndexesToKeep;
}

int ModConfiguration::getKeptTrackCount() const noexcept
{
    return static_cast<int>(trackIndexesToKeep.size());
}


}   // namespace arkostracker

