#include "QuickTestingDialog.h"

#include "../../../business/song/tool/detection/FeatureDetector.h"
#include "../../../business/song/tool/songStripper/SongStripper.h"
#include "../../../controllers/MainController.h"
#include "../../../export/quickExport/QuickTesting.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

int QuickTestingDialog::previouslySelectedTargetToIndex = 0;         // Stored here, simple enough.
QuickTestingDialog::SelectedFormat QuickTestingDialog::previouslySelectedFormat = SelectedFormat::akg;  // Stored here, simple enough.

QuickTestingDialog::QuickTestingDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ModalDialog(juce::translate("Quick testing"), 560, 310,
                            [&] { onExportButtonClicked(); },
                            [&] { onCancelButtonClicked(); },
                            true, true),
        mainController(pMainController),
        listener(std::move(pListener)),
        exportTargetGroup(juce::translate("Export to"), 2, false, true, 400,
            [&] (const int index) {
                if (index == targetIndexDsk) {
                    return juce::translate("DSK (Amstrad CPC)");
                }
                jassert(index == targetIndexSna);
                return juce::translate("SNA (Amstrad CPC)");
            },
            [&] (const int index) { return (index == previouslySelectedTargetToIndex); }),
        exportFormatGroup(juce::String(), juce::translate("Export format")),
        akgToggleButton(juce::translate("AKG")),
        akgWarningLabel(),
        akyToggleButton(juce::translate("AKY")),
        akyWarningLabel(),
        akySidToggleButton(juce::translate("AKY SID (1 SID channel)")),
        akySidWarningLabel(),
        akyModCpcOldToggleButton(juce::translate("MOD")),
        akyModCpcOldWarningLabel(),
        song(),
        akyDeterminedPlayerType(),
        backgroundTask(),
        alertDialog()
{
    setOkButtonText(juce::translate("Export"));
    setOkButtonWidth(70);
    setCancelButtonText(juce::translate("Close"));

    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto groupMarginsX = LookAndFeelConstants::groupMarginsX;
    const auto groupMarginsY = LookAndFeelConstants::groupMarginsY;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    exportTargetGroup.setBounds(left, top, width, 80);
    exportFormatGroup.setBounds(left, exportTargetGroup.getBottom() + margins, width, 130);
    const auto widthInGroup = exportFormatGroup.getWidth() - (2 * groupMarginsX);
    constexpr auto akxToggleButtonWidth = 80;
    constexpr auto akyToggleButtonWidth = 80;
    constexpr auto akySidToggleButtonWidth = 200;
    constexpr auto akyModCpcOldToggleButtonWidth = 80;
    akgToggleButton.setBounds(exportFormatGroup.getX() + groupMarginsX, exportFormatGroup.getY() + groupMarginsY, akxToggleButtonWidth, labelsHeight);
    akgWarningLabel.setBounds(akgToggleButton.getRight(), akgToggleButton.getY(), widthInGroup - akxToggleButtonWidth, labelsHeight);
    akyToggleButton.setBounds(akgToggleButton.getX(), akgToggleButton.getBottom(), akyToggleButtonWidth, labelsHeight);
    akyWarningLabel.setBounds(akyToggleButton.getRight(), akyToggleButton.getY(), widthInGroup - akyToggleButtonWidth, labelsHeight);
    akySidToggleButton.setBounds(akgToggleButton.getX(), akyToggleButton.getBottom(), akySidToggleButtonWidth, labelsHeight);
    akySidWarningLabel.setBounds(akySidToggleButton.getRight(), akySidToggleButton.getY(), widthInGroup - akySidToggleButtonWidth, labelsHeight);
    akyModCpcOldToggleButton.setBounds(akgToggleButton.getX(), akySidToggleButton.getBottom(), akyModCpcOldToggleButtonWidth, labelsHeight);
    akyModCpcOldWarningLabel.setBounds(akyModCpcOldToggleButton.getRight(), akyModCpcOldToggleButton.getY(), widthInGroup - akyModCpcOldToggleButtonWidth, labelsHeight);

    for (auto* toggleButton : { &akgToggleButton, &akyToggleButton, &akySidToggleButton, &akyModCpcOldToggleButton } ) {
        toggleButton->setRadioGroupId(642);
    }

    const static auto errorColor = juce::Colours::red;
    for (auto* label : { &akgWarningLabel, &akyWarningLabel, &akySidWarningLabel, &akyModCpcOldWarningLabel} ) {
        label->setColour(juce::Label::ColourIds::textColourId, errorColor);
    }

    exportTargetGroup.setSelected({ previouslySelectedTargetToIndex });

    addComponentToModalDialog(exportTargetGroup);
    addComponentToModalDialog(exportFormatGroup);
    addComponentToModalDialog(akgToggleButton);
    addComponentToModalDialog(akgWarningLabel);
    addComponentToModalDialog(akyToggleButton);
    addComponentToModalDialog(akyWarningLabel);
    addComponentToModalDialog(akySidToggleButton);
    addComponentToModalDialog(akySidWarningLabel);
    addComponentToModalDialog(akyModCpcOldToggleButton);
    addComponentToModalDialog(akyModCpcOldWarningLabel);

    // Only one subsong is extracted.
    const auto& originalSong = mainController.getSongController().getSong();
    const auto subsongId = mainController.getSongController().getCurrentSubsongId();
    const SubsongsAndPsgs subsongsAndPsgsToKeep(*originalSong, { subsongId });
    song = SongStripper::split(*originalSong, subsongsAndPsgsToKeep);

    determineFeaturesAndUpdateUi();
}

void QuickTestingDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void QuickTestingDialog::determineFeaturesAndUpdateUi() noexcept
{
    const auto result = FeatureDetector::perform(*song);
    const auto channelCount = result.maximumChannelCount;
    const auto areDigidrumsUsed = result.areDigidrumsUsed;
    const auto areSidUsed = result.areSidUsed;
    const auto arePsgInstrumentsUsed = result.arePsgInstrumentsUsed;
    const auto areSampleInstrumentsUsed = result.areSampleInstrumentsUsed;

    // AKG doesn't support more than 3 channels, digidrums and SID.
    const auto psgCount = PsgValues::getPsgCount(channelCount);
    const auto onlyOnePsg = (psgCount == 1);
    const auto isAkgValid = onlyOnePsg;
    juce::String akgWarning;
    if (!isAkgValid) {
        akgWarning = juce::translate("Only one PSG is supported.");
    } else if (areDigidrumsUsed) {
        akgWarning = juce::translate("Digidrums are not supported by this format.");
    } else if (areSidUsed) {
        akgWarning = juce::translate("SID is not supported by this format.");
    }
    akgWarningLabel.setText(akgWarning, juce::NotificationType::dontSendNotification);
    akgToggleButton.setEnabled(isAkgValid);

    // AKY only supports 3 or 9 channels.
    const auto isAkyValid = onlyOnePsg || (psgCount == 3);
    juce::String akyWarning;
    if (!isAkyValid) {
        akyWarning = juce::translate("AKY only supports 3 or 9 channels.");
    } else if (areSidUsed) {
        akyWarning = juce::translate("SID is not supported by this format.");
    } else if (psgCount == 3) {
        akyWarning = juce::translate("Don't forget to connect your PlayCity.");
    }
    akyWarningLabel.setText(akyWarning, juce::NotificationType::dontSendNotification);
    akyToggleButton.setEnabled(isAkyValid);

    // To improve: right now, not possible to have 9 channels and digidrums.
    akyDeterminedPlayerType = {};
    if (isAkyValid) {
        if (psgCount == 3) {
            akyDeterminedPlayerType = PlayerType::akyMultiPsg9Channels;
        } else if (areDigidrumsUsed) {
            akyDeterminedPlayerType = PlayerType::akyMultiPsgDigidrums;
        } else {
            akyDeterminedPlayerType = PlayerType::aky;
        }
    }

    // AKY SID.
    const auto isAkySidValid = (psgCount == 1) && areSidUsed;
    juce::String akySidWarning;
    if (!onlyOnePsg) {
        akySidWarning = juce::translate("The SID player only supports 3 channels.");
    } else if (areDigidrumsUsed) {
        akySidWarning = juce::translate("Digidrums are not supported along with SID.");
    }
    akySidWarningLabel.setText(akySidWarning, juce::NotificationType::dontSendNotification);
    akySidToggleButton.setEnabled(isAkySidValid);

    // MOD.
    const auto isModValid = onlyOnePsg && result.areSampleInstrumentsUsed && !result.arePsgInstrumentsUsed;
    if (!isModValid) {
        juce::String modWarning;
        if (!onlyOnePsg) {
            modWarning = juce::translate("The MOD player only supports 3 channels.");
        } else if (arePsgInstrumentsUsed) {
            modWarning = juce::translate("MOD only supports full-samples, not PSG instruments.");
        } else if (areSampleInstrumentsUsed) {
            modWarning = juce::translate("No samples in tracks were used!");
        }
        akyModCpcOldWarningLabel.setText(modWarning, juce::NotificationType::dontSendNotification);
    }
    akyModCpcOldToggleButton.setEnabled(isModValid);

    // Tries to select the same as before.
    OptionalValue<SelectedFormat> newSelectedFormat;
    if (isAkgValid && (previouslySelectedFormat == SelectedFormat::akg)) {
        newSelectedFormat = SelectedFormat::akg;
    } else if (isAkyValid && (previouslySelectedFormat == SelectedFormat::aky)) {
        newSelectedFormat = SelectedFormat::aky;
    } else if (isAkySidValid && (previouslySelectedFormat == SelectedFormat::akySid)) {
        newSelectedFormat = SelectedFormat::akySid;
    } else if (isModValid && (previouslySelectedFormat == SelectedFormat::modCpcOld)) {
        newSelectedFormat = SelectedFormat::modCpcOld;
    }

    // Couldn't. Then, apply the first possible one.
    if (newSelectedFormat.isAbsent()) {
        if (isAkgValid) {
            newSelectedFormat = SelectedFormat::akg;
        } else if (isAkyValid) {
            newSelectedFormat = SelectedFormat::aky;
        } else if (isAkySidValid) {
            newSelectedFormat = SelectedFormat::akySid;
        } else if (isModValid) {
            newSelectedFormat = SelectedFormat::modCpcOld;
        }
    }

    // Selects the toggle.
    if (newSelectedFormat.isPresent()) {
        previouslySelectedFormat = newSelectedFormat.getValue();

        juce::ToggleButton* toggleToSelect = nullptr;
        switch (newSelectedFormat.getValue()) {
            case SelectedFormat::akg:
                toggleToSelect = &akgToggleButton;
                break;
            case SelectedFormat::aky:
                toggleToSelect = &akyToggleButton;
                break;
            case SelectedFormat::akySid:
                toggleToSelect = &akySidToggleButton;
                break;
            case SelectedFormat::modCpcOld:
                toggleToSelect = &akyModCpcOldToggleButton;
                break;
        }
        if (toggleToSelect != nullptr) {
            toggleToSelect->setToggleState(true, juce::NotificationType::dontSendNotification);
        }
    }

    setOkButtonEnable(newSelectedFormat.isPresent());
}

void QuickTestingDialog::onExportButtonClicked() noexcept
{
    OptionalValue<PlayerType> playerType;       // The possible player type.

    // Determines what player.
    if (akgToggleButton.getToggleState()) {
        playerType = PlayerType::akg;
    } else if (akyToggleButton.getToggleState() && akyDeterminedPlayerType.isPresent()) {
        playerType = akyDeterminedPlayerType;
    } else if (akySidToggleButton.getToggleState()) {
        playerType = PlayerType::akySid;
    } else if (akyModCpcOldToggleButton.getToggleState()) {
        playerType = PlayerType::modCpcOld;
    }

    if (playerType.isAbsent()) {
        jassertfalse;       // Should have been prevented by the UI.
        return;
    }

    // DSK or SNA?
    const auto targetToggledIndexes = exportTargetGroup.getToggledIndexes();
    const auto toDsk = targetToggledIndexes.find(targetIndexDsk) != targetToggledIndexes.cend();

    // Opens the file picker.
    const auto fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::any,
        toDsk ? FileExtensions::dskExtensionWithoutDot : FileExtensions::snaExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }
    const auto targetFile = juce::File(fileToSaveTo);

    // Stores the data.
    previouslySelectedTargetToIndex = toDsk ? targetIndexDsk : targetIndexSna;
    if (akgToggleButton.getToggleState()) {
        previouslySelectedFormat = SelectedFormat::akg;
    } else if (akyToggleButton.getToggleState()) {
        previouslySelectedFormat = SelectedFormat::aky;
    } else if (akySidToggleButton.getToggleState()) {
        previouslySelectedFormat = SelectedFormat::akySid;
    } else if (akyModCpcOldToggleButton.getToggleState()) {
        previouslySelectedFormat = SelectedFormat::modCpcOld;
    };

    // Performs the operation.
    auto quickTestingTask = std::make_unique<QuickTesting>(song, toDsk, playerType.getValue(), targetFile);
    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<bool>>>(
        juce::translate("Exporting..."),
        juce::translate("Please wait"),
        *this,
        std::move(quickTestingTask));
    backgroundTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void QuickTestingDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<bool> result) noexcept
{
    backgroundTask.reset();

    if (taskOutputState == TaskOutputState::canceled) {
        listener();
        return;
    }

    const auto success = (taskOutputState == TaskOutputState::finished) && (*result);
    alertDialog = success ? SuccessOrErrorDialog::buildForSuccess(juce::translate("Export successful!"), [&] { listener(); }) :
                            SuccessOrErrorDialog::buildForError(juce::translate("Export failed!"), [&] { listener(); });
}

}       // namespace arkostracker