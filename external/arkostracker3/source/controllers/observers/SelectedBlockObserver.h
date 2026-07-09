#pragma once

namespace arkostracker 
{

/** Observer of a selection block (such as on the "line" on the PatternViewer). */
class SelectedBlockObserver
{
public:
    /** Destructor. */
    virtual ~SelectedBlockObserver() = default;

    /**
     * Called when a new selection block is set. An empty range means "from cursor" selection (the start indicates the start line).
     * @param range the range.
     */
    virtual void onNewSelectedBlock(juce::Range<int> range) noexcept = 0;
};

}   // namespace arkostracker
