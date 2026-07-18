#include "StreamedMusicAnalyzer.h"

#include <BinaryData.h>

#include "../../../controllers/AudioController.h"
#include "../../../reader/vgm/VgmReader.h"
#include "../../../reader/ym/YmReader.h"
#include "../../../song/subsong/SubsongConstants.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/NumberUtil.h"
#include "../../../utils/PsgValues.h"
#include "../../../utils/StringUtil.h"
#include "../../../utils/ZipHelper.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../components/dialogs/TextFieldDialog.h"
#include "../../utils/TextEditorUtil.h"
#include "InstrumentStreamExporter.h"

namespace arkostracker 
{

juce::String StreamedMusicAnalyzer::previouslyLoadedSongPath = juce::String();                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
int StreamedMusicAnalyzer::previouslyLoadedSongCurrentIteration = 0;
int StreamedMusicAnalyzer::previouslyLoadedSongLoopStartIndex = 0;
int StreamedMusicAnalyzer::previouslyLoadedSongEndIndex = 0;

const int StreamedMusicAnalyzer::refreshTimerMs = 50;
const int StreamedMusicAnalyzer::maximumPsgCount = 2;       // For now.
const int StreamedMusicAnalyzer::nowLoopDefaultDuration = 32;
const int StreamedMusicAnalyzer::exportMaxDuration = 256;

StreamedMusicAnalyzer::StreamedMusicAnalyzer(const MainController& pMainController, std::function<void()> pListener) noexcept:
        ModalDialog(juce::translate("Streamed music analyzer"), 600, 570,
                    [&] { onOkButtonClicked(); },
                    [&] { /* Never used. */ }),
        modalCallback(std::move(pListener)),

        songController(pMainController.getSongController()),
        song(*songController.getSong()),

        loadedSongPath(),

        streamedMusicReader(),
        streamedMusic(),
        currentIteration(),
        iterationCount(),
        lastMaxIteration(),
        loopStartIndex(),
        loopEndIndex(),

        audioDeviceManager(pMainController.getAudioController().getAudioDeviceManager()),
        psgStreamGenerators(),
        psgsMixer(),
        audioSourcePlayer(),

        isPlaying(),
        isLooping(),
        currentTick(),
        speed(1),
        psgCount(1),
        psgToMutedChannelIndexes(),

        exportToCsvButton(juce::translate("Export to CSV")),
        loadButton(juce::translate("Load")),
        loadOrDropFileLabel(juce::String(), juce::translate("Load a YM/VGM by drag'n'dropping a file, or via the Load Button above.")),
        musicInfoLabel(),
        playStopButton(BinaryData::IconPlaySong_png, static_cast<size_t>(BinaryData::IconPlaySong_pngSize),
                       BinaryData::IconStop_png, static_cast<size_t>(BinaryData::IconStop_pngSize),
                       juce::translate("Play/stop"), [&](int, bool, bool) { onPlayStopSongButtonClicked(); }),
        nextFrameButton(BinaryData::IconForward_png, static_cast<size_t>(BinaryData::IconForward_pngSize), juce::translate("Next frame"),
                        [&](int, bool, const bool isLeftButtonClicked) {
                            onUserWantsToMoveToRelativeFrame(isLeftButtonClicked ? 1 : fastBrowseSpeed);
                        }),
        previousFrameButton(BinaryData::IconBack_png, static_cast<size_t>(BinaryData::IconBack_pngSize), juce::translate("Previous frame"),
                            [&](int, bool, const bool isLeftButtonClicked) {
                                onUserWantsToMoveToRelativeFrame(isLeftButtonClicked ? -1 : -fastBrowseSpeed);
                            }),
        progressSlider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::NoTextBox),
        progressButton(juce::String(), "- / -"),
        lockLengthButton(BinaryData::IconLockLengthOff_png, BinaryData::IconLockLengthOff_pngSize,
                         BinaryData::IconLockLengthOn_png, BinaryData::IconLockLengthOn_pngSize,
                         juce::translate("Lock length (changing the start will change the end)"),
                         [&](int, bool, bool) { onLockLengthButtonClicked(); }),
        loopButton(BinaryData::IconLoopOff_png, BinaryData::IconLoopOff_pngSize,
                   BinaryData::IconLoopOn_png, BinaryData::IconLoopOn_pngSize,
                   juce::translate("Toggle the loop"),
                   [&](int, bool, bool) { onLoopButtonClicked(); }),
        loopStartSlider(*this, juce::translate("Start"), 6, 1.0, 8.0, SliderIncDec::Filter::integer),
        loopEndSlider(*this, juce::translate("End"), 6, 1.0, 8.0, SliderIncDec::Filter::integer),
        nowIsLoopButton(juce::translate("Loop now"), juce::translate("Sets a small loop starting where the cursor is")),
        speedSlider(*this, juce::translate("Speed"), 3, 1.0, 2.0, SliderIncDec::Filter::integer),
        channelOnToggleButtons(),

        psgValuesGroup(),
        psgSlider(*this, juce::translate("PSG"), 2, 1.0, 2.0, SliderIncDec::Filter::integer),
        psgInfo(),
        channel0PeriodCaption(juce::String(), juce::translate("Channel 1 period (R0/R1):")),
        channel1PeriodCaption(juce::String(), juce::translate("Channel 2 period (R2/R3):")),
        channel2PeriodCaption(juce::String(), juce::translate("Channel 3 period (R4/R5):")),
        volume0Caption(juce::String(), juce::translate("Channel 1 volume (R8):")),
        volume1Caption(juce::String(), juce::translate("Channel 2 volume (R9):")),
        volume2Caption(juce::String(), juce::translate("Channel 3 volume (R10):")),
        mixerCaption(juce::String(), juce::translate("Mixer (R7):")),
        noiseCaption(juce::String(), juce::translate("Noise (R6):")),
        hardwarePeriodCaption(juce::String(), juce::translate("Hardware period (R11/R12):")),
        hardwareEnvelopeCaption(juce::String(), juce::translate("Hardware envelope (R13):")),

        channel0PeriodLabel(),
        channel1PeriodLabel(),
        channel2PeriodLabel(),
        channel0VolumeLabel(),
        channel1VolumeLabel(),
        channel2VolumeLabel(),
        mixerLabel(),
        mixerBinaryLabel(),
        noiseLabel(),
        hardwarePeriodLabel(),
        hardwareEnvelopeLabel(),

        exportGroup(),
        instrumentNameLabel(juce::String(), juce::translate("Instrument name")),
        instrumentNameTextEditor(),
        outputPsgLabel(juce::String(), juce::translate("Output PSG")),
        outputPsgComboBox(),
        encodeAsPeriodsToggle(juce::translate("Encode as fixed periods")),
        encodeAsNotesToggle(juce::translate("Encode as notes")),
        exportButton(juce::translate("Export")),

        shownPsgIndex(),

        shownPsgChannel0Period(),
        shownPsgChannel1Period(),
        shownPsgChannel2Period(),
        shownPsgChannel0Volume(),
        shownPsgChannel1Volume(),
        shownPsgChannel2Volume(),
        shownPsgNoise(),
        shownPsgMixer(),
        shownPsgHardwarePeriod(),
        shownPsgHardwareEnvelope(),
        shownPsgHardwareRetrig(),

        psgs(),

        loadMusicOperation(),

        modalDialog(),

        isLoopLengthLocked(false),

        progressDialogEditTextRestriction(progressMaxLengthInEditText, TextEditorUtil::restrictionInt)
{
    const auto innerBounds = getUsableModalDialogBounds();
    const auto left = innerBounds.getX();
    const auto top = innerBounds.getY();
    const auto margins = LookAndFeelConstants::margins;
    const auto width = innerBounds.getWidth();
    const auto height = innerBounds.getHeight();

    constexpr auto smallIconsWidth = 30;
    const auto iconsSize = LookAndFeelConstants::buttonsHeight;
    constexpr auto sliderHeight = 30;

    const auto labelsHeight = LookAndFeelConstants::labelsHeight;

    setOkButtonText(juce::translate("Close"));

    exportToCsvButton.setBounds(left, getButtonsY(), 130, getButtonsHeight());
    exportToCsvButton.onClick = [this] { onExportToCsvClicked(); };

    loadButton.setBounds(left, top, 60, labelsHeight);
    const auto loadOrDropFileLabelHeight = labelsHeight * 2;
    loadOrDropFileLabel.setBounds(left, (height - loadOrDropFileLabelHeight) / 2, width, loadOrDropFileLabelHeight);
    loadOrDropFileLabel.setJustificationType(juce::Justification::centred);

    const auto musicInfoLabelX = loadButton.getRight() + margins;
    musicInfoLabel.setBounds(musicInfoLabelX, loadButton.getY(), width - musicInfoLabelX, labelsHeight);

    constexpr auto progressButtonWidth = 140;
    const auto progressButtonX = left + width - progressButtonWidth;
    playStopButton.setBounds(left, loadButton.getBottom() + margins, smallIconsWidth, iconsSize);
    previousFrameButton.setBounds(playStopButton.getRight() + margins, playStopButton.getY(), smallIconsWidth, iconsSize);
    nextFrameButton.setBounds(previousFrameButton.getRight(), playStopButton.getY(), smallIconsWidth, iconsSize);
    const auto sliderX = nextFrameButton.getRight() + margins;
    const auto sliderWidth = progressButtonX - sliderX - margins;
    progressSlider.setBounds(sliderX, playStopButton.getY(), sliderWidth, sliderHeight);
    progressButton.setBounds(progressButtonX, progressSlider.getY(), progressButtonWidth, labelsHeight);

    loopButton.setBounds(left, progressButton.getBottom() + margins, iconsSize, iconsSize);
    loopStartSlider.setTopLeftPosition(loopButton.getRight() + (margins * 3), loopButton.getY());
    loopEndSlider.setTopLeftPosition(loopStartSlider.getRight() + margins, loopButton.getY());
    lockLengthButton.setBounds(loopEndSlider.getRight() + static_cast<int>(margins * 1.5), loopButton.getY(), iconsSize, iconsSize);

    constexpr auto nowIsLoopWidth = 100;
    const auto nowIsLoopX = left + width - nowIsLoopWidth;
    nowIsLoopButton.setBounds(nowIsLoopX, loopButton.getY(), nowIsLoopWidth, labelsHeight);

    speedSlider.setTopLeftPosition(left, loopButton.getBottom() + (margins * 2));

    // Rigs listeners.
    loadButton.onClick = [&] { onLoadButtonClicked(); };
    playStopButton.setState(isPlaying);
    progressSlider.onValueChange = [&] { onUserMovedProgressSlider(); };
    progressSlider.setEnabled(false);
    speedSlider.setShownValue(SubsongConstants::minimumSpeed);
    nowIsLoopButton.onClick = [&] { onNowIsLoopButtonClicked(); };
    progressButton.onClick = [&] { onProgressButtonClicked(); };

    // Creates a maximum of channel on/off.
    auto channelOnX = speedSlider.getRight() + (margins * 3);
    const auto channelOnY = speedSlider.getY();
    for (auto channelIndex = 0; channelIndex < PsgValues::getChannelCount(maximumPsgCount); ++channelIndex) {
        constexpr auto channelOnWidth = 50;
        auto channelToggleButton = std::make_unique<juce::ToggleButton>(juce::String(channelIndex + 1));
        channelToggleButton->onStateChange = [&] { onUserChangedMutedChannels(); };

        channelToggleButton->setBounds(channelOnX, channelOnY, channelOnWidth, labelsHeight);
        addComponentToModalDialog(*channelToggleButton, false);
        channelOnX += channelOnWidth;

        channelOnToggleButtons.push_back(std::move(channelToggleButton));
    }

    // The PSG values group.
    constexpr auto psgValuesGroupHeight = 200;
    constexpr auto psgSliderWidth = 130;
    constexpr auto mixerBinaryValueWidth = 100;
    constexpr auto exportGroupHeight = 126;
    psgValuesGroup.setBounds(left, speedSlider.getBottom() + margins, width, psgValuesGroupHeight);
    psgValuesGroup.setText(juce::translate("PSG register values"));
    const auto psgValueLeftColumnX = left + (margins * 2);
    const auto psgValueRightColumnX = left + (width / 2) + 20;
    psgSlider.setBounds(psgValueLeftColumnX, static_cast<int>(psgValuesGroup.getY() + (margins * 2.5)), psgSliderWidth, labelsHeight);
    const auto psgInfoX = psgSlider.getRight();
    psgInfo.setBounds(psgInfoX, psgSlider.getY(), width - psgInfoX - margins, labelsHeight);
    auto psgValueY = psgSlider.getBottom() + margins;

    // Two columns, caption and value intertwined.
    auto psgValueComponentIndex = 0;
    const juce::Font monospaceFont(LookAndFeelConstants::typefaceMonospace, channel0PeriodCaption.getFont().getHeight(), 0);

    for (auto* view : {
            &channel0PeriodCaption, &channel0PeriodLabel, &volume0Caption, &channel0VolumeLabel,
            &channel1PeriodCaption, &channel1PeriodLabel, &volume1Caption, &channel1VolumeLabel,
            &channel2PeriodCaption, &channel2PeriodLabel, &volume2Caption, &channel2VolumeLabel,
            &mixerCaption, &mixerLabel, &noiseCaption, &noiseLabel,
            &hardwarePeriodCaption, &hardwarePeriodLabel, &hardwareEnvelopeCaption, &hardwareEnvelopeLabel,
    }) {
        constexpr auto psgCaptionWidth = 190;

        const auto isCaption = ((psgValueComponentIndex % 2) == 0);
        const auto isLeftColumn = ((psgValueComponentIndex % 4) < 2);
        const auto isLastInLine = ((psgValueComponentIndex % 4) == 3);

        if (!isCaption) {
            view->setFont(monospaceFont);
        }

        const auto columnX = isLeftColumn ? psgValueLeftColumnX : psgValueRightColumnX;
        if (isCaption) {
            view->setBounds(columnX, psgValueY, psgCaptionWidth, labelsHeight);
        } else {
            // A value.
            constexpr auto psgValueWidth = 70;
            view->setBounds(columnX + psgCaptionWidth, psgValueY, psgValueWidth, labelsHeight);
        }
        addComponentToModalDialog(*view, false);

        if (isLastInLine) {
            psgValueY += labelsHeight;
        }
        ++psgValueComponentIndex;
    }
    // Exception for the Mixer Binary label, which is displayed on its own.
    mixerBinaryLabel.setBounds(mixerLabel.getX() - mixerBinaryValueWidth, mixerLabel.getY(), mixerBinaryValueWidth, mixerLabel.getHeight());
    mixerBinaryLabel.setFont(monospaceFont);
    mixerBinaryLabel.setJustificationType(juce::Justification::centredRight);

    // Export group.
    constexpr auto exportGroupLabelsWidth = 130;
    constexpr auto encodeAsPeriodsToggleWidth = 200;
    constexpr auto encodeAsNotesToggleWidth = 180;
    constexpr auto exportButtonWidth = 80;
    constexpr auto exportRadioGroup = 189;      // Why not?
    exportGroup.setBounds(left, psgValuesGroup.getBottom() + margins, psgValuesGroup.getWidth(), exportGroupHeight);
    exportGroup.setText(juce::translate("Export instrument"));
    instrumentNameLabel.setBounds(exportGroup.getX() + margins, exportGroup.getY() + (2 * margins), exportGroupLabelsWidth, labelsHeight);
    instrumentNameTextEditor.setBounds(instrumentNameLabel.getRight(), instrumentNameLabel.getY(),
        exportGroup.getWidth() - exportGroupLabelsWidth - left - (margins * 2), labelsHeight);
    outputPsgLabel.setBounds(instrumentNameLabel.getX(), instrumentNameLabel.getBottom() + margins, exportGroupLabelsWidth, labelsHeight);
    outputPsgComboBox.setBounds(outputPsgLabel.getRight(), outputPsgLabel.getY(), instrumentNameTextEditor.getWidth(), labelsHeight);
    encodeAsPeriodsToggle.setBounds(outputPsgLabel.getX(), outputPsgLabel.getBottom() + margins, encodeAsPeriodsToggleWidth, labelsHeight);
    encodeAsNotesToggle.setBounds(encodeAsPeriodsToggle.getRight(), encodeAsPeriodsToggle.getY(), encodeAsNotesToggleWidth, labelsHeight);
    exportButton.setBounds(exportGroup.getRight() - exportButtonWidth - (margins * 2), encodeAsPeriodsToggle.getY(), exportButtonWidth, labelsHeight);
    encodeAsPeriodsToggle.setRadioGroupId(exportRadioGroup, juce::NotificationType::dontSendNotification);

    instrumentNameTextEditor.setText("New", false);
    encodeAsNotesToggle.setRadioGroupId(exportRadioGroup, juce::NotificationType::dontSendNotification);
    encodeAsNotesToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    fillPsgComboBox();
    exportButton.onClick = [this]{ onExportButtonClicked(); };
    //progressLabel.addListener()

    updateLoopComponentsEnabledFromLoopState();

    addComponentToModalDialog(loadButton);
    addComponentToModalDialog(loadOrDropFileLabel);
    constexpr auto visibility = false;
    addComponentToModalDialog(exportToCsvButton, visibility);
    addComponentToModalDialog(musicInfoLabel, visibility);
    addComponentToModalDialog(playStopButton, visibility);
    addComponentToModalDialog(nextFrameButton, visibility);
    addComponentToModalDialog(previousFrameButton, visibility);
    addComponentToModalDialog(progressSlider, visibility);
    addComponentToModalDialog(progressButton, visibility);
    addComponentToModalDialog(loopButton, visibility);
    addComponentToModalDialog(loopStartSlider, visibility);
    addComponentToModalDialog(loopEndSlider, visibility);
    addComponentToModalDialog(lockLengthButton, visibility);
    addComponentToModalDialog(nowIsLoopButton, visibility);
    addComponentToModalDialog(speedSlider, visibility);
    addComponentToModalDialog(psgValuesGroup, visibility);
    addComponentToModalDialog(psgSlider, visibility);
    addComponentToModalDialog(psgInfo, visibility);
    addComponentToModalDialog(mixerBinaryLabel, visibility);
    addComponentToModalDialog(exportGroup, visibility);
    addComponentToModalDialog(instrumentNameLabel, visibility);
    addComponentToModalDialog(instrumentNameTextEditor, visibility);
    addComponentToModalDialog(outputPsgLabel, visibility);
    addComponentToModalDialog(outputPsgComboBox, visibility);
    addComponentToModalDialog(encodeAsPeriodsToggle, visibility);
    addComponentToModalDialog(encodeAsNotesToggle, visibility);
    addComponentToModalDialog(exportButton, visibility);

    // Tries to load the previously loaded file.
    if (const auto file = juce::File(previouslyLoadedSongPath); file.existsAsFile()) {
        loadFile(file);
    } else {
        clearPreviouslyLoadedData();
    }
}

StreamedMusicAnalyzer::~StreamedMusicAnalyzer()
{
    stopTimer();
    audioDeviceManager.removeAudioCallback(audioSourcePlayer.get());
    releaseAudioSource();

    // Stores the data for reopening the song.
    previouslyLoadedSongPath = loadedSongPath;
    previouslyLoadedSongCurrentIteration = currentIteration;
    previouslyLoadedSongLoopStartIndex = loopStartIndex;
    previouslyLoadedSongEndIndex = loopEndIndex;
}


// FileDragAndDropTarget methods implementation.
// ===================================================================

bool StreamedMusicAnalyzer::isInterestedInFileDrag(const juce::StringArray& files)
{
    // By default, we like all the files! But there must be only once.
    return (files.size() == 1);
}

void StreamedMusicAnalyzer::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    // Security.
    if (files.isEmpty()) {
        return;
    }
    loadFile(files[0]);
}


// ===================================================================

void StreamedMusicAnalyzer::onOkButtonClicked() const noexcept
{
    modalCallback();
}

void StreamedMusicAnalyzer::closeButtonPressed()
{
    modalCallback();
}

void StreamedMusicAnalyzer::releaseAudioSource() noexcept
{
    if (audioSourcePlayer != nullptr) {
        audioDeviceManager.removeAudioCallback(audioSourcePlayer.get());
        audioSourcePlayer->setSource(nullptr);
    }
    psgsMixer.removeAllInputs();        // Using releaseResources crashes when switching from a several PSG song to one with one PSG.

    psgStreamGenerators.clear();
    audioSourcePlayer.reset();
}

void StreamedMusicAnalyzer::setupAudio() noexcept
{
    jassert(psgStreamGenerators.empty());
    jassert(streamedMusic != nullptr);
    jassert(streamedMusicReader != nullptr);
    jassert(audioSourcePlayer == nullptr);

    // Builds the AY Buffer Generator.
    audioSourcePlayer = std::make_unique<juce::AudioSourcePlayer>();
    audioDeviceManager.addAudioCallback(audioSourcePlayer.get());

    // Builds as many generators as there are PSGs.
    for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
        auto psgStreamGenerator = std::make_unique<PsgStreamGenerator>(*this, streamedMusicReader->getPsgType(psgIndex),
                                                                       psgIndex, streamedMusicReader->getPlayerReplayHz(),
                                                                       streamedMusicReader->getPsgFrequencyHz(psgIndex),
                                                                       PsgFrequency::defaultSamplePlayerFrequencyHz, // We don't play samples, so it doesn't matter.
                                                                       PsgMixingOutput::ABC, 1.0, 1.0, 1.0,
                                                                       SidPlayerCapability::buildForAtariSt());     // SID player not available for now...
        // Adds the stream generator to the Mixer.
        psgsMixer.addInputSource(psgStreamGenerator.get(), false);

        psgStreamGenerators.push_back(std::move(psgStreamGenerator));
    }
    audioSourcePlayer->setSource(&psgsMixer);
}

void StreamedMusicAnalyzer::loadFile(const juce::File& file) noexcept
{
    if (file.isDirectory()) {       // Security on Mac: a folder can be opened instead!
        jassertfalse;
        return;
    }

    stopTimer();            // Don't refresh while switching the music.
    isPlaying = false;

    loadedSongPath = file.getFullPathName();  // Will be cleared if an error occurs.

    // Starts a background thread with a dialog to open, uncompress and choose a reader for the selected file.
    jassert(loadMusicOperation == nullptr);     // Operation already started?
    loadMusicOperation = std::make_unique<BackgroundOperationWithDialog<std::pair<std::unique_ptr<StreamedMusicReader>, std::unique_ptr<juce::InputStream>>>>(
            juce::translate("Loading"),
            juce::translate("Loading the music..."),
            [this](std::pair<std::unique_ptr<StreamedMusicReader>, std::unique_ptr<juce::InputStream>> readerAndMusic) {
                onLoadMusicOperationFinished(std::move(readerAndMusic));
            },
            [file]() -> std::pair<std::unique_ptr<StreamedMusicReader>, std::unique_ptr<juce::InputStream>> {

                auto originalInputStream = std::make_unique<juce::FileInputStream>(file);
                // Tried to unzip it. If it fails, uses the original Stream.
                auto unzippedFile = ZipHelper::unzip(*originalInputStream);
                auto localStreamedMusic = (unzippedFile == nullptr) ? std::move(originalInputStream) : std::move(unzippedFile);

                // What Reader to use?
                std::vector<std::unique_ptr<StreamedMusicReader>> readers;
                readers.push_back(std::make_unique<YmReader>(*localStreamedMusic));
                readers.push_back(std::make_unique<VgmReader>(*localStreamedMusic));
                std::unique_ptr<StreamedMusicReader> chosenReader;
                for (auto& reader : readers) {
                    if (reader->checkFormat() && reader->prepare()) {
                        chosenReader = std::move(reader);
                        break;
                    }
                }

                // Returns the reader and music, if successful.
                return (chosenReader == nullptr)
                       ? std::make_pair(nullptr, nullptr)
                       : std::make_pair(std::move(chosenReader), std::move(localStreamedMusic));
            }
    );
    loadMusicOperation->performOperation();
}

void StreamedMusicAnalyzer::onLoadMusicOperationFinished(std::pair<std::unique_ptr<StreamedMusicReader>, std::unique_ptr<juce::InputStream>> readerAndMusic) noexcept
{
    // Main thread.
    loadMusicOperation.reset();

    if ((readerAndMusic.first != nullptr) && (readerAndMusic.second != nullptr)) {
        releaseAudioSource();
        streamedMusic = std::move(readerAndMusic.second);
        streamedMusicReader = std::move(readerAndMusic.first);
        onMusicLoaded();
    } else {
        // Failure!
        loadedSongPath = juce::String();
        clearPreviouslyLoadedData();

        // Shows a Dialog.
        jassert(modalDialog == nullptr);
        modalDialog = SuccessOrErrorDialog::buildForError(juce::translate("Unable to open the music file!"), [&] { closeAdditionalDialog(); });
    }
}

void StreamedMusicAnalyzer::onLoadButtonClicked() noexcept
{
    // Opens the file picker.
    FileChooserCustom fileChooser("Open an YM/VGM file", FolderContext::other, "*.ym,*.vgm,*.vgz");
    if (fileChooser.browseForFileToOpen()) {
        loadFile(fileChooser.getResult());
    }
}

void StreamedMusicAnalyzer::onMusicLoaded() noexcept
{
    iterationCount = streamedMusicReader->getIterationCount();
    const auto newLastMaxIteration = std::max(0, iterationCount - 1);
    lastMaxIteration = newLastMaxIteration;

    // Tries to use the previously stored data.
    const auto usePreviousData = arePreviouslyLoadedDataPresent();
    currentIteration = usePreviousData ? std::min(previouslyLoadedSongCurrentIteration, newLastMaxIteration) : 0;
    const auto newLoopStartIndex = usePreviousData ? std::min(previouslyLoadedSongLoopStartIndex, newLastMaxIteration) : 0;
    const auto newEndIndex = usePreviousData ? std::min(previouslyLoadedSongEndIndex, newLastMaxIteration) : newLastMaxIteration;
    jassert(newLoopStartIndex <= newEndIndex);
    clearPreviouslyLoadedData();        // Clears them, useful in case another song is loaded, we don't want to re-use these data.

    shownPsgIndex = 0;
    psgSlider.setShownValue(shownPsgIndex + 1);
    displayPsgInfo();

    // We don't "follow" the Song, only what the Slider indicates. This is not a music player after all!
    updateLoopValueAndSlider(newLoopStartIndex, newEndIndex);
    currentTick = 0;
    updateSpeedValueAndSlider(SubsongConstants::minimumSpeed);

    progressSlider.setRange(0, newLastMaxIteration, 1);
    progressSlider.setValue(0, juce::NotificationType::dontSendNotification);
    progressSlider.setEnabled(true);

    // Shows the possible title and author.
    const auto originalTitle = streamedMusicReader->getTitle();
    const auto author = streamedMusicReader->getAuthor();
    const auto localPsgCount = streamedMusicReader->getPsgCount();
    psgCount = localPsgCount;
    const auto displayedTitle = originalTitle.isEmpty() ? juce::translate("Untitled") : originalTitle;
    const auto songInfo = displayedTitle + (author.isNotEmpty() ? juce::translate(" by ") + author : juce::String()) + " (PSG count: "
                          + juce::String(localPsgCount) + ")";
    musicInfoLabel.setText(songInfo, juce::NotificationType::dontSendNotification);

    setupAudio();

    // Creates the PSG to Muted channels, and make the UI buttons visible or not.
    psgToMutedChannelIndexes.clear();
    for (auto psgIndex = 0; psgIndex < localPsgCount; ++psgIndex) {
        psgToMutedChannelIndexes.emplace_back();        // No muted channels.
    }
    // Makes the Channel button visible or not.
    auto channelToggleIndex = 0;
    for (const auto& channelOnToggleButton : channelOnToggleButtons) {
        channelOnToggleButton->setToggleState(true, juce::NotificationType::dontSendNotification);
        const auto visible = (PsgValues::getPsgIndex(channelToggleIndex) < localPsgCount);
        channelOnToggleButton->setVisible(visible);
        ++channelToggleIndex;
    }

    startTimer(refreshTimerMs);

    // Reveals the interface.
    loadOrDropFileLabel.setVisible(false);
    const std::vector<Component*> views = {
            &musicInfoLabel,
            &playStopButton,
            &nextFrameButton,
            &previousFrameButton,
            &progressSlider, &progressButton,
            &loopButton, &loopStartSlider, &loopEndSlider,
            &lockLengthButton,
            &nowIsLoopButton,
            &speedSlider,
            &psgValuesGroup,
            &psgSlider, &psgInfo,
            &channel0PeriodCaption, &channel1PeriodCaption, &channel2PeriodCaption,
            &volume0Caption, &volume1Caption, &volume2Caption,
            &mixerCaption, &noiseCaption,
            &hardwarePeriodCaption, &hardwareEnvelopeCaption,
            &channel0PeriodLabel, &channel1PeriodLabel, &channel2PeriodLabel,
            &channel0VolumeLabel, &channel1VolumeLabel, &channel2VolumeLabel,
            &mixerLabel, &mixerBinaryLabel,
            &noiseLabel,
            &hardwarePeriodLabel, &hardwareEnvelopeLabel,
            &exportGroup, &instrumentNameLabel, &instrumentNameTextEditor, &outputPsgLabel, &outputPsgComboBox,
            &encodeAsPeriodsToggle, &encodeAsNotesToggle, &exportButton,
            &exportToCsvButton,
    };
    for (auto* view : views) {
        view->setVisible(true);
    }
}

void StreamedMusicAnalyzer::updateLoopComponentsEnabledFromLoopState() noexcept
{
    const auto looping = isLooping.load();
    loopButton.setState(looping);

    loopStartSlider.setEnabled(looping);
    loopEndSlider.setEnabled(looping);
}


// PsgRegistersProvider method implementations.
// ===================================================

std::pair<std::unique_ptr<PsgRegisters>, std::unique_ptr<SampleData>> StreamedMusicAnalyzer::getNextRegisters(const int psgIndex) noexcept
{
    // Audio thread.
    // =====================

    // Even if not playing, gets the PSG data for display.
    const auto iterationIndex = currentIteration.load();

    const auto [psgRegisters, success] = streamedMusicReader->getRegisters(psgIndex, iterationIndex);
    if (!success) {
        jassertfalse;
        return { std::make_unique<PsgRegisters>(), nullptr };
    }
    const auto volume0 = psgRegisters.getVolume(0);
    const auto volume1 = psgRegisters.getVolume(1);
    const auto volume2 = psgRegisters.getVolume(2);

    // Stores the registers for display.
    if (psgIndex == shownPsgIndex) {
        shownPsgChannel0Period = psgRegisters.getSoftwarePeriod(0);
        shownPsgChannel1Period = psgRegisters.getSoftwarePeriod(1);
        shownPsgChannel2Period = psgRegisters.getSoftwarePeriod(2);
        shownPsgChannel0Volume = volume0;
        shownPsgChannel1Volume = volume1;
        shownPsgChannel2Volume = volume2;
        shownPsgNoise = psgRegisters.getNoise();
        shownPsgMixer = psgRegisters.getMixer();
        shownPsgHardwarePeriod = psgRegisters.getHardwarePeriod();
        const auto envelopeAndRetrig = psgRegisters.getHardwareEnvelopeAndRetrig();
        shownPsgHardwareEnvelope = envelopeAndRetrig.getEnvelope();
        shownPsgHardwareRetrig = envelopeAndRetrig.isRetrig();
    }

    // Not playing? Then returns empty sound.
    if (!isPlaying) {
        return { std::make_unique<PsgRegisters>(), nullptr };
    }

    auto psgRegistersPtr = std::make_unique<PsgRegisters>(psgRegisters);

    // Digidrums handling. Only on first step!
    auto sampleData = std::make_unique<SampleData>();
    if ((currentTick == 0) && streamedMusicReader->areDigidrumsAllowed()) {
        if (const auto digidrumBits = static_cast<unsigned int>(psgRegistersPtr->getValueFromRegister(3)) & 0b00110000U; digidrumBits != 0) {
            auto digidrumVolumeRegister = -1;
            auto digiChannel = 0;
            switch (digidrumBits) {
                case 0b010000:
                    digidrumVolumeRegister = volume0;
                    break;
                case 0b100000:
                    digiChannel = 1;
                    digidrumVolumeRegister = volume1;
                    break;
                case 0b110000:
                    digiChannel = 2;
                    digidrumVolumeRegister = volume2;
                    break;
                default:
                    jassertfalse;   // Unknown digi-channel. Shouldn't happen!
                    break;
            }
            if (digidrumVolumeRegister >= 0) {
                // A digidrum is present.
                const auto sampleIndex = static_cast<unsigned int>(digidrumVolumeRegister) & 0b11111U;
                // This mess is courtesy of the YM doc.
                const auto tp = static_cast<unsigned int>(psgRegistersPtr->getValueFromRegister(8)) & 0b11100000U;
                const auto tc = std::max(1U, static_cast<unsigned int>(psgRegistersPtr->getValueFromRegister(15))); // Max is a security to avoid division by 0!

                auto predivider = 1U;
                switch (tp >> 5U) {
                    case 0b000:         // Timer stop. What to do with that?
                        jassertfalse;
                        break;
                    default:
                        jassertfalse;   // Cannot happen.
                        break;
                    case 0b001:
                        predivider = 4;
                        break;
                    case 0b010:
                        predivider = 10;
                        break;
                    case 0b011:
                        predivider = 16;
                        break;
                    case 0b100:
                        predivider = 50;
                        break;
                    case 0b101:
                        predivider = 64;
                        break;
                    case 0b110:
                        predivider = 100;
                        break;
                    case 0b111:
                        predivider = 200;
                        break;
                }
                const auto sampleReplayFrequencyHz = 2457600 / predivider / tc;

                const auto samplePlayInfo = streamedMusicReader->getSamplePlayInfo(static_cast<int>(sampleIndex), static_cast<int>(sampleReplayFrequencyHz));
                sampleData = std::make_unique<SampleData>();
                sampleData->addSamplePlayInfoIfNotEmpty(digiChannel, samplePlayInfo);
            }
        }
    }

    // Only when all the PSGs are managed the iteration must be increased.
    if (psgIndex == (psgCount - 1)) {
        // Move forward only if the current tick has reached the speed.
        if (++currentTick >= speed) {
            currentTick = 0;

            // End reached? Loops to loop start if looping, else stop and loops to the start.
            ++currentIteration;
            if (isLooping && (currentIteration > loopEndIndex)) {
                currentIteration = loopStartIndex.load();
            } else if (!isLooping && (currentIteration > lastMaxIteration)) {
                currentIteration = 0;

                // Stops the playing when the end is reached if no loop is present.
                isPlaying = false;
                return { std::make_unique<PsgRegisters>(), nullptr };
            }
        }
    }

    return { std::move(psgRegistersPtr), std::move(sampleData) };
}


// juce::Timer method implementations.
// ===================================================

void StreamedMusicAnalyzer::timerCallback()
{
    // If no music is loaded, don't do anything.
    if (psgStreamGenerators.empty()) {
        return;
    }

    // Updates the slider (even if not playing).
    const auto iteration = currentIteration.load();

    progressSlider.setValue(iteration, juce::NotificationType::dontSendNotification);
    progressButton.setButtonText(juce::String(iteration) + " / " + juce::String(lastMaxIteration.load()));

    playStopButton.setState(isPlaying);

    displayPsgValues();
}


// SliderIncDec::Listener method implementations.
// ===================================================

void StreamedMusicAnalyzer::onWantToChangeSliderValue(SliderIncDec& slider, const double value)
{
    const auto originalNewValue = static_cast<int>(value);
    const auto lastIteration = lastMaxIteration.load();
    if (&slider == &loopStartSlider) {
        // If the lock length is on, the end will be changed according to the start.
        const auto loopLength = getLoopLength();
        const auto newLoopStart = NumberUtil::correctNumber(originalNewValue, 0, lastIteration);
        const auto newLoopEnd = NumberUtil::correctNumber(isLoopLengthLocked ? newLoopStart + loopLength : loopEndIndex.load(), newLoopStart, lastIteration);
        updateLoopValueAndSlider(newLoopStart, newLoopEnd);
    } else if (&slider == &loopEndSlider) {
        const auto newLoopEnd = NumberUtil::correctNumber(originalNewValue, 0, lastIteration);
        const auto newLoopStart = NumberUtil::correctNumber(loopStartIndex.load(), 0, newLoopEnd);
        updateLoopValueAndSlider(newLoopStart, newLoopEnd);
    } else if (&slider == &speedSlider) {
        const auto newSpeed = NumberUtil::correctNumber(originalNewValue, SubsongConstants::minimumSpeed, SubsongConstants::maximumSpeed);
        updateSpeedValueAndSlider(newSpeed);
    } else if (&slider == &psgSlider) {
        const auto newPsgIndex = NumberUtil::correctNumber(originalNewValue, 1, psgCount.load());
        shownPsgIndex = newPsgIndex - 1;
        psgSlider.setShownValue(newPsgIndex);
        displayPsgInfo();
    } else {
        jassertfalse;       // Unknown slider!
    }
}


// ===================================================

int StreamedMusicAnalyzer::getLoopLength() const noexcept
{
    return loopEndIndex.load() - loopStartIndex.load();
}

void StreamedMusicAnalyzer::onLoopButtonClicked() noexcept
{
    isLooping = !isLooping;

    updateLoopComponentsEnabledFromLoopState();
}

void StreamedMusicAnalyzer::onLockLengthButtonClicked() noexcept
{
    isLoopLengthLocked = !isLoopLengthLocked;
    lockLengthButton.setState(isLoopLengthLocked);
}

void StreamedMusicAnalyzer::onUserWantsToMoveToRelativeFrame(const int offset) noexcept
{
    gotoFrame(currentIteration.load() + offset);
}

void StreamedMusicAnalyzer::onNowIsLoopButtonClicked() noexcept
{
    constexpr auto halfLoopDuration = nowLoopDefaultDuration / 2 ;
    const auto maxIteration = lastMaxIteration.load();

    const auto newLoopStart = NumberUtil::correctNumber(currentIteration.load() - halfLoopDuration, 0, maxIteration);
    const auto newLoopEnd = NumberUtil::correctNumber(newLoopStart + (halfLoopDuration * 2) - 1, 0, maxIteration);

    updateLoopValueAndSlider(newLoopStart, newLoopEnd);

    // As a convenience, the loop is forced to on.
    isLooping = true;
    updateLoopComponentsEnabledFromLoopState();
}

void StreamedMusicAnalyzer::onProgressButtonClicked() noexcept
{
    // Shows a panel to set the name.
    jassert(modalDialog == nullptr);      // Dialog already present?
    auto textFieldDialog = std::make_unique<TextFieldDialog>(juce::translate("Go to frame"), juce::translate("Enter the frame number to go to:"),
                                                    juce::String(currentIteration),
                                                    [&](const juce::String& newText) {
                                                        const auto newProgress = newText.getIntValue();
                                                        gotoFrame(newProgress);
                                                        closeAdditionalDialog();
                                                    },
                                                    [&] { closeAdditionalDialog(); },
                                                    false, 250
    );
    textFieldDialog->setTextRestriction(progressDialogEditTextRestriction);
    modalDialog = std::move(textFieldDialog);
}

void StreamedMusicAnalyzer::updateLoopValueAndSlider(const int newLoopStart, const int newLoopEnd) noexcept
{
    loopStartIndex = newLoopStart;
    loopEndIndex = newLoopEnd;
    loopStartSlider.setShownValue(newLoopStart);
    loopEndSlider.setShownValue(newLoopEnd);
}

void StreamedMusicAnalyzer::updateSpeedValueAndSlider(const int newSpeed) noexcept
{
    speed = newSpeed;
    speedSlider.setShownValue(newSpeed);
}

void StreamedMusicAnalyzer::onPlayStopSongButtonClicked() noexcept
{
    if (psgStreamGenerators.empty()) {
        jassertfalse;       // Music not loaded?
        return;
    }

    isPlaying = !isPlaying;

    // When starting playing, makes sure we are not past, in which case goes to the start.
    if (isPlaying && isLooping) {
        currentIteration = getCorrectedIteration(currentIteration);
    } else if (!isPlaying && isLooping) {
        // When stopped, if the loop was on, it is handy to go back to the start.
        currentIteration = loopStartIndex.load();
    }

    playStopButton.setState(isPlaying);
}

void StreamedMusicAnalyzer::onUserMovedProgressSlider() noexcept
{
    const auto newIterationIndex = static_cast<int>(progressSlider.getValue());
    gotoFrame(newIterationIndex);
}

void StreamedMusicAnalyzer::gotoFrame(const int frameIndex) noexcept
{
    currentIteration = getCorrectedIteration(frameIndex);
}

int StreamedMusicAnalyzer::getCorrectedIteration(const int iterationToCorrect) const noexcept
{
    const auto startIndex = isLooping ? loopStartIndex.load() : 0;
    const auto endIndex = isLooping ? loopEndIndex.load() : lastMaxIteration.load();

    // If beyond, plays from the start.
    if (iterationToCorrect > endIndex) {
        return startIndex;
    }

    return NumberUtil::correctNumber(iterationToCorrect, startIndex, endIndex);
}

void StreamedMusicAnalyzer::onUserChangedMutedChannels() noexcept
{
    // Even called when hovered!
    if (psgStreamGenerators.empty()) {
        return;
    }

    // Rebuilds the set according to the UI.
    // Clears the sets.
    for (auto& mutedSet : psgToMutedChannelIndexes) {
        mutedSet.clear();
    }

    auto channelToggleIndex = 0;
    for (const auto& channelOnToggleButton : channelOnToggleButtons) {
        const auto psgIndex = PsgValues::getPsgIndex(channelToggleIndex);

        if (!channelOnToggleButton->getToggleState()) {
            auto& mutedSet = psgToMutedChannelIndexes.at(static_cast<size_t>(psgIndex));
            // Converts the linear channel index into a PSG index.
            const auto channelIndexInPsg = PsgValues::getPsgChannelIndex(channelToggleIndex);
            mutedSet.insert(channelIndexInPsg);
        }

        ++channelToggleIndex;
    }

    // Notifies the stream generators.
    auto psgIndex = 0;
    jassert(psgStreamGenerators.size() == psgToMutedChannelIndexes.size());     // Not the same amount of PSG?!
    for (const auto& psgGenerator : psgStreamGenerators) {
        auto& mutedChannelIndexes = psgToMutedChannelIndexes.at(static_cast<size_t>(psgIndex));
        psgGenerator->setMutedChannelIndexes(mutedChannelIndexes);

        ++psgIndex;
    }
}

void StreamedMusicAnalyzer::displayPsgValues() noexcept
{
    static const juce::String sharp = "#";

    channel0PeriodLabel.setText(sharp + NumberUtil::toHexFourDigits(shownPsgChannel0Period.load()), juce::NotificationType::dontSendNotification);
    channel1PeriodLabel.setText(sharp + NumberUtil::toHexFourDigits(shownPsgChannel1Period.load()), juce::NotificationType::dontSendNotification);
    channel2PeriodLabel.setText(sharp + NumberUtil::toHexFourDigits(shownPsgChannel2Period.load()), juce::NotificationType::dontSendNotification);
    channel0VolumeLabel.setText(sharp + NumberUtil::toHexByte(shownPsgChannel0Volume.load()), juce::NotificationType::dontSendNotification);
    channel1VolumeLabel.setText(sharp + NumberUtil::toHexByte(shownPsgChannel1Volume.load()), juce::NotificationType::dontSendNotification);
    channel2VolumeLabel.setText(sharp + NumberUtil::toHexByte(shownPsgChannel2Volume.load()), juce::NotificationType::dontSendNotification);
    noiseLabel.setText(sharp + NumberUtil::toHexByte(shownPsgNoise.load()), juce::NotificationType::dontSendNotification);
    const auto psgMixerValue = shownPsgMixer.load();
    mixerLabel.setText(sharp + NumberUtil::toHexByte(psgMixerValue), juce::NotificationType::dontSendNotification);
    mixerBinaryLabel.setText("(" + NumberUtil::toBinaryString(psgMixerValue) + ")", juce::NotificationType::dontSendNotification);
    hardwarePeriodLabel.setText(sharp + NumberUtil::toHexFourDigits(shownPsgHardwarePeriod.load()), juce::NotificationType::dontSendNotification);
    hardwareEnvelopeLabel.setText(sharp + NumberUtil::toHexByte(shownPsgHardwareEnvelope.load()) + (shownPsgHardwareRetrig ? " (R)" : juce::String()),
                                  juce::NotificationType::dontSendNotification);
}

void StreamedMusicAnalyzer::displayPsgInfo() noexcept
{
    const auto psgFrequencyHz = streamedMusicReader->getPsgFrequencyHz(shownPsgIndex);
    const auto psgType = streamedMusicReader->getPsgType(shownPsgIndex);
    const auto psgTypeString = PsgTypeUtil::psgTypeToDisplayedText(psgType);
    const auto replayHz = streamedMusicReader->getPlayerReplayHz();
    psgInfo.setText(juce::translate("Type: " + psgTypeString + ", frequency: "
                                    + juce::String(psgFrequencyHz) + " Hz, replay: " + juce::String(replayHz) + " Hz."),
                    juce::NotificationType::dontSendNotification);
}

void StreamedMusicAnalyzer::closeAdditionalDialog() noexcept
{
    modalDialog = nullptr;
}

void StreamedMusicAnalyzer::fillPsgComboBox() noexcept
{
    outputPsgComboBox.clear(juce::NotificationType::dontSendNotification);

    // Gets the PSGs from all the Subsongs.
    std::set<Psg, PsgMetadataPsgAndReferenceFrequencyComparator> psgSet;       // Comparator to keep them unique.
    for (const auto& subsongId : song.getSubsongIds()) {
        song.performOnConstSubsong(subsongId, [&psgSet](const Subsong& subsong) {
            const auto allPsgs = subsong.getPsgs();
            psgSet.insert(allPsgs.cbegin(), allPsgs.cend());
        });
    }

    // Stores the PSGs and adds them to the ComboBox.
    psgs.clear();
    auto id = 1;     // 0 is reserved by JUCE.
    for (const auto& psg : psgSet) {
        psgs.push_back(psg);

        outputPsgComboBox.addItem(juce::translate("Frequency: ") + juce::String(psg.getPsgFrequency())
                            + juce::translate("hz, reference frequency: ") + juce::String(psg.getReferenceFrequency()) + "hz", id);
        ++id;
    }

    // Selects the first.
    outputPsgComboBox.setSelectedItemIndex(0, juce::NotificationType::dontSendNotification);
}

void StreamedMusicAnalyzer::onExportButtonClicked() noexcept
{
    // Only one channel must be selected.
    auto selectedLinearChannelIndex = -1;
    auto channelIndex = 0;
    auto selectedChannelCount = 0;
    for (const auto& toggleButton : channelOnToggleButtons) {
        if (toggleButton->getToggleState() && toggleButton->isVisible()) {
            ++selectedChannelCount;
            selectedLinearChannelIndex = channelIndex;
        }
        ++channelIndex;
    }

    if (selectedChannelCount != 1) {
        modalDialog = SuccessOrErrorDialog::buildForError(juce::translate("Please select exactly one channel."),
            [this] { closeAdditionalDialog(); });
        return;
    }

    // Loop not too big?
    const auto startIndex = static_cast<int>(loopStartSlider.getValue());
    const auto endIndex = static_cast<int>(loopEndSlider.getValue());
    if ((endIndex - startIndex) >= exportMaxDuration) {
        modalDialog = SuccessOrErrorDialog::buildForError(juce::translate("The loop is too large! It must be inferior to ")
                + juce::String(exportMaxDuration) + " frames.",
                [this] { closeAdditionalDialog(); });
        return;
    }

    // Builds the frames.
    std::vector<PsgRegisters> psgRegisterFrames;
    const auto psgIndex = PsgValues::getPsgIndex(selectedLinearChannelIndex);
    for (auto frameIndex = startIndex; frameIndex <= endIndex; ++frameIndex) {
        const auto& [psgRegisters, success] = streamedMusicReader->getRegisters(psgIndex, frameIndex);
        jassert(success);
        psgRegisterFrames.push_back(psgRegisters);
    }

    const auto selectedChannelIndex = PsgValues::getChannelIndex(selectedLinearChannelIndex, psgIndex);
    const auto instrumentName = instrumentNameTextEditor.getText();

    const auto targetPsg = psgs.at(static_cast<size_t>(outputPsgComboBox.getSelectedItemIndex()));
    const auto sourcePsgFrequencyHz = streamedMusicReader->getPsgFrequencyHz(psgIndex);
    const auto targetPsgFrequencyHz = targetPsg.getPsgFrequency();
    const auto referenceFrequencyHz = targetPsg.getReferenceFrequency();

    const auto encodeAsForcedPeriods = encodeAsPeriodsToggle.getToggleState();

    // Performs the export.
    auto instrument = InstrumentStreamExporter::exportStreamAsInstrument(instrumentName, selectedChannelIndex, psgRegisterFrames,
                                                                         isLooping, encodeAsForcedPeriods,
                                                                         sourcePsgFrequencyHz, targetPsgFrequencyHz, referenceFrequencyHz);
    // Stores it.
    songController.addInstrument(std::move(instrument));

    jassert(modalDialog == nullptr);
    modalDialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Instrument generated!"), [&] { closeAdditionalDialog(); });
}

void StreamedMusicAnalyzer::onExportToCsvClicked() noexcept
{
    // Chooses a target file.
    const auto extensionFilter = FileExtensions::csvExtensionWithWildcard;
    FileChooserCustom fileChooser(juce::translate("Export to CSV"), FolderContext::other, extensionFilter);
    const auto success = fileChooser.browseForFileToSave(true);
    if (!success) {
        return;
    }
    const auto fileToSaveTo = fileChooser.getResultWithExtensionIfNone(extensionFilter);

    (void)fileToSaveTo.deleteFile();
    juce::FileOutputStream fis(fileToSaveTo);

    const auto returnCarriage = fis.getNewLineString();
    fis.writeText("frameIndex;psgIndex;r0;r1;r2;r3;r4;r5;r6;r7;r8;r9;r10;r11;r12;r13;isR13Retrigged" + returnCarriage, false,
                  false,nullptr);

    const auto localIterationCount = iterationCount.load();
    const auto localPsgCount = psgCount.load();
    // Reads each frame, dumbly exports each registers for each PSG.
    for (auto frameIndex = 0; frameIndex < localIterationCount; ++frameIndex) {
        for (auto psgIndex = 0; psgIndex < localPsgCount; ++psgIndex) {
            constexpr auto lastExportedRegisterInCsv = 13;
            fis.writeText(juce::String(frameIndex) + ";" + juce::String(psgIndex), false, false, nullptr);

            const auto psgRegistersResult = streamedMusicReader->getRegisters(psgIndex, frameIndex);

            jassert(psgRegistersResult.second);
            const auto& psgRegisters = psgRegistersResult.first;
            for (auto registerIndex = 0; registerIndex <= lastExportedRegisterInCsv; ++registerIndex) {
                const auto value = psgRegisters.getValueFromRegister(registerIndex);

                fis.writeText(";" + juce::String(value), false, false, nullptr);
            }
            // Retrig?
            fis.writeText(";" + StringUtil::boolToString(psgRegisters.isRetrig()), false, false, nullptr);
            // EOL.
            fis.writeText(returnCarriage, false, false, nullptr);
        }
    }

    fis.flush();

    modalDialog = SuccessOrErrorDialog::buildForSuccess(juce::translate("Export to CSV done!"), [&] { closeAdditionalDialog(); });
}

void StreamedMusicAnalyzer::clearPreviouslyLoadedData() noexcept
{
    previouslyLoadedSongPath = juce::String();
    previouslyLoadedSongCurrentIteration = 0;
    previouslyLoadedSongLoopStartIndex = 0;
    previouslyLoadedSongEndIndex = 0;
}

bool StreamedMusicAnalyzer::arePreviouslyLoadedDataPresent() noexcept
{
    return (previouslyLoadedSongPath.isNotEmpty());
}

}   // namespace arkostracker
