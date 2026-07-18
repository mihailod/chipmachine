#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "Tip.h"
#include "../../keyboard/CommandIds.h"

namespace arkostracker 
{

/** Manages the tips. */
class TipsController
{
public:

    /**
     * Constructor.
     * @param applicationCommandManager useful to get the key related to actions.
     */
    explicit TipsController(juce::ApplicationCommandManager& applicationCommandManager) noexcept;

    /** @return a random tip. */
    Tip getRandomTip() const noexcept;

    /** @return the next tip. */
    Tip getNextTip() const noexcept;

private:
    /** @return the tip according to the current internal tip index. If out of bounds, the index is corrected. */
    Tip getTip() const noexcept;

    /**
     * @return a Tip includes the text possibly including CommandIds, and with absolute image/video paths.
     * @param rawTip the raw tip, with the raw text and local file name(s).
     */
    Tip buildTip(const Tip& rawTip) const noexcept;

    /** @return the full path from the local path. If empty, returns empty. In Release, the path is different. */
    static juce::String getFullPath(const juce::String& localPath) noexcept;

    juce::ApplicationCommandManager& applicationCommandManager;
    mutable int lastTipIndex;           // May be out of bounds!
};

}   // namespace arkostracker
