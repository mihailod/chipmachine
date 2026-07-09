#pragma once

#include "../../components/EditText.h"
#include "../../components/colors/ColorChooser.h"
#include "../../components/colors/ColorView.h"
#include "../../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

/** Dialog to edit a marker. */
class EditMarkerDialog final : public ModalDialog,
                               public ColorView::Listener,
                               public ColorChooser::Listener
{
public:
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** The user wants to delete the Marker. */
        virtual void onWantToDeleteMarker(int position) = 0;
        /** The user cancelled the Marker editing. */
        virtual void onWantToCancelEditMarker() = 0;
        /** The user wants to set new data. */
        virtual void onWantToEditMarker(int position, const juce::String& newMarkerName, juce::Colour newMarkerColor) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener of the user's intent.
     * @param position the position of the Marker.
     * @param markerName the original Marker name.
     * @param markerColor the original Marker colour.
     */
    EditMarkerDialog(EditMarkerDialog::Listener& listener, int position, const juce::String& markerName, juce::Colour markerColor) noexcept;

    // ColorView::Listener method implementations.
    // ============================================
    void onColorClicked(const juce::Colour& color, int viewId) noexcept override;

    // ColorChooser::Listener method implementations.
    // ================================================
    void onColorSelectedInColorChooser(juce::Colour color) noexcept override;
    void onColorInColorChooserCancelled() noexcept override;

    // TextEditor::Listener method implementations.
    // ============================================
protected:
    void textEditorTextChanged(juce::TextEditor& editor) override;

private:
    /** If the text is empty after trimming, the OK button is disabled. */
    void updateOkButtonEnablementFromText() noexcept;

    /** Called when the Ok button is clicked. */
    void onDialogOk() noexcept;
    /** Called when the Cancel button is clicked. */
    void onDialogCancelled() noexcept;

    /** Called when the Delete button is clicked. */
    void onDeleteButtonClicked() noexcept;

    Listener& listener;
    const int position;

    juce::Label nameLabel;
    EditText nameTextEditor;
    ColorView colorView;

    juce::TextButton deleteButton;

    std::unique_ptr<ModalDialog> modalDialog;
};

}   // namespace arkostracker

