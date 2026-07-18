#pragma once

#include "../BaseHeaderView.h"

namespace arkostracker 
{

/** The header at the top of a LineTrack. */
class LineHeader final : public BaseHeaderView
{
public:
    /** Listener to the events of this Component. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** The user clicked on the height number. */
        virtual void onUserClickedOnHeight() = 0;

        /**
         * The user use the mouse wheel on the height number.
         * @param up true if going up, false if going down.
         * @param isShiftPressed true if shift is pressed.
         */
        virtual void onUserUsedMouseWheelOnHeight(bool up, bool isShiftPressed) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the event of this Component.
     * @param heightToShow the height to show.
     * @param showInHex true to show the height in hex, false for decimal.
     */
    explicit LineHeader(Listener& listener, int heightToShow, bool showInHex) noexcept;

    /**
     * Sets the displayed data. The UI is refreshed if needed.
     * @param newHeight the new height.
     * @param showInHex true to show the height in hex, false for decimal.
     */
    void setDisplayedData(int newHeight, bool showInHex) noexcept;

    // Component method implementations.
    // =====================================
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

protected:
    // BaseHeaderView method implementations.
    // =======================================
    void paintAfter(juce::Graphics& g, juce::Colour backgroundColor) noexcept override;

    juce::Colour getBackgroundColor() const noexcept override;

private:
    Listener& listener;
    int shownHeight;
    bool showInHex;
    juce::Font font;                            // The font to use to display the height.
};

}   // namespace arkostracker

