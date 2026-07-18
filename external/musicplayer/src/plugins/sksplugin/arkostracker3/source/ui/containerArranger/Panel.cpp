#include "Panel.h"

#include "../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const int Panel::borderThickness = 1;

Panel::Panel(Listener& pListener) noexcept :
        listener(pListener),
        hasFocus(false)
{
    setOpaque(true);
    setWantsKeyboardFocus(true);        // In order to have Commands always available.
}

bool Panel::mustDrawBorder() const noexcept
{
    return true;
}


// Component method implementations.
// ==================================

void Panel::paint(juce::Graphics& g)
{
    const auto backgroundColor = findColour(static_cast<int>(
            hasFocus ? LookAndFeelConstants::Colors::panelBackgroundFocused : LookAndFeelConstants::Colors::panelBackgroundUnfocused));
    g.fillAll(backgroundColor);

    if (mustDrawBorder()) {
        drawBorder(g, hasFocus);
    }
}

void Panel::focusOfChildComponentChanged(juce::Component::FocusChangeType /*cause*/)
{
    const auto newFocus = hasKeyboardFocus(true);
    if (hasFocus != newFocus) {                 // Keeps the focus not to have to call hasKeyboardFocus again, may be CPU intensive.
        hasFocus = newFocus;
        repaint();

        if (hasFocus) {
            listener.onPanelFocused(getType());
        }
    }
}

void Panel::drawBorder(juce::Graphics& g, const bool focused) noexcept
{
    const auto borderColor = findColour(static_cast<int>(
                focused ? LookAndFeelConstants::Colors::panelBorderFocused : LookAndFeelConstants::Colors::panelBorderUnfocused));
    g.setColour(borderColor);
    g.drawRect(0, 0, getWidth(), getHeight());
}

bool Panel::keyPressed(const juce::KeyPress& key)
{
    // Warning: make sure that no panel use Tab (it doesn't seem to be readable anyway).
    if (key.getKeyCode() == juce::KeyPress::tabKey) {
        const auto panelType = getType();
        const auto modifiers = key.getModifiers();
        if (modifiers.isShiftDown()) {
            listener.focusNextOrPreviousContainer(panelType, false);
            return true;
        }
        if (!modifiers.isAnyModifierKeyDown()) {
            listener.focusNextOrPreviousContainer(panelType, true);
            return true;
        }
    }

    return false;
}


// BoundedComponent method implementations.
// ============================================

int Panel::getXInsideComponent() const noexcept
{
    return mustDrawBorder() ? borderThickness : 0;
}

int Panel::getYInsideComponent() const noexcept
{
    return mustDrawBorder() ? borderThickness : 0;
}

int Panel::getAvailableWidthInComponent() const noexcept
{
    return getWidth() - (mustDrawBorder() ? (borderThickness * 2) : 0);
}

int Panel::getAvailableHeightInComponent() const noexcept
{
    return getHeight() - (mustDrawBorder() ? (borderThickness * 2) : 0);
}

}   // namespace arkostracker
