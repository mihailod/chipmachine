#pragma once

#include <vector>

#include "LineContainerArrangement.h"

namespace arkostracker 
{

/** Represents a document. */
class DocumentArrangement
{
public:
    /** Constructor. */
    DocumentArrangement() noexcept;

    /** Equality operator. */
    friend bool operator==(const DocumentArrangement& left, const DocumentArrangement& right);

    /**
     * Adds a lineContainerArrangement.
     * @param lineContainerArrangement the line container arrangement to add.
     */
    void addLineContainerArrangement(const LineContainerArrangement& lineContainerArrangement) noexcept;

    /** @return the LineContainerArrangements. */
    const std::vector<LineContainerArrangement>& getLineContainerArrangements() const noexcept;

    /**
     * Sets what panel types are selected.
     * @param focusedPanelType the panel to focus on.
     * @param panelTypes the selected panel types.
     */
    void setSelectedPanels(PanelType focusedPanelType, std::set<PanelType> panelTypes) noexcept;

    /** @return the panel types that are selected. */
    std::set<PanelType> getSelectedPanels() const noexcept;

    /** @return the focused panel. */
    PanelType getFocusedPanel() const noexcept;

private:
    std::vector<LineContainerArrangement> lineContainerArrangements;
    std::set<PanelType> selectedPanelTypes;
    PanelType focusedPanelType;
};

}   // namespace arkostracker

