#pragma once

#include "../containerArranger/Panel.h"

namespace arkostracker 
{

/** A dummy Panel, only to be replaced. */
class DummyPanel final : public Panel
{
public:
    /**
     * Constructor.
     * @param listener to get the panel events.
     */
    explicit DummyPanel(Panel::Listener& listener) noexcept;

    // Panel method implementations.
    // ================================
    PanelType getType() const noexcept override;
    void getKeyboardFocus() noexcept override;

    // Component method implementations.
    // ================================
    void resized() override;

private:
    juce::Label label;
};

}   // namespace arkostracker
