#include <utility>

#include "ColorChooser.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "ColorView.h"

namespace arkostracker 
{

ColorChooser::ColorChooser(Listener& pListener, const juce::Colour* pSelectedColour, const int pColumnCountInX, const std::vector<juce::uint32>& pNewColors) noexcept :
        ModalDialog(juce::translate("Choose a color"),
                    2 * LookAndFeelConstants::margins + pColumnCountInX * (ColorView::preferredSize + colorMargins) - colorMargins,
                    static_cast<int>(2 * LookAndFeelConstants::margins + ceil((static_cast<double>(pNewColors.size()) / static_cast<double>(pColumnCountInX))) * (ColorView::preferredSize + colorMargins)
                    - colorMargins + LookAndFeelConstants::buttonsHeight + 40),
                    [&] { onDialogOk(); },
                    [&] { onDialogCancelled(); },
                    true, true),
        listener(pListener),
        colorsArgb(pNewColors),
        colorViews(),
        selectedColorIndex(-1)
{
    // Locates the ColorViews.
    const int startX = LookAndFeelConstants::margins;

    // As long as there are colors...
    size_t colorIndex = 0;
    const auto colorCount = colorsArgb.size();
    auto y = LookAndFeelConstants::margins;
    while (colorIndex < colorCount) {
        for (auto colorIndexInLine = 0, x = startX;
            (colorIndexInLine < pColumnCountInX) && (colorIndex < colorCount);
            ++colorIndexInLine, x += ColorView::preferredSize + colorMargins, ++colorIndex) {

            const auto& color = juce::Colour(colorsArgb.at(colorIndex));
            const bool selected = ((pSelectedColour != nullptr) && (color == *pSelectedColour));

            // Keeps the selected index of the color.
            if (selected) {
                selectedColorIndex = static_cast<int>(colorIndex);
            }

            auto colorView = std::make_unique<ColorView>(*this, color, static_cast<int>(colorIndex), selected);
            colorView->setBounds(x, y, ColorView::preferredSize, ColorView::preferredSize);

            addComponentToModalDialog(*colorView);

            colorViews.emplace_back(std::move(colorView));
        }

        y += ColorView::preferredSize + colorMargins;
    }

    updateOkButtonEnable();
}

void ColorChooser::updateOkButtonEnable() noexcept
{
    setOkButtonEnable(selectedColorIndex >= 0);
}


// ColorView::Listener method implementations
// ==========================================

void ColorChooser::onColorClicked(const juce::Colour& /*color*/, const int viewId) noexcept
{
    // Nothing to do if the selected color is the same as before.
    if (viewId == selectedColorIndex) {
        return;
    }
    jassert(viewId >= 0);

    // Un-selects the previous color, if any.
    if (selectedColorIndex >= 0) {
        colorViews.at(static_cast<size_t>(selectedColorIndex))->setSelectedAndRepaint(false);
    }
    // Selects the new item.
    jassert(viewId < static_cast<int>(colorViews.size()));
    colorViews.at(static_cast<size_t>(viewId))->setSelectedAndRepaint(true);

    selectedColorIndex = viewId;

    updateOkButtonEnable();
}

void ColorChooser::onColorDoubleClicked() noexcept
{
    okPressed();
}

void ColorChooser::onDialogOk() const noexcept
{
    const auto colour = (selectedColorIndex >= 0) ? juce::Colour(colorsArgb.at(static_cast<size_t>(selectedColorIndex))) : juce::Colours::black;
    listener.onColorSelectedInColorChooser(colour);
}

void ColorChooser::onDialogCancelled() const noexcept
{
    listener.onColorInColorChooserCancelled();
}

}   // namespace arkostracker
