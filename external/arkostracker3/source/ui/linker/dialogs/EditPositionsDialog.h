#pragma once

#include "../../../utils/OptionalValue.h"
#include "../../components/AutoSizeLabel.h"
#include "../../components/BlankComponent.h"
#include "../../components/SliderIncDec.h"
#include "../../components/colors/ColorChooser.h"
#include "../../components/colors/ColorView.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

class SongController;

/** Dialog to edit several positions. Only a few data can be edited. */
class EditPositionsDialog final : public ModalDialog,
                                  public SliderIncDec::Listener,
                                  public ColorView::Listener,
                                  public ColorChooser::Listener
{
public:
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** The user cancelled the Position editing. */
        virtual void onWantToCancelEditPosition() = 0;

        /**
         * The user wants to set new data.
         * @param positionIndexes the indexes of the Positions to change.
         * @param newHeight the possible new height.
         * @param newPatternColorArgb the possible new color.
         */
        virtual void onWantToEditPositions(std::set<int> positionIndexes, OptionalInt newHeight, OptionalValue<juce::uint32> newPatternColorArgb) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener of the user's intent.
     * @param songController the Song Controller.
     * @param subsongId the ID of the Subsong.
     * @param positionIndexes the indexes of the position to edit.
     */
    EditPositionsDialog(Listener& listener, SongController& songController, Id subsongId, const std::set<int>& positionIndexes) noexcept;

    // SliderIncDec::Listener method implementations.
    // ================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

    // ColorView::Listener method implementations.
    // =============================================
    void onColorClicked(const juce::Colour& color, int viewId) noexcept override;

    // ColorChooser::Listener method implementations.
    // ================================================
    void onColorSelectedInColorChooser(juce::Colour color) noexcept override;
    void onColorInColorChooserCancelled() noexcept override;

private:
    static const int labelsWidth;

    /** Called when the Ok button is clicked. */
    void onDialogOk() const noexcept;
    /** Called when the Cancel button is clicked. */
    void onDialogCancelled() const noexcept;

    /** Call this when a new color must be shown. This updates the UI and the internal color. */
    void onNewColor(const juce::Colour& color) noexcept;

    SongController& songController;
    const Id subsongId;
    Listener& listener;
    std::set<int> positionIndexes;
    juce::Colour selectedColor;

    SliderIncDec heightSlider;
    BlankComponent separator;
    AutoSizeLabel colorLabel;
    ColorView colorView;
    juce::Label noteLabel;

    std::unique_ptr<ModalDialog> modalDialog;

    bool hasHeightChanged;
    bool hasSelectedColorChanged;
};

}   // namespace arkostracker
