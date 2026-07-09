#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** Handy class of a Group with a Viewport inside, where others Components can be easily added. */
class GroupWithViewport final : public juce::Component
{
public:
    /**
     * Constructor.
     * @param title the title to show in the Group.
     * @param removeVerticalScrollbar true to remove the vertical scrollbar.
     * @param autoHideVerticalScrollbar false to always show the vertical scrollbar.
     */
    explicit GroupWithViewport(const juce::String& title, bool removeVerticalScrollbar = false, bool autoHideVerticalScrollbar = true) noexcept;

    /**
     * Sets a new title to the Group.
     * @param title the title.
     */
    void setGroupTitle(const juce::String& title) noexcept;

    /**
     * Adds a Component, made visible. The bounds of the viewport can only be set right if the given
     * Component has its bounds set. Else, use setViewportHeight method after.
     * The client remains handler of the life-cycle of the Component!
     * @param component the Component to add. Still managed by the client.
     */
    void addComponentToGroup(juce::Component& component) noexcept;

    /**
     * Forces the height of the Viewport. This is when the Components are added the client knows their location.
     * @param height the height.
     */
    void setViewportHeight(int height) noexcept;

    /**
     * Removes all the displayed children that were added in the Viewport.
     * The objects are not deleted, as they are not owned by this object.
     */
    void removeAllGroupChildren() noexcept;

    /** @return the inner space inside the Group. */
    juce::Rectangle<int> getGroupInnerArea() const noexcept;

    /** @return the X of the scrolling. */
    int getScrollingX() const noexcept;
    /** @return the Y of the scrolling. */
    int getScrollingY() const noexcept;

    /**
     * Sets a X position of the scrolling.
     * @param scrollingX the X position.
     */
    void setScrollingX(int scrollingX) noexcept;

    /**
     * Sets a Y position of the scrolling.
     * @param scrollingY the Y position.
     */
    void setScrollingY(int scrollingY) noexcept;

    // Component method implementations.
    // ==============================================================
    void resized() override;

private:
    juce::GroupComponent group;                                                     // The Group containing the Viewport.
    juce::Viewport viewport;                                                        // The Viewport inside the Group, where all the Views are added.
    juce::Component viewportContainer;                                              // Where all the Views are added, inside the Viewport.
};

}   // namespace arkostracker
