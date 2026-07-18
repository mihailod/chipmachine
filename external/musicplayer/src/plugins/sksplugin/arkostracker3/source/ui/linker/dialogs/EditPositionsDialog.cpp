#include "EditPositionsDialog.h"

#include "../../../business/song/validation/CheckTrackHeight.h"
#include "../../../controllers/SongController.h"
#include "../../lookAndFeel/CustomLookAndFeel.h"

namespace arkostracker
{

const int EditPositionsDialog::labelsWidth = 180;

EditPositionsDialog::EditPositionsDialog(Listener& pListener, SongController& pSongController, Id pSubsongId, const std::set<int>& pPositionIndexes) noexcept :
    ModalDialog(
        juce::translate("Edit positions"), 270, 220,
        [&] { onDialogOk(); },
        [&] { onDialogCancelled(); },
        true, true),
    songController(pSongController),
    subsongId(std::move(pSubsongId)),
    listener(pListener),
    positionIndexes(pPositionIndexes),
    selectedColor(LookAndFeelConstants::defaultPatternColor),
    heightSlider(*this, juce::translate("Heights"), 2, 1.0, 2.0, SliderIncDec::Filter::hexadecimal,
                 labelsWidth - 13),
    separator(juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::panelBackgroundFocused))
        .contrasting(0.2F)),
    colorLabel(juce::translate("Patterns color")),
    colorView(*this, selectedColor),
    noteLabel(juce::String(), juce::translate("Only modified fields will be applied.")),
    modalDialog(),
    hasHeightChanged(),
    hasSelectedColorChanged()
{
    // Gets the initial data to show.
    auto positionHeight = TrackConstants::defaultPositionHeight;
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        const auto firstPositionIndex = *positionIndexes.cbegin();
        const auto firstPosition = subsong.getPosition(firstPositionIndex);
        positionHeight = firstPosition.getHeight();

        const auto patternIndex = firstPosition.getPatternIndex();
        const auto colorArgb = subsong.getPatternColor(patternIndex);
        selectedColor = juce::Colour(colorArgb);
        colorView.setColorAndRepaint(selectedColor);
    });

    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    const auto& usableBounds = getUsableModalDialogBounds();
    const auto left = usableBounds.getX();
    const auto width = usableBounds.getWidth();
    const auto top = usableBounds.getY();
    const auto bottom = usableBounds.getBottom();
    constexpr auto colorViewSize = ColorView::preferredSizeSmall;

    // Sets up the UI.
    constexpr auto middleX = labelsWidth;
    heightSlider.setTopLeftPosition(left, top + margins);
    separator.setBounds(left, heightSlider.getBottom() + margins, width, 1);
    colorLabel.setTopLeftPosition(left, separator.getBottom() + margins);
    colorView.setBounds(middleX, colorLabel.getY(), colorViewSize, colorViewSize);
    noteLabel.setBounds(left, bottom - labelsHeight, width, labelsHeight);
    noteLabel.setJustificationType(juce::Justification::centred);

    addComponentToModalDialog(heightSlider);
    addComponentToModalDialog(separator);
    addComponentToModalDialog(colorLabel);
    addComponentToModalDialog(colorView);
    addComponentToModalDialog(noteLabel);

    heightSlider.setShownValue(positionHeight);
}

void EditPositionsDialog::onDialogOk() const noexcept
{
    listener.onWantToEditPositions(positionIndexes,
        hasHeightChanged ? static_cast<int>(heightSlider.getValue()) : OptionalInt(),
        hasSelectedColorChanged ? selectedColor.getARGB() : OptionalValue<juce::uint32>()
    );
}

void EditPositionsDialog::onDialogCancelled() const noexcept
{
    listener.onWantToCancelEditPosition();
}

void EditPositionsDialog::onNewColor(const juce::Colour& color) noexcept
{
    selectedColor = color;
    colorView.setColorAndRepaint(color);
}


// SliderIncDec::Listener method implementations.
// ==============================================

void EditPositionsDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double valueDouble)
{
    jassert(&slider == &heightSlider); (void)slider;               // Any other undeclared Slider?

    hasHeightChanged = true;

    // Corrects the value.
    const auto newHeight = CheckTrackHeight::correctHeight(static_cast<int>(valueDouble));

    heightSlider.setShownValue(newHeight);
}


// ColorView::Listener method implementations.
// ============================================

void EditPositionsDialog::onColorClicked(const juce::Colour& /*color*/, int /*viewId*/) noexcept
{
    jassert(modalDialog == nullptr);        // Modal already opened?

    modalDialog = std::make_unique<ColorChooser>(*this, &selectedColor);
}

// ColorChooser::Listener method implementations.
// ================================================

void EditPositionsDialog::onColorSelectedInColorChooser(const juce::Colour color) noexcept
{
    modalDialog.reset();

    hasSelectedColorChanged = true;

    onNewColor(color);
}

void EditPositionsDialog::onColorInColorChooserCancelled() noexcept
{
    modalDialog.reset();
}

} // namespace arkostracker
