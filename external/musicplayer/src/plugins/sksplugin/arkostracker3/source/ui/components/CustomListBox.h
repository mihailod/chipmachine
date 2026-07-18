#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace arkostracker 
{

/** A ListBox with some modifications. */
class CustomListBox final : public juce::ListBox
{
public:
    /**
     * Constructor.
     * @param model the possible Model.
     * @param allowSelectAll true to allow CTRL+A.
     */
    explicit CustomListBox(juce::ListBoxModel* model = nullptr, bool allowSelectAll = true) noexcept;

    bool keyPressed(const juce::KeyPress& keyPress) override;

private:
    bool allowSelectAll;                    // True to all CTRL+A.
};

}   // namespace arkostracker
