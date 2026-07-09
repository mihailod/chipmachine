#include "QuickExportAkgDialog.h"

#include "../../../controllers/MainController.h"
#include "../../../export/akg/AkgExporter.h"
#include "../../../export/quickExport/QuickAkgExport.h"
#include "../../../export/sfx/SfxExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/NumberUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../common/task/CompileSource.h"
#include "../sfx/ExportSfxsDialog.h"

namespace arkostracker
{
QuickExportAkgDialog::QuickExportAkgDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ModalDialog(juce::translate("Quick export to AKG"), 490, 290,
                                    [&] { onExportButtonClicked(); },
                                    [&] { onCancelButtonClicked(); },
                                    true, true),
        song(pMainController.getSongController().getSong()),
        listener(std::move(pListener)),
        topLabel(juce::String(), juce::translate("Export to AKG in a DSK, with interruptions, and sound effects if wanted.")),
        sizePlayerWithoutSfx(),
        sizePlayerWithSfx(),
        playerAddress(defaultPlayerAddress),
        musicAddress(),
        musicSize(),
        sfxAddress(),
        startLabel(juce::String(), juce::translate("Start")),
        sizeLabel(juce::String(), juce::translate("Size")),
        endLabel(juce::String(), juce::translate("End")),
        playerAddressSlider(*this, juce::translate("Player"), 4, 1, 16, SliderIncDec::Filter::hexadecimal, 80),
        playerSizeLabel(),
        playerEndLabel(),
        musicLabel(juce::String(), juce::translate("Music")),
        musicAddressLabel(),
        musicSizeLabel(),
        musicEndLabel(),
        sfxLabel(juce::String(), juce::translate("Sfx")),
        sfxAddressLabel(),
        sfxSizeLabel(),
        sfxEndLabel(),
        sfxButton(),
        errorLabel(),
        isError(false),
        alertDialog(),
        loadSfxTask(),
        sfxFileToLoad(),
        exportTask(),
        initTask(),
        loadSfxTaskListener(*this),
        exportTaskListener(*this),
        initTaskListener(*this)
{
    setOkButtonText(juce::translate("Export"));
    setOkButtonWidth(70);
    setCancelButtonText(juce::translate("Close"));

    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto addressLabelWidth = 60;
    constexpr auto topLabelWidth = 60;

    topLabel.setBounds(left, top, width, labelsHeight);
    startLabel.setBounds(left + 100, topLabel.getBottom() + margins, topLabelWidth, labelsHeight);
    sizeLabel.setBounds(startLabel.getRight() + (margins * 5) + 6, startLabel.getY(), topLabelWidth, labelsHeight);
    endLabel.setBounds(sizeLabel.getRight() + margins, startLabel.getY(), topLabelWidth, labelsHeight);
    playerAddressSlider.setTopLeftPosition(left, startLabel.getBottom());
    playerSizeLabel.setBounds(playerAddressSlider.getRight() + (margins * 4), startLabel.getBottom(), addressLabelWidth, labelsHeight);
    playerEndLabel.setBounds(playerSizeLabel.getRight() + margins, playerAddressSlider.getY(), addressLabelWidth, labelsHeight);

    musicLabel.setBounds(left, playerAddressSlider.getBottom() + margins, 80, labelsHeight);
    musicAddressLabel.setBounds(musicLabel.getRight(), musicLabel.getY(), addressLabelWidth, labelsHeight);
    musicSizeLabel.setBounds(playerSizeLabel.getX(), musicAddressLabel.getY(), addressLabelWidth, labelsHeight);
    musicEndLabel.setBounds(playerEndLabel.getX(), musicAddressLabel.getY(), addressLabelWidth, labelsHeight);

    sfxLabel.setBounds(left, musicLabel.getBottom() + margins, musicLabel.getWidth(), labelsHeight);
    sfxAddressLabel.setBounds(musicAddressLabel.getX(), sfxLabel.getY(), addressLabelWidth, labelsHeight);
    sfxSizeLabel.setBounds(musicSizeLabel.getX(), sfxLabel.getY(), addressLabelWidth, labelsHeight);
    sfxEndLabel.setBounds(musicEndLabel.getX(), sfxLabel.getY(), addressLabelWidth, labelsHeight);
    sfxButton.setBounds(sfxEndLabel.getRight() + (margins * 2), sfxLabel.getY(), 80, labelsHeight);

    const auto errorLabelHeight = labelsHeight * 2;
    errorLabel.setBounds(left, getButtonsY() - errorLabelHeight - margins, width, errorLabelHeight);

    for (auto* label : {
        &startLabel, &sizeLabel, &endLabel,
        &playerSizeLabel, &playerEndLabel,
        &musicAddressLabel, &musicSizeLabel, &musicEndLabel,
        &sfxAddressLabel, &sfxSizeLabel, &sfxEndLabel,
    }) {
        label->setJustificationType(juce::Justification::centredTop);
    }
    errorLabel.setColour(juce::Label::ColourIds::textColourId, juce::Colours::red);
    sfxButton.onClick = [&] { onSfxButtonClicked(); };

    addComponentToModalDialog(topLabel);
    addComponentToModalDialog(startLabel);
    addComponentToModalDialog(sizeLabel);
    addComponentToModalDialog(endLabel);
    addComponentToModalDialog(playerAddressSlider);
    addComponentToModalDialog(playerSizeLabel);
    addComponentToModalDialog(playerEndLabel);
    addComponentToModalDialog(musicLabel);
    addComponentToModalDialog(musicAddressLabel);
    addComponentToModalDialog(musicSizeLabel);
    addComponentToModalDialog(musicEndLabel);
    addComponentToModalDialog(sfxLabel);
    addComponentToModalDialog(sfxAddressLabel);
    addComponentToModalDialog(sfxSizeLabel);
    addComponentToModalDialog(sfxEndLabel);
    addComponentToModalDialog(sfxButton);
    addComponentToModalDialog(errorLabel);

    setOkButtonEnable(false);       // Set to on after the initial task is successful.

    // Creates a task to assemble the players and song.
    auto init = std::make_unique<InitTask>(*this, song);
    initTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<bool>>>(
        juce::translate("Initializing"), juce::translate("Please wait..."), initTaskListener, std::move(init));
    initTask->performTask();
}


// InitTask method implementations.
// ======================================================

QuickExportAkgDialog::InitTask::InitTask(QuickExportAkgDialog& pParent, std::shared_ptr<const Song> pSong) noexcept :
        WithParent(pParent),
        song(std::move(pSong))
{
}

std::pair<bool, std::unique_ptr<bool>> QuickExportAkgDialog::InitTask::performTask() noexcept
{
    auto success = true;

    // Quick assemble of the players to know their size.
    const auto [successWithoutSfx, playerBinaryWithoutSfx] = QuickAkgPlayerExport(0, false).performTask();
    if (successWithoutSfx && (playerBinaryWithoutSfx != nullptr)) {
        parentObject.sizePlayerWithoutSfx = static_cast<int>(playerBinaryWithoutSfx->getSize());
    } else {
        success = false;
        jassertfalse;       // Arg!
    }
    const auto [successWithSfx, playerBinaryWithSfx] = QuickAkgPlayerExport(0, true).performTask();
    if (successWithSfx && (playerBinaryWithSfx != nullptr)) {
        parentObject.sizePlayerWithSfx = static_cast<int>(playerBinaryWithSfx->getSize());
    } else {
        success = false;
        jassertfalse;       // Arg!
    }

    const auto exportConfiguration = ExportConfiguration(
        SourceGeneratorConfiguration::buildZ80(), song->getSubsongIds(), "mySong", 0
    );
    AkgExporter akgExporter(*song, exportConfiguration);
    const auto [successSong, musicResult] = akgExporter.performTask();
    if (successSong && (musicResult != nullptr)) {
        const auto musicSourceMemoryBlock = musicResult->getAggregatedData();
        const auto binaryMemoryBlock = CompileSource::compile(musicSourceMemoryBlock);
        if (binaryMemoryBlock != nullptr) {
            parentObject.musicSize = static_cast<int>(binaryMemoryBlock->getSize());
        } else {
            success = false;
            jassertfalse;       // Arg!
        }
    } else {
        success = false;
        jassertfalse;       // Arg!
    }

    return { success, std::make_unique<bool>(success) };
}


// InitTaskListener method implementations.
// ======================================================

QuickExportAkgDialog::InitTaskListener::InitTaskListener(QuickExportAkgDialog& pParent) noexcept :
        WithParent(pParent)
{
}

void QuickExportAkgDialog::InitTaskListener::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<bool> result) noexcept
{
    parentObject.initTask.reset();

    if ((taskOutputState != TaskOutputState::finished) || (result == nullptr) || !*result) {
        parentObject.alertDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to export the song! Abnormal!"),
            [&] {
                parentObject.closeAlertDialog();
                parentObject.listener();
            });
        jassertfalse;       // Should never happen!
    } else {
        parentObject.updateAndShowAddressesAndSize();
    }
}


// ======================================================

void QuickExportAkgDialog::updateAndShowAddressesAndSize() noexcept
{
    isError = false;
    juce::String errorText;

    // Corrects the player address.
    const auto isSfx = areSfxPresent();
    const auto playerSize = isSfx ? sizePlayerWithSfx : sizePlayerWithoutSfx;
    if (playerSize <= 0) {
        isError = true;     // Security, should never happen.
        jassertfalse;
    }

    playerAddress = NumberUtil::correctNumber(playerAddress, 0, 0x10000 - playerSize);

    // Sets the music address.
    if (musicSize <= 0) {
        isError = true;
        errorText = juce::translate("Unable to export the song. Make a regular export and check the report.");   // Shouldn't happen...
    } else {
        // Music must be before the player.
        musicAddress = std::max(0, playerAddress - musicSize);
        if ((musicAddress + musicSize) > playerAddress) {
            isError = true;
            errorText = juce::translate("Raise the player address! The music has no room.");
        } else {
            musicAddress = static_cast<int>(static_cast<unsigned int>(musicAddress) & addressMask);
        }
    }

    // Sets the sfx address, before the music.
    if (isSfx) {
        const auto localSfxSize = sfxSize.getValue();
        sfxAddress = std::max(0, musicAddress - localSfxSize);
        if ((sfxAddress + localSfxSize) > musicAddress) {
            isError = true;
            errorText = juce::translate("No room for the SFXs! SFX or music too long, or player too low.");
        } else {
            sfxAddress = static_cast<int>(static_cast<unsigned int>(sfxAddress) & addressMask);
        }
    }

    // Displays the whole.
    playerAddressSlider.setShownValue(playerAddress);
    const auto playerEnd = playerAddress + playerSize - 1;
    playerSizeLabel.setText(NumberUtil::toHexFourDigits(playerSize, true), juce::NotificationType::dontSendNotification);
    playerEndLabel.setText(NumberUtil::toHexFourDigits(playerEnd, true), juce::NotificationType::dontSendNotification);

    const auto musicEnd = musicAddress + musicSize - 1;
    musicAddressLabel.setText(NumberUtil::toHexFourDigits(musicAddress, true), juce::NotificationType::dontSendNotification);
    musicSizeLabel.setText(NumberUtil::toHexFourDigits(musicSize, true), juce::NotificationType::dontSendNotification);
    musicEndLabel.setText(NumberUtil::toHexFourDigits(musicEnd, true), juce::NotificationType::dontSendNotification);

    auto sfxAddressString = juce::String("----");
    auto sfxSizeString = sfxAddressString;
    auto sfxEndString = sfxAddressString;
    if (isSfx) {
        const auto localSfxSize = sfxSize.getValue();
        const auto sfxEnd = sfxAddress + localSfxSize - 1;
        sfxAddressString = NumberUtil::toHexFourDigits(sfxAddress, true);
        sfxSizeString = NumberUtil::toHexFourDigits(localSfxSize, true);
        sfxEndString = NumberUtil::toHexFourDigits(sfxEnd, true);
    }
    sfxAddressLabel.setText(sfxAddressString, juce::NotificationType::dontSendNotification);
    sfxSizeLabel.setText(sfxSizeString, juce::NotificationType::dontSendNotification);
    sfxEndLabel.setText(sfxEndString, juce::NotificationType::dontSendNotification);

    sfxButton.setButtonText(isSfx ? juce::translate("Remove") : juce::translate("Load"));

    errorLabel.setText(errorText, juce::NotificationType::dontSendNotification);
    setOkButtonEnable(!isError);
}

void QuickExportAkgDialog::onExportButtonClicked() noexcept
{
    updateAndShowAddressesAndSize();
    if (isError) {
        jassertfalse;       // Should have been detected by the business!
        return;
    }

    // Opens a dialog to select the output DSK.
    FileChooserCustom fileChooser(juce::translate("Save to DSK"),
                                  FolderContext::anyExport,
                                  FileExtensions::dskExtensionWithWildcard
    );
    const auto fileChooserSuccess = fileChooser.browseForFileToSave(true);
    if (!fileChooserSuccess) {
        return;
    }
    const auto dskFileToCreate = fileChooser.getResult();

    // Creates the task and executes it.
    auto quickExport = std::make_unique<QuickExport>(dskFileToCreate, playerAddress, musicAddress, song,
        areSfxPresent() ? sfxAddress : OptionalInt(), sfxFileToLoad);
    exportTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<bool>>>(
        juce::translate("Exporting"), juce::translate("Please wait..."), exportTaskListener, std::move(quickExport));
    exportTask->performTask();
}

void QuickExportAkgDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void QuickExportAkgDialog::onSfxButtonClicked() noexcept
{
    if (areSfxPresent()) {
        // Removes the SFX.
        sfxSize = { };
        updateAndShowAddressesAndSize();

        return;
    }

    // Opens a dialog to load the SFX.
    FileChooserCustom fileChooser(juce::translate("Load a SFX song"),
                                  FolderContext::loadOrSaveSong, FileExtensions::loadSongExtensionsFilter);
    const auto success = fileChooser.browseForFileToOpen(nullptr);
    if (!success) {
        return;
    }

    sfxFileToLoad = fileChooser.getResult();        // Not great, but relies on the file to compile it again at the end.

    // Loads and compiles the SFX. This is actually only to know its size.
    auto task = std::make_unique<QuickLoadAndCompileSfxExport>(sfxFileToLoad, 0);
    loadSfxTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryBlock>>>(
        juce::translate("Loading"), juce::translate("Please wait..."), loadSfxTaskListener, std::move(task));
    loadSfxTask->performTask();
}

void QuickExportAkgDialog::closeAlertDialog() noexcept
{
    alertDialog.reset();
}

bool QuickExportAkgDialog::areSfxPresent() const noexcept
{
    return sfxSize.isPresent();
}


// LoadSfxTaskListener method implementations.
// ======================================================

QuickExportAkgDialog::LoadSfxTaskListener::LoadSfxTaskListener(QuickExportAkgDialog& pParent) noexcept :
       WithParent(pParent)
{
}

void QuickExportAkgDialog::LoadSfxTaskListener::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<juce::MemoryBlock> result) noexcept
{
    parentObject.loadSfxTask.reset();
    parentObject.sfxSize = { };     // For now. Set below in case of a success.

    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    if ((taskOutputState == TaskOutputState::error) || (result == nullptr)) {
        parentObject.alertDialog = SuccessOrErrorDialog::buildForError(juce::translate("Loading the SFX song failed!"),
            [&] { parentObject.closeAlertDialog(); });
        return;
    }

    // Now we know the size of the SFX. Finally!
    parentObject.sfxSize = static_cast<int>(result->getSize());
    parentObject.updateAndShowAddressesAndSize();
}


// ExportTaskListener method implementations.
// ======================================================

QuickExportAkgDialog::ExportTaskListener::ExportTaskListener(QuickExportAkgDialog& pParent) noexcept :
        WithParent(pParent)
{
}

void QuickExportAkgDialog::ExportTaskListener::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<bool> result) noexcept
{
    parentObject.exportTask.reset();

    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    if ((taskOutputState == TaskOutputState::error) || (result == nullptr)) {
        parentObject.alertDialog = SuccessOrErrorDialog::buildForError(juce::translate("Export has failed!"),
            [&] { parentObject.closeAlertDialog(); });
    } else {
        parentObject.alertDialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Export successful!"),
            [&] {
                parentObject.closeAlertDialog();
                parentObject.listener();
            });
    }
}


// SliderIncDec::Listener method implementations.
// ======================================================

void QuickExportAkgDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    if (&slider == &playerAddressSlider) {
        playerAddress = static_cast<int>(value);
        updateAndShowAddressesAndSize();
    } else {
        jassertfalse;       // Not managed?
    }
}

}       // namespace arkostracker
