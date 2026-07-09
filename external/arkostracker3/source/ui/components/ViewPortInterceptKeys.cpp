#include "ViewPortInterceptKeys.h"

namespace arkostracker 
{

ViewPortInterceptKeys::ViewPortInterceptKeys(const bool pAllowKeys) noexcept :
        allowKeys(pAllowKeys)
{
}


// juce::Viewport method implementations.
// =========================================

bool ViewPortInterceptKeys::keyPressed(const juce::KeyPress& keyPressed)
{
    // If no keys are not allowed, returns false for children to handle the keys.
    return allowKeys && Viewport::keyPressed(keyPressed);
}

}   // namespace arkostracker
