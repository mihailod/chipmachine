#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/Id.h"
#include "../../components/GroupWithToggles.h"

namespace arkostracker
{

class Song;

/** Reusable Component to select at least one Subsongs. */
class SubsongsChooser final : public juce::Component
{
public:
    static constexpr auto desiredHeight = 104;
    static constexpr auto toggleWidth = 180;

    /**
     * Constructor.
     * @param song the Song.
     * @param onSubsongsChanged called when selected Subsong changed. Are given the Subsong IDs.
     */
    SubsongsChooser(const Song& song, std::function<void(const std::vector<Id>&)> onSubsongsChanged) noexcept;

    /** @return the ID of the selected Subsongs. */
    std::vector<Id> getSelectedSubsongIds() const noexcept;

    /** Selects all the items, if allowed. */
    void selectedAll() const noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;

private:
    std::vector<std::pair<juce::String, Id>> subsongNamesAndIds;
    std::function<void(const std::vector<Id>&)> onSubsongsChanged;

    GroupWithToggles group;
};

}   // namespace arkostracker
