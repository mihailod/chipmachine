#include "GroupWithToggles.h"

namespace arkostracker 
{

GroupWithToggles::GroupWithToggles(const juce::String& pGroupText, const int pToggleCount, const bool pAllowNoneSelected, const bool pCanOnlyOneBeSelected,
                                   const int pTogglesWidth,
                                   const std::function<juce::String(int)>& pIndexToName,
                                   const std::function<bool(int)>& pIndexToTicked) noexcept :
        onSelectionChanged(),
        allowNoneSelected(pAllowNoneSelected),
        canOnlyOneBeSelected(pCanOnlyOneBeSelected),
        group(juce::String(), pGroupText),
        toggleButtons(),
        viewport(),
        innerViewportComponent(),
        baseTogglesWidth(pTogglesWidth)
{
    addAndMakeVisible(group);
    addAndMakeVisible(viewport);
    addAndMakeVisible(innerViewportComponent);

    viewport.setViewedComponent(&innerViewportComponent, false);
    viewport.setScrollBarsShown(true, false, true, false);

    // Creates as many ToggleButtons as necessary. Their size is set later.
    for (auto toggleIndex = 0; toggleIndex < pToggleCount; ++toggleIndex) {
        const auto string = pIndexToName(toggleIndex);
        auto toggleButton = std::make_unique<juce::ToggleButton>(string);

        const auto ticked = pIndexToTicked(toggleIndex);
        toggleButton->setToggleState(ticked, juce::NotificationType::dontSendNotification);
        toggleButton->onClick = [&, toggleIndex] { onToggleButtonClicked(toggleIndex); };
        // Sets a Radio Group if the Toggles are exclusive.
        if (canOnlyOneBeSelected) {
            constexpr auto radioGroup = 1234;
            toggleButton->setRadioGroupId(radioGroup, juce::NotificationType::dontSendNotification);
        }
        innerViewportComponent.addAndMakeVisible(*toggleButton);

        toggleButtons.push_back(std::move(toggleButton));
    }
}

void GroupWithToggles::resized()
{
    group.setBounds(0, 0, getWidth(), getHeight());

    constexpr auto marginTop = 18;
    constexpr auto margin = 10;
    constexpr auto togglesHeight = 25;

    // Adds the Toggles inside the Group.
    auto toggleArea = group.getBounds().reduced(margin, 0);
    (void)toggleArea.removeFromTop(marginTop);
    (void)toggleArea.removeFromBottom(margin);
    viewport.setBounds(toggleArea);

    const auto innerWidth = toggleArea.getWidth();
    // Have the Toggles used the whole width if wanted.
    const auto togglesWidth = (baseTogglesWidth <= 0) ? innerWidth : baseTogglesWidth;

    // There should be at least enough room!
    jassert(togglesWidth <= innerWidth);
    if (togglesWidth > innerWidth) {
        return;
    }

    auto x = 0;
    auto y = 0;
    auto latestUsedY = y;
    for (const auto& toggleButton : toggleButtons) {
        // Shows the Toggle.
        toggleButton->setBounds(x, y, togglesWidth, togglesHeight);
        latestUsedY = y;

        // Calculates the location of the next ToggleButton.
        // Is there enough room for one more toggle on this line?
        x += togglesWidth;
        if ((x + togglesWidth) >= innerWidth) {
            // No more room! Goes to the next line.
            x = 0;
            y += togglesHeight;
        }
    }

    // Sets the bounds of the inner Viewport Component.
    innerViewportComponent.setBounds(0, 0, innerWidth, (latestUsedY + togglesHeight));
}

std::set<int> GroupWithToggles::getToggledIndexes() const noexcept
{
    std::set<int> result;

    // Stores in the Vector the indexes of the ToggleButtons that are toggled.
    auto index = 0;
    for (const auto& toggleButton : toggleButtons) {
        if (toggleButton->getToggleState()) {
            result.insert(index);
        }
        ++index;
    }

    return result;
}

void GroupWithToggles::selectAll() const noexcept
{
    // Sanity check.
    if (canOnlyOneBeSelected && (toggleButtons.size() > 1U)) {
        jassertfalse;           // Only one can be selected.
        return;
    }

    for (const auto& toggle : toggleButtons) {
        toggle->setToggleState(true, juce::NotificationType::dontSendNotification);
    }
}

void GroupWithToggles::setSelected(const std::set<int>& indexesToSelect) const noexcept
{
    // Sanity check.
    const auto moreThanOneToggle = (toggleButtons.size() > 1U);
    const auto emptyIndexes = indexesToSelect.empty();
    if (allowNoneSelected && moreThanOneToggle && emptyIndexes) {
        jassertfalse;           // Nothing to be selected, but this is not allowed.
        return;
    }
    if (canOnlyOneBeSelected && moreThanOneToggle && (indexesToSelect.size() > 1U)) {
        jassertfalse;           // Only one Toggle can be selected max, yet more are asked to be selected.
        return;
    }

    auto index = 0;
    for (const auto& toggle : toggleButtons) {
        const auto selected = (indexesToSelect.find(index) != indexesToSelect.cend());
        toggle->setToggleState(selected, juce::NotificationType::dontSendNotification);

        ++index;
    }
}

void GroupWithToggles::onToggleButtonClicked(const int toggleIndex) const noexcept
{
    const auto& toggleButton = toggleButtons.at(static_cast<size_t>(toggleIndex));
    const auto ticked = toggleButton->getToggleState();     // This is the new state.

    // If this was the last one ticked, and that is not allowed, ticks it back!
    if (!ticked && !allowNoneSelected && !canOnlyOneBeSelected) {
        // Is there at least one ticked?
        for (const auto& toggle : toggleButtons) {
            if (toggle->getToggleState()) {
                return;         // Ticked! We can stop now.
            }
        }

        // No more ticked! Sets the latest modified.
        toggleButton->setToggleState(true, juce::NotificationType::dontSendNotification);
    }

    if (onSelectionChanged != nullptr) {
        onSelectionChanged();
    }
}

}   // namespace arkostracker
