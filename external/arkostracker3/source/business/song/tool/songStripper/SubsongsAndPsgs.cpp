#include "SubsongsAndPsgs.h"

#include "../../../../song/Song.h"

namespace arkostracker 
{

SubsongsAndPsgs::SubsongsAndPsgs(const Song& song) noexcept
{
    const auto subsongIds = song.getSubsongIds();
    addSubsongs(song, subsongIds);
}

SubsongsAndPsgs::SubsongsAndPsgs(const Song& song, const std::vector<Id>& subsongIds) noexcept
{
    addSubsongs(song, subsongIds);
}

void SubsongsAndPsgs::addSubsongs(const Song& song, const std::vector<Id>& subsongIds) noexcept
{
    for (const auto& subsongId : subsongIds) {
        auto psgCount = 0;
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
            psgCount = subsong.getPsgCount();
        });

        // Adds as many entries as there are PSGs.
        std::set<int> psgIndexes;
        for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
            psgIndexes.insert(psgIndex);
        }
        addSubsongAndPsgs(subsongId, psgIndexes);
    }
}

void SubsongsAndPsgs::addSubsongAndPsgs(const Id& subsongId, const std::set<int>& psgs) noexcept
{
    // Does the Subsong already exist? If not, creates the entry.
    if (const auto iterator = subsongsIdToPsgIndexes.find(subsongId); iterator == subsongsIdToPsgIndexes.cend()) {
        subsongsIdToPsgIndexes.insert(std::make_pair(subsongId, psgs));
    } else {
        // The Subsong already exists.
        subsongsIdToPsgIndexes.at(subsongId).insert(psgs.cbegin(), psgs.cend());
    }
}

bool SubsongsAndPsgs::isSubsongPresent(const Id& subsongId) const noexcept
{
    return (subsongsIdToPsgIndexes.find(subsongId) != subsongsIdToPsgIndexes.cend());
}

std::set<int> SubsongsAndPsgs::getPsgIndexes(const Id& subsongId) const noexcept
{
    return subsongsIdToPsgIndexes.find(subsongId)->second;
}

std::set<int> SubsongsAndPsgs::getChannelIndexes(const Id& subsongId) const noexcept
{
    const auto psgIndexes = getPsgIndexes(subsongId);

    // Converts each PSG index to channels.
    std::set<int> channelIndexesToKeep;
    for (const auto psgIndex : psgIndexes) {
        const auto channels = PsgValues::getChannelsFromPsg(psgIndex);
        channelIndexesToKeep.insert(channels.cbegin(), channels.cend());
    }

    return channelIndexesToKeep;
}

}   // namespace arkostracker
