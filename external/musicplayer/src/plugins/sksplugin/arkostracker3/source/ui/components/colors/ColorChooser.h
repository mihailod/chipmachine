#pragma once

#include <memory>
#include <vector>

#include "../dialogs/ModalDialog.h"
#include "ColorView.h"
#include "ColorConstants.h"

namespace arkostracker 
{

/** Generic dialog to allow picking a color among the ones that are given. One can be selected. */
class ColorChooser final : public ModalDialog,
                           public ColorView::Listener
{
public:
    /** Listener to be aware of the return. */
    class Listener {
    public:
        /** Destructor.*/
        virtual ~Listener() = default;
        /**
         * Called when the color has been clicked.
         * @param color the selected color.
         */
        virtual void onColorSelectedInColorChooser(juce::Colour color) noexcept = 0;

        /** Called when the color chooser has been cancelled. */
        virtual void onColorInColorChooserCancelled() noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to notify the result to.
     * @param columnCountInX how many colors in X.
     * @param colors the colors to display.
     * @param selectedColour a possible selected color, or nullptr if there is none.
     */
    ColorChooser(Listener& listener, const juce::Colour* selectedColour, int columnCountInX = ColorConstants::colorPackCount,
                 const std::vector<juce::uint32>& colors = ColorConstants::colors) noexcept;

    // ColorView::Listener method implementations
    // ==========================================
    void onColorClicked(const juce::Colour& color, int viewId) noexcept override;
    void onColorDoubleClicked() noexcept override;

private:
    static const int colorMargins = 5;                      // Space between the colors.

    /** Enables the OK button if one color is selected. */
    void updateOkButtonEnable() noexcept;

    /** Called when OK was clicked. */
    void onDialogOk() const noexcept;
    /** Called when the Dialog was cancelled. */
    void onDialogCancelled() const noexcept;

    Listener& listener;
    std::vector<juce::uint32> colorsArgb;                   // The colors to display.

    std::vector<std::unique_ptr<ColorView>> colorViews;     // Holds the Color Views.
    int selectedColorIndex;                                 // The index of the selected color. -1 if none is.
};

}   // namespace arkostracker
