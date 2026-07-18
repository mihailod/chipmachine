#pragma once

#include <map>
#include <set>
#include <vector>

#include "../../../../utils/Id.h"

namespace arkostracker 
{

class Song;

/** Holds Subsongs IDs and psgs indexes, to know which one to keep. */
class SubsongsAndPsgs
{
public:

    /**
     * Convenience constructor to use if all the PSGs of Subsongs are included.
     * @param song the Song.
     */
    explicit SubsongsAndPsgs(const Song& song) noexcept;

    /**
     * Constructor. By using an empty array, we can add subsongs one by one.
     * @param song the Song.
     * @param subsongIds the Subsong IDs we are interested in. If empty, interested in none. All the PSGs are added.
     */
    SubsongsAndPsgs(const Song& song, const std::vector<Id>& subsongIds) noexcept;

    /**
     * Adds Subsongs and all their PSGs.
     * @param song the Song.
     * @param subsongIds the Subsong IDs we are interested in. They may already exist, in which case the already stored PSGs are replaced.
     */
    void addSubsongs(const Song& song, const std::vector<Id>& subsongIds) noexcept;

    /**
     * Adds a Subsong and PSG. The Subsong may already exist.
     * @param subsongId the Subsong id.
     * @param psgs the PSGs to add. If already present, a PSG is not included.
     */
    void addSubsongAndPsgs(const Id& subsongId, const std::set<int>& psgs) noexcept;

    /**
     * @return true if the Subsong is present in the list, and thus must be kept.
     * @param subsongId the ID of the Subsong.
     */
    bool isSubsongPresent(const Id& subsongId) const noexcept;

    /**
     * @return the PSG indexes to keep, from the given SubsongId.
     * @param subsongId the Subsong ID. It must exist.
     */
    std::set<int> getPsgIndexes(const Id& subsongId) const noexcept;

    /**
     * @return the channel indexes to keep, from the given SubsongId.
     * @param subsongId the Subsong ID. It must exist.
     */
    std::set<int> getChannelIndexes(const Id& subsongId) const noexcept;

private:
    std::map<Id, std::set<int>> subsongsIdToPsgIndexes;
};

}   // namespace arkostracker
