#pragma once

#include "PanelType.h"

namespace arkostracker 
{

/** Abstract class for objects that can search and open panels. */
class PanelSearcher
{
public:
    /** Destructor. */
    virtual ~PanelSearcher() = default;

    /**
     * Searches a panel in the current layout, and opens it if needed. It is focused in all cases.
     * @param panelType the panel type.
     * @return true if found or already opened. False if not found.
     */
    virtual bool searchAndOpenPanel(PanelType panelType) noexcept = 0;

    /**
     * Searches a panel in the current layout.
     * @param panelType the panel type.
     * @return true if found.
     */
    virtual bool isPanelVisible(PanelType panelType) const noexcept = 0;
};

}   // namespace arkostracker
