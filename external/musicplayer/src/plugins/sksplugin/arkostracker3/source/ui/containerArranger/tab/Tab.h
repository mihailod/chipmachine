#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../PanelType.h"

namespace arkostracker 
{

/** Abstract class for a Tab. */
class Tab : public juce::Component
{
public:
    /** To be aware of events from the Tab. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the tab is clicked. The Tab itself does nothing.
         * @param id the ID of this item.
         */
        virtual void onTabClicked(int id) = 0;
    };

    /** Constructor. */
    Tab() noexcept;

    /** @return the ID of this item. */
    int getArrangerId() const noexcept;

    /** @return the Panel type of this tab. */
    virtual PanelType getPanelType() const noexcept = 0;

    /**
     * Sets the tab as selected or not, updates the UI.
     * @param selected true if selected.
     */
    void setSelected(bool selected) noexcept;

    /** @return true if selected. */
    bool isSelected() const noexcept;

private:
    const int tabId;                            // The ID of this tab.
    bool selected;                              // True if the tab is selected (highlighted).
};

}   // namespace arkostracker
