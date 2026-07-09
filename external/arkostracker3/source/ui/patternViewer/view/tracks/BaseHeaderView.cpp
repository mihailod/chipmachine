#include "BaseHeaderView.h"

namespace arkostracker 
{

const float BaseHeaderView::cornerSize = 8.0F;

BaseHeaderView::BaseHeaderView() noexcept :
        backgroundPath()                // Defined lazily.
{
}


// Component method implementations.
// =====================================

void BaseHeaderView::resized()
{
    const auto widthFloat = static_cast<float>(getWidth());
    const auto heightFloat = static_cast<float>(getHeight());

    // Refreshes the channel path.
    backgroundPath.clear();
    backgroundPath.addRoundedRectangle(0.0F, 0.0F, widthFloat, heightFloat, cornerSize, cornerSize,
                                       true, true, false, false);
}

void BaseHeaderView::paint(juce::Graphics& g)
{
    // The background.
    const auto backgroundColor = getBackgroundColor();
    g.setColour(backgroundColor);
    jassert(!backgroundPath.isEmpty());         // Path not ready yet?!
    g.fillPath(backgroundPath);

    paintAfter(g, backgroundColor);
}

}   // namespace arkostracker
