#pragma once

#include "juce_gui_basics/juce_gui_basics.h"

namespace arkostracker 
{

/**
 * A Viewport that bypasses its own management of keys, else, if scrollbars are present, the cursor will scroll, and the parent won't be able
 * to use these keys. Not user-friendly in the PatternViewer!
 */
class ViewPortInterceptKeys : public juce::Viewport
{
public:
    /**
     * Constructor.
     * @param allowKeys true to let the component handle the keys, false to let its children do it if they want.
     */
    explicit ViewPortInterceptKeys(bool allowKeys = false) noexcept;

    // juce::Viewport method implementations.
    // =========================================
    bool keyPressed(const juce::KeyPress& keyPressed) override;

private:
    const bool allowKeys;                                                 // True to intercept the key.
};

}   // namespace arkostracker
