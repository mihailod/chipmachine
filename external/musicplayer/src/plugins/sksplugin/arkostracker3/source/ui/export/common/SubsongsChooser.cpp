#include "SubsongsChooser.h"

#include "../../../song/Song.h"
#include "../../../utils/NumberUtil.h"

namespace arkostracker
{
SubsongsChooser::SubsongsChooser(const Song& pSong, std::function<void(const std::vector<Id>&)> pOnSubsongsChanged) noexcept :
        subsongNamesAndIds(pSong.getSubsongNamesAndIds()),
        onSubsongsChanged(std::move(pOnSubsongsChanged)),
        group(juce::translate("Subsongs to export"), static_cast<int>(subsongNamesAndIds.size()), false, false,
            toggleWidth,
            [&](const int subsongIndex) {
                return juce::String(subsongIndex) + juce::translate(": ") + subsongNamesAndIds.at(static_cast<size_t>(subsongIndex)).first;
            },
            [&](const int /*subsongIndex*/) {
                return true;        // Selects all at first.
            })
{
    addAndMakeVisible(group);
}

std::vector<Id> SubsongsChooser::getSelectedSubsongIds() const noexcept
{
    const auto indexes = group.getToggledIndexes();
    std::vector<Id> ids;
    ids.reserve(indexes.size());

    for (const auto index : indexes) {
        const auto subsongId = subsongNamesAndIds.at(static_cast<size_t>(index)).second;
        ids.push_back(subsongId);
    }

    return ids;
}

void SubsongsChooser::selectedAll() const noexcept
{
    group.selectAll();
}


// Component method implementations.
// ====================================

void SubsongsChooser::resized()
{
    group.setBounds(0, 0, getWidth(), getHeight());
}

}   // namespace arkostracker
