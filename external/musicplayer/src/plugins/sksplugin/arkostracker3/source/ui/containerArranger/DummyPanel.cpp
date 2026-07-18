#include "DummyPanel.h"

namespace arkostracker 
{

DummyPanel::DummyPanel(Panel::Listener& pListener) noexcept:
        Panel(pListener),
        label(juce::String(), juce::translate("Nothing to see here."))
{
    addAndMakeVisible(label);
}

PanelType DummyPanel::getType() const noexcept
{
    return PanelType::dummy;
}

void DummyPanel::resized()
{
    auto totalWidth = getAvailableWidthInComponent();
    auto totalHeight = getAvailableHeightInComponent();

    auto labelWidth = 150;
    auto labelHeight = 25;

    label.setBounds((totalWidth - labelWidth) / 2, (totalHeight - labelHeight) / 2, labelWidth, labelHeight);
}

void DummyPanel::getKeyboardFocus() noexcept
{
    // Nothing to do, not supposed to be seen.
}

}   // namespace arkostracker
