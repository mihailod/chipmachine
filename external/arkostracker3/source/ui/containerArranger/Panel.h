#pragma once

#include "BoundedComponent.h"
#include "PanelType.h"

namespace arkostracker 
{

/** A Component that has a type and a symbol. */
class Panel : public BoundedComponent
{
public:
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when a panel becomes focused.
         * @param panelType the panel type.
         */
        virtual void onPanelFocused(PanelType panelType) noexcept = 0;

        /**
         * Called when Tab is pressed and a "next/previous" container must be focused.
         * @param panelType the current panel type.
         * @param next true to go to the next, false to go to the previous.
         */
        virtual void focusNextOrPreviousContainer(PanelType panelType, bool next) noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     */
    explicit Panel(Listener& listener) noexcept;

    /** @return the type of the panel. */
    virtual PanelType getType() const noexcept = 0;

    /** @return false not to draw the border around the panel. The default is true. */
    virtual bool mustDrawBorder() const noexcept;

    /** Gets the keyboard focus to the view. */
    virtual void getKeyboardFocus() noexcept = 0;

    // BoundedComponent method implementations.
    // ============================================
    int getXInsideComponent() const noexcept override;
    int getYInsideComponent() const noexcept override;
    int getAvailableWidthInComponent() const noexcept override;
    int getAvailableHeightInComponent() const noexcept override;

    // Component method implementations.
    // ==================================
    void paint(juce::Graphics& g) override;
    void focusOfChildComponentChanged(FocusChangeType cause) override;
    bool keyPressed(const juce::KeyPress& key) override;

protected:
    static const int borderThickness;

    /**
     * Draws the border. Called only when needed. Implementation can use classical getWidth and getHeight to get the size.
     * @param g the graphics object.
     * @param hasFocus true if the panel has focus.
     */
    virtual void drawBorder(juce::Graphics& g, bool hasFocus) noexcept;

private:
    Listener& listener;
    bool hasFocus;
};

}   // namespace arkostracker
