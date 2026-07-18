#pragma once

#include "LookAndFeelConstants.h"

namespace arkostracker 
{

class ThemeColor;
enum class ColorCategory : juce::uint8;

/** Groups the colors from the themes. */
class ThemeColors
{
public:
    /** @return the unique instance of the Manager. */
    static ThemeColors& getInstance();
    /** @return the map linking a Color ID to its Theme Color. */
    const std::map<int, ThemeColor>& getColorIdToThemeColor() const noexcept;

private:
    /** Constructor. */
    ThemeColors() noexcept;
    /** Destructor. */
    ~ThemeColors() = default;

    /**
     * Adds a color to the map.
     * @param colorId the color ID.
     * @param colorCategory where the color belongs to.
     * @param displayedName the name of the color, for display.
     * @param allowTransparency false to prevent an alpha layer. True for an alpha < 255.
     */
    void addColor(int colorId, ColorCategory colorCategory, juce::String displayedName, bool allowTransparency = false) noexcept;

    /**
     * Adds a color to the map.
     * @param colorId the color ID.
     * @param colorCategory where the color belongs to.
     * @param displayedName the name of the color, for display.
     * @param allowTransparency false to prevent an alpha layer. True for an alpha < 255.
     */
    void addColor(LookAndFeelConstants::Colors colorId, ColorCategory colorCategory, juce::String displayedName, bool allowTransparency = false) noexcept;

    std::map<int, ThemeColor> colorIdToThemeColor;       // Links a ColorId to its Theme Color.
};


// =======================================

/** The category to put a color in. */
enum class ColorCategory : juce::uint8
{
    general,
    panels,
    toolbars,
    buttons,
    meters,
    lists,
    linker,
    editorWithBars,
    patternViewer,

    count
};

// ======================================

/** Represents a color for the theme. */
class ThemeColor
{
public:
    /**
     * Constructor.
     * @param allowTransparency false to prevent an alpha layer. True for an alpha < 255.
     * @param colorId the color ID from the themes.
     * @param category where belongs the color.
     * @param displayedName the name of the color, for display.
     */
    ThemeColor(bool allowTransparency, int colorId, ColorCategory category, juce::String displayedName);

    const bool allowTransparency;                       // False to prevent an alpha layer. True for an alpha < 255.
    const int colorId;                                  // The color ID from the themes.
    const ColorCategory category;                       // Where belongs the color.
    const juce::String displayedName;                   // The name of the color, for display.
};

}   // namespace arkostracker
