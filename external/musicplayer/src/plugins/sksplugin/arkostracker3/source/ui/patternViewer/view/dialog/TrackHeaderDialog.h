#pragma once

#include "../../../../utils/OptionalValue.h"
#include "../../../components/EditText.h"
#include "../../../components/colors/ColorChooser.h"
#include "../../../components/colors/ColorView.h"
#include "../../../components/dialogs/ModalDialog.h"

namespace arkostracker
{

/** Dialog to ask for the track name and possible track color. */
class TrackHeaderDialog final : public ModalDialog,
                                public ColorView::Listener,
                                public ColorChooser::Listener
{
public:
    class Result
    {
    public:
        Result(juce::String pNewName, const OptionalValue<juce::Colour> pNewColor) :
                newName(std::move(pNewName)),
                newColor(pNewColor)
        {
        }

        const juce::String newName;
        const OptionalValue<juce::Colour> newColor;
    };

    /**
     * Constructor.
     * @param currentName the current name, or empty.
     * @param currentColor the possible color.
     * @param okCallback called when the dialog is validated.
     * @param cancelCallback called when the dialog is canceled.
     */
    TrackHeaderDialog(const juce::String& currentName, OptionalValue<juce::Colour> currentColor, const std::function<void(Result)>& okCallback,
        const std::function<void()>& cancelCallback) noexcept;

    // ColorView::Listener method implementations.
    // =============================================
    void onColorClicked(const juce::Colour& color, int viewId) noexcept override;

    // ColorChooser::Listener method implementations.
    // ==================================================
    void onColorSelectedInColorChooser(juce::Colour color) noexcept override;
    void onColorInColorChooserCancelled() noexcept override;

private:
    void onOkClicked() const noexcept;
    void onCancelClicked() const noexcept;

    void onTrackColorToggleChanged() noexcept;
    void showColorDialog() noexcept;
    void refreshViews() noexcept;

    std::function<void(Result)> okCallback;
    std::function<void()> cancelCallback;

    OptionalValue<juce::Colour> currentColor;

    juce::Label trackNameLabel;
    EditText trackNameEditText;

    juce::ToggleButton trackColorToggle;
    ColorView trackColorView;
    std::unique_ptr<ModalDialog> modalDialog;

    //juce::Colour tempColor;
};

}       // namespace arkostracker
