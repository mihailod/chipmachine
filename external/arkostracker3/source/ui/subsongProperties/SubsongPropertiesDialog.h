#pragma once

#include "../../song/subsong/Subsong.h"
#include "../../business/model/Properties.h"
#include "../../utils/Id.h"
#include "../../utils/OptionalValue.h"
#include "../components/GroupWithViewport.h"
#include "../components/SliderIncDec.h"
#include "../components/dialogs/ModalDialog.h"
#include "EditPsgDialog.h"
#include "PsgView.h"
#include "SidPlayerDialog.h"

namespace arkostracker 
{

/** Dialog showing a dialog to edit a Subsong properties. It does not change anything in the Subsong itself: it only returns the client about what to do. */
class SubsongPropertiesDialog final : public ModalDialog,
                                      SliderIncDec::Listener,
                                      public PsgView::Listener,
                                      public EditPsgDialog::Listener,
                                      public SidPlayerDialog::Listener
{
public:
    /** Listener of the result of this Dialog. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the Dialog is validated.
         * @param newPsgs the psgs. May be identical to the current ones.
         * @param newMetadata the metadata. May be identical to the current ones.
         */
        virtual void onSubsongPropertiesDialogOk(std::vector<Psg> newPsgs, Properties newMetadata) noexcept = 0;
        /** Called when the Dialog is canceled. */
        virtual void onSubsongPropertiesDialogCanceled() noexcept = 0;
        /** Called when the user wants the Subsong to be deleted. */
        virtual void onSubsongPropertiesDialogDeleteWanted() noexcept = 0;
    };

    /**
     * Constructor.
     * @param subsongId the subsong id to edit, if absent if "add new".
     * @param metadata the metadata of the Subsong.
     * @param psgs the PSGs of the Subsong.
     * @param listener listener to be notified of the result.
     * @param canDelete true if the Subsong can be deleted (not if New, or if the last existing one).
     */
    SubsongPropertiesDialog(const OptionalValue<Id>& subsongId, Subsong::Metadata metadata, std::vector<Psg> psgs,
        Listener& listener, bool canDelete) noexcept;

    // PsgView::Listener method implementations.
    // ============================================
    void onEditPsgButtonClicked(int psgIndex) noexcept override;
    void onDeletePsgButtonClicked(int psgIndex) noexcept override;
    void onAddPsgButtonClicked(int psgIndex) noexcept override;

    // EditPsgDialog::Listener method implementations.
    // ==================================================
    void onPsgValidated(int psgIndex, Psg psg) noexcept override;
    void onPsgNotModified() noexcept override;

    // SidPlayerDialog::Listener method implementations.
    // ==================================================
    void onSidPlayerSelected(SidPlayerCapability sidPlayerFrequency) noexcept override;
    void onSidPlayerCanceled() noexcept override;

private:
    static const int leftSlidersWidth;
    static const int rightSlidersWidth;

    static const int minimumPrimaryHighlight;
    static const int maximumPrimaryHighlight;
    static const int minimumSecondaryHighlight;
    static const int maximumSecondaryHighlight;

    // SliderIncDec::Listener method implementations.
    // =================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

    /** Called when the OK Button is clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called when the Cancel Button is clicked. */
    void onCancelButtonClicked() const noexcept;
    /** Called when the Delete Subsong Button is clicked. */
    void onDeleteSubsongButtonClicked() const noexcept;

    /** Called when the custom replay frequency changes. */
    void onCustomReplayFrequencyChanged() noexcept;
    /** Called when the custom replay frequency is "left". The value must be corrected. */
    void onCustomReplayFrequencyExit() noexcept;

    /**
     * Selects the right toggle according to the Subsong replay frequency, and fills the custom value if needed.
     * Should be called only once.
     */
    void updateReplayFrequencyToggles() noexcept;

    /** Fills the PSG Views from the data of the Song. */
    void fillPsgViews() noexcept;

    /** Updates the Views according the inner PSGs. */
    void onPsgsChanged() noexcept;

    /** Called when the SID player Edit button is clicked. Opens a pop-up. */
    void onSidPlayerFrequencyEditButtonClicked() noexcept;

    const Subsong::Metadata originalMetadata;
    std::vector<Psg> subsongPsgs;
    SidPlayerCapability sidPlayerCapability;
    OptionalValue<Id> subsongIdOptional;
    Listener& listener;

    juce::TextButton deleteButton;
    juce::Label titleLabel;
    EditText titleTextEditor;
    SliderIncDec initialSpeedSlider;
    SliderIncDec digiChannelSlider;
    SliderIncDec primaryHighlightSlider;
    SliderIncDec secondaryHighlightSlider;

    juce::GroupComponent replayFrequenciesGroupComponent;   // Displays a border around the replay rates.
    juce::ToggleButton replayFrequency12p5HzToggleButton;   // The ToggleButton showing 12.5hz.
    juce::ToggleButton replayFrequency25HzToggleButton;     // The ToggleButton showing 25hz.
    juce::ToggleButton replayFrequency50HzToggleButton;     // The ToggleButton showing 50hz.
    juce::ToggleButton replayFrequency60HzToggleButton;     // The ToggleButton showing 60hz.
    juce::ToggleButton replayFrequency100HzToggleButton;    // The ToggleButton showing 100hz.
    juce::ToggleButton replayFrequency150HzToggleButton;    // The ToggleButton showing 150hz.
    juce::ToggleButton replayFrequency200HzToggleButton;    // The ToggleButton showing 200hz.
    juce::ToggleButton replayFrequency300HzToggleButton;    // The ToggleButton showing 300hz.
    juce::ToggleButton replayFrequencyCustomToggleButton;   // The ToggleButton showing the custom replay rate.
    EditText replayFrequencyCustomTextEditor;               // The TextEditor showing the custom replay rate.

    GroupWithViewport psgsGroup;
    std::vector<std::unique_ptr<PsgView>> psgViews;

    std::unique_ptr<ModalDialog> editDialog;

    const int minimumDigiChannel;                           // >=0.
    int maximumDigiChannel;                                 // >=0.

    juce::GroupComponent sidPlayerFrequencyGroup;
    juce::Label sidPlayerFrequencyText;
    juce::TextButton sidPlayerFrequencyEditButton;
};

}   // namespace arkostracker
