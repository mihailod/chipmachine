#pragma once

#include "ViewPortInterceptKeys.h"

namespace arkostracker 
{

/**
 * A Viewport that bypasses its own management of keys, else, if scrollbars are present, the cursor will scroll, and the parent won't be able
 * to use these keys. Not user-friendly in the PatternViewer!
 * Also, using Alt+wheel will not do a horizontal scroll.
 */
class ViewPortWithHorizontalMouseWheel : public ViewPortInterceptKeys
{
public:
    /**
     * Constructor.
     * @param allowKeys true to let the component handle the keys, false to let its children do it if they want.
     */
    explicit ViewPortWithHorizontalMouseWheel(bool allowKeys = false) noexcept;

    // juce::Viewport method implementations.
    // =========================================
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
};

}   // namespace arkostracker
