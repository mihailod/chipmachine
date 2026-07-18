#pragma once

#include "../../../song/subsong/Pattern.h"
#include "../../../song/subsong/Position.h"
#include "../../../utils/Id.h"
#include "../../components/AutoSizeLabel.h"
#include "../../components/BlankComponent.h"
#include "../../components/SliderIncDec.h"
#include "../../components/colors/ColorChooser.h"
#include "../../components/colors/ColorView.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

class SongController;

/** Dialog to edit a Position. */
class EditPositionDialog final :  public ModalDialog,
                                  public SliderIncDec::Listener,
                                  public ColorView::Listener,
                                  public ColorChooser::Listener,
                                  public juce::ComboBox::Listener
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
         * @param positionIndex the Position to change.
         * @param position the new Position data.
         * @param pattern the new Pattern data for this position, to replace the one in the Position.
         */
        virtual void onWantToEditPosition(int positionIndex, Position position, Pattern pattern) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener of the user's intent.
     * @param songController the Song Controller.
     * @param subsongId the ID of the Subsong.
     * @param positionIndex the index of the position to edit.
     */
    EditPositionDialog(Listener& listener, SongController& songController, Id subsongId, int positionIndex) noexcept;

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

    // juce::ComboBox::Listener method implementations.
    // ================================================
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

private:
    static const int idOffset;
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
    const int positionIndex;
    Position originalPosition;
    int selectedPatternIndex;

    AutoSizeLabel changePositionLabel;
    juce::ComboBox changePositionComboBox;
    SliderIncDec heightSlider;
    AutoSizeLabel markerLabel;
    EditText markerNameTextEdit;

    BlankComponent separator;
    AutoSizeLabel colorLabel;
    ColorView colorView;
    std::unique_ptr<ModalDialog> modalDialog;

    std::vector<juce::Colour> patternColors;
};

}   // namespace arkostracker
