#include "ExportInstrumentToWavDialog.h"

#include "../../../controllers/MainController.h"
#include "../../../export/instrumentAsWav/InstrumentAsWavExporter.h"
#include "../../../song/cells/CellConstants.h"
#include "../../../utils/NoteUtil.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/StringUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../utils/TextEditorUtil.h"

namespace arkostracker
{

ExportInstrumentToWavDialog::ExportInstrumentToWavDialog(const MainController& mainController, std::function<void()> pListener) noexcept :
            ModalDialog(juce::translate("Export instrument to WAV"), 430, 450,
                        [&] { onExportButtonClicked(); },
                        [&] { onCancelButtonClicked(); },
                        true, true),
        song(mainController.getSongController().getSong()),
        listener(std::move(pListener)),
        progressTask(),
        dialog(),
        folderToSaveTo(),
        instrumentToExportLabel(juce::String(), juce::translate("Instrument to export")),
        instrumentToExportComboBox(),
        psgLabel(juce::String(), juce::translate("Psg to use")),
        psgComboBox(),
        replayFrequencyHzLabel(juce::String(), juce::translate("Replay frequency to use")),
        replayFrequencyHzComboBox(),
        baseFilenameLabel(juce::String(), juce::translate("Base filename")),
        baseFilenameEditText(),
        noteRangeLabel(juce::String(), juce::translate("Export range")),
        fromNoteComboBox(),
        toNoteComboBox(),
        minimumDurationSecondsLabel(juce::String(), juce::translate("Minimum duration in seconds for PSG instruments - optional")),
        minimumDurationSecondsEditText(),
        baseFilenameRestriction(TextEditorUtil::buildRestrictionForFilename(64)),
        minimumDurationSecondsRestriction(TextEditorUtil::buildRestrictionForInt(2)),
        psgs(),
        replayFrequenciesHz(),
        selectedInstrumentId(mainController.getSelectedInstrumentId())
{
    setOkButtonText(juce::translate("Export"));
    setOkButtonWidth(70);
    setCancelButtonText(juce::translate("Close"));

    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    constexpr auto rangeComboboxWidth = 100;
    constexpr auto replayFrequencyComboboxWidth = 100;
    constexpr auto minimumDurationSecondsEditTextWidth = 40;

    instrumentToExportLabel.setBounds(left, top, width, labelsHeight);
    instrumentToExportComboBox.setBounds(left, instrumentToExportLabel.getBottom(), width, labelsHeight);
    psgLabel.setBounds(left, instrumentToExportComboBox.getBottom() + margins * 2, width, labelsHeight);
    psgComboBox.setBounds(left, psgLabel.getBottom(), width, labelsHeight);
    replayFrequencyHzLabel.setBounds(left, psgComboBox.getBottom() + margins, width, labelsHeight);
    replayFrequencyHzComboBox.setBounds(left, replayFrequencyHzLabel.getBottom(), replayFrequencyComboboxWidth, labelsHeight);

    baseFilenameLabel.setBounds(left, replayFrequencyHzComboBox.getBottom() + margins * 2, width, labelsHeight);
    baseFilenameEditText.setBounds(left, baseFilenameLabel.getBottom(), width, labelsHeight);
    noteRangeLabel.setBounds(left, baseFilenameEditText.getBottom() + margins, width, labelsHeight);
    fromNoteComboBox.setBounds(left, noteRangeLabel.getBottom(), rangeComboboxWidth, labelsHeight);
    toNoteComboBox.setBounds(fromNoteComboBox.getRight() + margins, fromNoteComboBox.getY(), rangeComboboxWidth, labelsHeight);
    minimumDurationSecondsLabel.setBounds(left, fromNoteComboBox.getBottom() + margins, width, labelsHeight);
    minimumDurationSecondsEditText.setBounds(left, minimumDurationSecondsLabel.getBottom(), minimumDurationSecondsEditTextWidth, labelsHeight);

    addComponentToModalDialog(instrumentToExportLabel);
    addComponentToModalDialog(instrumentToExportComboBox);
    addComponentToModalDialog(psgLabel);
    addComponentToModalDialog(psgComboBox);
    addComponentToModalDialog(replayFrequencyHzLabel);
    addComponentToModalDialog(replayFrequencyHzComboBox);
    addComponentToModalDialog(baseFilenameLabel);
    addComponentToModalDialog(baseFilenameEditText);
    addComponentToModalDialog(noteRangeLabel);
    addComponentToModalDialog(fromNoteComboBox);
    addComponentToModalDialog(toNoteComboBox);
    addComponentToModalDialog(minimumDurationSecondsLabel);
    addComponentToModalDialog(minimumDurationSecondsEditText);

    baseFilenameEditText.setInputFilter(baseFilenameRestriction.get(), false);
    minimumDurationSecondsEditText.setInputFilter(&minimumDurationSecondsRestriction, false);
    instrumentToExportComboBox.addListener(this);

    fillInstrumentComboBox();
    fillPsgComboBox();
    fillReplayFrequencyComboBox();
    fillRangeComboBoxes();

    fromNoteComboBox.addListener(this);
    toNoteComboBox.addListener(this);
}

void ExportInstrumentToWavDialog::fillInstrumentComboBox() noexcept
{
    instrumentToExportComboBox.clear();

    song->performOnConstInstruments([&] (const std::vector<std::unique_ptr<Instrument>>& instruments) {
        auto index = -1;
        for (const auto& instrument : instruments) {
            if (++index == 0) {
                continue;
            }

            const auto text = NumberUtil::toHexByte(index) + ": " + instrument->getName();
            instrumentToExportComboBox.addItem(text, index);        // 0 is skipped already, so the index makes a valid ID.
        }
    });

    // Tries to select the already selected instrument.
    auto selectedInstrumentIndex = 0;
    if (selectedInstrumentId.isPresent()) {
        const auto optionalInstrumentIndex = song->getInstrumentIndex(selectedInstrumentId.getValue());
        if (optionalInstrumentIndex.isPresent() && (optionalInstrumentIndex.getValue() > 0)) { // We don't want 0 to be selectable.
            selectedInstrumentIndex = optionalInstrumentIndex.getValue() - 1;       // -1 because the 0 is not present.
        }
    }

    instrumentToExportComboBox.setSelectedItemIndex(selectedInstrumentIndex, juce::NotificationType::sendNotificationSync);
}

void ExportInstrumentToWavDialog::fillPsgComboBox() noexcept
{
    psgComboBox.clear();
    psgs.clear();

    // Takes all the PSGs, removing the clones.
    std::set<Psg> uniquePsgs;
    for (const auto& subsongId : song->getSubsongIds()) {
        song->performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            const auto subsongPsgs = subsong.getPsgs();
            uniquePsgs.insert(subsongPsgs.cbegin(), subsongPsgs.cend());
        });
    }

    psgs.insert(psgs.begin(), uniquePsgs.cbegin(), uniquePsgs.cend());

    auto psgId = offsetPsgId;
    for (const auto& psg : psgs) {
        const auto text = PsgTypeUtil::psgTypeToDisplayedText(psg.getType()) + ", frequency: " + juce::String(psg.getPsgFrequency())
            + " Hz, reference frequency: " + juce::String(psg.getReferenceFrequency()) + " Hz";
        psgComboBox.addItem(text, psgId);

        ++psgId;
    }

    psgComboBox.setSelectedItemIndex(0);
}

void ExportInstrumentToWavDialog::fillReplayFrequencyComboBox() noexcept
{
    replayFrequencyHzComboBox.clear();
    replayFrequenciesHz.clear();

    // Takes all the replay frequencies, removing the clones.
    std::set<float> uniqueReplayFrequencies;
    for (const auto& subsongId : song->getSubsongIds()) {
        song->performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
            uniqueReplayFrequencies.insert(subsong.getReplayFrequencyHz());
        });
    }

    replayFrequenciesHz.insert(replayFrequenciesHz.begin(), uniqueReplayFrequencies.cbegin(), uniqueReplayFrequencies.cend());

    auto id = 1;
    for (const auto& replayFrequencyHz : replayFrequenciesHz) {
        const auto text = juce::String(replayFrequencyHz) + " Hz";
        replayFrequencyHzComboBox.addItem(text, id);
        ++id;
    }

    replayFrequencyHzComboBox.setSelectedItemIndex(0);
}

void ExportInstrumentToWavDialog::fillRangeComboBoxes() noexcept
{
    fillRangeComboBox(juce::translate("From "), fromNoteComboBox);
    fillRangeComboBox(juce::translate("to "), toNoteComboBox);

    constexpr auto selectedNote = 4 * 12;       // Arbitrary.
    fromNoteComboBox.setSelectedItemIndex(selectedNote);
    toNoteComboBox.setSelectedItemIndex(selectedNote);
}

void ExportInstrumentToWavDialog::fillRangeComboBox(const juce::String& prefix, juce::ComboBox& comboBox) noexcept
{
    for (auto noteIndex = CellConstants::minimumNote; noteIndex <= CellConstants::maximumNote; ++noteIndex) {
        const auto text = prefix + NoteUtil::getStringFromNote(noteIndex);
        comboBox.addItem(text, noteIndex + offsetRangeId);
    }
}

void ExportInstrumentToWavDialog::onExportButtonClicked() noexcept
{
    if (selectedInstrumentId.isAbsent()) {
        jassertfalse;       // Not supposed to happen.
        return;
    }
    const auto instrumentId = selectedInstrumentId.getValue();

    // Opens the file picker.
    FileChooserCustom fileChooser(juce::translate("Select a folder"), FolderContext::anyExport);
    const auto success = fileChooser.browseForDirectory();
    if (!success) {
        return;     // Cancel.
    }
    folderToSaveTo = fileChooser.getResult();
    if (folderToSaveTo.getFullPathName().isEmpty()) {
        return;
    }

    auto baseOutputFile = baseFilenameEditText.getText();
    if (baseOutputFile.isEmpty()) {
        baseOutputFile = "out";
    }
    const auto firstNote = fromNoteComboBox.getSelectedId() - offsetRangeId;
    const auto lastNote = toNoteComboBox.getSelectedId() - offsetRangeId;
    const auto selectedPsg = psgs.at(static_cast<size_t>(psgComboBox.getSelectedItemIndex()));
    const auto selectedReplayFrequencyHz = replayFrequenciesHz.at(static_cast<size_t>(replayFrequencyHzComboBox.getSelectedItemIndex()));
    auto intSuccess = true;
    auto minimumDurationMs = StringUtil::stringToInt(minimumDurationSecondsEditText.getText(), intSuccess) * 1000;
    if (!intSuccess) {
        minimumDurationMs = 0;      // Happens when empty.
    }
    const OptionalInt maximumSize;       // No maximum size from UI.

    // Simplification, not sure if it is useful to bother for more.
    const auto sidPlayerCapability = song->getSubsongMetadata(song->getFirstSubsongId()).getSidPlayerCapability();

    auto exporterTask = std::make_unique<InstrumentAsWavExporter>(song, instrumentId, folderToSaveTo, baseOutputFile,
        firstNote, lastNote, selectedPsg, selectedReplayFrequencyHz, sidPlayerCapability, minimumDurationMs, maximumSize);

    progressTask = std::make_unique<BackgroundTaskWithProgress<std::unique_ptr<std::vector<juce::File>>>>(juce::translate("Exporting..."),
        *this, std::move(exporterTask));
    progressTask->performTask();
}

void ExportInstrumentToWavDialog::onCancelButtonClicked() const noexcept
{
    exit();
}

void ExportInstrumentToWavDialog::closeShownDialog() const noexcept
{
    exit();
}

void ExportInstrumentToWavDialog::exit() const noexcept
{
    listener();
}


// juce::ComboBox::Listener method implementations.
// ======================================================

void ExportInstrumentToWavDialog::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &instrumentToExportComboBox) {
        const auto instrumentId = instrumentToExportComboBox.getSelectedId();

        // Updates the base file name.
        selectedInstrumentId = song->getInstrumentId(instrumentId);
        jassert(selectedInstrumentId.isPresent());      // How is this possible?
        if (selectedInstrumentId.isPresent()) {
            song->performOnConstInstrument(selectedInstrumentId.getValue(), [&](const Instrument& instrument) {
                const auto rawName = instrument.getName();
                const auto normalizedName = baseFilenameRestriction->filterNewText(baseFilenameEditText, rawName);   // Strange API...
                baseFilenameEditText.setText(normalizedName);
            });
        }
    } else if (comboBoxThatHasChanged == &fromNoteComboBox) {
        // Makes sure the other is higher.
        const auto fromId = fromNoteComboBox.getSelectedId();
        if (toNoteComboBox.getSelectedId() < fromId) {
            toNoteComboBox.setSelectedId(fromId);
        }
    } else if (comboBoxThatHasChanged == &toNoteComboBox) {
        // Makes sure the other is lower.
        const auto toId = toNoteComboBox.getSelectedId();
        if (fromNoteComboBox.getSelectedId() > toId) {
            fromNoteComboBox.setSelectedId(toId);
        }
    }
}


// BackgroundTaskListener method implementations.
// ======================================================

void ExportInstrumentToWavDialog::onBackgroundTaskFinished(const TaskOutputState taskOutputState, const std::unique_ptr<std::vector<juce::File>> result) noexcept
{
    const auto generatedFileCountSuccess = (result != nullptr) && (result->size() > 0);

    progressTask->clear();

    if (taskOutputState == TaskOutputState::canceled) {
        return;
    }
    if (generatedFileCountSuccess && (taskOutputState == TaskOutputState::finished)) {
        dialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Export to WAV finished!"), [&] { exit(); });
    } else {
        dialog = SuccessOrErrorDialog::buildForError(juce::translate("An error occurred while exporting to WAV!"), [&] { closeShownDialog(); });
    }
}

}   // namespace arkostracker
