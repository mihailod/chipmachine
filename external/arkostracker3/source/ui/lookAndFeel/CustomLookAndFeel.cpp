#include "CustomLookAndFeel.h"

#include <BinaryData.h>

#include "../../app/preferences/PreferencesManager.h"
#include "LookAndFeelConstants.h"
#include "LookAndFeelFactory.h"

namespace arkostracker 
{

const float CustomLookAndFeel::sliderHeight = 7.0F;

const float CustomLookAndFeel::outlineContrasting = 0.3F;
const float CustomLookAndFeel::focusedOutlineContrasting = 0.4F;

CustomLookAndFeel::CustomLookAndFeel(int themeId) noexcept :
        preferencesManager(PreferencesManager::getInstance()),
        labelFont(juce::Typeface::createSystemTypefaceFor(BinaryData::LTWave_ttf, BinaryData::LTWave_ttfSize)),
        customTypeface(labelFont.getTypefacePtr()),
        buttonFont(labelFont),
        menuBarFont(labelFont),
        titleBarFont(labelFont),
        popupMenuItemsAndComboBoxFont(labelFont),
        toolTipFont(labelFont),
        titleBarNull(),
        titleBarClose(juce::ImageFileFormat::loadFrom(BinaryData::IconTitlebarClose_png, BinaryData::IconTitlebarClose_pngSize)),
        titleBarMinimize(juce::ImageFileFormat::loadFrom(BinaryData::IconTitlebarMinimize_png, BinaryData::IconTitlebarMinimize_pngSize)),
        titleBarMaximize(juce::ImageFileFormat::loadFrom(BinaryData::IconTitlebarMaximize_png, BinaryData::IconTitlebarMaximize_pngSize))
{
    juce::Font::setDefaultMinimumHorizontalScaleFactor(1.0F);       // By default, don't compress the text horizontally.

    labelFont.setHeight(static_cast<float>(LookAndFeelConstants::labelFontSize));
    buttonFont.setHeight(static_cast<float>(LookAndFeelConstants::buttonTextSize));
    menuBarFont.setHeight(static_cast<float>(LookAndFeelConstants::menuBarFontSize));
    titleBarFont.setHeight(static_cast<float>(LookAndFeelConstants::titleBarTextSize));
    toolTipFont.setHeight(static_cast<float>(LookAndFeelConstants::tooltipFontSize));
    // The combobox items size are capped by box drawing.
    popupMenuItemsAndComboBoxFont.setHeight(static_cast<float>(LookAndFeelConstants::popupMenuItemsAndComboBoxFontSize));

    // Any theme requested? If not, gets the latest from the Preferences.
    if (themeId <= 0) {
        themeId = preferencesManager.getCurrentThemeId();
    }
    const auto lookAndFeelToUse = LookAndFeelFactory::retrieveLookAndFeel(themeId);
    jassert(lookAndFeelToUse != nullptr);       // Should never happen!

    // Sets the colors from the read Look and Feel.
    // ********** WARNING ************* If adding a value, don't forget to modify:
    // - the setColour below.
    // - the StoredLookAndFeel.
    // - Xml Serializer.
    // ********************************
    // The other text colors are set at the bottom.
    const auto& generalColors = lookAndFeelToUse->generalColors;
    const auto& toolbarColors = lookAndFeelToUse->toolbarColors;
    const auto& buttonColors = lookAndFeelToUse->buttonColors;
    const auto& panelColors = lookAndFeelToUse->panelColors;
    setColour(juce::Label::ColourIds::textColourId, juce::Colour(generalColors.textArgb));
    // Popup colors.
    setColour(juce::PopupMenu::ColourIds::textColourId, juce::Colour(toolbarColors.popupMenuTextArgb));
    setColour(juce::PopupMenu::ColourIds::headerTextColourId, juce::Colour(toolbarColors.popupMenuTextArgb));
    setColour(juce::PopupMenu::ColourIds::highlightedTextColourId, juce::Colour(toolbarColors.popupMenuTextHighlightArgb));
    setColour(juce::PopupMenu::ColourIds::backgroundColourId, juce::Colour(toolbarColors.popupMenuBackgroundArgb));
    setColour(juce::PopupMenu::ColourIds::highlightedBackgroundColourId, juce::Colour(toolbarColors.popupMenuBackgroundHighlightArgb));
    // Buttons text color.
    setColour(juce::TextButton::ColourIds::textColourOnId, juce::Colour(buttonColors.buttonTextArgb));
    setColour(juce::TextButton::ColourIds::textColourOffId, juce::Colour(buttonColors.buttonTextOffArgb));
    // ScrollBar.
    setColour(juce::ScrollBar::ColourIds::thumbColourId, juce::Colour(generalColors.scrollBarArgb));
    // Tooltip.
    setColour(juce::TooltipWindow::ColourIds::backgroundColourId, juce::Colour(generalColors.toolTipBackgroundArgb));
    setColour(juce::TooltipWindow::ColourIds::textColourId, juce::Colour(generalColors.toolTipTextArgb));

    // Some debug code to make sure no colors were forgotten. Only our custom colors can be counted.
    static_assert(static_cast<int>(LookAndFeelConstants::Colors::count) == 97, "Forgot to add the nodes below!");

    setColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground), juce::Colour(generalColors.dialogBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBorder), juce::Colour(generalColors.dialogBorderArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::interspace), juce::Colour(generalColors.interspace));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::titleBarBackground), juce::Colour(generalColors.titleBarBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::loopStartEnd), juce::Colour(generalColors.loopStartEndArgb));

    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundFocused), juce::Colour(panelColors.panelBackgroundFocusedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundUnfocused), juce::Colour(panelColors.panelBackgroundUnfocusedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelBorderFocused), juce::Colour(panelColors.panelBorderFocusedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelBorderUnfocused), juce::Colour(panelColors.panelBorderUnfocusedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelTabIcon), juce::Colour(panelColors.panelTabIconArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelTabBackgroundUnselected), juce::Colour(panelColors.panelTabBackgroundUnselectedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelTabBackgroundSelected), juce::Colour(panelColors.panelTabBackgroundSelectedArgb));

    setColour(static_cast<int>(LookAndFeelConstants::Colors::panelHandleMouseOver), juce::Colour(panelColors.panelHandleMouseOverArgb));

    const auto& meterColors = lookAndFeelToUse->meterColors;
    setColour(static_cast<int>(LookAndFeelConstants::Colors::metersBackground), juce::Colour(meterColors.metersBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::metersScaleLines), juce::Colour(meterColors.metersScaleLinesArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::metersLabelBackground), juce::Colour(meterColors.metersLabelBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::metersLabel), juce::Colour(meterColors.metersLabelArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::metersShape), juce::Colour(meterColors.metersShapeArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::metersMuteFilter), juce::Colour(meterColors.metersMuteFilterArgb));

    setColour(static_cast<int>(LookAndFeelConstants::Colors::buttonBorder), juce::Colour(buttonColors.buttonBorderArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::button), juce::Colour(buttonColors.buttonArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::buttonClicked), juce::Colour(buttonColors.buttonTopClickedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::buttonMouseOver), juce::Colour(buttonColors.buttonTopMouseOverArgb));

    setColour(static_cast<int>(LookAndFeelConstants::Colors::menuBar), juce::Colour(toolbarColors.menuBarArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::menuBarText), juce::Colour(toolbarColors.menuBarTextArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::menuBarTextHighlight), juce::Colour(toolbarColors.menuBarTextHighlightArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::menuBarTextBackgroundHighlight), juce::Colour(toolbarColors.menuBarTextBackgroundHighlightArgb));

    const auto& listColors = lookAndFeelToUse->listColors;
    setColour(static_cast<int>(LookAndFeelConstants::Colors::arpeggioListItemBackgroundEven), juce::Colour(listColors.arpeggioListItemBackgroundEvenArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::arpeggioListItemBackgroundOdd), juce::Colour(listColors.arpeggioListItemBackgroundOddArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::arpeggioListItemBackgroundSelected), juce::Colour(listColors.arpeggioListItemBackgroundSelectedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::pitchListItemBackgroundEven), juce::Colour(listColors.pitchListItemBackgroundEvenArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::pitchListItemBackgroundOdd), juce::Colour(listColors.pitchListItemBackgroundOddArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::pitchListItemBackgroundSelected), juce::Colour(listColors.pitchListItemBackgroundSelectedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::instrumentListItemBackgroundEven), juce::Colour(listColors.instrumentListItemBackgroundEvenArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::instrumentListItemBackgroundOdd), juce::Colour(listColors.instrumentListItemBackgroundOddArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::instrumentListItemBackgroundSelected), juce::Colour(listColors.instrumentListItemBackgroundSelectedArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::listItemText), juce::Colour(listColors.listItemTextArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::listItemTextSelected), juce::Colour(listColors.listItemTextSelectedArgb));

    const auto& linkerColors = lookAndFeelToUse->linkerColors;
    setColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPlayCursorTimeline), juce::Colour(linkerColors.linkerPlayCursorTimelineArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionWithinSongRangeBackground), juce::Colour(linkerColors.linkerPositionWithinSongRangeBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionOutsideSongRangeBackground), juce::Colour(linkerColors.linkerPositionOutsideSongRangeBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionBorder), juce::Colour(linkerColors.linkerPositionBorderArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::linkerPositionCurrentlyPlayedBorder), juce::Colour(linkerColors.linkerPositionCurrentlyPlayedBorderArgb));

    const auto& patternViewerColors = lookAndFeelToUse->patternViewerColors;
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerBackground), juce::Colour(patternViewerColors.patternViewerBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerNoNote), juce::Colour(patternViewerColors.patternViewerNoNoteArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerError), juce::Colour(patternViewerColors.patternViewerErrorArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerNoEffect), juce::Colour(patternViewerColors.patternViewerNoEffectArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerArpeggioEffect), juce::Colour(patternViewerColors.patternViewerArpeggioEffectArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerPitchEffect), juce::Colour(patternViewerColors.patternViewerPitchEffectArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerVolumeEffect), juce::Colour(patternViewerColors.patternViewerVolumeEffectArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerResetEffect), juce::Colour(patternViewerColors.patternViewerResetEffectArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerInstrumentSpeedEffect), juce::Colour(patternViewerColors.patternViewerInstrumentSpeedEffectArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorTextHighlight), juce::Colour(patternViewerColors.patternViewerCursorTextHighlightArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorBackground), juce::Colour(patternViewerColors.patternViewerCursorBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerSelection), juce::Colour(patternViewerColors.patternViewerSelectionArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorLineBackground), juce::Colour(patternViewerColors.patternViewerCursorLineBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerCursorLineHighlightText), juce::Colour(patternViewerColors.patternViewerCursorLineHighlightTextArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundPrimary), juce::Colour(patternViewerColors.patternViewerHighlightedLineBackgroundPrimaryArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightedLineBackgroundSecondary), juce::Colour(patternViewerColors.patternViewerHighlightedLineBackgroundSecondaryArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightTextPrimary), juce::Colour(patternViewerColors.patternViewerHighlightTextPrimaryArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerHighlightTextSecondary), juce::Colour(patternViewerColors.patternViewerHighlightTextSecondaryArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerTrackBorder), juce::Colour(patternViewerColors.patternViewerTrackBorderArgb));

    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOn), juce::Colour(patternViewerColors.patternViewerTrackHeaderBackgroundOnArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundOff), juce::Colour(patternViewerColors.patternViewerTrackHeaderBackgroundOffArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOn), juce::Colour(patternViewerColors.patternViewerTrackHeaderBackgroundChannelOnArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerTrackHeaderBackgroundChannelOff), juce::Colour(patternViewerColors.patternViewerTrackHeaderBackgroundChannelOffArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerLineTrackHeaderBackground), juce::Colour(patternViewerColors.patternViewerLineTrackHeaderBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerLineTrackText), juce::Colour(patternViewerColors.patternViewerLineTrackTextArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerSpeedTrackHeaderBackground), juce::Colour(patternViewerColors.patternViewerSpeedTrackHeaderBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerSpeedTrackText), juce::Colour(patternViewerColors.patternViewerSpeedTrackTextArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerEventTrackHeaderBackground), juce::Colour(patternViewerColors.patternViewerEventTrackHeaderBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerEventTrackText), juce::Colour(patternViewerColors.patternViewerEventTrackTextArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerPlayedLineBackground), juce::Colour(patternViewerColors.patternViewerPlayedLineBackgroundArgb));

    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerMetersSoftwareBar), juce::Colour(patternViewerColors.patternViewerMetersSoftwareBarArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerMetersHardwareBar), juce::Colour(patternViewerColors.patternViewerMetersHardwareBarArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::patternViewerNoiseBar), juce::Colour(patternViewerColors.patternViewerMetersNoiseBarArgb));

    const auto& editorWithBarsColors = lookAndFeelToUse->editorWithBarsColors;
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barSoundTypeBackground), juce::Colour(editorWithBarsColors.barSoundTypeBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barSoundTypeNoSoftNoHard), juce::Colour(editorWithBarsColors.barSoundTypeNoSoftNoHardArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barSoundTypeSoft), juce::Colour(editorWithBarsColors.barSoundTypeSoftArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barSoundTypeHard), juce::Colour(editorWithBarsColors.barSoundTypeHardArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barSoundTypeSoftToHard), juce::Colour(editorWithBarsColors.barSoundTypeSoftToHardArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barSoundTypeHardToSoft), juce::Colour(editorWithBarsColors.barSoundTypeHardToSoftArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barSoundTypeSoftAndHard), juce::Colour(editorWithBarsColors.barSoundTypeSoftAndHardArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barEnvelopeBackground), juce::Colour(editorWithBarsColors.barEnvelopeBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barEnvelopeSoft), juce::Colour(editorWithBarsColors.barEnvelopeSoftArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barEnvelopeHard), juce::Colour(editorWithBarsColors.barEnvelopeHardArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barNoiseBackground), juce::Colour(editorWithBarsColors.barNoiseBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barNoise), juce::Colour(editorWithBarsColors.barNoiseArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barPeriodBackground), juce::Colour(editorWithBarsColors.barPeriodBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barPeriod), juce::Colour(editorWithBarsColors.barPeriodArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barArpeggioNoteBackground), juce::Colour(editorWithBarsColors.barArpeggioNoteBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barArpeggioNote), juce::Colour(editorWithBarsColors.barArpeggioNoteArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barArpeggioOctaveBackground), juce::Colour(editorWithBarsColors.barArpeggioOctaveBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barArpeggioOctave), juce::Colour(editorWithBarsColors.barArpeggioOctaveArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barPitchBackground), juce::Colour(editorWithBarsColors.barPitchBackgroundArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::barPitch), juce::Colour(editorWithBarsColors.barPitchArgb));
    setColour(static_cast<int>(LookAndFeelConstants::Colors::autoSpreadLoop), juce::Colour(editorWithBarsColors.autoSpreadLoopArgb));

    setAllBackgroundAndOutlineColors(*this);
    setAllTextColors(*this);
}

const juce::Font& CustomLookAndFeel::getBaseFont() const noexcept
{
    return labelFont;
}

void CustomLookAndFeel::setAllBackgroundAndOutlineColors(LookAndFeel& lookAndFeel) noexcept
{
    const auto backgroundColor = lookAndFeel.findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground));
    const auto outlineColor = backgroundColor.contrasting(outlineContrasting);
    const auto focusedOutlineColor = backgroundColor.contrasting(focusedOutlineContrasting);

    lookAndFeel.setColour(juce::ResizableWindow::ColourIds::backgroundColourId, backgroundColor);
    lookAndFeel.setColour(juce::ListBox::ColourIds::backgroundColourId, backgroundColor);
    lookAndFeel.setColour(juce::KeyMappingEditorComponent::ColourIds::backgroundColourId, backgroundColor);
    lookAndFeel.setColour(juce::ComboBox::ColourIds::backgroundColourId, backgroundColor);
    lookAndFeel.setColour(juce::TextEditor::ColourIds::backgroundColourId, backgroundColor);

    lookAndFeel.setColour(juce::Slider::ColourIds::textBoxBackgroundColourId, backgroundColor);
    lookAndFeel.setColour(juce::Slider::ColourIds::textBoxOutlineColourId, outlineColor);

    // Bubble. The text color is set on use (UiUtil).
    lookAndFeel.setColour(juce::BubbleComponent::ColourIds::backgroundColourId, backgroundColor);
    lookAndFeel.setColour(juce::BubbleComponent::ColourIds::outlineColourId, outlineColor);

    lookAndFeel.setColour(juce::TextEditor::ColourIds::outlineColourId, outlineColor);
    lookAndFeel.setColour(juce::TextEditor::ColourIds::focusedOutlineColourId, focusedOutlineColor);
}

void CustomLookAndFeel::setAllTextColors(LookAndFeel& lookAndFeel) noexcept
{
    const auto textColor = lookAndFeel.findColour(juce::Label::ColourIds::textColourId);
    lookAndFeel.setColour(juce::KeyMappingEditorComponent::ColourIds::textColourId, textColor);
    lookAndFeel.setColour(juce::ComboBox::ColourIds::textColourId, textColor);
    lookAndFeel.setColour(juce::ComboBox::ColourIds::arrowColourId, textColor);     // Shown with the text when selected.
    lookAndFeel.setColour(juce::TextEditor::ColourIds::textColourId, textColor);
    lookAndFeel.setColour(juce::ToggleButton::ColourIds::textColourId, textColor);
    lookAndFeel.setColour(juce::ToggleButton::ColourIds::tickColourId, textColor);
    lookAndFeel.setColour(juce::ToggleButton::ColourIds::tickDisabledColourId, textColor.contrasting(focusedOutlineContrasting));
    lookAndFeel.setColour(juce::Slider::ColourIds::textBoxTextColourId, textColor);
}

juce::Colour CustomLookAndFeel::getColor(const int colorId, const bool contrasted) const noexcept
{
    const auto color = findColour(colorId);
    if (contrasted) {
        return color.contrasting(0.85F);
    }

    return color;
}


// LookAndFeel method implementations
// ==========================================================
juce::Typeface::Ptr CustomLookAndFeel::getTypefaceForFont(const juce::Font& /*font*/)
{
    // This seems to be called only from text drawn by paint!
    return customTypeface;
}

juce::Font CustomLookAndFeel::getTextButtonFont(juce::TextButton& /*textButton*/, int /*buttonHeight*/)
{
    return buttonFont;
}

void CustomLookAndFeel::drawResizableFrame(juce::Graphics& g, const int w, const int h, const juce::BorderSize<int>& border)
{
    if (border.isEmpty()) {
        return;
    }
    const juce::Rectangle fullSize(0, 0, w, h);

    const auto borderThickness = border.getLeft();
    //const auto resizeableBorderColor = findColour(static_cast<int>(LookAndFeelConstants::Colors::windowBorder));
    g.setColour(juce::Colours::black);      // Used by main window. Probably not useful to have a user-defined color for this.
    g.drawRect(fullSize, borderThickness);
}

void CustomLookAndFeel::drawResizableWindowBorder(juce::Graphics& g, const int w, const int h, const juce::BorderSize<int>& border, juce::ResizableWindow& /*window*/)
{
    if (border.isEmpty()) {
        return;
    }
    const juce::Rectangle fullSize(0, 0, w, h);

    // Color used by dialogs.
    g.setColour(findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBorder)));
    g.drawRect(fullSize, 1);
}

void CustomLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& /*backgroundColour*/, const bool isMouseOverButton, const bool isButtonDown)
{
    const auto width = button.getWidth();
    const auto height = button.getHeight();

    fillHorizontalWithTwoLevels(g, width, height, isButtonDown, isMouseOverButton, button.getToggleState());

    // Border.
    const auto buttonColorBorder = findColour(static_cast<int>(LookAndFeelConstants::Colors::buttonBorder));
    g.setColour(buttonColorBorder);
    g.drawRect(0, 0, width, height, 1);
}

void CustomLookAndFeel::fillHorizontalWithTwoLevels(juce::Graphics& g, const int width, const int height, const bool isClicked,
                                                    const bool isMouseOver, const bool isToggled) const noexcept
{
    auto color = findColour(static_cast<int>(LookAndFeelConstants::Colors::button));

    // Fills the inside.
    if (isClicked) {
        color = findColour(static_cast<int>(LookAndFeelConstants::Colors::buttonClicked));
    } else if (isMouseOver) {
        color = findColour(static_cast<int>(LookAndFeelConstants::Colors::buttonMouseOver));
    }

    // Toggled?
    if (isToggled) {
        constexpr auto contrastingRatio = 0.7F;
        color = color.contrasting(contrastingRatio);
    }

    g.setColour(color);
    g.fillRect(0, 0, width, height);
}


// ====================================================

void CustomLookAndFeel::drawDocumentWindowTitleBar(juce::DocumentWindow& documentWindow, juce::Graphics& g, const int w, const int h, int titleSpaceX,
    int /*titleSpaceW*/, const juce::Image* /*icon*/, bool /*drawTitleTextOnLeft*/)
{
    // Gets the colours. Darker if the window is inactive.
    auto backgroundColor(findColour(static_cast<int>(LookAndFeelConstants::Colors::titleBarBackground)));
    auto textColor(findColour(juce::Label::ColourIds::textColourId));
    if (!documentWindow.isActiveWindow()) {
        constexpr auto darkerRate = 0.5F;
        backgroundColor = backgroundColor.darker(darkerRate);
        textColor = textColor.darker(darkerRate);
    }

    g.fillAll(backgroundColor);

    // Displays the title.
    //const auto hf = (float)h;
    g.setColour(textColor);

    g.setFont(titleBarFont);
    const auto& title(documentWindow.getName());
    //const auto y = (int)(hf - (hf - fontHeight) / 2.0F) - 3;     // A little offset, more beautiful.
    constexpr auto y = 3;           // A little offset, more beautiful.

    // Draws the text. Why this X on Mac? No idea, I don't understand how these parameters work! But it looks ok this way.
#ifdef JUCE_MAC
    const auto textX = static_cast<int>(static_cast<float>(titleSpaceX) / 2.3F);
#else
    constexpr auto textX = 10;
    (void)titleSpaceX;      // To remove the warning about not using the parameter (used on Mac).
#endif

    const auto textWidth = static_cast<float>(w) - static_cast<float>(textX) - 100.0F;      // Removes the button to the right...
    g.drawText(title, textX, y, static_cast<int>(textWidth), h,
               juce::Justification::topLeft, true);
}

juce::Button* CustomLookAndFeel::createDocumentWindowButton(const int buttonType)
{
    juce::ImageButton* button = nullptr;

    // Creates a Button from an Image, according to the Button type.
    juce::String buttonText;
    const juce::Image* image = nullptr;

    switch (buttonType) {
        case juce::DocumentWindow::closeButton:
            buttonText = juce::translate("Close");
            image = &titleBarClose;
            break;
        case juce::DocumentWindow::minimiseButton:
            buttonText = juce::translate("Minimize");
            image = &titleBarMinimize;
            break;
        case juce::DocumentWindow::maximiseButton:            // TO IMPROVE? Can check if maximized already to display the "windowed" icon?
            buttonText = juce::translate("Maximize");
            image = &titleBarMaximize;
            break;
        default:
            jassertfalse;
            break;
    }

    // Creates the Button, if the image could be found (it should!).
    if (image != nullptr) {
        const auto color = findColour(juce::Label::ColourIds::textColourId);

        button = new juce::ImageButton(buttonText);     // Owned by JUCE.
        button->setImages(true, false, true,
            *image, 0.7F, color,
            titleBarNull, 1.0F, color,
            titleBarNull, 1.0F, color
        );
    }

    return button;
}


// Menubar.

void CustomLookAndFeel::drawMenuBarBackground(juce::Graphics& g, const int /*width*/, const int /*height*/, bool /*isMouseOverBar*/, juce::MenuBarComponent& /*component*/)
{
    const auto backgroundTopPrimaryColor(findColour(static_cast<int>(LookAndFeelConstants::Colors::menuBar)));
    g.fillAll(backgroundTopPrimaryColor);
}

juce::Font CustomLookAndFeel::getMenuBarFont(juce::MenuBarComponent& /*component*/, int /*itemIndex*/, const juce::String& /*itemText*/)
{
    return menuBarFont;
}

void CustomLookAndFeel::drawMenuBarItem(juce::Graphics& g, const int width, const int height, const int itemIndex, const juce::String& itemText, const bool isMouseOverItem, const bool isMenuOpen,
                                        bool /*isMouseOverBar*/, juce::MenuBarComponent& menuBar)
{
    // This draws the menu bar choices (not the drop-down).
    // Coming from LookAndFeel_V4::drawMenuBarItem, but color modified.
    if (!menuBar.isEnabled()) {
        g.setColour(menuBar.findColour(static_cast<int>(LookAndFeelConstants::Colors::menuBarText)).withMultipliedAlpha(0.5F));
    }
    else if (isMenuOpen || isMouseOverItem) {
        g.fillAll(menuBar.findColour(static_cast<int>(LookAndFeelConstants::Colors::menuBarTextBackgroundHighlight)));
        g.setColour(menuBar.findColour(static_cast<int>(LookAndFeelConstants::Colors::menuBarTextHighlight)));
    }
    else {
        g.setColour(menuBar.findColour(static_cast<int>(LookAndFeelConstants::Colors::menuBarText)));
    }

    g.setFont(getMenuBarFont(menuBar, itemIndex, itemText));
    g.drawFittedText(itemText, 0, 0, width, height, juce::Justification::centred, 1);
}

juce::Font CustomLookAndFeel::getLabelFont(juce::Label& label)
{
    // If a special property about the font size is set in the label, uses it. Else, it a default font, which has the effect of changing the font
    // of all the labels by default.
    const auto& properties = label.getProperties();
    if (properties.contains(LookAndFeelConstants::labelPropertyFontSize)) {
        const auto height = static_cast<float>(properties[LookAndFeelConstants::labelPropertyFontSize]);
        return labelFont.withHeight(height);
    }
    // Uses the label font if it has the "custom font" properties.
    if (properties.contains(LookAndFeelConstants::labelPropertyUseCustomFont)) {
        return label.getFont();
    }

    return labelFont;
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    return popupMenuItemsAndComboBoxFont;
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, const int width, const int height)
{
    // This draws the background in a menu, but also in an opened ComboBox.
    // Comes from the original JUCE code, but the rectangle color couldn't be customized, so we do here it.
    g.fillAll(findColour(juce::PopupMenu::backgroundColourId));

    g.setColour(findColour(static_cast<int>(LookAndFeelConstants::Colors::panelBorderFocused)));     // Was juce::PopupMenu::textColourId.
    g.drawRect(0, 0, width, height);
}

void CustomLookAndFeel::drawTreeviewPlusMinusBox(juce::Graphics& graphics, const juce::Rectangle<float>& area, juce::Colour /*backgroundColour*/, const bool isOpen, const bool isMouseOver)
{
    // Only changes the color. The called method has a "contrasting" color, so using the background is fine.
    const auto newBackgroundColour = findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground));
    LookAndFeel_V3::drawTreeviewPlusMinusBox(graphics, area, newBackgroundColour, isOpen, isMouseOver);
}

// Scrollbar.
int CustomLookAndFeel::getDefaultScrollbarWidth()
{
    return 12;
}

void CustomLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button, bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    // This code comes directly from JUCE's look'n'feel V4. The fontSize has been modified, and the minimum horizontal scale (which doesn't seem to work much).
    // Also, the button has now an alpha when disabled.

    const auto fontSize = static_cast<float>(LookAndFeelConstants::labelFontSize);          //jmin (15.0f, button.getHeight() * 0.75f);
    const auto tickWidth = fontSize * 1.1F;

    // The following comes from "drawTickBox", but the JUCE original method doesn't support "disable" colors.
    // -------------------------------------
    const juce::Rectangle tickBounds(
            4.0F, (static_cast<float>(button.getHeight()) - tickWidth) * 0.5F,
            tickWidth, tickWidth
    );

    const auto opacityRatio = button.isEnabled() ? 1.0F : 0.5F;

    g.setColour(button.findColour(juce::ToggleButton::tickDisabledColourId).withMultipliedAlpha(opacityRatio));
    g.drawRoundedRectangle(tickBounds, 4.0F, 1.0F);

    if (button.getToggleState()) {
        g.setColour(button.findColour(juce::ToggleButton::tickColourId).withMultipliedAlpha(opacityRatio));
        const auto tick = getTickShape(0.75F);
        g.fillPath(tick, tick.getTransformToScaleToFit(tickBounds.reduced(4.0F, 5.0F).toFloat(), false));
    }
    // -------------------------------------

    g.setColour(button.findColour(juce::ToggleButton::textColourId));
    g.setFont(fontSize);
    g.setOpacity(opacityRatio);

    g.drawFittedText(button.getButtonText(),
                     button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 10)
                             .withTrimmedRight(2),
                     juce::Justification::centredLeft, 10, 1.0F);
}

void CustomLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar, const int x, const int y, const int width, const int height,
                                      const bool isScrollbarVertical, const int thumbStartPosition, const int thumbSize, const bool isMouseOver, const bool isMouseDown)
{
    // This derives mostly from the parent JUCE code.
    const auto thumbBounds = isScrollbarVertical
            ? juce::Rectangle(x, thumbStartPosition, width, thumbSize)
            : juce::Rectangle(thumbStartPosition, y, thumbSize, height);

    const auto color = scrollbar.findColour(juce::ScrollBar::ColourIds::thumbColourId);
    g.setColour(isMouseOver ? color.brighter(isMouseDown ? 0.25F : 0.15F) : color);
    g.fillRoundedRectangle(thumbBounds.reduced(1).toFloat(), 4.0F);
}

void CustomLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, const int width, const int height, const juce::String& text, const juce::Justification& position,
                                                  juce::GroupComponent& group)
{
    // Mostly comes from JUCE, with tweaking to remove the corners.

    constexpr auto textH = 15.0F;
    constexpr auto indent = 3.0F;
    constexpr auto textEdgeGap = 4.0F;
    auto cs = 0.0F;

    const juce::Font f(textH);

    juce::Path p;
    constexpr auto x = indent;
    const auto y = f.getAscent() - 3.0F;
    const auto w = juce::jmax(0.0F, static_cast<float>(width) - (x * 2.0F));
    const auto h = juce::jmax(0.0F, static_cast<float>(height) - y - indent);
    cs = juce::jmin(cs, w * 0.5F, h * 0.5F);
    const auto cs2 = 2.0F * cs;

    const auto textW = text.isEmpty() ? 0.0F : juce::jlimit(0.0F, juce::jmax(0.0F, w - cs2 - (textEdgeGap * 2.0F)),
                                                            static_cast<float>(f.getStringWidth(text)) + (textEdgeGap * 2.0F));
    auto textX = cs + textEdgeGap;
    if (position.testFlags(juce::Justification::horizontallyCentred)) {
        textX = cs + (w - cs2 - textW) * 0.5F;
    } else if (position.testFlags(juce::Justification::right)) {
        textX = w - cs - textW - textEdgeGap;
    }
    p.startNewSubPath(x + textX + textW, y);
    p.lineTo(x + w - cs, y);
    p.addArc(x + w - cs2, y, cs2, cs2, 0.0F, juce::MathConstants<float>::halfPi);

    p.lineTo(x + w, y + h - cs);
    p.addArc(x + w - cs2, y + h - cs2, cs2, cs2, juce::MathConstants<float>::halfPi, juce::MathConstants<float>::pi);

    p.lineTo(x + cs, y + h);
    p.addArc(x, y + h - cs2, cs2, cs2, juce::MathConstants<float>::pi, juce::MathConstants<float>::pi * 1.5F);

    p.lineTo(x, y + cs);
    p.addArc(x, y, cs2, cs2, juce::MathConstants<float>::pi * 1.5F, juce::MathConstants<float>::twoPi);

    p.lineTo(x + textX, y);

    //auto alpha = group.isEnabled() ? 1.0F : 0.5F;

    // Was juce::GroupComponent::outlineColourId
    g.setColour(group.findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground)).contrasting(0.7F));

    g.strokePath(p, juce::PathStrokeType(1.0F));
    g.setColour(group.findColour(juce::Label::ColourIds::textColourId));      // Was juce::GroupComponent::textColourId
    g.setFont(f);
    g.drawText(text,
               juce::roundToInt(x + textX), 0,
               juce::roundToInt(textW),
               juce::roundToInt(textH),
               juce::Justification::centred, true);
}

// Slider.
void CustomLookAndFeel::drawLinearSlider(juce::Graphics& g, const int x, const int y, const int width, const int height, const float sliderPos, const float minSliderPos, const float maxSliderPos,
                                    const juce::Slider::SliderStyle style, juce::Slider& /*slider*/)
{
    jassert((style == juce::Slider::LinearHorizontal) || (style == juce::Slider::ThreeValueHorizontal));        // Only these types are well drawn, for now. NOLINT(*)

    const auto barHeight = height / 4;
    const auto middleBarHeight = barHeight / 2;     // Empirical, looks good.
    const auto barY = y + barHeight;

    const auto sliderColor = findColour(juce::Label::ColourIds::textColourId);
    const auto barBaseColor = findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground));
    const auto barLeftColor = barBaseColor.contrasting(0.125F);
    const auto barRightColor = barBaseColor.contrasting(0.2F);

    // Draws a horizontal bar, in two parts (before and after the cursor).
    g.setColour(barLeftColor);
    const auto leftPartWidth = static_cast<int>(sliderPos - static_cast<float>(x));
    g.fillRect(x, barY, leftPartWidth, barHeight);
    g.setColour(barRightColor);
    g.fillRect(x + leftPartWidth, barY, width - leftPartWidth, barHeight);

    // Draws the slider.
    {
        g.setColour(sliderColor);
        juce::Path path;
        path.addPolygon(juce::Point(sliderPos, static_cast<float>(barY) + static_cast<float>(middleBarHeight) + sliderHeight), 3,
                sliderHeight);
        g.fillPath(path);
    }

    // Draws the min/max sliders, if the Slider allows them.
    if (style == juce::Slider::SliderStyle::ThreeValueHorizontal) {
        const auto minMaxSlidersColor = sliderColor.contrasting(0.4F);

        const auto minMaxY = static_cast<float>(barY) - sliderHeight + static_cast<float>(middleBarHeight);
        addPolygonToPath(g, minMaxSlidersColor, minSliderPos, minMaxY);
        addPolygonToPath(g, minMaxSlidersColor, maxSliderPos, minMaxY);
    }
}

void CustomLookAndFeel::addPolygonToPath(juce::Graphics& g, const juce::Colour& colour, const float x, const float y) noexcept
{
    g.setColour(colour);
    juce::Path path;
    path.addPolygon(juce::Point(x, y), 3, sliderHeight, 1.0F);
    g.fillPath(path);
}


// Combobox.

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, const int width, const int height, const bool /*isButtonDown*/, const int buttonX,
    const int buttonY, const int buttonW, const int buttonH, juce::ComboBox& box)
{
    // Displays the outer box and the arrow of the selected item (not the dropdown items).
    // Uses most of the JUCE LookAndFeel_V3::drawComboBox code, but I wanted to customize the arrow, so...
    g.fillAll(box.findColour(juce::ComboBox::backgroundColourId));

    if (box.isEnabled() && box.hasKeyboardFocus(false)) {
        g.setColour(box.findColour(juce::TextEditor::ColourIds::focusedOutlineColourId));  // Don't use the Combobox color, share this one.
        g.drawRect(0, 0, width, height, 1);
    } else {
        g.setColour(box.findColour(juce::TextEditor::ColourIds::outlineColourId));         // Don't use the Combobox color, share this one.
        g.drawRect(0, 0, width, height);
    }

    constexpr auto arrowX = 0.2F;       // Was 0.3F.
    constexpr auto arrowH = 0.3F;       // Was 0.2F.

    const auto x = static_cast<float>(buttonX) - 2.0F;      // Small shift added.
    const auto y = static_cast<float>(buttonY);
    const auto w = static_cast<float>(buttonW);
    const auto h = static_cast<float>(buttonH);
    constexpr auto yRatio = 0.38F;

    juce::Path p;
    p.addTriangle(x + (w * 0.5F), y + (h * (yRatio + arrowH)),
                  x + (w * (1.0F - arrowX)), y + (h * yRatio),
                  x + (w * arrowX), y + (h * yRatio));

    g.setColour(box.findColour(juce::ComboBox::ColourIds::arrowColourId).withMultipliedAlpha(box.isEnabled() ? 1.0F : 0.3F));
    g.fillPath(p);
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox& box)
{
    // Draws the selected item only (not the dropdown item).
    // Strange, modifying the font height does not seem to do anything...
    return LookAndFeel_V3::getComboBoxFont(box);    // NOLINT(*-parent-virtual-call)
}

void CustomLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    // Called to display the selected items (not the dropdown items).
    LookAndFeel_V3::positionComboBoxText(box, label);   // NOLINT(*-parent-virtual-call)
}

juce::Path CustomLookAndFeel::getTickShape(const float height)
{
    // This is used by ComboBoxes but also CheckBoxes!

    juce::Path path;
    path.addRoundedRectangle(0.0F, 0.0F, height, height, 0.2F);

    return path;
}


// Text Editor.

void CustomLookAndFeel::drawTextEditorOutline(juce::Graphics& g, const int width, const int height, juce::TextEditor& textEditor)
{
    // Mostly comes from the JUCE V4, but:
    // - Wanted to change the line thickness to 1 (!), just like the ComboBox.
    // - The original code doesn't draw anything if not enabled. So we do it by ourselves.
    if (textEditor.isEnabled() && textEditor.hasKeyboardFocus(true) && !textEditor.isReadOnly()) {
        g.setColour(textEditor.findColour(juce::TextEditor::focusedOutlineColourId));
        g.drawRect(0, 0, width, height, 1);     // Was 2!
    } else {
        // Displays something even if disabled.
        g.setColour(textEditor.findColour(juce::TextEditor::outlineColourId));
        g.drawRect(0, 0, width, height);
    }
}


// Tooltip.

juce::Rectangle<int> CustomLookAndFeel::getTooltipBounds(const juce::String& tipText, const juce::Point<int> screenPos, const juce::Rectangle<int> parentArea)
{
    // Coming from juce_LookAndFeel_V4.cpp, because the we need a bigger size.

    const auto tl(layoutTooltipText(tipText, toolTipFont, juce::Colours::black));

    const auto w = static_cast<int>(tl.getWidth() + 36.0F);
    const auto h = static_cast<int>(tl.getHeight() + 20.0F);

    return juce::Rectangle(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
                           screenPos.y > parentArea.getCentreY() ? screenPos.y - (h + 6) : screenPos.y + 6,
                           w, h)
            .constrainedWithin(parentArea);
}

void CustomLookAndFeel::drawTooltip(juce::Graphics& g, const juce::String& text, const int width, const int height)
{
    // Coming from juce_LookAndFeel_V4.cpp, because the font size is hardcoded!! Pity...
    // The corner rectangle is also removed, strangely didn't work.

    const juce::Rectangle bounds(width, height);

    g.setColour(findColour(juce::TooltipWindow::ColourIds::backgroundColourId));
    g.fillRect(bounds);

    const auto textColor = findColour(juce::TooltipWindow::ColourIds::textColourId);
    g.setColour(textColor);       // Original color was juce::TooltipWindow::outlineColourId. No need to create 3 colors for the tooltip...
    g.drawRect(bounds, 1);

    const auto tl = layoutTooltipText(text, toolTipFont, textColor);
    tl.draw(g, { static_cast<float>(width), static_cast<float>(height) });
}

juce::TextLayout CustomLookAndFeel::layoutTooltipText(const juce::String& text, const juce::Font& font, const juce::Colour colour) noexcept
{
    // Coming from detail::LookAndFeelHelpers::layoutTooltipText, because the text size is hardcoded... Pity.

    constexpr auto maxToolTipWidth = 400;

    juce::AttributedString s;
    s.setJustification(juce::Justification::centred);

    s.append(text, font, colour);

    juce::TextLayout tl;
    tl.createLayoutWithBalancedLineLengths(s, static_cast<float>(maxToolTipWidth));
    return tl;
}

} // namespace arkostracker
