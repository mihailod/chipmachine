#include "ExportMidiDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../../export/ExportConfiguration.h"
#include "../../../export/midi/MidiExporter.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/FileUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"

namespace arkostracker
{

ExportMidiDialog::ExportMidiDialog(const MainController& pMainController, std::function<void()> pListener) noexcept :
        ExportDialog(pMainController, std::move(pListener), juce::translate("Export to MIDI"), 600, 530),
        subsongChooser(pMainController, [&](const std::pair<juce::String, Id>& /*nameAndId*/) { /** Nothing to do. */ }),
        instrumentGroup(juce::translate("Export...")),
        exportMarkersToggle(juce::translate("Export position markers.")),
        drumChannelLabel(juce::String(), juce::translate("Drum channel (10 is standard):")),
        drumChannelSlider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight),
        backgroundTask(),
        fileToSaveTo(),
        dialog(),
        instrumentItems()
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    subsongChooser.setBounds(left, top, width, SubsongChooser::desiredHeight);
    instrumentGroup.setBounds(left, subsongChooser.getBottom() + margins, width, 300);
    exportMarkersToggle.setBounds(left, instrumentGroup.getBottom() + margins, width, labelsHeight);
    drumChannelLabel.setBounds(left, exportMarkersToggle.getBottom() + margins, width, labelsHeight);
    drumChannelSlider.setBounds(left, drumChannelLabel.getBottom(), 200, labelsHeight);

    addComponentToModalDialog(subsongChooser);
    addComponentToModalDialog(instrumentGroup);
    addComponentToModalDialog(exportMarkersToggle);
    addComponentToModalDialog(drumChannelLabel);
    addComponentToModalDialog(drumChannelSlider);

    exportMarkersToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    drumChannelSlider.setRange(1, 16, 1);
    drumChannelSlider.setValue(10, juce::NotificationType::dontSendNotification);

    buildInstrumentGroup();
}

void ExportMidiDialog::buildInstrumentGroup() noexcept
{
    instrumentItems.clear();

    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto itemsHeight = LookAndFeelConstants::labelsHeight + margins;
    const auto maximumTrackIndex = songController.getInstrumentCount() - 1; // Removes the 0.

    auto instrumentIndex = -1;
    auto y = 0;
    songController.performOnInstruments([&] (const std::vector<std::unique_ptr<Instrument>>& instruments) {
        for (const auto& instrument : instruments) {
            ++instrumentIndex;
            if (instrumentIndex == 0) {
                continue;
            }
            const auto trackIndex = instrumentIndex - 1;

            constexpr auto nameViewWidth = 230;
            const auto name = NumberUtil::toHexByte(instrumentIndex) + juce::translate(": ") + instrument->getName();
            auto nameView = std::make_unique<juce::Label>(juce::String(), name);
            nameView->setBounds(0, y, nameViewWidth, labelsHeight);

            const auto targetViewWidth = instrumentGroup.getGroupInnerArea().getWidth() - nameViewWidth - (margins * 5);
            auto targetView = std::make_unique<juce::ComboBox>();
            targetView->setBounds(nameView->getRight() + margins, y, targetViewWidth, labelsHeight);
            fillTargetView(*targetView, trackIndex, maximumTrackIndex);

            instrumentGroup.addComponentToGroup(*nameView);
            instrumentGroup.addComponentToGroup(*targetView);

            auto instrumentItem = std::make_unique<InstrumentItem>(instrumentIndex, std::move(nameView), std::move(targetView));
            instrumentItems.push_back(std::move(instrumentItem));

            y += itemsHeight;
        }
    });
}

void ExportMidiDialog::fillTargetView(juce::ComboBox& comboBox, const int selectedTrackIndex, const int maximumTrackIndex) noexcept
{
    comboBox.addItem(juce::translate("Don't export"), idDontExport);

    for (auto trackIndex = 0; trackIndex < maximumTrackIndex; ++trackIndex) {
        comboBox.addItem(juce::translate("To track ") + juce::String(trackIndex), trackIndex + offsetIdNormalTracks);
    }

    // Then adds the drums.
    for (auto drumIndex = midiDrumsMinimumIndex; drumIndex <= midiDrumsMaximumIndex; ++drumIndex) {
        const auto* rawDrumName = juce::MidiMessage::getRhythmInstrumentName(drumIndex);
        if (rawDrumName == nullptr) {
            jassertfalse;       // Shouldn't happen!
            continue;
        }

        const auto drumName = juce::String(rawDrumName);
        comboBox.addItem(juce::translate("To drum track as ") + drumName + " (" + juce::String(drumIndex) + ")", drumIndex + offsetDrumsId);
    }

    comboBox.setSelectedId(selectedTrackIndex + offsetIdNormalTracks);
}


// ======================================================

void ExportMidiDialog::onCancelButtonClicked() const noexcept
{
    listener();
}

void ExportMidiDialog::onDialogExit() noexcept
{
    dialog.reset();

    // Don't exit the page: the user may want to test the MIDI file and make some other tests.
}

void ExportMidiDialog::onExportButtonClicked() noexcept
{
    // Opens the file picker.
    fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::midi, FileExtensions::midiExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    // Gather data from the UI.
    std::map<int, MidiExporter::InstrumentWithMidiData> instrumentIndexToMidiData;

    for (const auto& instrumentItem : instrumentItems) {
        const auto instrumentIndex = instrumentItem->instrumentIndex;
        const auto selectedTargetId = instrumentItem->target->getSelectedId();

        // Is it a drum, a normal sound, or should it be skipped?
        auto trackIndex = -1;
        auto midiDrumIndex = OptionalInt();
        if (selectedTargetId == idDontExport) {
            continue;
        }
        if (selectedTargetId >= offsetDrumsId) {
            midiDrumIndex = selectedTargetId - offsetDrumsId;
        } else {
            trackIndex = selectedTargetId - offsetIdNormalTracks;
        }

        MidiExporter::InstrumentWithMidiData const midiData(trackIndex, midiDrumIndex);

        instrumentIndexToMidiData.insert({ instrumentIndex, midiData });
    }

    const auto exportMarkers = exportMarkersToggle.getToggleState();
    const MidiExporter::ExportData exportData(instrumentIndexToMidiData, static_cast<int>(drumChannelSlider.getValue()), exportMarkers);

    // Creates the exporter, and the Task to perform it asynchronously.
    const auto subsongId = subsongChooser.getSelectedSubsongId();
    auto midiExporterTask = std::make_unique<MidiExporter>(songController, subsongId, exportData);

    backgroundTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<juce::MemoryBlock>>>(juce::translate("Exporting..."), *this,
                                                                                                        std::move(midiExporterTask));
    backgroundTask->performTask();
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportMidiDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<juce::MemoryBlock> result) noexcept
{
    backgroundTask->clear();

    // If canceled, nothing more to do.
    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }

    // Pop-up if failure (not supposed to happen, though).
    if (taskOutputState != TaskOutputState::finished) {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to convert to song to MIDI! Please report this."),
                                                            [&] { onDialogExit(); });
        return;
    }

    // Ok, not on a background thread... Oh, well. The task should have been given an output stream.
    const auto success = FileUtil::saveMemoryBlockToFile(fileToSaveTo, *result);
    if (success) {
        dialog = SuccessOrErrorDialog::buildForSuccess("Export to MIDI finished!", [&] { onDialogExit(); });
    } else {
        dialog = SuccessOrErrorDialog::buildForError("Unable to save the MIDI file.", [&] { onDialogExit(); });
    }
}

}   // namespace arkostracker
