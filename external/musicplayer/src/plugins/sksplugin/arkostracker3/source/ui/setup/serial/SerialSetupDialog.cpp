#include "SerialSetupDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/serial/SerialController.h"
#include "../../../utils/StringUtil.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "../../utils/ComboBoxItem.h"
#include "TestTask.h"

namespace arkostracker
{
SerialSetupDialog::SerialSetupDialog(std::function<void()> pOkCallback,
                                     std::function<void()> pCancelCallback) noexcept:
        ModalDialog(juce::translate("Serial communication"), 460, 400,
                    [&] { onOkButtonClicked(); },
                    std::move(pCancelCallback),
                    true, true),
        preferences(PreferencesManager::getInstance()),
        okCallback(std::move(pOkCallback)),
        explanations(juce::String(), juce::translate("This allows to send PSG data in real-time to a computer via\nserial communication.")),
        portLabel(juce::String(), juce::translate("Port")),
        portComboBox(),
        portRefreshTextButton(juce::translate("Refresh")),

        profileLabel(juce::String(), juce::translate("Profile")),
        profileComboBox(),

        baudRateLabel(juce::String(), juce::translate("Baud rate")),
        baudRateTextEditor(),

        byteSizeLabel(juce::String(), juce::translate("Byte size")),
        byteSizeComboBox(),
        parityLabel(juce::String(), juce::translate("Parity")),
        parityComboBox(),

        stopBitsLabel(juce::String(), juce::translate("Stop bits")),
        stopBitsComboBox(),
        flowControlLabel(juce::String(), juce::translate("Flow control")),
        flowControlComboBox(),

        testButton(juce::translate("Test")),

        latestSelectedProfile(SerialProfile::custom),

        backgroundTask(),
        alertDialog()
{
    addComponentToModalDialog(explanations);
    addComponentToModalDialog(portLabel);
    addComponentToModalDialog(portComboBox);
    addComponentToModalDialog(portRefreshTextButton);

    addComponentToModalDialog(profileLabel);
    addComponentToModalDialog(profileComboBox);

    addComponentToModalDialog(baudRateLabel);
    addComponentToModalDialog(baudRateTextEditor);

    addComponentToModalDialog(byteSizeLabel);
    addComponentToModalDialog(byteSizeComboBox);

    addComponentToModalDialog(parityLabel);
    addComponentToModalDialog(parityComboBox);

    addComponentToModalDialog(stopBitsLabel);
    addComponentToModalDialog(stopBitsComboBox);
    addComponentToModalDialog(flowControlLabel);
    addComponentToModalDialog(flowControlComboBox);

    addComponentToModalDialog(testButton);

    baudRateTextEditor.setInputRestrictions(10, StringUtil::integerNumbers);

    locateUiItems();
    fillComboBoxes();

    profileComboBox.onChange = [&] { onProfileComboBoxChanged(); };
    portComboBox.onChange = [&] { onPortComboBoxChanged(); };
    portRefreshTextButton.onClick = [&] { fillPorts(false); };
    testButton.onClick = [&] { testPortConnectivity(); };

    // Selects the profile, this will update the views.
    const auto profile = preferences.getSelectedSerialProfile();
    latestSelectedProfile = profile;
    fillFromSerialProfile(profile);
}

void SerialSetupDialog::onOkButtonClicked() noexcept
{
    saveDataFromUi();

    okCallback();
}

void SerialSetupDialog::locateUiItems() noexcept
{
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto comboBoxesHeight = labelsHeight;
    constexpr auto topComboBoxesWidth = 180;
    const auto buttonsHeight = labelsHeight;
    constexpr auto secondColumnX = 240;

    constexpr auto leftLabelsSize = 90;
    constexpr auto rightLabelsSize = 100;
    constexpr auto testButtonWidth = 80;

    auto area = getUsableModalDialogBounds();

    explanations.setBounds(area.removeFromTop(labelsHeight * 2 + margins * 2));
    explanations.setJustificationType(juce::Justification::topLeft);
    portLabel.setBounds(area.getX(), area.getY(), leftLabelsSize, labelsHeight);
    const auto leftX = portLabel.getX();
    portComboBox.setBounds(portLabel.getRight(), portLabel.getY(), topComboBoxesWidth, comboBoxesHeight);
    portRefreshTextButton.setBounds(portComboBox.getRight() + margins, portLabel.getY(), 80, buttonsHeight);

    profileLabel.setBounds(leftX, portLabel.getBottom() + 2 * margins, leftLabelsSize, labelsHeight);
    profileComboBox.setBounds(profileLabel.getRight(), profileLabel.getY(), topComboBoxesWidth, comboBoxesHeight);

    baudRateLabel.setBounds(leftX, profileLabel.getBottom() + margins * 2, leftLabelsSize, labelsHeight);
    baudRateTextEditor.setBounds(baudRateLabel.getRight(), baudRateLabel.getY(), 140, labelsHeight);

    byteSizeLabel.setBounds(leftX, baudRateLabel.getBottom() + margins, leftLabelsSize, labelsHeight);
    byteSizeComboBox.setBounds(byteSizeLabel.getRight(), byteSizeLabel.getY(), 80, labelsHeight);
    parityLabel.setBounds(secondColumnX, byteSizeLabel.getY(), rightLabelsSize, labelsHeight);
    parityComboBox.setBounds(parityLabel.getRight(), parityLabel.getY(), 100, labelsHeight);

    stopBitsLabel.setBounds(leftX, byteSizeLabel.getBottom() + margins, leftLabelsSize, labelsHeight);
    stopBitsComboBox.setBounds(stopBitsLabel.getRight(), stopBitsLabel.getY(), 80, labelsHeight);
    flowControlLabel.setBounds(secondColumnX, stopBitsLabel.getY(), rightLabelsSize, labelsHeight);
    flowControlComboBox.setBounds(flowControlLabel.getRight(), flowControlLabel.getY(), 100, labelsHeight);

    testButton.setBounds((getWidth() - testButtonWidth) / 2, stopBitsLabel.getBottom() + 2 * margins, testButtonWidth, labelsHeight);
}

void SerialSetupDialog::fillFromSerialProfile(const SerialProfile& serialProfile) noexcept
{
    const auto serialData = SerialController::buildFromSerialProfile(serialProfile, preferences);
    auto parity = serialData.getParity();

    profileComboBox.setSelectedId(static_cast<int>(serialProfile), juce::NotificationType::dontSendNotification);
    baudRateTextEditor.setText(juce::String(serialData.getBaudRate()), false);
    byteSizeComboBox.setSelectedId(static_cast<int>(serialData.getByteSize()), juce::NotificationType::dontSendNotification);
    if (parity == 0) {        // Juce doesn't like 0, reserved in the ComboBox!
        parity = -1;
    }
    parityComboBox.setSelectedId(parity, juce::NotificationType::dontSendNotification);
    stopBitsComboBox.setSelectedId(static_cast<int>(serialData.getStopBits()), juce::NotificationType::dontSendNotification);
    flowControlComboBox.setSelectedId(static_cast<int>(serialData.getFlowControl()), juce::NotificationType::dontSendNotification);

    const auto readOnly = (serialProfile != SerialProfile::custom);
    updateEnability(readOnly);
}

void SerialSetupDialog::updateEnability(bool readOnlyProfile) noexcept
{
    const auto enabled = !readOnlyProfile;

    baudRateTextEditor.setEnabled(enabled);
    byteSizeComboBox.setEnabled(enabled);
    parityComboBox.setEnabled(enabled);
    stopBitsComboBox.setEnabled(enabled);
    flowControlComboBox.setEnabled(enabled);
}

void SerialSetupDialog::fillComboBoxes() noexcept
{
    fillPorts(true);

    // Profiles.
    ComboBoxItem::addItemsToComboBox(profileComboBox, {
            { juce::translate("Custom"), static_cast<int>(SerialProfile::custom) },
            { juce::translate("Mini/CPC Booster"), static_cast<int>(SerialProfile::booster) },
            { juce::translate("Albireo (CPC)"), static_cast<int>(SerialProfile::albireo) },
            { juce::translate("USIfAC (CPC)"), static_cast<int>(SerialProfile::usifac) }
    });

    // Byte size.
    for (const auto byteSize : { 5, 6, 7, 8 }) {
        ComboBoxItem::addItemToComboBox(byteSizeComboBox, { juce::String(byteSize), byteSize });
    }

    // Parity.
    ComboBoxItem::addItemsToComboBox(parityComboBox, {
            { juce::translate("None"), parityIdNone },    // Because Juce reserves 0...
            { juce::translate("Odd (1)"), 1 },
            { juce::translate("Even (2)"), 2 },
            { juce::translate("Mark (3)"), 3 },
            { juce::translate("Space (4)"), 4 }
    });

    // Stop bits.
    ComboBoxItem::addItemsToComboBox(stopBitsComboBox, {
            { juce::translate("1"), static_cast<int>(StopBit::stopBit1) },
            { juce::translate("1.5"), static_cast<int>(StopBit::stopBit1p5) },
            { juce::translate("2"), static_cast<int>(StopBit::stopBit2) }
    });

    // Flow control.
    ComboBoxItem::addItemsToComboBox(flowControlComboBox, {
            { juce::translate("None"), static_cast<int>(FlowControl::none) },
            { juce::translate("Software"), static_cast<int>(FlowControl::software) },
            { juce::translate("Hardware"), static_cast<int>(FlowControl::hardware) }
    });
}

void SerialSetupDialog::fillPorts(bool selectStoredProfile) noexcept
{
    // Gets all the ports. Synchronous, shouldn't be a problem...
    const auto ports = SerialAccess::getPorts();

    // What was the already selected port, if any?
    const auto selectedPort = selectStoredProfile ? preferences.getSerialPort() : portComboBox.getText();

    // Clears and fills the ComboBox.
    portComboBox.clear(juce::NotificationType::dontSendNotification);
    ComboBoxItem::addItemToComboBox(portComboBox, { juce::translate("None"), portIdNone });
    auto id = 1;
    auto selectedPortId = portIdNone;
    for (const auto& port : ports) {
        ComboBoxItem::addItemToComboBox(portComboBox, { port, id });

        // Is it the selected one?
        if (selectedPort == port) {
            selectedPortId = id;
        }
        ++id;
    }
    portComboBox.setSelectedId(selectedPortId, juce::NotificationType::sendNotification);
}

void SerialSetupDialog::onProfileComboBoxChanged() noexcept
{
    // Before changing the values, changes the profile if Custom.
    saveCustomProfileDataIfNeeded();

    const auto serialProfile = static_cast<SerialProfile>(profileComboBox.getSelectedId());
    latestSelectedProfile = serialProfile;
    fillFromSerialProfile(serialProfile);
}

void SerialSetupDialog::onPortComboBoxChanged() noexcept
{
    const auto visible = (portComboBox.getSelectedId() != portIdNone);

    profileLabel.setVisible(visible);
    profileComboBox.setVisible(visible);
    baudRateLabel.setVisible(visible);
    baudRateTextEditor.setVisible(visible);
    byteSizeLabel.setVisible(visible);
    byteSizeComboBox.setVisible(visible);
    parityLabel.setVisible(visible);
    parityComboBox.setVisible(visible);
    stopBitsLabel.setVisible(visible);
    stopBitsComboBox.setVisible(visible);
    flowControlLabel.setVisible(visible);
    flowControlComboBox.setVisible(visible);
    testButton.setVisible(visible);
}

void SerialSetupDialog::saveDataFromUi() noexcept
{
    // Saves the port.
    const auto port = (portComboBox.getSelectedId() == portIdNone) ? "" : portComboBox.getText();
    preferences.storeSerialPort(port);

    // Save the current profile.
    const auto serialProfile = static_cast<SerialProfile>(profileComboBox.getSelectedId());
    preferences.storeSelectedSerialProfile(serialProfile);

    saveCustomProfileDataIfNeeded();
}

void SerialSetupDialog::saveCustomProfileDataIfNeeded() noexcept
{
    if (latestSelectedProfile != SerialProfile::custom) {
        return;
    }

    const auto serialData = buildSerialDataFromUi();
    preferences.storeSerialCustomProfile(serialData);
}

SerialData SerialSetupDialog::buildSerialDataFromUi() const noexcept
{
    auto parity = parityComboBox.getSelectedId();
    if (parity < 0) {       // 0 is reserved in Juce.
        parity = 0;
    }
    return {
            portComboBox.getText(),
            baudRateTextEditor.getText().getIntValue(),
            byteSizeComboBox.getSelectedId(),
            parity,
            static_cast<StopBit>(stopBitsComboBox.getSelectedId()),
            static_cast<FlowControl>(flowControlComboBox.getSelectedId())
    };
}

void SerialSetupDialog::testPortConnectivity() noexcept
{
    if (portComboBox.getSelectedId() == portIdNone) {
        jassertfalse;       // Shouldn't be possible!
        return;
    }

    const auto serialData = buildSerialDataFromUi();

    // Creates the test Task to perform it asynchronously.
    auto task = std::make_unique<TestTask>(serialData);

    backgroundTask = std::make_unique<BackgroundTask<std::unique_ptr<bool>>>(*this, std::move(task));
    backgroundTask->performTask();
}


// Background Task method implementations.
// ==================================================================

void SerialSetupDialog::onBackgroundTaskFinished(TaskOutputState /*taskOutputState*/, std::unique_ptr<bool> result) noexcept
{
    jassert(result != nullptr);
    if ((result != nullptr) && *result) {
        alertDialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Success! The serial port could be communicated with."), [&] {
            onAlertDialogClicked();
        });
    } else {
        alertDialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Error. No communication with the port could be established."), [&] {
            onAlertDialogClicked();
        });
    }
}


// ==================================================================

void SerialSetupDialog::onAlertDialogClicked() noexcept
{
    alertDialog = nullptr;
}

}   // namespace arkostracker
