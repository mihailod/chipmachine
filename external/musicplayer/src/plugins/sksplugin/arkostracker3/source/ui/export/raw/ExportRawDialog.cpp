#include "ExportRawDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/events/EventsExporter.h"
#include "../../../export/raw/RawExporter.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/StringUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

ExportRawDialog::ExportRawDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to RAW"), 820, 616),
        preferences(PreferencesManager::getInstance()),
        subsongChooser(pMainController, [&](const std::pair<juce::String, Id>& nameAndId) { onSelectedSubsongChanged(nameAndId.first); }),
        exportAs(false, juce::String(), false),
        sampleExport(),
        backgroundTask(),
        fileToSaveTo(),
        exportAsResult(),
        failureDialog(),
        saveSourceOrBinary(),
        encodeSongSubsongMetadataToggleButton(juce::translate("Encode song/subsong metadata")),
        encodeReferenceTablesToggleButton(juce::translate("Encode reference tables")),
        encodeSpeedTracksToggleButton(juce::translate("Encode speed tracks")),
        encodeEventTracksToggleButton(juce::translate("Encode event tracks")),
        encodeInstrumentsToggleButton(juce::translate("Encode instruments")),
        encodeArpeggiosToggleButton(juce::translate("Encode arpeggios")),
        encodePitchesToggleButton(juce::translate("Encode pitches")),
        encodeEffectsToggleButton(juce::translate("Encode effects")),
        encodeEmptyLinesAsRleToggleButton(juce::translate("Encode empty lines as RLE")),
        encodeTranspositionsInLinkerToggleButton(juce::translate("Encode transpositions in linker")),
        encodeHeightsInLinkerToggleButton(juce::translate("Encode heights in linker")),
        pitchTrackRatioLabel(juce::String(), juce::translate("Pitch track ratio")),
        pitchTrackRatioTextEditor()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    constexpr auto exportAsWidth = 390;
    const auto columnWidth = width / 3;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);

    const auto column1X = margins;
    const auto column2X = margins + columnWidth;
    const auto column3X = margins + (columnWidth * 2);
    auto y = subsongChooser.getBottom() + (margins * 2);
    encodeSongSubsongMetadataToggleButton.setBounds(column1X, y, columnWidth, labelsHeight);
    encodeReferenceTablesToggleButton.setBounds(column2X, y, columnWidth, labelsHeight);
    encodeSpeedTracksToggleButton.setBounds(column3X, y, columnWidth, labelsHeight);
    y += labelsHeight;
    encodeEventTracksToggleButton.setBounds(column1X, y, columnWidth, labelsHeight);
    encodeInstrumentsToggleButton.setBounds(column2X, y, columnWidth, labelsHeight);
    encodeArpeggiosToggleButton.setBounds(column3X, y, columnWidth, labelsHeight);
    y += labelsHeight;
    encodePitchesToggleButton.setBounds(column1X, y, columnWidth, labelsHeight);
    encodeEffectsToggleButton.setBounds(column2X, y, columnWidth, labelsHeight);
    encodeEmptyLinesAsRleToggleButton.setBounds(column3X, y, columnWidth, labelsHeight);
    y += labelsHeight;
    encodeTranspositionsInLinkerToggleButton.setBounds(column1X, y, columnWidth, labelsHeight);
    encodeHeightsInLinkerToggleButton.setBounds(column2X, y, columnWidth, labelsHeight);
    y += labelsHeight + margins;
    pitchTrackRatioLabel.setBounds(column1X, y, 140, labelsHeight);
    pitchTrackRatioTextEditor.setBounds(pitchTrackRatioLabel.getRight(), pitchTrackRatioLabel.getY(), 50, labelsHeight);
    y += labelsHeight;

    exportAs.setBounds(left, y + margins, exportAsWidth, ExportAs::desiredHeight);
    sampleExport.setBounds(exportAs.getRight() + (margins / 2), exportAs.getY(), width - exportAsWidth - (margins / 2), SampleExport::componentDesiredHeight);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(exportAs);
    addComponentToModalDialog(sampleExport);
    addComponentToModalDialog(encodeSongSubsongMetadataToggleButton);
    addComponentToModalDialog(encodeReferenceTablesToggleButton);
    addComponentToModalDialog(encodeSpeedTracksToggleButton);
    addComponentToModalDialog(encodeEventTracksToggleButton);
    addComponentToModalDialog(encodeInstrumentsToggleButton);
    addComponentToModalDialog(encodeArpeggiosToggleButton);
    addComponentToModalDialog(encodePitchesToggleButton);
    addComponentToModalDialog(encodeEffectsToggleButton);
    addComponentToModalDialog(encodeEmptyLinesAsRleToggleButton);
    addComponentToModalDialog(encodeTranspositionsInLinkerToggleButton);
    addComponentToModalDialog(encodeHeightsInLinkerToggleButton);
    addComponentToModalDialog(pitchTrackRatioLabel);
    addComponentToModalDialog(pitchTrackRatioTextEditor);

    // Sets the Sample UI part from the Preferences.
    const auto sampleEncoderFlags = preferences.getSampleEncoderFlags();
    sampleExport.setValuesAndRefreshUi(sampleEncoderFlags);

    // Sets their value from the object in the Preferences.
    const auto rawExporterFlags = PreferencesManager::getInstance().getRawExportFlags();
    encodeSongSubsongMetadataToggleButton.setToggleState(rawExporterFlags.mustEncodeSongAndSubsongMetadata(), juce::NotificationType::dontSendNotification);
    encodeReferenceTablesToggleButton.setToggleState(rawExporterFlags.mustEncodeReferenceTables(), juce::NotificationType::dontSendNotification);
    encodeSpeedTracksToggleButton.setToggleState(rawExporterFlags.mustEncodeSpeedTrack(), juce::NotificationType::dontSendNotification);
    encodeEventTracksToggleButton.setToggleState(rawExporterFlags.mustEncodeEventTrack(), juce::NotificationType::dontSendNotification);
    encodeInstrumentsToggleButton.setToggleState(rawExporterFlags.mustEncodeInstruments(), juce::NotificationType::dontSendNotification);
    encodeArpeggiosToggleButton.setToggleState(rawExporterFlags.mustEncodeArpeggioTables(), juce::NotificationType::dontSendNotification);
    encodePitchesToggleButton.setToggleState(rawExporterFlags.mustEncodePitchTables(), juce::NotificationType::dontSendNotification);
    encodeEffectsToggleButton.setToggleState(rawExporterFlags.mustEncodeEffects(), juce::NotificationType::dontSendNotification);
    encodeEmptyLinesAsRleToggleButton.setToggleState(rawExporterFlags.mustEncodeRleForEmptyLines(), juce::NotificationType::dontSendNotification);
    encodeTranspositionsInLinkerToggleButton.setToggleState(rawExporterFlags.mustEncodeTranspositions(), juce::NotificationType::dontSendNotification);
    encodeHeightsInLinkerToggleButton.setToggleState(rawExporterFlags.mustEncodeHeight(), juce::NotificationType::dontSendNotification);

    pitchTrackRatioTextEditor.setText(juce::String(rawExporterFlags.getPitchTrackEffectRatio()), false);
    pitchTrackRatioTextEditor.setInputRestrictions(4, StringUtil::decimalNumbers);
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportRawDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<SongExportResult> result) noexcept
{
    backgroundTask.reset();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if (taskOutputState != TaskOutputState::finished) {
        failureDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to export to RAW! Please report this."),
                                                            [&] { onFailureDialogExit(); });
        return;
    }

    // Saves to source/binary, with configuration file.
    const auto saveToBinary = !exportAsResult->isExportAsSource();
    const auto sourceConfiguration = exportAsResult->getConfiguration();

    saveSourceOrBinary = std::make_unique<SaveSourceOrBinaryDialog>(result->getSongData(), result->getSubsongData(),
                                                              fileToSaveTo, false, saveToBinary,
                                                              false, PlayerConfiguration(),
                                                              sourceConfiguration,
                                                              juce::translate("Export to RAW finished successfully!"),
                                                              juce::translate("Export to RAW failed!"),
                                                              result->getErrorReportRef(), [&](const bool success) {
                                                                  onSaveSourceDialogOkClicked(success);
                                                              });
    saveSourceOrBinary->perform();
}

// ======================================================


void ExportRawDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportRawDialog::onSelectedSubsongChanged(const juce::String& subsongName) noexcept
{
    exportAs.onSubsongChanged(subsongName);
}

void ExportRawDialog::onSaveSourceDialogOkClicked(const bool success) noexcept
{
    saveSourceOrBinary.reset();

    // If success, we can also close the base dialog.
    if (success) {
        onCancelButtonClicked();
    }
}

void ExportRawDialog::onFailureDialogExit() noexcept
{
    failureDialog.reset();
}

void ExportRawDialog::onExportButtonClicked() noexcept
{
    // Saves the data from the UI.
    exportAsResult = std::make_unique<ExportAs::Result>(exportAs.storeChanges());
    // Gets and silently corrects the pitch track effect ratio.
    const auto pitchTrackRatio = NumberUtil::correctNumber(pitchTrackRatioTextEditor.getText().getDoubleValue(), 0.01, 5.0);
    const EncodedDataFlag encodedDataFlag(
        encodeSongSubsongMetadataToggleButton.getToggleState(),
        encodeReferenceTablesToggleButton.getToggleState(),
        encodeSpeedTracksToggleButton.getToggleState(),
        encodeEventTracksToggleButton.getToggleState(),
        encodeInstrumentsToggleButton.getToggleState(),
        encodeArpeggiosToggleButton.getToggleState(),
        encodePitchesToggleButton.getToggleState(),
        encodeEffectsToggleButton.getToggleState(),
        encodeEmptyLinesAsRleToggleButton.getToggleState(),
        encodeTranspositionsInLinkerToggleButton.getToggleState(),
        encodeHeightsInLinkerToggleButton.getToggleState(),
        pitchTrackRatio
    );
    PreferencesManager::getInstance().setRawExportFlags(encodedDataFlag);
    // The same for the samples.
    const auto sampleEncoderFlags = sampleExport.getSampleEncoderFlags();
    preferences.setSampleEncoderFlags(sampleEncoderFlags);

    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::raw, exportAsResult->getExtension());
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto subsongId = subsongChooser.getSelectedSubsongId();
    const auto sourceConfiguration = exportAsResult->getConfiguration();
    const auto baseLabel = exportAsResult->getBaseLabel();
    const auto address = exportAsResult->getAddress();
    const auto song = songController.getSong();
    const ExportConfiguration exportConfiguration(sourceConfiguration, { subsongId }, baseLabel, address, sampleEncoderFlags);

    // Creates the exporter, and the Task to perform it asynchronously.
    auto exporter = std::make_unique<RawExporter>(*song, encodedDataFlag, exportConfiguration);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<SongExportResult>>>(juce::translate("Exporting..."), *this,
                                                                                                     std::move(exporter));
    backgroundTask->performTask();
}

}   // namespace arkostracker
