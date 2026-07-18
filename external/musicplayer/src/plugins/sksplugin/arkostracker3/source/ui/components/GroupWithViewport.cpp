#include "GroupWithViewport.h"

namespace arkostracker 
{

GroupWithViewport::GroupWithViewport(const juce::String& title, const bool removeVerticalScrollbar, const bool autoHideVerticalScrollbar) noexcept :
        group(juce::String(), title),
        viewport(),
        viewportContainer()
{
    addAndMakeVisible(group);
    addAndMakeVisible(viewport);
    viewport.addAndMakeVisible(viewportContainer);
    viewport.setViewedComponent(&viewportContainer, false);

    viewport.setScrollBarsShown(!removeVerticalScrollbar, false,
                                !removeVerticalScrollbar, false);      // Only vertical scroll bars.
    viewport.getVerticalScrollBar().setAutoHide(autoHideVerticalScrollbar);
}

void GroupWithViewport::setGroupTitle(const juce::String& title) noexcept
{
    group.setText(title);
}

void GroupWithViewport::addComponentToGroup(Component& component) noexcept
{
    viewportContainer.addAndMakeVisible(component);

    // The bounds of the added Component must be been set before, else we can't
    // set the bottom of the Viewport.
    const auto addedComponentBottom = component.getBottom();
    if (addedComponentBottom <= 0) {
        return;
    }

    // Defines the bounds of the container, if the added Component is "below".
    const auto viewportContainerHeight = viewportContainer.getHeight();
    if (viewportContainerHeight < addedComponentBottom) {
        setViewportHeight(addedComponentBottom);
    }
}

void GroupWithViewport::setViewportHeight(const int height) noexcept
{
    viewportContainer.setBounds(0, 0, viewport.getWidth(), height);
}

void GroupWithViewport::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto margins = 10;

    const auto topBorderInViewport = 7;
    const auto bottomBorderInViewport = margins;
    const auto viewportWidth = width - 2 * margins;
    const auto viewportHeight = height - margins - topBorderInViewport - bottomBorderInViewport;

    group.setBounds(0, 0, width, height);
    viewport.setBounds(margins, margins + topBorderInViewport, viewportWidth, viewportHeight);
}

void GroupWithViewport::removeAllGroupChildren() noexcept
{
    viewportContainer.removeAllChildren();

    setViewportHeight(0);
}

juce::Rectangle<int> GroupWithViewport::getGroupInnerArea() const noexcept
{
    return group.getLocalBounds();
}

int GroupWithViewport::getScrollingX() const noexcept
{
    return viewport.getViewPositionX();
}

int GroupWithViewport::getScrollingY() const noexcept
{
    return viewport.getViewPositionY();
}

void GroupWithViewport::setScrollingX(const int scrollingX) noexcept
{
    viewport.setViewPosition(scrollingX, viewport.getViewPositionY());
}

void GroupWithViewport::setScrollingY(const int scrollingY) noexcept
{
    viewport.setViewPosition(viewport.getViewPositionX(), scrollingY);
}

}   // namespace arkostracker
