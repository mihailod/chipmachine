#pragma once

#include <functional>
#include <memory>
#include <set>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A Group with as many Toggles inside as wanted, along with a Viewport in case it is not big enough. */
class GroupWithToggles final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param groupText the text at the top of the Group.
     * @param toggleCount how many Toggles there are.
     * @param allowNoneSelected true to allow no Toggle to be selected.
     * @param canOnlyOneBeSelected true to force that only one Toggle can be selected at a time.
     * @param togglesWidth the width of each Toggle, or 0 for each Toggle to fill the whole width.
     * @param indexToName function to get the name of the Toggle, according to its index. Only used on initialization.
     * @param indexToTicked function indicating whether the Toggle should be ticked, according to its index. Only used on initialization.
     */
    GroupWithToggles(const juce::String& groupText, int toggleCount, bool allowNoneSelected, bool canOnlyOneBeSelected, int togglesWidth,
                     const std::function<juce::String(int)>& indexToName, const std::function<bool(int)>& indexToTicked) noexcept;

    /** @return the indexes of the Toggles that are ticked. */
    std::set<int> getToggledIndexes() const noexcept;

    /** Selects all the Toggles. Does nothing if not allowed. */
    void selectAll() const noexcept;

    /**
     * Sets the given index of the Toggle to select.
     * @param indexesToSelect the indexes of the Toggle.
     */
    void setSelected(const std::set<int>& indexesToSelect) const noexcept;

    // Component method implementations.
    // ==============================================
    void resized() override;

    // ==============================================

    /** Sets this to be called when the selection changes. */
    std::function<void()> onSelectionChanged;           // NOLINT(*-non-private-member-variables-in-classes)

private:
    /**
     * Called when a Toggle Button is clicked. Its state has changed already.
     * @param toggleIndex the index of the Toggle.
     */
    void onToggleButtonClicked(int toggleIndex) const noexcept;

    bool allowNoneSelected;
    bool canOnlyOneBeSelected;
    juce::GroupComponent group;                                             // The Group around the Toggles.
    std::vector<std::unique_ptr<juce::ToggleButton>> toggleButtons;         // The toggle Buttons.
    juce::Viewport viewport;                                                // The Viewport inside the Group.
    Component innerViewportComponent;                                       // The component inside the Viewport, inside which Toggles will be added.
    int baseTogglesWidth;                                                   // 0 to take all the width.
};

}   // namespace arkostracker

