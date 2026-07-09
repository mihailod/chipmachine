#pragma once

#include <memory>

#include <juce_gui_extra/juce_gui_extra.h>

namespace arkostracker
{

/** Some utility class for UI. */
class UiUtil
{
public:
    static constexpr auto defaultDurationMs = 4000;
    static constexpr auto fadeOutLengthMs = 500;

    /**
     * Creates and shows a Bubble MessageComponent near the given Component. The parent is simply the direct parent of the target component.
     * Use the other method in case the bubble appears out of bounds.
     * @param targetComponent the target Component.
     * @param text the text to show.
     * @param locateAboveOnly true to locate the bubble only above the target Component. False not to limit.
     * @param durationMs how to long to show the bubble, in ms.
     * @return the shown bubble.
     */
    static std::unique_ptr<juce::BubbleMessageComponent> createBubble(juce::Component& targetComponent,
        const juce::String& text, bool locateAboveOnly = false, int durationMs = defaultDurationMs) noexcept;

    /**
     * Creates and shows a Bubble MessageComponent near the given Component.
     * @param targetComponent the target Component.
     * @param parentComponent the parent. Useful in case the bubble appears out of bounds, another parent may be needed.
     * @param text the text to show.
     * @param locateAboveOnly true to locate the bubble only above the target Component. False not to limit.
     * @param durationMs how to long to show the bubble, in ms.
     * @return the shown bubble.
     */
    static std::unique_ptr<juce::BubbleMessageComponent> createBubble(juce::Component& targetComponent, juce::Component& parentComponent,
        const juce::String& text, bool locateAboveOnly = false, int durationMs = defaultDurationMs) noexcept;
};

}   // namespace arkostracker
