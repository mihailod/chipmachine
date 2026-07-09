#pragma once



namespace arkostracker 
{

/**
 * Marker class. Inherit from this if your class changes the mouse. Useful when, after a drag'n'drop, you want the cursor to go back to its normal
 * form. JUCE does not handle this very easily... So the DragAndDropContainer has to notify the Component once the dnd is finished.
 */
class MouseChanger
{
};

}   // namespace arkostracker

