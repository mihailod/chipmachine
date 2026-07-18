#pragma once

#include <vector>

#include "../PanelType.h"

namespace arkostracker 
{

/** Represents the data of one Container. */
class ContainerArrangement
{
public:
    /**
     * Constructor.
     * @param desiredWidth how large the Container is.
     * @param hiddenHeader true if the header is hidden (only of there is one tab).
     * @param panelTypes the panels to display.
     * @param sidePanelTypes the possible side-panels to display (not linked to this Container).
     * @param focusOrder a possible order (>=0) (0, 1, 2...) to focus from one panel to another (when using Tab). MUST be contiguous!
     */
    ContainerArrangement(Dimension desiredWidth, bool hiddenHeader, std::vector<PanelType> panelTypes, std::vector<PanelType> sidePanelTypes = { },
        OptionalInt focusOrder = OptionalInt()) noexcept;

    /** Equality operator. */
    friend bool operator==(const ContainerArrangement& left, const ContainerArrangement& right);

    /** @return the desired width of the Container. */
    Dimension getDesiredWidth() const noexcept;

    /** @return true if the header is hidden. */
    bool isHiddenHeader() const noexcept;

    /** @return the panels to display. */
    const std::vector<PanelType>& getPanelTypes() const noexcept;

    /** @return the possible side-panels to display. */
    const std::vector<PanelType>& getSidePanelTypes() const noexcept;

    /** @return the possible focus order. */
    OptionalInt getFocusOrder() const noexcept;

private:
    Dimension desiredWidth;                     // How large the Container is.
    bool hiddenHeader;                          // True if the header is hidden (only of there is one tab).
    std::vector<PanelType> panelTypes;          // The panels to display.
    std::vector<PanelType> sidePanelTypes;      // The possible side-panels to display (not linked to this Container).
    OptionalInt focusOrder;                     // A possible order (0, 1, 2...) to focus from one panel to another (when using Tab).
};

}   // namespace arkostracker
