#pragma once

#include "../../../../utils/WithParent.h"
#include "../../../components/ColoredImage.h"
#include "../../../components/colors/ColorChooser.h"
#include "../../../components/colors/ColorView.h"
#include "../../../components/dialogs/ModalDialog.h"
#include "../../../lists/view/item/ListItem.h"

namespace arkostracker 
{

class InstrumentDataController;

/** A list item for Instruments. */
class InstrumentListItem final : public ListItem,
                                 public ColorView::Listener,
                                 public ColorChooser::Listener
{
public:
    /**
     * Constructor.
     * @param rowNumber the row number (>=0).
     * @param isRowSelected true if the row is selected.
     * @param listener to be notified on item click.
     * @param dataProvider indicates what to display. Generic data.
     * @param instrumentDataController provides specialized data about the instrument, and can be sent events about it.
     */
    InstrumentListItem(int rowNumber, bool isRowSelected, ListItemListener& listener, DataProvider& dataProvider,
                       InstrumentDataController& instrumentDataController) noexcept;

    // ColorView::Listener method implementations.
    // ==============================================
    void onColorClicked(const juce::Colour& color, int viewId) noexcept override;

    // Component method implementations.
    // ====================================
    void resized() override;
    void lookAndFeelChanged() override;

    // ColorChooser::Listener method implementations.
    // ================================================
    void onColorSelectedInColorChooser(juce::Colour color) noexcept override;
    void onColorInColorChooserCancelled() noexcept override;

protected:
    static const int instrumentTypeViewX;
    static const int instrumentTypeViewWidth;
    static const int colorViewSupposedWidth;            // Not the real width, only an approximation.
    static const int textX;
    static const int textMinimumWidth;

    bool isDragAllowed(int rowNumber) const noexcept override;
    bool isDropAllowedHere(int rowNumber) const noexcept override;
    juce::Colour getBackgroundColorForEvenItems(const juce::LookAndFeel& lookAndFeel) noexcept override;
    juce::Colour getBackgroundColorForOddItems(const juce::LookAndFeel& lookAndFeel) noexcept override;
    juce::Colour getBackgroundColorForSelectedItems(const juce::LookAndFeel& lookAndFeel) noexcept override;

    void drawText(juce::Graphics& g, int rowNumber, int width, int height, int suggestedTextX, int textY, int maximumTextWidth, int textHeight) noexcept override;
    void drawOnGraphics(juce::Graphics& graphics, int rowNumber, int width, int height, int textY, int textHeight) noexcept override;

private:
    /** To receive the click on the Instrument Type image. */
    class ImageClickListener final : public juce::MouseListener,
                                     public WithParent<InstrumentListItem>
    {
    public:
        explicit ImageClickListener(InstrumentListItem& parent);
        void mouseDown(const juce::MouseEvent& event) override;
    };

    /** @return the color for the image. */
    static juce::Colour getImageColor() noexcept;

    InstrumentDataController& instrumentDataController;

    ColoredImage instrumentTypeView;
    ColorView colorView;
    std::unique_ptr<ModalDialog> modalDialog;

    ImageClickListener instrumentTypeImageClickListener;
};

}   // namespace arkostracker
