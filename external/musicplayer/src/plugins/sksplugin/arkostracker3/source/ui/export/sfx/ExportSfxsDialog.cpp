#include "ExportSfxsDialog.h"

#include "../../../controllers/AudioController.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/PlayerController.h"
#include "../../../export/ExportConfiguration.h"
#include "../../../export/sfx/SfxExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/NoteUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

// SfxItem method implementations.
// ===================================================

ExportSfxsDialog::SfxItem::SfxItem(std::unique_ptr<juce::Label> pInstrumentLabel,
                                   std::unique_ptr<juce::TextButton> pNoteButton, std::unique_ptr<juce::ToggleButton> pExportedToggle,
                                   std::unique_ptr<juce::Label> pOutputInstrumentIndexLabel) :
        instrumentLabel(std::move(pInstrumentLabel)),
        noteButton(std::move(pNoteButton)),
        exportedToggle(std::move(pExportedToggle)),
        outputInstrumentIndexLabel(std::move(pOutputInstrumentIndexLabel))
{
}

const std::unique_ptr<juce::Label>& ExportSfxsDialog::SfxItem::getInstrumentLabel() const noexcept
{
    return instrumentLabel;
}

const std::unique_ptr<juce::TextButton>& ExportSfxsDialog::SfxItem::getNoteButton() const noexcept
{
    return noteButton;
}

const std::unique_ptr<juce::ToggleButton>& ExportSfxsDialog::SfxItem::getExportedToggle() const noexcept
{
    return exportedToggle;
}

const std::unique_ptr<juce::Label>& ExportSfxsDialog::SfxItem::getOutputInstrumentIndexLabel() const noexcept
{
    return outputInstrumentIndexLabel;
}


// ===================================================

ExportSfxsDialog::ExportSfxsDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ModalDialog(juce::translate("Export sound effects (AKX)"), 500, 640,
                    [&] { onSaveButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                    true, true),
        playerController(pMainController.getPlayerController()),
        songController(pMainController.getSongController()),
        listener(std::move(pListener)),
        instrumentToFirstNote(FirstNotePerInstrumentFinder::find(*songController.getSong())),    // Extracts the sfx data.
        sfxViewedItems(),

        exportButton(juce::String("Export")),
        sfxGroup(juce::translate("Sound effects")),
        exportAsDialog(false, FileExtensions::akxExtensionWithoutDot),

        instrumentNameLegend(juce::String(), juce::String("Instrument")),
        noteLegend(juce::String(), juce::String("Found note")),
        exportedLegend(juce::String(), juce::String("Exported?")),
        outputInstrumentIndexLegend(juce::String(), juce::String("Output index")),
        separator(sfxGroup.findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground)).contrasting(0.3F)),

        testParametersLabel(juce::String(), juce::translate("Play test parameter:")),
        volumeComboBox(),
        stopButton(juce::translate("Stop"), juce::translate("Stop all the sounds")),

        exportAsResult(),
        fileToSaveTo(),
        backgroundTask(),
        failureDialog(),
        saveSourceOrBinary()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto stopButtonWidth = 80;

    exportButton.setBounds(left, getButtonsY(), 80, getButtonsHeight());
    exportButton.onClick = [&] { onExportButtonClicked(); };

    sfxGroup.setBounds(left, top, width, 200);

    testParametersLabel.setBounds(left, sfxGroup.getBottom() + margins, 160, labelsHeight);
    volumeComboBox.setBounds(testParametersLabel.getRight() + (margins * 2), testParametersLabel.getY(), 130, labelsHeight);
    stopButton.setBounds(width - stopButtonWidth, volumeComboBox.getY(), stopButtonWidth, labelsHeight);
    stopButton.onClick = [&] { onStopButtonClicked(); };

    exportAsDialog.setBounds(left, testParametersLabel.getBottom() + margins, width, ExportAs::desiredHeight);

    // Fills the volume ComboBox.
    for (auto i = 15; i >= 0; --i) {
        volumeComboBox.addItem(juce::translate("Volume: ") + juce::String(i), i + 1);      // Juce doesn't allow index 0.
    }
    volumeComboBox.setSelectedId(15 + 1);         // Selects volume 15.

    addComponentToModalDialog(exportButton);
    addComponentToModalDialog(exportAsDialog);
    addComponentToModalDialog(sfxGroup);
    addComponentToModalDialog(testParametersLabel);
    addComponentToModalDialog(volumeComboBox);
    addComponentToModalDialog(stopButton);

    createSoundEffectRawItems();
    // Fills the UI with relevant data.
    updateUiFromData();

    playerController.stopPlaying();
}

void ExportSfxsDialog::onSaveButtonClicked() noexcept
{
    exportAsDialog.storeChanges();
    storeSfxData();

    exit();
}

void ExportSfxsDialog::onCancelButtonClicked() const noexcept
{
    exit();
}

void ExportSfxsDialog::exit() const noexcept
{
    playerController.stopPlaying();

    listener();
}

void ExportSfxsDialog::createSoundEffectRawItems() noexcept
{
    jassert(sfxViewedItems.empty());

    constexpr auto left = 0;
    auto y = 0;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto itemsHeight = labelsHeight + (margins / 2);

    constexpr auto instrumentNameLegendWidth = 150;
    constexpr auto noteNameLegendWidth = 100;
    constexpr auto exportedLegendWidth = 100;
    constexpr auto outputIndexLegendWidth = 100;
    constexpr auto separatorHeight = 1;

    constexpr auto noteNameWidth = noteNameLegendWidth - 30;
    constexpr auto exportedWidth = exportedLegendWidth - 50;

    // First, displays the legend.
    instrumentNameLegend.setBounds(left, y, instrumentNameLegendWidth, labelsHeight);
    noteLegend.setBounds(instrumentNameLegend.getRight(), y, noteNameLegendWidth, labelsHeight);
    exportedLegend.setBounds(noteLegend.getRight(), y, exportedLegendWidth, labelsHeight);
    outputInstrumentIndexLegend.setBounds(exportedLegend.getRight(), y, outputIndexLegendWidth, labelsHeight);
    y += labelsHeight;
    separator.setBounds(left, y, sfxGroup.getGroupInnerArea().getWidth(), separatorHeight);
    y = separator.getBottom() + margins;

    instrumentNameLegend.setJustificationType(juce::Justification::horizontallyCentred);
    outputInstrumentIndexLegend.setJustificationType(juce::Justification::horizontallyCentred);

    sfxGroup.addComponentToGroup(instrumentNameLegend);
    sfxGroup.addComponentToGroup(noteLegend);
    sfxGroup.addComponentToGroup(exportedLegend);
    sfxGroup.addComponentToGroup(outputInstrumentIndexLegend);
    sfxGroup.addComponentToGroup(separator);

    // Builds the SFX list, and displays it. No data is displayed for now (except the non-dynamic ones).
    auto itemIndex = 0;
    for (const auto& [instrumentIndex, item] : instrumentToFirstNote) {
        const auto localInstrumentIndex = instrumentIndex;

        // Creates each buttons of a line.
        const auto instrumentName = songController.getInstrumentNameFromIndex(instrumentIndex);
        auto instrumentLabel = std::make_unique<juce::Label>(juce::String(), NumberUtil::toHexByte(instrumentIndex) + ": " + instrumentName);
        auto noteButton = std::make_unique<juce::TextButton>();
        noteButton->onClick = [&, localInstrumentIndex] {
            onPlayButtonClicked(localInstrumentIndex);
        };

        auto exportedToggle = std::make_unique<juce::ToggleButton>();
        exportedToggle->onClick = [&, localInstrumentIndex] {
            onExportedToggled(localInstrumentIndex);
        };

        auto outputInstrumentIndexLabel = std::make_unique<juce::Label>();
        outputInstrumentIndexLabel->setJustificationType(juce::Justification::horizontallyCentred);

        // Locates the items in the group.
        instrumentLabel->setBounds(instrumentNameLegend.getX(), y, instrumentNameLegendWidth, labelsHeight);
        noteButton->setBounds(noteLegend.getX() + ((noteNameLegendWidth - noteNameWidth) / 3), y, noteNameWidth, labelsHeight);   // Why 3 instead of 2... ?
        exportedToggle->setBounds(exportedLegend.getX() + ((exportedLegendWidth - exportedWidth) / 2), y, exportedWidth, labelsHeight);
        outputInstrumentIndexLabel->setBounds(outputInstrumentIndexLegend.getX(), y, outputIndexLegendWidth, labelsHeight);

        sfxGroup.addComponentToGroup(*instrumentLabel);
        sfxGroup.addComponentToGroup(*noteButton);
        sfxGroup.addComponentToGroup(*exportedToggle);
        sfxGroup.addComponentToGroup(*outputInstrumentIndexLabel);

        sfxViewedItems.emplace_back(std::move(instrumentLabel), std::move(noteButton), std::move(exportedToggle), std::move(outputInstrumentIndexLabel));

        y += itemsHeight;
        ++itemIndex;
    }
}

void ExportSfxsDialog::onExportedToggled(const int instrumentIndex) noexcept
{
    // Changes the data.
    const auto it = instrumentToFirstNote.find(instrumentIndex);
    if (it != instrumentToFirstNote.cend()) {
        const auto result = it->second;
        const auto newResult = FirstNotePerInstrumentFinder::ResultItem(result.getFoundNote(), !result.isExported());
        it->second = newResult;

        updateUiFromData();
    } else {
        jassertfalse;       // Not found, abnormal!
    }
}

void ExportSfxsDialog::updateUiFromData() noexcept
{
    jassert(instrumentToFirstNote.size() == sfxViewedItems.size());     // MUST be the same!

    // Builds the SFX list, and displays it.
    auto outputExportedIndex = 1;
    auto uiItemIndex = 0U;
    auto atLeastOnlyExport = false;
    for (const auto& [instrumentIndex, item] : instrumentToFirstNote) {
        auto& uiItem = sfxViewedItems.at(uiItemIndex);

        const auto foundNote = item.getFoundNote();
        const auto found = foundNote.isPresent();
        const auto exported = item.isExported();
        const auto canBeUsed = item.canUseUsed();

        atLeastOnlyExport = atLeastOnlyExport || canBeUsed;

        // Sets the data.
        const auto& nameLabel = uiItem.getInstrumentLabel();
        nameLabel->setEnabled(found);

        const auto& noteButton = uiItem.getNoteButton();
        noteButton->setButtonText(found ? NoteUtil::getStringFromNote(foundNote.getValue()) : "Unused");
        noteButton->setEnabled(found);

        const auto& exportedToggle = uiItem.getExportedToggle();
        // May be exported but not found. The UI looks better as unchecked if not exported and/not found.
        exportedToggle->setToggleState(canBeUsed, juce::NotificationType::dontSendNotification);
        exportedToggle->setEnabled(found);

        const auto& outputInstrumentIndexLabel = uiItem.getOutputInstrumentIndexLabel();
        outputInstrumentIndexLabel->setText(canBeUsed ? NumberUtil::toHexByte(outputExportedIndex) : "--", juce::NotificationType::dontSendNotification);
        outputInstrumentIndexLabel->setEnabled(canBeUsed);

        if (found && exported) {
            ++outputExportedIndex;
        }
        ++uiItemIndex;
    }

    exportButton.setEnabled(atLeastOnlyExport);     // If nothing to export, don't allow it.
}

void ExportSfxsDialog::onStopButtonClicked() const noexcept
{
    playerController.stopPlaying();
}

void ExportSfxsDialog::onPlayButtonClicked(const int instrumentIndex) noexcept
{
    if (const auto it = instrumentToFirstNote.find(instrumentIndex); it != instrumentToFirstNote.cend()) {
        const auto& result = it->second;
        const auto note = result.getFoundNote();
        jassert(note.isPresent());
        if (note.isPresent()) {
            CellEffects effects;
            effects.addEffect(Effect::reset, volumeComboBox.getSelectedItemIndex()); // Trick, the index is reversed to the volume, filling the inverted volume!
            const Cell cell(Note::buildNote(note.getValue()), instrumentIndex, effects);
            constexpr auto channelIndex = 1;
            const auto subsongId = songController.getSong()->getFirstSubsongId();
            playerController.playFromEditor(cell, CellLocationInPosition(subsongId, 0, 0, channelIndex));
        }
    } else {
        jassertfalse;       // Not found, abnormal!
    }
}

void ExportSfxsDialog::onExportButtonClicked() noexcept
{
    exportAsResult = std::make_unique<ExportAs::Result>(exportAsDialog.storeChanges());
    storeSfxData();

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::akx, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const auto subsongIds = song->getSubsongIds();
    const ExportConfiguration exportConfiguration(sourceConfiguration, subsongIds, baseLabel, address);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto exporterTask = std::make_unique<SfxExporter>(*song, exportConfiguration);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."),
            *this, std::move(exporterTask));
    backgroundTask->performTask();
}

void ExportSfxsDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}

void ExportSfxsDialog::onSaveSourceDialogOkClicked(const bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportSfxsDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if ((taskOutputState != TaskOutputState::finished)) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to AKX! Please report this."),
                                                            [&] { onFailureDialogExit(); });
        return;
    }

    const auto& playerConfiguration = result->getPlayerConfigurationRef();

    // Saves to source/binary, with configuration file.
    const auto saveToBinary = !exportAsResult->isExportAsSource();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto exportPlayerConfiguration = exportAsResult->getExportConfigurationFile();
    const auto exportToSeveralFiles = exportAsResult->isExportToSeveralFiles();

    saveSourceOrBinary = std::make_unique<SaveSourceOrBinaryDialog>(result->getSongData(), result->getSubsongData(), fileToSaveTo,
                                                              exportToSeveralFiles, saveToBinary, exportPlayerConfiguration, playerConfiguration,
                                                              sourceConfiguration,
                                                              juce::translate("Export to AKX finished successfully!"), juce::translate("Export to AKX failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) { onSaveSourceDialogOkClicked(success); });
    saveSourceOrBinary->perform();
}


// ======================================================

void ExportSfxsDialog::storeSfxData() noexcept
{
    // Gathers the data.
    std::unordered_set<int> exportedInstrumentIndexes;
    for (const auto& [instrumentIndex, result] : instrumentToFirstNote) {
        if (result.isExported()) {
            exportedInstrumentIndexes.insert(instrumentIndex);
        }
    }

    // Saves them.
    songController.setExportedSoundEffects(exportedInstrumentIndexes);
}

}   // namespace arkostracker
