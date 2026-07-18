#pragma once

#include "ArrangerView.h"

namespace arkostracker 
{

/**
 * A "handle" that highlights when the user's mouse is over, else invisible.
 * It is also used by the arranger to drop Containers on it.
 *
 * Note that dragging this Handle notifies the listener, but the Handles does nothing about it by itself.
 */
class Handle final : public ArrangerView
{
public:

    /** Listener to this Handler events. */
    class HandleListener
    {
    public:
        /** Destructor. */
        virtual ~HandleListener() = default;

        /**
         * Called when a drag has been performed.
         * @param handleId the ID of the handle.
         * @param distancePixels the distance, in pixels, according to the orientation of the Handle.
         */
        virtual void onHandleDragged(int handleId, int distancePixels) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to be notified of the events.
     * @param horizontal true if the view is horizontal, false if vertical.
     * @param isExtremity true if first or last (cannot be dragged, maybe other dimensions).
     */
    Handle(HandleListener& listener, bool horizontal, bool isExtremity) noexcept;

    // Component method implementations.
    // =======================================
    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

    // ArrangerView method implementations.
    // =======================================
    Dimension getArrangerViewWidth() const noexcept override;
    Dimension getArrangerViewHeight() const noexcept override;

private:
    static const int thickness;
    static const int thicknessOnExtremities;

    /**
     * @return the dimension.
     * @param isExtremity true if an extremity.
     */
    static int getDimension(bool isExtremity) noexcept;

    const Dimension lockedDimension;                            // The dimension the bar (thickness).

    HandleListener& listener;                                   // The listener of the events.
    const bool horizontal;                                      // True if the handle is horizontal, false if vertical.
    const bool isExtremity;

    bool mouseOverOrDragged;                                    // True if the mouse is over or the View is being dragged.
};

}   // namespace arkostracker

