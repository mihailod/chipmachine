#include "LookAndFeelConstants.h"

namespace arkostracker 
{

const int LookAndFeelConstants::margins = 10;
const int LookAndFeelConstants::smallMargins = 4;
const int LookAndFeelConstants::buttonSeparator = 4;
const int LookAndFeelConstants::buttonsHeight = 25;
const int LookAndFeelConstants::iconButtonsWidth = 25;
const int LookAndFeelConstants::labelsHeight = buttonsHeight;
const int LookAndFeelConstants::comboBoxesHeight = buttonsHeight;
const int LookAndFeelConstants::borderThickness = 2;

const int LookAndFeelConstants::groupMarginsX = 10;
const int LookAndFeelConstants::groupMarginsY = 17;

const int LookAndFeelConstants::labelFontSize = 17;
const int LookAndFeelConstants::buttonTextSize = labelFontSize + 1;
const int LookAndFeelConstants::titleBarTextSize = labelFontSize + 1;
const int LookAndFeelConstants::menuBarFontSize = labelFontSize;
const int LookAndFeelConstants::popupMenuItemsAndComboBoxFontSize = labelFontSize + 1;              // Both for menu pop up items and ComboBox items!
const int LookAndFeelConstants::listItemFontSize = labelFontSize;
const int LookAndFeelConstants::tooltipFontSize = labelFontSize - 2;

const int LookAndFeelConstants::listItemHeights = 30;                           // Generic item height, in lists.

const int LookAndFeelConstants::buttonImagesWidth = 30;

const juce::uint32 LookAndFeelConstants::defaultPatternColor = 0xff808080;
const juce::uint32 LookAndFeelConstants::defaultMarkerColor = 0xff404080;

const juce::String LookAndFeelConstants::typefaceNameSquare = "square";                          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String LookAndFeelConstants::typefaceMonospace = "Default monospaced font";          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String LookAndFeelConstants::labelPropertyFontSize = "fontSize";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String LookAndFeelConstants::labelPropertyUseCustomFont = "customFont";              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

}   // namespace arkostracker
