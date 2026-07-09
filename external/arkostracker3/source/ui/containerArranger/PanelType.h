#pragma once

#include "Dimension.h"
#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** The types of panels that can be shown. */
enum class PanelType
{
    meters,
    linker,
    patternViewer,
    instrumentEditor,
    instrumentList,
    arpeggioTable,
    arpeggioList,
    pitchTable,
    pitchList,
    testArea,

    last = testArea,

    /** Used for init, when nothing is added yet. */
    dummy,          // At the end because not accounted for "instantiable".
};

class PanelTypeHelper
{
public:
    /** Prevents instantiation. */
    PanelTypeHelper() = delete;

    /**
     * Converts a PanelType to a displayable String.
     * @param panelType the PanelType.
     * @return the String.
     */
    static juce::String panelTypeToDisplayableString(PanelType panelType) noexcept;

    /**
     * @return the width of the Panel which type is given.
     * @param panelType the type of the Panel.
     */
    static Dimension getPanelWidth(PanelType panelType) noexcept;
};

}   // namespace arkostracker

