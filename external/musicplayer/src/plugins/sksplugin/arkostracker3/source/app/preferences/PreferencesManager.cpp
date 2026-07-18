#include "PreferencesManager.h"

#include "../../business/patternViewer/DisplayedEffects.h"
#include "../../business/serialization/patternViewer/EffectToCharSerializer.h"
#include "../../business/serialization/sourceProfile/SourceProfileSerializer.h"
#include "../../ui/lookAndFeel/LookAndFeelConstants.h"
#include "../../ui/lookAndFeel/LookAndFeelFactory.h"
#include "../../ui/lookAndFeel/serialization/XmlSerializer.h"
#include "../../ui/lookAndFeel/theme/ThemeDefaultBuilder.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/StringUtil.h"
#include "LastFolderTempStorage.h"

namespace arkostracker
{

const juce::String PreferencesManager::recentFilesSeparator(":::://::::");     // Must be something all the OSes won't accept. // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const size_t PreferencesManager::maximumCountRecentFiles = 10U;

const juce::String PreferencesManager::serializedStereoMixTypeFlat = "flat";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PreferencesManager::serializedStereoMixTypeCpc = "cpc";                  // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String PreferencesManager::serializedStereoMixTypeCustom = "custom";            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

PreferencesManager::PreferencesManager() noexcept :
        applicationProperties(),
        propertiesFile(),
        cachedStoredOutputMix()
{
    juce::PropertiesFile::Options options;                              // Stores as XML by default.
    options.applicationName = getApplicationName();
    options.filenameSuffix = ".properties";
    options.osxLibrarySubFolder = "Application Support";                // Required for Mac OSX.
    applicationProperties.setStorageParameters(options);

    // Gets the file to store the data.
    propertiesFile = applicationProperties.getUserSettings();           // NOLINT(*-prefer-member-initializer)
    jassert(propertiesFile != nullptr);
}

PreferencesManager& PreferencesManager::getInstance()
{
    static PreferencesManager instance;         // Uses "magic static". Lazy and thread-safe.
    return instance;
}

juce::String PreferencesManager::getApplicationName()
{
    return "ArkosTracker3";
}

void PreferencesManager::saveValue(const juce::String& key, const juce::var& value) const noexcept
{
    propertiesFile->setValue(key, value);
}

void PreferencesManager::saveXmlValue(const juce::String& key, const juce::XmlElement& xmlElement) const noexcept
{
    propertiesFile->setValue(key, &xmlElement);
}

std::unique_ptr<juce::XmlElement> PreferencesManager::getLayoutArrangement(const int number) const noexcept
{
    return propertiesFile->getXmlValue(getKeyForLayoutArrangement(number));
}

void PreferencesManager::storeLayoutArrangement(const int number, const juce::XmlElement& rootXlmElement) const noexcept
{
    propertiesFile->setValue(getKeyForLayoutArrangement(number), &rootXlmElement);
}

juce::String PreferencesManager::getKeyForLayoutArrangement(const int number) const noexcept
{
    return keyBaseLayoutArrangement + juce::String(number);
}

juce::String PreferencesManager::getKeyForCustomTheme(const int themeId) const noexcept
{
    return keyBaseCustomTheme + juce::String(themeId);        // Do not change this, some data can be extracted from this.
}

std::unique_ptr<juce::XmlElement> PreferencesManager::getKeyMapping() const noexcept
{
    return propertiesFile->getXmlValue(keyKeyboardMapping);
}
void PreferencesManager::storeKeyMapping(const juce::KeyMappingEditorComponent& keyMappingEditorComponent) const noexcept
{
    const auto xmlElement(keyMappingEditorComponent.getMappings().createXml(true));            // Only gets the difference.
    propertiesFile->setValue(keyKeyboardMapping, xmlElement.get());
}


// Keyboard.
// ========================================================

bool PreferencesManager::isVirtualKeyboardLayoutExplicitlySet() const noexcept
{
    return propertiesFile->getBoolValue(keyIsVirtualKeyboardLayoutSetExplicitly, false);
}

VirtualKeyboardLayout PreferencesManager::getVirtualKeyboardLayout() const noexcept
{
    auto keyboardInt = propertiesFile->getIntValue(keyVirtualKeyboardLayout, static_cast<int>(VirtualKeyboardLayout::qwerty));  // Not "none", raise too many warnings.
    return static_cast<VirtualKeyboardLayout>(keyboardInt);
}

void PreferencesManager::setVirtualKeyboardLayout(VirtualKeyboardLayout keyboardLayout) const noexcept
{
    jassert(keyboardLayout != VirtualKeyboardLayout::none);

    saveValue(keyVirtualKeyboardLayout, static_cast<int>(keyboardLayout));
    saveValue(keyIsVirtualKeyboardLayoutSetExplicitly, true);
}


// General properties.
// =============================================================

void PreferencesManager::storeMainWindowSize(const int width, const int height, const bool isFullscreen) const noexcept
{
    propertiesFile->setValue(keyWindowWidth, width);
    propertiesFile->setValue(keyWindowHeight, height);
    propertiesFile->setValue(keyIsWindowFullscreen, isFullscreen);
}

PreferencesManager::MainWindowSize PreferencesManager::getMainWindowSize() const
{
    return {
            propertiesFile->getIntValue(keyWindowWidth, 0),
            propertiesFile->getIntValue(keyWindowHeight, 0),
            propertiesFile->getBoolValue(keyIsWindowFullscreen, false)
    };
}

void PreferencesManager::setNativeTitleBarUsed(const bool nativeTitleBarUsed) const noexcept
{
    propertiesFile->setValue(keyUseNativeTitleBar, nativeTitleBarUsed);
}

bool PreferencesManager::isNativeTitleBarUsed() const noexcept
{
#if JUCE_MAC
    constexpr auto defaultValue = true;     // On Mac, advised to show the native title bar first.
#else
    constexpr auto defaultValue = false;
#endif
    return propertiesFile->getBoolValue(keyUseNativeTitleBar, defaultValue);
}

void PreferencesManager::setUseOpenGl(const bool useOpenGl) const noexcept
{
    propertiesFile->setValue(keyUseOpenGl, useOpenGl);
}

bool PreferencesManager::mustUseOpenGl() const noexcept
{
#if JUCE_MAC
    constexpr auto defaultValue = true;         // On MAC, strongly advised, else repaints are performed continuously.
#else
    constexpr auto defaultValue = false;        // By default, don't use OpenGL.
#endif
    return propertiesFile->getBoolValue(keyUseOpenGl, defaultValue);
}

bool PreferencesManager::isSongInfoShownAfterLoad() const noexcept
{
    return propertiesFile->getBoolValue(keyShowSongInfoAfterLoad, true);
}

bool PreferencesManager::isSplashShown() const noexcept
{
    return propertiesFile->getBoolValue(keyShowSplash, true);
}

bool PreferencesManager::isForceNonNativeFileChooserOnLoadInstrument() const noexcept
{
    return propertiesFile->getBoolValue(keyForceNonNativeFileChooserOnLoadInstrument, false);
}

void PreferencesManager::setShowSongInfoAfterLoad(const bool show) const noexcept
{
    propertiesFile->setValue(keyShowSongInfoAfterLoad, show);
}

void PreferencesManager::setShowSplash(const bool show) const noexcept
{
    propertiesFile->setValue(keyShowSplash, show);
}

void PreferencesManager::setForceNonNativeFileChooserOnLoadInstrument(const bool useNonNative) const noexcept
{
    propertiesFile->setValue(keyForceNonNativeFileChooserOnLoadInstrument, useNonNative);
}

void PreferencesManager::setMustAddOctaveToInputMidiNotes(const bool mustAdd) const noexcept
{
    propertiesFile->setValue(keyMustAddOctaveToInputMidiNotes, mustAdd);
}

bool PreferencesManager::mustAddOctaveToInputMidiNotes() const noexcept
{
    return propertiesFile->getBoolValue(keyMustAddOctaveToInputMidiNotes, false);
}


// Audio interfaces.
// =============================================================

std::unique_ptr<juce::XmlElement> PreferencesManager::getAudioProperties() const noexcept
{
    return propertiesFile->getXmlValue(keyAudioDeviceManagerProperties);
}

void PreferencesManager::saveAudioProperties(const juce::AudioDeviceSelectorComponent& audioDeviceSelectorComponent) const noexcept
{
    const auto xmlElement(audioDeviceSelectorComponent.deviceManager.createStateXml());
    if (xmlElement != nullptr) {            // May be nullptr if unchanged.
        saveXmlValue(keyAudioDeviceManagerProperties, *xmlElement);
    }
}


// =============================================================

int PreferencesManager::getCurrentThemeId() const noexcept
{
    return propertiesFile->getIntValue(keyCurrentThemeId, static_cast<int>(LookAndFeelConstants::ThemeId::defaultAt3));
}

void PreferencesManager::setCurrentThemeId(const int themeId) const noexcept
{
    jassert(themeId >= static_cast<int>(LookAndFeelConstants::ThemeId::minimumId));

    propertiesFile->setValue(keyCurrentThemeId, themeId);
}

std::unique_ptr<StoredLookAndFeel> PreferencesManager::getCustomTheme(const int themeId) const noexcept
{
    // Is there an XML stored?
    const auto xmlLookAndFeel = propertiesFile->getXmlValue(getKeyForCustomTheme(themeId));
    if (xmlLookAndFeel == nullptr) {
        return nullptr;
    }

    // Tries to deserialize it, using the default to "fill the holes".
    const auto defaultStoredLookAndFeel = ThemeDefaultBuilder::build();
    const XmlSerializer xmlSerializer;
    return xmlSerializer.deserialize(*xmlLookAndFeel, *defaultStoredLookAndFeel);
}

void PreferencesManager::storeCustomLookAndFeel(const StoredLookAndFeel& storedLookAndFeel) const noexcept
{
    const XmlSerializer xmlSerializer;
    const auto xmlTheme = xmlSerializer.serialize(storedLookAndFeel);

    const auto id = storedLookAndFeel.id;
    jassert(id >= static_cast<int>(LookAndFeelConstants::ThemeId::minimumId));

    saveXmlValue(getKeyForCustomTheme(id), *xmlTheme);
}

int PreferencesManager::findNewFreeCustomLookAndFeelId() const noexcept
{
    // Gets all the custom IDs, finds the latest one and increases it.
    const auto ids = getAllCustomThemeIds();
    if (ids.empty()) {
        return static_cast<int>(LookAndFeelConstants::ThemeId::startIdCustomTheme);
    }

    // The items are sorted from lowest to highest.
    return ids.back() + 1;
}

std::vector<int> PreferencesManager::getAllCustomThemeIds() const noexcept
{
    const auto keyValues = propertiesFile->getAllProperties();
    const auto& keys = keyValues.getAllKeys();

    std::vector<int> ids;
    for (const auto& key : keys) {
        if (key.startsWith(keyBaseCustomTheme)) {
            // Extracts the id. Not really great... But deserializing would be the same (and much slower!).
            auto valueString = key.substring(keyBaseCustomTheme.length());
            const auto value = valueString.getIntValue();
            jassert(value > 0);

            ids.push_back(value);
        }
    }

    // Sorts, ascending.
    std::sort(ids.begin(), ids.end());

    return ids;
}

void PreferencesManager::deleteCustomLookAndFeel(const int themeId) const noexcept
{
    const auto key = getKeyForCustomTheme(themeId);
    propertiesFile->removeValue(key);
}


// PatternViewer.
// ============================================================

bool PreferencesManager::isPatternViewerLineNumberInHexadecimal() const noexcept
{
    return propertiesFile->getBoolValue(keyPatternViewerLineNumberInHexadecimal, true);
}

void PreferencesManager::setPatternViewerLineNumberInHexadecimal(const bool isHexadecimal) const noexcept
{
    propertiesFile->setValue(keyPatternViewerLineNumberInHexadecimal, isHexadecimal);
}

void PreferencesManager::setPatternViewerFontHeight(const int height) const noexcept
{
    propertiesFile->setValue(keyPatternViewerFontHeight, height);
}

int PreferencesManager::getPatternViewerFontHeight() const noexcept
{
    return propertiesFile->getIntValue(keyPatternViewerFontHeight, 14);
}

void PreferencesManager::setPatternViewerMouseWheelSteps(const int steps) const noexcept
{
    propertiesFile->setValue(keyPatternViewerMouseWheelSteps, steps);
}

void PreferencesManager::setPatternViewerMustZoomMiddleLine(const bool mustZoom) const noexcept
{
    propertiesFile->setValue(keyPatternViewerMustZoomMiddleLine, mustZoom);
}

bool PreferencesManager::getPatternViewerMustZoomMiddleLine() const noexcept
{
    return propertiesFile->getBoolValue(keyPatternViewerMustZoomMiddleLine, true);
}

int PreferencesManager::getPatternViewerMouseWheelSteps() const noexcept
{
    return propertiesFile->getIntValue(keyPatternViewerMouseWheelSteps, 4);
}

std::unordered_map<Effect, juce::juce_wchar> PreferencesManager::getEffectToChar() const noexcept
{
    // Any effect to char encoded? If not, gets a default map.
    const auto keyNode = propertiesFile->getXmlValue(keyEffectToChar);
    if (keyNode == nullptr) {
        return DisplayedEffects::getDefaultEffectToDisplayedChar();
    }

    return EffectToCharSerializer::deserialize(*keyNode);
}

void PreferencesManager::setEffectToChar(const std::unordered_map<Effect, juce::juce_wchar>& effectToChar) const noexcept
{
    const auto xmlNode = EffectToCharSerializer::serialize(effectToChar);
    saveXmlValue(keyEffectToChar, *xmlNode);
}


// Recent folders.
// ======================================================

void PreferencesManager::saveRecentFolders() const noexcept
{
    // Stores the locally stored folders into the Preferences.
    propertiesFile->setValue(keyRecentFolderLoadSaveSong, LastFolderTempStorage::getRecentFolder(FolderContext::loadOrSaveSong));
    propertiesFile->setValue(keyRecentFolderLoadSaveInstrument, LastFolderTempStorage::getRecentFolder(FolderContext::loadOrSaveInstrument));
    propertiesFile->setValue(keyRecentFolderLoadSaveArpeggio, LastFolderTempStorage::getRecentFolder(FolderContext::loadOrSaveArpeggio));
    propertiesFile->setValue(keyRecentFolderLoadSavePitch, LastFolderTempStorage::getRecentFolder(FolderContext::loadOrSavePitch));
    propertiesFile->setValue(keyRecentFolderAnyExport, LastFolderTempStorage::getRecentFolder(FolderContext::anyExport));
    propertiesFile->setValue(keyRecentFolderOther, LastFolderTempStorage::getRecentFolder(FolderContext::other));

    static_assert(static_cast<int>(FolderContext::last) == 5, "Missing folder?");
}

void PreferencesManager::readStoredFoldersAndFillLastFolderStorage() const noexcept
{
    // Stores locally each folder.
    LastFolderTempStorage::setRecentFolder(FolderContext::loadOrSaveSong, propertiesFile->getValue(keyRecentFolderLoadSaveSong));
    LastFolderTempStorage::setRecentFolder(FolderContext::loadOrSaveInstrument, propertiesFile->getValue(keyRecentFolderLoadSaveInstrument));
    LastFolderTempStorage::setRecentFolder(FolderContext::loadOrSaveArpeggio, propertiesFile->getValue(keyRecentFolderLoadSaveArpeggio));
    LastFolderTempStorage::setRecentFolder(FolderContext::loadOrSavePitch, propertiesFile->getValue(keyRecentFolderLoadSavePitch));
    LastFolderTempStorage::setRecentFolder(FolderContext::anyExport, propertiesFile->getValue(keyRecentFolderAnyExport));
    LastFolderTempStorage::setRecentFolder(FolderContext::other, propertiesFile->getValue(keyRecentFolderOther));

    static_assert(static_cast<int>(FolderContext::last) == 5, "Missing folder?");
}


// Recent files.
// ======================================================

std::vector<juce::String> PreferencesManager::getRecentFiles() const noexcept
{
    // The Strings are separated by forbidden characters. Slightly hackish, but works fine enough!
    const auto recentFilesConcatenated = propertiesFile->getValue(keyRecentFiles);
    return recentFilesConcatenated.isEmpty() ? std::vector<juce::String>() : StringUtil::split(recentFilesConcatenated, recentFilesSeparator);
}

void PreferencesManager::removeRecentFile(const juce::String& pathToRemove) const noexcept
{
    // We don't try to be efficient, so we can decode and encode the path back.
    auto paths = getRecentFiles();

    auto found = false;
    const auto iterator = std::find(paths.begin(), paths.end(), pathToRemove);
    if (iterator != paths.end()) {
        paths.erase(iterator);
        found = true;
    }

    if (found) {
        encodeRecentFiles(paths);
    }
}

void PreferencesManager::addOrUpdateRecentFile(const juce::String& path) const noexcept
{
    // Not efficient, we don't care.
    // First, we remove the path, if it exists.
    removeRecentFile(path);

    // Reloads the paths, adds the new one in the first position.
    auto paths = getRecentFiles();
    paths.insert(paths.begin(), path);

    // Re-encodes the whole. It will take care of the trimming.
    encodeRecentFiles(paths);
}

void PreferencesManager::clearRecentFiles() const noexcept
{
    propertiesFile->removeValue(keyRecentFiles);
}

void PreferencesManager::encodeRecentFiles(const std::vector<juce::String>& inputPaths) const noexcept
{
    const auto countToStore = std::min(inputPaths.size(), maximumCountRecentFiles);

    juce::String paths;
    // Concatenates the paths into one String.
    auto isFirst = true;
    for (size_t i = 0U; i < countToStore; ++i) {
        if (!isFirst) {
            // Adds the separator.
            paths += recentFilesSeparator;
        }
        isFirst = false;

        paths += inputPaths.at(i);
    }

    propertiesFile->setValue(keyRecentFiles, paths);
}

bool PreferencesManager::areSavedFilesCompressed() const noexcept
{
    return propertiesFile->getBoolValue(keyCompressOnSave, true);
}

void PreferencesManager::setFilesCompressionOnSave(const bool compress) const noexcept
{
    propertiesFile->setValue(keyCompressOnSave, compress);
}


// Source profiles.
// ===============================================================================

int PreferencesManager::getCurrentSourceProfileIndex() const noexcept
{
    return propertiesFile->getIntValue(keySourceProfileCurrentIndex);
}

void PreferencesManager::setCurrentSourceProfileFromIndex(const int index) const noexcept
{
    propertiesFile->setValue(keySourceProfileCurrentIndex, index);
}

std::vector<SourceProfile> PreferencesManager::getSourceProfiles() const noexcept
{
    // Gets the template profiles first.
    auto sourceProfiles = SourceProfiles::getTemplateSourceProfiles();

    // Any user profiles? If yes, adds them to the list.
    const auto userSourceProfilesNode = propertiesFile->getXmlValue(keySourceProfiles);
    if (userSourceProfilesNode != nullptr) {
        const auto userProfiles = SourceProfileSerializer::deserialize(*userSourceProfilesNode);

        sourceProfiles.insert(sourceProfiles.end(), userProfiles.cbegin(), userProfiles.cend());
    }

    return sourceProfiles;
}

SourceProfile PreferencesManager::getCurrentSourceProfile() const noexcept
{
    const auto profileIndex = static_cast<size_t>(getCurrentSourceProfileIndex());
    const auto profiles = getSourceProfiles();
    if (profileIndex >= profiles.size()) {
        jassertfalse;           // Out of bounds??
        return SourceProfiles::buildZ80Default();
    }

    return profiles.at(profileIndex);
}

void PreferencesManager::setSourceProfiles(const std::vector<SourceProfile>& sourceProfiles) const noexcept
{
    // What profiles to save?
    std::vector<SourceProfile> userProfiles;
    // Gets only the non-template ones.
    for (const auto& sourceProfile : sourceProfiles) {
        if (!sourceProfile.isReadOnly()) {
            userProfiles.push_back(sourceProfile);
        }
    }

    // Serializes them to XML.
    const auto node = SourceProfileSerializer::serialize(userProfiles);
    jassert(node != nullptr);

    saveXmlValue(keySourceProfiles, *node);
}

bool PreferencesManager::isExportAsSource() const noexcept
{
    return propertiesFile->getBoolValue(keyExportAsSource, true);
}

void PreferencesManager::setExportAsSource(const bool exportAsSource) const noexcept
{
    propertiesFile->setValue(keyExportAsSource, exportAsSource);
}

std::pair<bool, int> PreferencesManager::getUseAddressAndAddress() const noexcept
{
    const auto useAddress = propertiesFile->getBoolValue(keyMustExportAddress, false);
    const auto address = propertiesFile->getIntValue(keyExportedAddress, 0x4000);
    return { useAddress, address };
}

void PreferencesManager::setUseExportAddressAndAddress(const bool useAddress, const int address) const noexcept
{
    propertiesFile->setValue(keyMustExportAddress, useAddress);
    propertiesFile->setValue(keyExportedAddress, address);
}

bool PreferencesManager::isExportPlayerConfigurationFile() const noexcept
{
    return propertiesFile->getBoolValue(keyExportPlayerConfigurationFile, true);
}

void PreferencesManager::setExportPlayerConfigurationFile(const bool exportPlayerConfigurationFile) const noexcept
{
    propertiesFile->setValue(keyExportPlayerConfigurationFile, exportPlayerConfigurationFile);
}

bool PreferencesManager::isEncodeAllAddressesAsRelativeToSongStart() const noexcept
{
    return propertiesFile->getBoolValue(keyEncodeAllAddressesAsRelativeToSongStart, false);
}

void PreferencesManager::setEncodeAllAddressesAsRelativeToSongStart(const bool encodeAsRelative) const noexcept
{
    propertiesFile->setValue(keyEncodeAllAddressesAsRelativeToSongStart, encodeAsRelative);
}

bool PreferencesManager::isExportToSeveralFiles() const noexcept
{
    return propertiesFile->getBoolValue(keyExportToSeveralFiles, false);
}

void PreferencesManager::setExportToSeveralFiles(const bool exportToSeveralFiles) const noexcept
{
    propertiesFile->setValue(keyExportToSeveralFiles, exportToSeveralFiles);
}

juce::String PreferencesManager::getSourceLabelsPrefix() const noexcept
{
    return propertiesFile->getValue(keySourceLabelsPrefix, juce::String());
}

void PreferencesManager::setSourceLabelsPrefix(const juce::String& label) const noexcept
{
    propertiesFile->setValue(keySourceLabelsPrefix, label);
}


// Sample export.
// ======================================================

SampleEncoderFlags PreferencesManager::getSampleEncoderFlags() const noexcept
{
    return SampleEncoderFlags(
            propertiesFile->getBoolValue(keySampleExportEnabled, true),
            propertiesFile->getIntValue(keySampleExportAmplitude, 256),
            propertiesFile->getIntValue(keySampleExportOffset, 0),
            propertiesFile->getIntValue(keySampleExportPaddingLength, 0),
            propertiesFile->getIntValue(keySampleExportFadeOutSampleCountForNonLoopingSamples, 0),
            propertiesFile->getBoolValue(keySampleExportForcePaddingAndFadeOutToLowestValue, false),
            propertiesFile->getBoolValue(keySampleExportOnlyExportUsedLength, true),
            propertiesFile->getBoolValue(keySampleExportIgnoredUnusedSamples, true),
            propertiesFile->getBoolValue(keySampleExportGenerateIndexTable, true)
    );
}

void PreferencesManager::setSampleEncoderFlags(const SampleEncoderFlags& flags) const noexcept
{
    propertiesFile->setValue(keySampleExportEnabled, flags.areSamplesExported());
    propertiesFile->setValue(keySampleExportAmplitude, flags.getAmplitude());
    propertiesFile->setValue(keySampleExportOffset, flags.getOffset());
    propertiesFile->setValue(keySampleExportPaddingLength, flags.getPaddingLength());
    propertiesFile->setValue(keySampleExportFadeOutSampleCountForNonLoopingSamples, flags.getFadeOutSampleCountForNonLoopingSamples());
    propertiesFile->setValue(keySampleExportForcePaddingAndFadeOutToLowestValue, flags.isForcePaddingAndFadeOutToLowestValue());
    propertiesFile->setValue(keySampleExportOnlyExportUsedLength, flags.isOnlyExportUsedLength());
    propertiesFile->setValue(keySampleExportIgnoredUnusedSamples, flags.isIgnoreUnusedSamples());
    propertiesFile->setValue(keySampleExportGenerateIndexTable, flags.isGenerateIndexTable());
}

double PreferencesManager::getSampleExportPitch() const noexcept
{
    return propertiesFile->getDoubleValue(keySampleExportPitch, 0.0);
}

void PreferencesManager::storeSampleExportPitch(const double pitch) const noexcept
{
    propertiesFile->setValue(keySampleExportPitch, pitch);
}


// Output mix.
// ======================================================

StoredOutputMix PreferencesManager::getOutputMix() noexcept
{
    // Any cache ?
    if (cachedStoredOutputMix.isPresent()) {
        return cachedStoredOutputMix.getValue();
    }

    const auto outputMix = StoredOutputMix(
            propertiesFile->getIntValue(keyOutputMixGlobalVolumePercent, 100),
            deserializeStereoMixType(propertiesFile->getValue(keyOutputMixStereoMixType, serializedStereoMixTypeFlat)),
            propertiesFile->getIntValue(keyOutputMixCustomLeftVolumePercent, 100),
            propertiesFile->getIntValue(keyOutputMixCustomCenterVolumePercent, 100),
            propertiesFile->getIntValue(keyOutputMixCustomRightVolumePercent, 100),
            propertiesFile->getBoolValue(keyOutputMixEmulateSpeaker, false),
            propertiesFile->getIntValue(keyOutputMixStereoSeparationPercent, 100)
    );

    cachedStoredOutputMix = outputMix;

    return outputMix;
}

void PreferencesManager::storeOutputMix(const StoredOutputMix& outputMix) noexcept
{
    cachedStoredOutputMix = outputMix;

    propertiesFile->setValue(keyOutputMixGlobalVolumePercent, outputMix.getGlobalVolume());
    propertiesFile->setValue(keyOutputMixStereoMixType, serializeStereoMixType(outputMix.getStereoMixType()));
    propertiesFile->setValue(keyOutputMixCustomLeftVolumePercent, outputMix.getCustomVolumeLeft());
    propertiesFile->setValue(keyOutputMixCustomCenterVolumePercent, outputMix.getCustomVolumeCenter());
    propertiesFile->setValue(keyOutputMixCustomRightVolumePercent, outputMix.getCustomVolumeRight());
    propertiesFile->setValue(keyOutputMixEmulateSpeaker, outputMix.isSpeakerEmulated());
    propertiesFile->setValue(keyOutputMixStereoSeparationPercent, outputMix.getStereoSeparation());
}

StoredOutputMix::StereoMixType PreferencesManager::deserializeStereoMixType(const juce::String& inputString) noexcept
{
    if (inputString == serializedStereoMixTypeFlat) {
        return StoredOutputMix::StereoMixType::flat;
    }
    if (inputString == serializedStereoMixTypeCpc) {
        return StoredOutputMix::StereoMixType::cpc;
    }
    if (inputString == serializedStereoMixTypeCustom) {
        return StoredOutputMix::StereoMixType::custom;
    }

    jassertfalse;       // Unknown?
    return StoredOutputMix::StereoMixType::flat;
}

juce::String PreferencesManager::serializeStereoMixType(const StoredOutputMix::StereoMixType stereoMixType) noexcept
{
    switch (stereoMixType) {
        default:
            jassertfalse;
        case StoredOutputMix::StereoMixType::flat:
            return serializedStereoMixTypeFlat;
        case StoredOutputMix::StereoMixType::cpc:
            return serializedStereoMixTypeCpc;
        case StoredOutputMix::StereoMixType::custom:
            return serializedStereoMixTypeCustom;
    }
}


// Serial.
// ======================================================

juce::String PreferencesManager::getSerialPort() const noexcept
{
    return propertiesFile->getValue(keySerialSelectedPort);
}

void PreferencesManager::storeSerialPort(const juce::String& serialPort) const noexcept
{
    propertiesFile->setValue(keySerialSelectedPort, serialPort);
}

SerialProfile PreferencesManager::getSelectedSerialProfile() const noexcept
{
    auto storedId = static_cast<uint8_t>(propertiesFile->getIntValue(keySerialProfileId, static_cast<uint8_t>(SerialProfile::custom)));
    // Makes sure the stored id is valid.
    storedId = NumberUtil::correctNumber(storedId, static_cast<uint8_t>(SerialProfile::first), static_cast<uint8_t>(SerialProfile::last));

    return static_cast<SerialProfile>(storedId);
}

void PreferencesManager::storeSelectedSerialProfile(SerialProfile serialProfile) const noexcept
{
    propertiesFile->setValue(keySerialProfileId, static_cast<uint8_t>(serialProfile));
}

SerialData PreferencesManager::getSerialCustomProfile() const noexcept
{
    return {
        juce::String(),
        propertiesFile->getIntValue(keySerialCustomBaudRate, 9600),
        propertiesFile->getIntValue(keySerialCustomByteSize, 5),
        propertiesFile->getIntValue(keySerialCustomParity, 0),
        static_cast<StopBit>(propertiesFile->getIntValue(keySerialCustomStopBitId, static_cast<int>(StopBit::stopBit1))),
        static_cast<FlowControl>(propertiesFile->getIntValue(keySerialCustomFlowControlId, static_cast<int>(FlowControl::none))),
    };
}

void PreferencesManager::storeSerialCustomProfile(const SerialData& serialData) const noexcept
{
    propertiesFile->setValue(keySerialCustomBaudRate, serialData.getBaudRate());
    propertiesFile->setValue(keySerialCustomByteSize, serialData.getByteSize());
    propertiesFile->setValue(keySerialCustomParity, serialData.getParity());
    propertiesFile->setValue(keySerialCustomStopBitId, static_cast<int>(serialData.getStopBits()));
    propertiesFile->setValue(keySerialCustomFlowControlId, static_cast<int>(serialData.getFlowControl()));
}


// Raw exporter.
// ======================================================

EncodedDataFlag PreferencesManager::getRawExportFlags() const noexcept
{
    constexpr auto defaultValue = true;

    const EncodedDataFlag flags(
        propertiesFile->getBoolValue(keyRawExporterSongAndSubsongMetadata, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterReferenceTables, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterSpeedTrack, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterEventTrack, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterInstruments, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterArpeggioTables, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterPitchTables, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterEffects, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterRleForEmptyLines, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterTranspositions, defaultValue),
        propertiesFile->getBoolValue(keyRawExporterHeight, defaultValue),
        propertiesFile->getDoubleValue(keyRawExporterPitchTrackEffect, 1.0)
    );

    return flags;
}

void PreferencesManager::setRawExportFlags(const EncodedDataFlag& flags) const noexcept
{
    propertiesFile->setValue(keyRawExporterSongAndSubsongMetadata, flags.mustEncodeSongAndSubsongMetadata());
    propertiesFile->setValue(keyRawExporterReferenceTables, flags.mustEncodeReferenceTables()),
    propertiesFile->setValue(keyRawExporterSpeedTrack, flags.mustEncodeSpeedTrack()),
    propertiesFile->setValue(keyRawExporterEventTrack, flags.mustEncodeEventTrack()),
    propertiesFile->setValue(keyRawExporterInstruments, flags.mustEncodeInstruments()),
    propertiesFile->setValue(keyRawExporterArpeggioTables, flags.mustEncodeArpeggioTables()),
    propertiesFile->setValue(keyRawExporterPitchTables, flags.mustEncodePitchTables()),
    propertiesFile->setValue(keyRawExporterEffects, flags.mustEncodeEffects()),
    propertiesFile->setValue(keyRawExporterRleForEmptyLines, flags.mustEncodeRleForEmptyLines()),
    propertiesFile->setValue(keyRawExporterTranspositions, flags.mustEncodeTranspositions()),
    propertiesFile->setValue(keyRawExporterHeight, flags.mustEncodeHeight()),
    propertiesFile->setValue(keyRawExporterPitchTrackEffect, flags.getPitchTrackEffectRatio());
}


// Events exporter.
// ======================================================

EventsExportType PreferencesManager::getEventsExportType() const noexcept
{
    switch (propertiesFile->getIntValue(keyEventsExportType, static_cast<int>(EventsExportType::exportAllEvents))) {
        default:
            jassertfalse;
        case static_cast<int>(EventsExportType::exportAllEvents):
            return EventsExportType::exportAllEvents;
        case static_cast<int>(EventsExportType::exportOnlyEvents):
            return EventsExportType::exportOnlyEvents;
        case static_cast<int>(EventsExportType::exportOnlySamples):
            return EventsExportType::exportOnlySamples;
    }
}

void PreferencesManager::storeEventsExportType(const EventsExportType type) const noexcept
{
    propertiesFile->setValue(keyEventsExportType, static_cast<int>(type));
}


// Raw linear exporter.
// ======================================================

RawLinearExportType PreferencesManager::getRawLinearExportType() const noexcept
{
    switch (propertiesFile->getIntValue(keyRawLinearExportType, static_cast<int>(RawLinearExportType::samplesOnly))) {
        default:
            jassertfalse;
        case static_cast<int>(RawLinearExportType::all):
            return RawLinearExportType::all;
        case static_cast<int>(RawLinearExportType::psgOnly):
            return RawLinearExportType::psgOnly;
        case static_cast<int>(RawLinearExportType::samplesOnly):
            return RawLinearExportType::samplesOnly;
    }
}

void PreferencesManager::storeRawLinearExportType(RawLinearExportType type) const noexcept
{
    propertiesFile->setValue(keyRawLinearExportType, static_cast<int>(type));
}


// FAP.
// ======================================================

OptionalInt PreferencesManager::getFapOutputFrequencyHz() const noexcept
{
    const auto frequencyHz = propertiesFile->getIntValue(keyFapForcedOutputFrequencyHz, 0);
    return (frequencyHz <= 0) ? OptionalInt() : frequencyHz;
}

void PreferencesManager::storeFapOutputFrequencyHz(const OptionalInt frequencyHz) const noexcept
{
    const auto frequencyToEncode = frequencyHz.isPresent() ? frequencyHz.getValue() : 0;
    propertiesFile->setValue(keyFapForcedOutputFrequencyHz, frequencyToEncode);
}

}   // namespace arkostracker
