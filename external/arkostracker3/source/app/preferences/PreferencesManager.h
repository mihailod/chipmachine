#pragma once

#include <unordered_map>

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "../../business/sourceProfile/SourceProfiles.h"
#include "../../controllers/serial/model/SerialData.h"
#include "../../controllers/serial/model/SerialProfile.h"
#include "../../export/model/EncodedDataFlag.h"
#include "../../export/samples/SampleEncoderFlags.h"
#include "../../song/cells/Effect.h"
#include "../../ui/export/events/EventsExportType.h"
#include "../../ui/keyboard/VirtualKeyboardLayout.h"
#include "../../ui/lookAndFeel/serialization/StoredLookAndFeel.h"
#include "model/RawLinearExportType.h"
#include "model/StoredOutputMix.h"

namespace arkostracker
{

/** Singleton to loads and stores data for the application. */
class PreferencesManager
{
public:
    /** @return the unique instance of the Manager. */
    static PreferencesManager& getInstance();

    /** @return the application name. It can be use to create log/properties folder, for example. */
    static juce::String getApplicationName();

    // General properties.
    // =============================================================

    class MainWindowSize
    {
    public:
        MainWindowSize(const int pWidth, const int pHeight, const bool pFullscreen) :
                width(pWidth),
                height(pHeight),
                fullscreen(pFullscreen)
        {
        }

        int getWidth() const noexcept
        {
            return width;
        }

        int getHeight() const noexcept
        {
            return height;
        }

        bool isFullscreen() const noexcept
        {
            return fullscreen;
        }

    private:
        int width;
        int height;
        bool fullscreen;
    };
    /**
     * Stores the dimension of the main window.
     * @param width the width.
     * @param height the height.
     * @param isFullscreen true if fullscreen.
     */
    void storeMainWindowSize(int width, int height, bool isFullscreen) const noexcept;

    /** @return the previously stored window size, or 0 everywhere if not stored. */
    MainWindowSize getMainWindowSize() const;

    /**
     * Sets the property to use the native title bar or not.
     * @param nativeTitleBarUsed true to use the native title bar.
     */
    void setNativeTitleBarUsed(bool nativeTitleBarUsed) const noexcept;

    /** @return true if the native title bar must be used. */
    bool isNativeTitleBarUsed() const noexcept;

    /**
     * Sets the property to use OpenGL or not.
     * @param useOpenGl true to use OpenGL.
     */
    void setUseOpenGl(bool useOpenGl) const noexcept;

    /** @return true if OpenGL should be used. */
    bool mustUseOpenGl() const noexcept;

    /** @return true if the song info are shown on load. */
    bool isSongInfoShownAfterLoad() const noexcept;
    /**
     * Sets the property to show the song info after loading.
     * @param show true to show.
     */
    void setShowSongInfoAfterLoad(bool show) const noexcept;

    /** @return true if the splash is shown on startup. */
    bool isSplashShown() const noexcept;
    /**
     * Sets the property to show the splash screen on startup.
     * @param show true to show.
     */
    void setShowSplash(bool show) const noexcept;

    /** @return true if non-native file picker are chosen when loading instruments. */
    bool isForceNonNativeFileChooserOnLoadInstrument() const noexcept;
    /**
     * Sets the property to force the use of non-native file picker when loading instruments.
     * @param useNonNative true to use non-native file picker.
     */
    void setForceNonNativeFileChooserOnLoadInstrument(bool useNonNative) const noexcept;

    /**
     * Sets the property to add the octave to every received MIDI notes.
     * @param mustAdd true to add.
     */
    void setMustAddOctaveToInputMidiNotes(bool mustAdd) const noexcept;

    /** @return true to add the octave to every received MIDI notes. */
    bool mustAddOctaveToInputMidiNotes() const noexcept;


    // Audio interfaces.
    // =============================================================

    /** @return the possibly saved Audio Properties, as an XML node. May be nullptr if they are not already saved. */
    std::unique_ptr<juce::XmlElement> getAudioProperties() const noexcept;

    /**
     * Saves the audio properties from the given Audio Device Selector Component.
     * @param audioDeviceSelectorComponent the Audio Device Selector Component.
     */
    void saveAudioProperties(const juce::AudioDeviceSelectorComponent& audioDeviceSelectorComponent) const noexcept;


    // Keyboard mapping.
    // =============================================================

    /** @returns the possibly saved Key Mappings, as an XML node. May be nullptr if they have never been modified from the default. */
    std::unique_ptr<juce::XmlElement> getKeyMapping() const noexcept;

    /**
     * Saves the key mapping from the given KeyMapping Component.
     * @param keyMappingEditorComponent the key mapping Component.
     */
    void storeKeyMapping(const juce::KeyMappingEditorComponent& keyMappingEditorComponent) const noexcept;


    // Layout arrangement.
    // =============================================================

    /**
     * Returns the XML element of a Layout Arrangement, if found.
     * @param number the number (>=1).
     * @return the XML element, or nullptr.
     */
    std::unique_ptr<juce::XmlElement> getLayoutArrangement(int number) const noexcept;

    /**
     * Stores the given XML element of a Layout Arrangement. Its validity is not checked!
     * @param number the number (>=1).
     * @param rootXlmElement the element to store.
     */
    void storeLayoutArrangement(int number, const juce::XmlElement& rootXlmElement) const noexcept;


    // Look and feel.
    // ============================================================

    /** @return the currently used theme id (>0). */
    int getCurrentThemeId() const noexcept;
    /**
     * Stores the ID of the theme that currently used.
     * @param themeId the ID (>0).
     */
    void setCurrentThemeId(int themeId) const noexcept;

    /**
     * @return a theme. Only custom Themes are stored. Nullptr if not found.
     * @param themeId the ID of the Theme.
     */
    std::unique_ptr<StoredLookAndFeel> getCustomTheme(int themeId) const noexcept;

    /**
     * Stores the given Look And Feel as the current one. Note that its ID+1 may become the next ID for a new LookAndFeel.
     * @param storedLookAndFeel the theme to store.
     */
    void storeCustomLookAndFeel(const StoredLookAndFeel& storedLookAndFeel) const noexcept;

    /** @return an ID for a custom LookAndFeel. Note that it must be after the last one, so that newly created items are put at the end. */
    int findNewFreeCustomLookAndFeelId() const noexcept;

    /** @return all the custom Theme IDs, in order. */
    std::vector<int> getAllCustomThemeIds() const noexcept;

    /**
     * Deletes a Theme. It should exist.
     * @param themeId the ID of the Theme.
     */
    void deleteCustomLookAndFeel(int themeId) const noexcept;


    // Keyboard.
    // ========================================================

    /** @return true when the user has explicitly set the keyboard layout. */
    bool isVirtualKeyboardLayoutExplicitlySet() const noexcept;

    /** @return the default virtual keyboard layout. It is "none" if none has been defined. */
    VirtualKeyboardLayout getVirtualKeyboardLayout() const noexcept;

    /**
     * Stores the virtual keyboard layout currently used. This also set the "is keyboard layout set explicitly".
     * @param keyboardLayout the virtual keyboard layout.
     */
    void setVirtualKeyboardLayout(VirtualKeyboardLayout keyboardLayout) const noexcept;


    // PatternViewer.
    // ============================================================

    /** @return the true if the line number is displayed in hexadecimal, false if decimal, in the PatternViewer. */
    bool isPatternViewerLineNumberInHexadecimal() const noexcept;
    /**
     * Indicates whether the line number is displayed in hexadecimal or decimal, in the PatternViewer.
     * @param isHexadecimal true if hexadecimal, false if decimal.
     */
    void setPatternViewerLineNumberInHexadecimal(bool isHexadecimal) const noexcept;

    /**
     * Sets the height of the font used in the PatternViewer.
     * @param height the height.
     */
    void setPatternViewerFontHeight(int height) const noexcept;
    /** @return the height of the font used in the PatternViewer. */
    int getPatternViewerFontHeight() const noexcept;

    /**
     * Sets whether the middle line in the PatternViewer is zoomed.
     * @param mustZoom true if zoomed.
     */
    void setPatternViewerMustZoomMiddleLine(bool mustZoom) const noexcept;
    /** @return true if the middle line in the PatternViewer is zoomed. */
    bool getPatternViewerMustZoomMiddleLine() const noexcept;

    /**
     * Sets the steps for the scrolling with the mouse wheel in the PatternViewer.
     * @param steps the steps.
     */
    void setPatternViewerMouseWheelSteps(int steps) const noexcept;
    /** @return the height of the font used in the PatternViewer. */
    int getPatternViewerMouseWheelSteps() const noexcept;

    /** @return a map linking an effect to its displayed char. The result is always valid. */
    std::unordered_map<Effect, juce::juce_wchar> getEffectToChar() const noexcept;
    /**
     * Stores the effect to displayed char map.
     * @param effectToChar map linking an effect to its displayed char.
     */
    void setEffectToChar(const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) const noexcept;


    // Recent folders.
    // ======================================================

    /** Stores the folders from the local static LastFolderTempStorage. */
    void saveRecentFolders() const noexcept;

    /** Fills the local static LastFolderTempStorage from the stored folder in the Preferences. */
    void readStoredFoldersAndFillLastFolderStorage() const noexcept;


    // Recent files.
    // ======================================================

    /** @recent the path to the recently opened/saved files. May be empty. */
    std::vector<juce::String> getRecentFiles() const noexcept;

    /**
     * Removes the file which path is given, from the stored list.
     * @param pathToRemove the full path to the file to remove. It should exist. If not, nothing happens.
     */
    void removeRecentFile(const juce::String& pathToRemove) const noexcept;

    /**
     * Adds the file which path is given. It is added at the top of the list. Can be used whenever the file is accessed, so that it stays at the top.
     * @param path the full path to the file to add/update.
     */
    void addOrUpdateRecentFile(const juce::String& path) const noexcept;

    /** Clears all the recent files. */
    void clearRecentFiles() const noexcept;

    /** @return true if the saved files must be compressed. */
    bool areSavedFilesCompressed() const noexcept;

    /**
     * Indicates whether the files are compressed when saved.
     * @param compress true to compress.
     */
    void setFilesCompressionOnSave(bool compress) const noexcept;


    // Source profiles.
    // ===============================================================================

    /** @return the name of the currently used Source Profile. */
    int getCurrentSourceProfileIndex() const noexcept;

    /**
     * Sets the name of the currently used Source Profile.
     * @param index the index.
     */
    void setCurrentSourceProfileFromIndex(int index) const noexcept;

    /** @return the Source Profiles from both the templates and the user (if any). */
    std::vector<SourceProfile> getSourceProfiles() const noexcept;

    /** @return the current Source Profile. If unable, asserts but returns a default profile */
    SourceProfile getCurrentSourceProfile() const noexcept;

    /**
     * Stores the given Source Profiles. Only the user-template it contains will be stored.
     * @param sourceProfiles the profiles to store. The templates may also be given, they will be filtered.
     */
    void setSourceProfiles(const std::vector<SourceProfile>& sourceProfiles) const noexcept;

    /** @return true if export as source, false if binary. */
    bool isExportAsSource() const noexcept;
    /**
     * Sets whether to export as source or binary.
     * @param exportAsSource true to export as source, false as binary.
     */
    void setExportAsSource(bool exportAsSource) const noexcept;

    /** @return true if the address must be exported (for binary this flag can be ignored, as always true), and the address itself. */
    std::pair<bool, int> getUseAddressAndAddress() const noexcept;
    /**
     * Sets the parameters about whether the address must be encoded, and the address itself.
     * @param useAddress true to use the address.
     * @param address the address. Even if not used, must be valid.
     */
    void setUseExportAddressAndAddress(bool useAddress, int address) const noexcept;

    /** Indicates whether the export of the player configuration file is activated. */
    bool isExportPlayerConfigurationFile() const noexcept;
    /**
     * Sets the export of the player configuration file.
     * @param exportPlayerConfigurationFile true to export of the player configuration file.
     */
    void setExportPlayerConfigurationFile(bool exportPlayerConfigurationFile) const noexcept;

    /** @return true if the addresses must be encoded relative to the Song start. Useful for Atari ST players. */
    bool isEncodeAllAddressesAsRelativeToSongStart() const noexcept;
    /**
     * Stores the parameter about encoding the addresses as relative to the Song Start. Useful for Atari ST players.
     * @param encodeAsRelative true if the addresses must be encoded relative to the Song start.
     */
    void setEncodeAllAddressesAsRelativeToSongStart(bool encodeAsRelative) const noexcept;

    /** @return true if the export is done on several files. */
    bool isExportToSeveralFiles() const noexcept;
    /**
     * Stores the parameter about export is done on several files.
     * @param exportToSeveralFiles true if the export is done on several files.
     */
    void setExportToSeveralFiles(bool exportToSeveralFiles) const noexcept;

    /** @return the prefix used in the source labels. May be empty. */
    juce::String getSourceLabelsPrefix() const noexcept;
    /**
     * Sets the prefix used in the source labels.
     * @param label the label. May be empty. May contain a separator.
     */
    void setSourceLabelsPrefix(const juce::String& label) const noexcept;


    // Sample export.
    // ======================================================

    /** @return the stored flags of the Sample Export. */
    SampleEncoderFlags getSampleEncoderFlags() const noexcept;

    /**
     * Stores the flags of the Sample Encoder.
     * @param flags the flags to store.
     */
    void setSampleEncoderFlags(const SampleEncoderFlags& flags) const noexcept;

    /** @return the sample export pitch, in semi-tons + cents. */
    double getSampleExportPitch() const noexcept;

    /**
     * Stores the sample export pitch (in semi-tones + cents).
     * @param pitch the pitch (in semi-tones + cents).
     */
    void storeSampleExportPitch(double pitch) const noexcept;


    // Output mix.
    // ======================================================

    /** @return the output mix, from file or from cache, as it is often needed. */
    StoredOutputMix getOutputMix() noexcept;

    /**
     * Stores the output mix.
     * @param outputMix the mix.
     */
    void storeOutputMix(const StoredOutputMix& outputMix) noexcept;


    // Serial.
    // ======================================================

    /** @return the selected port ("COM1", "/etc/..."), or empty if none is. */
    juce::String getSerialPort() const noexcept;
    /**
     * Stores the selected serial port. It should be valid.
     * @param serialPort the serial port.
     */
    void storeSerialPort(const juce::String& serialPort) const noexcept;

    /** @return the stored Serial Profile (matching the one in the SerialTab). Custom if unknown. */
    SerialProfile getSelectedSerialProfile() const noexcept;
    /**
     * Stores the selected Serial Profile. If custom, don't forget to store it using the storeCustomProfile method.
     * @param serialProfile the profile.
     */
    void storeSelectedSerialProfile(SerialProfile serialProfile) const noexcept;

    /** @return the stored custom profile. The user may have never selected it. The port is empty and should not be used. Call getSerialPort to get it. */
    SerialData getSerialCustomProfile() const noexcept;
    /**
     * Stores the custom profile for serial. This does not change the selected profile.
     * @param serialData the data. The port is ignored.
     */
    void storeSerialCustomProfile(const SerialData& serialData) const noexcept;


    // Raw exporter.
    // ======================================================

    /** @return the flags of the Raw Exporter. */
    EncodedDataFlag getRawExportFlags() const noexcept;

    /**
     * Stores the flags of the Raw Exporter.
     * @param flags the flags to store.
     */
    void setRawExportFlags(const EncodedDataFlag& flags) const noexcept;


    // Events exporter.
    // ======================================================

    /** @return the export type for events. */
    EventsExportType getEventsExportType() const noexcept;

    /**
     * Stores the given events export type.
     * @param type the type to store.
     */
    void storeEventsExportType(EventsExportType type) const noexcept;


    // Raw linear exporter.
    // ======================================================

    /** @return the RAW linear export type. */
    RawLinearExportType getRawLinearExportType() const noexcept;

    /**
     * Stores the given RAW linear export type.
     * @param type the type to store.
     */
    void storeRawLinearExportType(RawLinearExportType type) const noexcept;


    // FAP.
    // ======================================================

    /** @return the possible FAP output frequency in Hz, or absent if none is forced. */
    OptionalInt getFapOutputFrequencyHz() const noexcept;

    /** Sets the possible FAP output frequency in Hz, or absent if none is forced. */
    void storeFapOutputFrequencyHz(OptionalInt frequencyHz) const noexcept;

private:
    static const juce::String recentFilesSeparator;                             // The separator between each path stored for the recent files.
    static const size_t maximumCountRecentFiles;                                // How many recent files are stored.

    juce::String keyBaseLayoutArrangement = "baseLayoutArrangement";            // Must be followed by a number (>=1).
    juce::String keyKeyboardMapping = "keyboardMapping";                        // The key to the keyboard mapping.
    juce::String keyWindowWidth = "windowWidth";
    juce::String keyWindowHeight = "windowHeight";
    juce::String keyIsWindowFullscreen = "isWindowFullscreen";
    juce::String keyUseNativeTitleBar = "useNativeTitleBar";
    juce::String keyUseOpenGl = "mustUseOpenGl";
    juce::String keyShowSongInfoAfterLoad = "showSongInfoAfterLoad";
    juce::String keyShowSplash = "showSplash";
    juce::String keyForceNonNativeFileChooserOnLoadInstrument = "forceNonNativeFileChooserOnLoadInstrument";
    juce::String keyMustAddOctaveToInputMidiNotes = "mustAddOctaveToInputMidiNotes";
    juce::String keyCurrentThemeId = "currentThemeId";
    juce::String keyBaseCustomTheme = "baseCustomTheme";                        // Must be followed by a number (>=1).
    juce::String keyPatternViewerFontHeight = "patternViewerFontHeight";
    juce::String keyPatternViewerMustZoomMiddleLine = "patternViewerMustZoomMiddleLine";
    juce::String keyPatternViewerMouseWheelSteps = "patternViewerMouseWheelSteps";
    juce::String keyPatternViewerLineNumberInHexadecimal = "patternViewerLineNumberInHexadecimal";
    juce::String keyEffectToChar = "effectToChar";
    juce::String keyIsVirtualKeyboardLayoutSetExplicitly = "isVirtualKeyboardLayoutSetExplicitly";
    juce::String keyVirtualKeyboardLayout = "virtualKeyboardLayout";
    juce::String keyAudioDeviceManagerProperties = "audioDeviceManagerProperties";

    juce::String keyRecentFolderLoadSaveSong = "recentFolderLoadSaveSong";                        // The key to the most recent folder.
    juce::String keyRecentFolderLoadSaveInstrument = "recentFolderLoadSaveInstrument";
    juce::String keyRecentFolderLoadSaveArpeggio = "recentFolderLoadSaveArpeggio";
    juce::String keyRecentFolderLoadSavePitch = "recentFolderLoadSavePitch";
    juce::String keyRecentFolderAnyExport = "recentFolderAnyExport";
    juce::String keyRecentFolderOther = "recentFolderOther";
    juce::String keyRecentFiles = "recentFiles";
    juce::String keyCompressOnSave = "compressOnSave";

    juce::String keySourceProfileCurrentIndex = "sourceProfileCurrentIndex";
    juce::String keySourceProfiles = "sourceProfiles";
    juce::String keyExportAsSource = "exportAsSource";
    juce::String keyExportedAddress = "exportedAddress";
    juce::String keyMustExportAddress = "mustExportAddress";
    juce::String keyExportPlayerConfigurationFile = "exportPlayerConfigurationFile";
    juce::String keyEncodeAllAddressesAsRelativeToSongStart = "encodeAllAddressesAsRelativeToSongStart";
    juce::String keyExportToSeveralFiles = "exportToSeveralFiles";
    juce::String keySourceLabelsPrefix = "sourceLabelsPrefix";

    juce::String keySampleExportEnabled = "sampleExportEnabled";
    juce::String keySampleExportAmplitude = "sampleExportAmplitude";
    juce::String keySampleExportOffset = "sampleExportOffset";
    juce::String keySampleExportPaddingLength = "sampleExportPaddingLength";
    juce::String keySampleExportFadeOutSampleCountForNonLoopingSamples = "sampleExportFadeOutSampleCountForNonLoopingSamples";
    juce::String keySampleExportForcePaddingAndFadeOutToLowestValue = "sampleExportForcePaddingAndFadeOutToLowestValue";
    juce::String keySampleExportOnlyExportUsedLength = "sampleExportOnlyExportUsedLength";
    juce::String keySampleExportGenerateIndexTable = "sampleExportGenerateIndexTable";
    juce::String keySampleExportIgnoredUnusedSamples = "sampleExportIgnoredUnusedSamples";

    juce::String keySampleExportPitch = "sampleExportPitch";

    juce::String keyOutputMixGlobalVolumePercent = "outputMixGlobalVolumePercent";
    juce::String keyOutputMixStereoMixType = "outputMixStereoMixType";
    juce::String keyOutputMixCustomLeftVolumePercent = "outputMixCustomLeftVolumePercent";
    juce::String keyOutputMixCustomCenterVolumePercent = "outputMixCustomCenterVolumePercent";
    juce::String keyOutputMixCustomRightVolumePercent = "outputMixCustomRightVolumePercent";
    juce::String keyOutputMixEmulateSpeaker = "outputMixEmulateSpeaker";
    juce::String keyOutputMixStereoSeparationPercent = "outputMixStereoSeparationPercent";

    juce::String keySerialSelectedPort = "serialSelectedPort";
    juce::String keySerialProfileId = "serialProfileId";
    juce::String keySerialCustomBaudRate = "serialCustomBaudRate";
    juce::String keySerialCustomByteSize = "serialCustomByteSize";
    juce::String keySerialCustomParity = "serialCustomParity";
    juce::String keySerialCustomStopBitId = "serialCustomStopBitId";
    juce::String keySerialCustomFlowControlId = "serialCustomFlowControlId";

    juce::String keyRawExporterSongAndSubsongMetadata = "rawExporterSongAndSubsongMetadata";
    juce::String keyRawExporterReferenceTables = "rawExporterReferenceTables";
    juce::String keyRawExporterSpeedTrack = "rawExporterSpeedTrack";
    juce::String keyRawExporterEventTrack = "rawExporterEventTrack";
    juce::String keyRawExporterInstruments = "rawExporterInstruments";
    juce::String keyRawExporterArpeggioTables = "rawExporterArpeggioTables";
    juce::String keyRawExporterPitchTables = "rawExporterPitchTables";
    juce::String keyRawExporterEffects = "rawExporterEffects";
    juce::String keyRawExporterRleForEmptyLines = "rawExporterRleForEmptyLines";
    juce::String keyRawExporterTranspositions = "rawExporterTranspositions";
    juce::String keyRawExporterHeight = "rawExporterHeight";
    juce::String keyRawExporterPitchTrackEffect = "rawExporterPitchTrackEffect";

    juce::String keyEventsExportType = "eventsExportType";
    juce::String keyRawLinearExportType = "rawLinearExportType";
    juce::String keyFapForcedOutputFrequencyHz = "fapForcedOutputFrequencyHz";

    /** Constructor. */
    PreferencesManager() noexcept;

    /**
     * Saves a value.
     * @param key the key.
     * @param value the value.
     */
    void saveValue(const juce::String& key, const juce::var& value) const noexcept;

    /**
     * Saves an XML element.
     * @param key the key.
     * @param xmlElement the element to save.
     */
    void saveXmlValue(const juce::String& key, const juce::XmlElement& xmlElement) const noexcept;

    /**
     * @returns the key to store/retrieve a layout arrangement.
     * @param number the number (>=1).
     */
    juce::String getKeyForLayoutArrangement(int number) const noexcept;
    /**
     * @returns the key to store/retrieve a custom profile ID.
     * @param profileId the profileId. Must be >= LookAndFeelConstants::ThemeId::minimumId.
     */
    juce::String getKeyForCustomTheme(int profileId) const noexcept;

    /**
     * Stores the recent files, replacing all the others. Only a certain amount of paths are saved, the others are ignored.
     * @param paths the paths to save.
     */
    void encodeRecentFiles(const std::vector<juce::String>& paths) const noexcept;

    /**
     * @return the stereo mix type from the given String. If not found, asserts and uses a default.
     * @param inputString a serialized StereoMixType.
     */
    static StoredOutputMix::StereoMixType deserializeStereoMixType(const juce::String& inputString) noexcept;

    /**
     * @return the serialized stereo mix type.
     * @param stereoMixType the mix type.
     */
    static juce::String serializeStereoMixType(StoredOutputMix::StereoMixType stereoMixType) noexcept;

    juce::ApplicationProperties applicationProperties;        // The properties.
    juce::PropertiesFile* propertiesFile;                     // The file where the properties are stored.
    OptionalValue<StoredOutputMix> cachedStoredOutputMix;

    static const juce::String serializedStereoMixTypeFlat;
    static const juce::String serializedStereoMixTypeCpc;
    static const juce::String serializedStereoMixTypeCustom;
};

}   // namespace arkostracker
