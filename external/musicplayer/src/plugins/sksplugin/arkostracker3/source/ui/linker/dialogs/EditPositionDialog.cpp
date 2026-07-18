#include "EditPositionDialog.h"

#include <utility>

#include "../../../business/song/validation/CheckTrackHeight.h"
#include "../../../controllers/SongController.h"

namespace arkostracker 
{

const int EditPositionDialog::idOffset = 1;
const int EditPositionDialog::labelsWidth = 120;

EditPositionDialog::EditPositionDialog(EditPositionDialog::Listener& pListener, SongController& pSongController, Id pSubsongId, const int pPositionIndex) noexcept :
        ModalDialog(juce::translate("Edit position"), 270, 240,
                    [&] { onDialogOk(); },
                    [&] { onDialogCancelled(); },
                    true, true),
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        listener(pListener),
        positionIndex(pPositionIndex),
        originalPosition(Position::buildEmptyInstance()),       // For now.
        selectedPatternIndex(0),
        changePositionLabel(juce::translate("Change to")),
        changePositionComboBox(),
        heightSlider(*this, juce::translate("Height"), 2, 1.0, 2.0, SliderIncDec::Filter::hexadecimal,
                     labelsWidth - 13),
        markerLabel(juce::translate("Marker name")),
        markerNameTextEdit(),
        separator(juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundFocused))
                          .contrasting(0.2F)),
        colorLabel(juce::translate("Pattern color")),
        colorView(*this, juce::Colour(originalPosition.getMarkerColor())),
        modalDialog(),
        patternColors()
{
    // Gets the position to edit, and all the pattern colors.
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        originalPosition = subsong.getPosition(positionIndex);
        const auto patternCount = subsong.getPatternCount();

        // I don't know why the "pattern colors" and selectedPatternIndex exist... Maybe some more elaborated UI?
        patternColors.reserve(static_cast<size_t>(patternCount));
        for (auto patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
            const auto colorArgb = subsong.getPatternColor(patternIndex);
            patternColors.emplace_back(colorArgb);
        }
    });
    const auto patternCount = static_cast<int>(patternColors.size());

    const auto margins = LookAndFeelConstants::margins;

    const auto& usableBounds = getUsableModalDialogBounds();
    const auto left = usableBounds.getX();
    const auto width = usableBounds.getWidth();
    const auto top = usableBounds.getY();
    constexpr auto colorViewSize = ColorView::preferredSizeSmall;

    // Sets up the labels, TextEditor and ColorView.
    changePositionLabel.setTopLeftPosition(left, top);
    constexpr auto middleX = labelsWidth;
    changePositionComboBox.setBounds(middleX, changePositionLabel.getY(), width - middleX, changePositionLabel.getHeight());
    heightSlider.setTopLeftPosition(left, changePositionLabel.getBottom() + margins);
    markerLabel.setTopLeftPosition(left, heightSlider.getBottom() + margins);
    markerNameTextEdit.setBounds(middleX, markerLabel.getY(), width - middleX, markerLabel.getHeight());
    separator.setBounds(left, markerLabel.getBottom() + margins, width, 1);
    colorLabel.setTopLeftPosition(left, separator.getBottom() + margins);
    colorView.setBounds(middleX, colorLabel.getY(), colorViewSize, colorViewSize);

    addComponentToModalDialog(changePositionLabel);
    addComponentToModalDialog(changePositionComboBox);
    addComponentToModalDialog(heightSlider);
    addComponentToModalDialog(markerLabel);
    addComponentToModalDialog(markerNameTextEdit);
    addComponentToModalDialog(separator);
    addComponentToModalDialog(colorLabel);
    addComponentToModalDialog(colorView);

    heightSlider.setShownValue(originalPosition.getHeight());
    markerNameTextEdit.setText(originalPosition.getMarkerName(), false);
    markerNameTextEdit.setMultiLine(false, false);
    markerNameTextEdit.setInputRestrictions(16);
    markerNameTextEdit.setEscapeAndReturnKeysConsumed(false);

    // Fills the juce::ComboBox. Very simple.
    for (auto patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
        changePositionComboBox.addItem(juce::translate("Pattern " + juce::String::toHexString(patternIndex)),
                                       patternIndex + idOffset);            // Offset in ID, 0 is JUCE reserved.
    }
    changePositionComboBox.setSelectedId(originalPosition.getPatternIndex() + idOffset, juce::NotificationType::sendNotification);
    changePositionComboBox.addListener(this);
}

void EditPositionDialog::onDialogOk() const noexcept
{
    // Most of the values are coming from the original Position.
    const auto newPatternIndex = changePositionComboBox.getSelectedId() - idOffset;
    const auto height = static_cast<int>(heightSlider.getValue());
    const auto markerName = markerNameTextEdit.getText().trim();
    const Position newPosition(newPatternIndex, height, markerName, originalPosition.getMarkerColor(),
                         originalPosition.getChannelToTransposition());
    // Getting the current color is enough. We don't want, for now, to be able to change all the modified colors.
    const auto newPatternColor = colorView.getColor();
    // Gets the target Pattern of the Position, and changes its color.
    Pattern newPattern;
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        newPattern = subsong.getPatternFromIndex(newPatternIndex).withColor(newPatternColor.getARGB());
    });

    listener.onWantToEditPosition(positionIndex, newPosition, newPattern);
}

void EditPositionDialog::onDialogCancelled() const noexcept
{
    listener.onWantToCancelEditPosition();
}


// SliderIncDec::Listener method implementations.
// ==============================================

void EditPositionDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double valueDouble)
{
    jassert(&slider == &heightSlider); (void)slider;               // Any other undeclared Slider?

    // Corrects the value.
    const auto newHeight = CheckTrackHeight::correctHeight(static_cast<int>(valueDouble));

    heightSlider.setShownValue(newHeight);
}


// ColorView::Listener method implementations.
// ============================================

void EditPositionDialog::onColorClicked(const juce::Colour& /*color*/, int /*viewId*/) noexcept
{
    jassert(modalDialog == nullptr);        // Modal already opened?

    const auto& selectedColor = patternColors.at(static_cast<size_t>(selectedPatternIndex));
    modalDialog = std::make_unique<ColorChooser>(*this, &selectedColor);
}


// ColorChooser::Listener method implementations.
// ================================================

void EditPositionDialog::onColorSelectedInColorChooser(const juce::Colour color) noexcept
{
    modalDialog.reset();

    onNewColor(color);
}

void EditPositionDialog::onColorInColorChooserCancelled() noexcept
{
    modalDialog.reset();
}


// juce::ComboBox::Listener method implementations.
// ================================================

void EditPositionDialog::comboBoxChanged(juce::ComboBox* comboBox)
{
    jassert(comboBox == &changePositionComboBox);
    const auto selectedIndex = comboBox->getSelectedItemIndex();
    const auto& newColor = patternColors.at(static_cast<size_t>(selectedIndex));
    selectedPatternIndex = selectedIndex;

    onNewColor(newColor);
}


// ================================================

void EditPositionDialog::onNewColor(const juce::Colour& color) noexcept
{
    colorView.setColorAndRepaint(color);

    // Stores the new color besides to the selected pattern.
    patternColors.at(static_cast<size_t>(selectedPatternIndex)) = color;
}

}   // namespace arkostracker
