#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "LookAndFeelConstants.h"

namespace arkostracker
{

class PreferencesManager;

/**
 * The current look and feel of AT3! Note that if selecting a new one, the custom look-and-feel takes its values.
 * It is the not "the" AT3 look, but the current one, modified or not.
 */
class CustomLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    /**
     * Constructor.
     * @param themeId the ID of the theme to apply. If <=0 or not found, the one from the preferences is used.
     */
    explicit CustomLookAndFeel(int themeId = 0) noexcept;

    /**
     * Sets the colors of all the (basic) backgrounds from our own background and outline Color. This is useful because we consider them linked, but JUCE
     * does not (they are under different ColorId, this would be too bothersome to allow to edit them all).
     * Warning, the dialogBackground must have been set before.
     * @param lookAndFeel the lookAndFeel to modify.
     */
    static void setAllBackgroundAndOutlineColors(juce::LookAndFeel& lookAndFeel) noexcept;

    /**
     * Sets the colors of all the (basic) texts from our own text Color. This is useful because we consider them linked, but JUCE
     * does not (they are under different ColorId, this would be too bothersome to allow to edit them all).
     * Warning, the JUCE Label text color must have been set before.
     * @param lookAndFeel the lookAndFeel to modify.
     */
    static void setAllTextColors(juce::LookAndFeel& lookAndFeel) noexcept;

    /** @return the base font. */
    const juce::Font& getBaseFont() const noexcept;

    /**
     * @return a color from its ID, possibly contrasted.
     * @param colorId the color id to find.
     * @param contrasted true if contrasted (opposite. But not fully).
     */
    juce::Colour getColor(int colorId, bool contrasted) const noexcept;

    // LookAndFeel method implementations
    // ==========================================================
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;
    // Texts (labels).
    juce::Font getLabelFont(juce::Label& label) override;

    // Buttons.
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override;
    juce::Font getTextButtonFont(juce::TextButton& textButton, int buttonHeight) override;
    // Resizable Window.
    void drawResizableFrame(juce::Graphics& g, int w, int h, const juce::BorderSize<int>& borderSize) override;
    void drawResizableWindowBorder(juce::Graphics& graphics, int w, int h, const juce::BorderSize<int>& border, juce::ResizableWindow& window) override;

    // Title bar.
    void drawDocumentWindowTitleBar(juce::DocumentWindow& window, juce::Graphics& g, int w, int h, int titleSpaceX, int titleSpaceW, const juce::Image* icon,
                                    bool drawTitleTextOnLeft) override;
    juce::Button* createDocumentWindowButton(int buttonType) override;
    // Menubar.
    void drawMenuBarBackground(juce::Graphics& g, int width, int height, bool isMouseOverBar, juce::MenuBarComponent& component) override;
    juce::Font getMenuBarFont(juce::MenuBarComponent& component, int itemIndex, const juce::String& itemText) override;
    void drawMenuBarItem(juce::Graphics& graphics, int width, int height, int itemIndex, const juce::String& itemText, bool isMouseOverItem, bool isMenuOpen,
                         bool isMouseOverBar, juce::MenuBarComponent& component) override;

    // Popup menu.
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    // Treeview.
    void drawTreeviewPlusMinusBox(juce::Graphics& graphics, const juce::Rectangle<float>& area, juce::Colour backgroundColour, bool isOpen, bool isMouseOver) override;
    // Scrollbar.
    int getDefaultScrollbarWidth() override;
    void drawScrollbar(juce::Graphics& graphics, juce::ScrollBar& bar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                       bool isMouseOver, bool isMouseDown) override;

    // ToggleButton.
    void drawToggleButton(juce::Graphics& graphics, juce::ToggleButton& button, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // Group Component.
    void drawGroupComponentOutline(juce::Graphics& graphics, int w, int h, const juce::String& text, const juce::Justification& justification,
                                   juce::GroupComponent& component) override;

    // Slider.
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Combobox.
    void drawComboBox(juce::Graphics& graphics, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) override;
    juce::Font getComboBoxFont(juce::ComboBox& box) override;
    void positionComboBoxText(juce::ComboBox& box, juce::Label& label) override;
    juce::Path getTickShape(float height) override;

    // Text Editor.
    void drawTextEditorOutline(juce::Graphics& graphics, int width, int height, juce::TextEditor& editor) override;

    // Tooltip.
    juce::Rectangle<int> getTooltipBounds(const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;
    void drawTooltip(juce::Graphics& g, const juce::String& text, int width, int height) override;

private:
    static const float sliderHeight;
    static const float outlineContrasting;
    static const float focusedOutlineContrasting;

    void fillHorizontalWithTwoLevels(juce::Graphics& g, int width, int height, bool isClicked, bool isMouseOver, bool isToggled) const noexcept;

    static void addPolygonToPath(juce::Graphics& g, const juce::Colour& colour, float x, float y) noexcept;

    static juce::TextLayout layoutTooltipText(const juce::String& text, const juce::Font& font, juce::Colour colour) noexcept;

    PreferencesManager& preferencesManager;                   // The Preferences, where the colors are stored.

    juce::Font labelFont;
    juce::Typeface::Ptr customTypeface;                       // The custom Typeface to use.
    //juce::Typeface::Ptr squareFontTypeface;
    juce::Font buttonFont;
    juce::Font menuBarFont;
    juce::Font titleBarFont;
    juce::Font popupMenuItemsAndComboBoxFont;                 // Both for menu pop up items and ComboBox items!
    juce::Font toolTipFont;

    juce::Image titleBarNull;                                 // A "null" image.
    juce::Image titleBarClose;                                // The "close" image for the title bar.
    juce::Image titleBarMinimize;                             // The "minimize" image for the title bar.
    juce::Image titleBarMaximize;                             // The "maximize" image for the title bar.
};

}   // namespace arkostracker
