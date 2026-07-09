#pragma once

#include "../../../controllers/serial/SerialAccess.h"
#include "../../../controllers/serial/model/SerialProfile.h"
#include "../../components/EditText.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundTask/BackgroundTask.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

class PreferencesManager;

/** Dialog to set up of the Serial. */
class SerialSetupDialog : public ModalDialog,
                          public BackgroundTaskListener<std::unique_ptr<bool>>
{
public:
    /**
     * Constructor.
     * @param okCallback called when the OK Button is clicked.
     * @param cancelCallback called when the Cancel Button is clicked.
     */
    SerialSetupDialog(std::function<void()> okCallback, std::function<void()> cancelCallback) noexcept;

private:
    static constexpr int portIdNone = -1;
    static constexpr int parityIdNone = -1;

    /** Setups the UI items. */
    void locateUiItems() noexcept;

    /** Fills the ComboBoxes with data. */
    void fillComboBoxes() noexcept;
    /**
     * Fills the port ComboBox with the existing ports.
     * @param selectStoredProfile true to select, if possible, the stored profile. If false, keeps the previously selected one. If not possible, selects "none".
     */
    void fillPorts(bool selectStoredProfile) noexcept;

    /** Called when the OK button is clicked. Saves the data and notifies the UI. */
    void onOkButtonClicked() noexcept;

    /**
     * Fills the various Views from the given profile.
     * @param profile the profile.
     */
    void fillFromSerialProfile(const SerialProfile& profile) noexcept;

    /** The profile ComboBox has changed. Updates the UI. */
    void onProfileComboBoxChanged() noexcept;
    /** The port ComboBox has changed. Updates the UI. */
    void onPortComboBoxChanged() noexcept;

    /** Tests the selected port connectivity. */
    void testPortConnectivity() noexcept;

    /**
     * Updates the views from the profile read-only flag.
     * @param readOnlyProfile true if the profile is read-only.
     */
    void updateEnability(bool readOnlyProfile) noexcept;

    /** Saves the data from the UI data. */
    void saveDataFromUi() noexcept;

    /** Stores the current Custom profile. Nothing happens if Custom is not selected. */
    void saveCustomProfileDataIfNeeded() noexcept;

    /** Builds a SerialData from the selected UI elements. */
    SerialData buildSerialDataFromUi() const noexcept;

    // Background Task method implementations.
    // ==================================================================
    void onBackgroundTaskFinished(TaskOutputState taskOutputState, std::unique_ptr<bool> result) noexcept override;

    /** Called when "ok" button on the alert dialog is clicked. Dismisses it. */
    void onAlertDialogClicked() noexcept;

    PreferencesManager& preferences;

    std::function<void()> okCallback;

    juce::Label explanations;

    juce::Label portLabel;
    juce::ComboBox portComboBox;
    juce::TextButton portRefreshTextButton;

    juce::Label profileLabel;
    juce::ComboBox profileComboBox;

    juce::Label baudRateLabel;
    EditText baudRateTextEditor;

    juce::Label byteSizeLabel;
    juce::ComboBox byteSizeComboBox;
    juce::Label parityLabel;
    juce::ComboBox parityComboBox;

    juce::Label stopBitsLabel;
    juce::ComboBox stopBitsComboBox;
    juce::Label flowControlLabel;
    juce::ComboBox flowControlComboBox;

    juce::TextButton testButton;

    SerialProfile latestSelectedProfile;

    std::unique_ptr<BackgroundTask<std::unique_ptr<bool>>> backgroundTask;

    std::unique_ptr<SuccessOrErrorDialog> alertDialog;
};

}   // namespace arkostracker
