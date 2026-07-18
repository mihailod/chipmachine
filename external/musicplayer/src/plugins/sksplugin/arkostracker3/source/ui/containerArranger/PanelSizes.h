#pragma once

namespace arkostracker 
{

/** Holds some constants about the size of the Panels, because shared between the Panels and the Layouts. */
class PanelSizes
{
public:
    static const int linkerDesiredHeight = 140;
    static const int testAreaDesiredHeight = 70;

    static const int headerHeight = 28;

    static const int genericListMinimumWidth = 220;     // Else, left/right icons may overlap.
    static const int genericListDesiredWidth = 260;
    static const int genericListMaximumWidth = genericListDesiredWidth;
};

}   // namespace arkostracker
