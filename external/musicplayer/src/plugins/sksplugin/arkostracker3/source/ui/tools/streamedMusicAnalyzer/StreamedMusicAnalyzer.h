#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../audio/sources/PsgStreamGenerator.h"
#include "../../../audio/sources/PsgsProcessor.h"
#include "../../../controllers/MainController.h"
#include "../../../player/PsgRegistersProvider.h"
#include "../../../reader/StreamedMusicReader.h"
#include "../../components/SliderIncDec.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../../utils/backgroundOperation/BackgroundOperationWithDialog.h"

namespace arkostracker 
{

class MainController;

/**
 * Dialog to load a YM/VGM, plays it, extracts instruments from it.
 */
class StreamedMusicAnalyzer final : public ModalDialog,
                                    public juce::FileDragAndDropTarget,                   // To detect drag'n'drop of external files.
                                    public PsgRegistersProvider,
                                    public SliderIncDec::Listener,
                                    juce::Timer                                           // Timer upon which to update the UI.
{
public:
    /**
     * Constructor.
     * @param mainController the main controller.
     * @param listener the listener to close this Dialog.
     */
    StreamedMusicAnalyzer(const MainController& mainController, std::function<void()> listener) noexcept;

    /** Destructor. */
    ~StreamedMusicAnalyzer() override;

    // ModalDialog method implementations.
    // ===================================================
    void closeButtonPressed() override;

    // PsgRegistersProvider method implementations.
    // ===================================================
    std::pair<std::unique_ptr<PsgRegisters>, std::unique_ptr<SampleData>> getNextRegisters(int psgIndex) noexcept override;

    // SliderIncDec::Listener method implementations.
    // ===================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

    // FileDragAndDropTarget methods implementation.
    // ===================================================================
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    static const int refreshTimerMs;
    static const int maximumPsgCount;
    static const int nowLoopDefaultDuration;                    // The loop duration used when "now is loop" is clicked.
    static const int exportMaxDuration;

    /** Called when OK button is clicked. Exits. */
    void onOkButtonClicked() const noexcept;

    /** Called when the Load button is clicked. */
    void onLoadButtonClicked() noexcept;

    /** Releases the audio source. Needed when a new song is loaded. */
    void releaseAudioSource() noexcept;
    /** Sets up the audio, from the current data. MUST be called AFTER the song is successfully loaded! */
    void setupAudio() noexcept;

    /** Loads a File, that can be of any type. If loaded correctly, updates the UI and starts playing it. */
    void loadFile(const juce::File& file) noexcept;

    /** Called on the UI thread when a song is successfully loaded. The music and reader are set. This sets-up the player and UI. */
    void onMusicLoaded() noexcept;

    /** Called when the user clicked on the play/stop song Button. */
    void onPlayStopSongButtonClicked() noexcept;
    /** Called when the user moved the progress slider. */
    void onUserMovedProgressSlider() noexcept;
    /**
     * Called when the user clicked on next/previous buttons.
     * @param offset relative offset where to go.
     */
    void onUserWantsToMoveToRelativeFrame(int offset) noexcept;

    /**
     * Sets the current frame.
     * @param frameIndex where to go. It is corrected.
     */
    void gotoFrame(int frameIndex) noexcept;

    /** Switches between loop on/off. */
    void onLoopButtonClicked() noexcept;
    /** Switches between lock length on/off. */
    void onLockLengthButtonClicked() noexcept;

    /**
     * Updates the loop internal values and the loop UI.
     * @param newLoopStart the new loop start. Must be valid.
     * @param newLoopEnd the new loop end. Must be valid.
     */
    void updateLoopValueAndSlider(int newLoopStart, int newLoopEnd) noexcept;

    /**
     * Updates the loop internal value and speed UI.
     * @param newSpeed the new speed. Must be valid.
     */
    void updateSpeedValueAndSlider(int newSpeed) noexcept;

    /** The user muted/unmuted a channel. */
    void onUserChangedMutedChannels() noexcept;

    /** Displays the PSG values that has been stored. */
    void displayPsgValues() noexcept;

    /** Displays the PSG info according to the current PSG index. */
    void displayPsgInfo() noexcept;

    /** Called when the "now is loop" button is clicked. */
    void onNowIsLoopButtonClicked() noexcept;

    /** Called when the "XXX/XXX" button is clicked. */
    void onProgressButtonClicked() noexcept;

    /** Closes the possible additional dialog (error, etc.). */
    void closeAdditionalDialog() noexcept;

    /**
     * Called when the background operation to load a music is finished. This is called on the UI thread.
     * @param readerAndMusic the reader and music if successful, both nullptr if couldn't load the music with an available format.
     */
    void onLoadMusicOperationFinished(std::pair<std::unique_ptr<StreamedMusicReader>, std::unique_ptr<juce::InputStream>> readerAndMusic) noexcept;

    /** Fills the PSG Combobox with the PSG frequencies the "real" Song contains. */
    void fillPsgComboBox() noexcept;

    /** Called when the Export button is clicked. */
    void onExportButtonClicked() noexcept;

    /** Called when the "export to CSV" button is clicked. */
    void onExportToCsvClicked() noexcept;

    /** @return the length of the loop. May be 0. */
    int getLoopLength() const noexcept;

    /** Makes the loop start/end enabled or not according to the loop state. */
    void updateLoopComponentsEnabledFromLoopState() noexcept;

    /**
     * @return the given iteration, corrected from the loop/length according to the play state.
     * @param iterationToCorrect the iteration.
     */
    int getCorrectedIteration(int iterationToCorrect) const noexcept;

    /** Clears the possible data of the previously loaded song. */
    static void clearPreviouslyLoadedData() noexcept;
    /** @return true if the data of the previously loaded song is stored. */
    static bool arePreviouslyLoadedDataPresent() noexcept;

    // juce::Timer method implementations.
    // ===================================================
    void timerCallback() override;

    // ===================================================

    static juce::String previouslyLoadedSongPath;                       // Quick storage for the last loaded song.
    static int previouslyLoadedSongCurrentIteration;
    static int previouslyLoadedSongLoopStartIndex;
    static int previouslyLoadedSongEndIndex;

    static constexpr auto fastBrowseSpeed = 20;
    static constexpr auto progressMaxLengthInEditText = 6;

    std::function<void()> modalCallback;

    SongController& songController;
    const Song& song;

    juce::String loadedSongPath;

    std::unique_ptr<StreamedMusicReader> streamedMusicReader;
    std::unique_ptr<juce::InputStream> streamedMusic;                   // The loaded music.
    std::atomic_int currentIteration;                                   // The current iteration, used by ui and audio thread.
    std::atomic_int iterationCount;
    std::atomic_int lastMaxIteration;                                   // According to the music.
    std::atomic_int loopStartIndex;                                     // As set by the user.
    std::atomic_int loopEndIndex;                                       // As set by the user.

    juce::AudioDeviceManager& audioDeviceManager;                       // The audio device to use when playing.
    std::vector<std::unique_ptr<PsgStreamGenerator>> psgStreamGenerators; // Produces the AY sounds. One per PSG! Must be deleted last.
    PsgsProcessor psgsMixer;                                            // Mixes all the PSG, applies the effects.
    std::unique_ptr<juce::AudioSourcePlayer> audioSourcePlayer;       // Streams continuously and plays to an output. Only one, linked to the MixerAudioSource. Must be deleted first!

    std::atomic_bool isPlaying;
    std::atomic_bool isLooping;
    std::atomic_int currentTick;                                            // From 0 to Speed excluded.
    std::atomic_int speed;
    std::atomic_int psgCount;
    std::vector<std::unordered_set<int>> psgToMutedChannelIndexes;          // Only muted changed are set inside. No need to "atomic" it.

    juce::TextButton exportToCsvButton;
    juce::TextButton loadButton;                                            // Button to load a music.
    juce::Label loadOrDropFileLabel;
    juce::Label musicInfoLabel;                                             // Shows the title/author.
    ButtonWithImage playStopButton;
    ButtonWithImage nextFrameButton;
    ButtonWithImage previousFrameButton;
    juce::Slider progressSlider;                                            // Shows the start/end/progress.
    juce::TextButton progressButton;                                        // Shows the iteration index/count.
    ButtonWithImage lockLengthButton;
    ButtonWithImage loopButton;
    SliderIncDec loopStartSlider;
    SliderIncDec loopEndSlider;
    juce::TextButton nowIsLoopButton;
    SliderIncDec speedSlider;
    std::vector<std::unique_ptr<juce::ToggleButton>> channelOnToggleButtons;    // A max is created (2 PSGs). Enough for now.

    juce::GroupComponent psgValuesGroup;
    SliderIncDec psgSlider;
    juce::Label psgInfo;
    juce::Label channel0PeriodCaption;
    juce::Label channel1PeriodCaption;
    juce::Label channel2PeriodCaption;
    juce::Label volume0Caption;
    juce::Label volume1Caption;
    juce::Label volume2Caption;
    juce::Label mixerCaption;
    juce::Label noiseCaption;
    juce::Label hardwarePeriodCaption;
    juce::Label hardwareEnvelopeCaption;
    
    juce::Label channel0PeriodLabel;
    juce::Label channel1PeriodLabel;
    juce::Label channel2PeriodLabel;
    juce::Label channel0VolumeLabel;
    juce::Label channel1VolumeLabel;
    juce::Label channel2VolumeLabel;
    juce::Label mixerLabel;
    juce::Label mixerBinaryLabel;
    juce::Label noiseLabel;
    juce::Label hardwarePeriodLabel;
    juce::Label hardwareEnvelopeLabel;

    juce::GroupComponent exportGroup;
    juce::Label instrumentNameLabel;
    EditText instrumentNameTextEditor;
    juce::Label outputPsgLabel;
    juce::ComboBox outputPsgComboBox;
    juce::ToggleButton encodeAsPeriodsToggle;
    juce::ToggleButton encodeAsNotesToggle;
    juce::TextButton exportButton;

    std::atomic_int shownPsgIndex;

    std::atomic_int shownPsgChannel0Period;
    std::atomic_int shownPsgChannel1Period;
    std::atomic_int shownPsgChannel2Period;
    std::atomic_int shownPsgChannel0Volume;
    std::atomic_int shownPsgChannel1Volume;
    std::atomic_int shownPsgChannel2Volume;
    std::atomic_int shownPsgNoise;
    std::atomic_int shownPsgMixer;
    std::atomic_int shownPsgHardwarePeriod;
    std::atomic_int shownPsgHardwareEnvelope;
    std::atomic_bool shownPsgHardwareRetrig;

    std::vector<Psg> psgs;                          // The unique PSGs, as displayed.

    std::unique_ptr<BackgroundOperationWithDialog<std::pair<std::unique_ptr<StreamedMusicReader>, std::unique_ptr<juce::InputStream>>>> loadMusicOperation;

    std::unique_ptr<ModalDialog> modalDialog;

    bool isLoopLengthLocked;

    juce::TextEditor::LengthAndCharacterRestriction progressDialogEditTextRestriction;
};

}   // namespace arkostracker
