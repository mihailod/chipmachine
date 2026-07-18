#include "ExportAs.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"
#include "../../utils/TextEditorUtil.h"

namespace arkostracker 
{

const int ExportAs::desiredHeight = 310;
const int ExportAs::sourceLabelMaximumLength = 32;

ExportAs::ExportAs(const bool pCanExportToSeveralFiles, juce::String pOverriddenBinaryExtensionWithoutDot, const bool pCanExportConfigurationFile) noexcept :
        canExportToSeveralFiles(pCanExportToSeveralFiles),
        overriddenBinaryExtensionWithoutDot(std::move(pOverriddenBinaryExtensionWithoutDot)),
        canExportConfigurationFile(pCanExportConfigurationFile),
        preferences(PreferencesManager::getInstance()),
        sourceProfiles(preferences.getSourceProfiles()),
        labelEditorRestriction(TextEditorUtil::buildRestrictionForSource(sourceLabelMaximumLength)),
        addressEditorRestriction(TextEditorUtil::buildRestrictionForHexadecimal(5)),
        group(juce::String(), juce::translate("Export as")),
        exportAsSourceToggle(juce::translate("Source")),
        exportAsBinaryToggle(juce::translate("Binary (z80)")),
        exportToSeveralFilesToggle(juce::translate("Export to several files")),
        generateConfigurationFileToggle(juce::translate("Generate a configuration file for players (as source)")),
        sourceProfileLabel(juce::String(), juce::translate("Source profile to use:")),
        sourceProfileComboBox(),
        sourceLabelsPrefixIfSourceLabel(juce::String(), juce::translate("Source labels prefix (may be empty):")),
        sourceLabelsPrefixIfSourceEditor(),
        encodeToAddressIfSourceToggle(juce::translate("Encode to address (hexadecimal):")),
        encodeToAddressIfBinaryLabel(juce::String(), juce::translate("Encode to address (hexadecimal):")),
        encodeToAddressEditor()
{
    constexpr auto exportRadioId = 479;
    exportAsSourceToggle.setRadioGroupId(exportRadioId);
    exportAsBinaryToggle.setRadioGroupId(exportRadioId);
    exportAsSourceToggle.onClick = [&] { onSourceToggleSelected(); };
    exportAsBinaryToggle.onClick = [&] { onBinaryToggleSelected(); };
    encodeToAddressIfSourceToggle.onClick = [&] { onSourceToggleSelected(); };      // Simplification, but works.

    // Fills the Profiles ComboBox.
    auto profileId = 1;
    for (const auto& sourceProfile : sourceProfiles) {
        sourceProfileComboBox.addItem(sourceProfile.getName(), profileId);
        ++profileId;
    }
    sourceProfileComboBox.setSelectedItemIndex(preferences.getCurrentSourceProfileIndex(), juce::NotificationType::dontSendNotification);

    // Fills the other fields.
    const auto exportAsSource = preferences.isExportAsSource();
    auto& exportAsToggle = exportAsSource ? exportAsSourceToggle : exportAsBinaryToggle;
    exportAsToggle.setToggleState(true, juce::NotificationType::dontSendNotification);
    exportToSeveralFilesToggle.setToggleState(
            canExportToSeveralFiles && preferences.isExportToSeveralFiles(), juce::NotificationType::dontSendNotification);

    const auto&[useAddress, address] = preferences.getUseAddressAndAddress();
    encodeToAddressEditor.setText(juce::String::toHexString(address));
    encodeToAddressEditor.setInputFilter(&addressEditorRestriction, false);
    encodeToAddressIfSourceToggle.setToggleState(useAddress, juce::NotificationType::dontSendNotification);

    generateConfigurationFileToggle.setToggleState(canExportConfigurationFile ? preferences.isExportPlayerConfigurationFile() : false,
        juce::NotificationType::dontSendNotification);

    sourceLabelsPrefixIfSourceEditor.setInputFilter(&labelEditorRestriction, false);

    const auto sourceLabelsPrefix = preferences.getSourceLabelsPrefix();
    sourceLabelsPrefixIfSourceEditor.setText(sourceLabelsPrefix, false);

    addAndMakeVisible(group);
    addAndMakeVisible(exportAsSourceToggle);
    addAndMakeVisible(exportAsBinaryToggle);
    addAndMakeVisible(exportToSeveralFilesToggle);
    addAndMakeVisible(generateConfigurationFileToggle);
    addAndMakeVisible(sourceProfileLabel);
    addAndMakeVisible(sourceProfileComboBox);
    // Not always visible.
    addChildComponent(sourceLabelsPrefixIfSourceLabel);
    addChildComponent(sourceLabelsPrefixIfSourceEditor);
    addChildComponent(encodeToAddressIfSourceToggle);
    addChildComponent(encodeToAddressIfBinaryLabel);
    addAndMakeVisible(encodeToAddressEditor);
}


// Component method implementations.
// ====================================

void ExportAs::resized()
{
    const auto width = getWidth();
    const auto height = getHeight();

    const auto groupMarginsX = LookAndFeelConstants::groupMarginsX;
    const auto groupMarginsY = LookAndFeelConstants::groupMarginsY;
    const auto usableWidth = width - (2 * groupMarginsX);
    const auto top = groupMarginsY;
    const auto left = groupMarginsX;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    // These are always visible.
    group.setBounds(0, 0, width, height);
    exportAsSourceToggle.setBounds(left, top, 120, labelsHeight);
    exportAsBinaryToggle.setBounds(exportAsSourceToggle.getRight(), exportAsSourceToggle.getY(), 120, labelsHeight);
    exportToSeveralFilesToggle.setBounds(left, exportAsBinaryToggle.getBottom() + margins, usableWidth, labelsHeight);
    generateConfigurationFileToggle.setBounds(left, exportToSeveralFilesToggle.getBottom() + margins, usableWidth, labelsHeight);
    sourceProfileLabel.setBounds(left, generateConfigurationFileToggle.getBottom() + margins, usableWidth, labelsHeight);
    sourceProfileComboBox.setBounds(left, sourceProfileLabel.getBottom(), usableWidth, labelsHeight);
    locateAndSetUpDynamicViews();
}


// ====================================

void ExportAs::locateAndSetUpDynamicViews() noexcept
{
    const auto isSourceExport = exportAsSourceToggle.getToggleState();

    const auto groupMarginsX = LookAndFeelConstants::groupMarginsX;
    const auto width = getWidth();
    const auto usableWidth = width - (2 * groupMarginsX);
    const auto left = groupMarginsX;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;

    if (isSourceExport) {
        sourceLabelsPrefixIfSourceLabel.setBounds(left, sourceProfileComboBox.getBottom() + margins, usableWidth, labelsHeight);
        sourceLabelsPrefixIfSourceEditor.setBounds(left, sourceLabelsPrefixIfSourceLabel.getBottom(), usableWidth, labelsHeight);
        encodeToAddressIfSourceToggle.setBounds(left, sourceLabelsPrefixIfSourceEditor.getBottom() + margins, usableWidth, labelsHeight);
        encodeToAddressEditor.setBounds(left, encodeToAddressIfSourceToggle.getBottom(), usableWidth, labelsHeight);
        encodeToAddressEditor.setEnabled(encodeToAddressIfSourceToggle.getToggleState());
    } else {
        encodeToAddressIfBinaryLabel.setBounds(left, generateConfigurationFileToggle.getBottom() + margins, usableWidth, labelsHeight);
        encodeToAddressEditor.setBounds(left, encodeToAddressIfBinaryLabel.getBottom(), usableWidth, labelsHeight);
        encodeToAddressEditor.setEnabled(true);
    }
    exportToSeveralFilesToggle.setEnabled(isSourceExport && canExportToSeveralFiles);
    generateConfigurationFileToggle.setEnabled(canExportConfigurationFile);

    sourceProfileLabel.setVisible(isSourceExport);
    sourceProfileComboBox.setVisible(isSourceExport);
    sourceLabelsPrefixIfSourceLabel.setVisible(isSourceExport);
    sourceLabelsPrefixIfSourceEditor.setVisible(isSourceExport);
    encodeToAddressIfSourceToggle.setVisible(isSourceExport);

    encodeToAddressIfBinaryLabel.setVisible(!isSourceExport);
}

void ExportAs::onSourceToggleSelected() noexcept
{
    locateAndSetUpDynamicViews();
}

void ExportAs::onBinaryToggleSelected() noexcept
{
    locateAndSetUpDynamicViews();
}

juce::String ExportAs::extractLabelPrefix(const juce::String& name) noexcept
{
    // Keeps only the right characters.
    const auto prefix = name.retainCharacters(TextEditorUtil::restrictionSource);
    return prefix.substring(0, sourceLabelMaximumLength);
}

void ExportAs::onSubsongChanged(const juce::String& subsongName) noexcept
{
    const auto newPrefix = extractLabelPrefix(subsongName);
    sourceLabelsPrefixIfSourceEditor.setText(newPrefix, false);
}

ExportAs::Result ExportAs::storeChanges() noexcept
{
    // Gets the data from the UI and stores them in the Preferences.
    const auto exportAsSource = exportAsSourceToggle.getToggleState();
    const auto exportAsBinary = !exportAsSource;
    const auto exportConfigurationFile = generateConfigurationFileToggle.getToggleState();
    const auto baseLabel = sourceLabelsPrefixIfSourceEditor.getText();
    const auto useAddress = exportAsBinary || encodeToAddressIfSourceToggle.getToggleState();   // Binary always uses the address.
    const auto address = encodeToAddressEditor.getText().getHexValue32();
    const auto sourceProfileIndex = sourceProfileComboBox.getSelectedItemIndex();
    const auto sourceProfile = sourceProfiles.at(static_cast<size_t>(sourceProfileIndex));
    const auto exportToSeveralFiles = canExportToSeveralFiles && exportToSeveralFilesToggle.getToggleState();

    // Stores in the Preferences.
    preferences.setExportAsSource(exportAsSource);
    if (canExportConfigurationFile) {
        preferences.setExportPlayerConfigurationFile(exportConfigurationFile);
    }

    preferences.setCurrentSourceProfileFromIndex(sourceProfileIndex);

    preferences.setUseExportAddressAndAddress(useAddress, address);
    preferences.setSourceLabelsPrefix(baseLabel);
    // Only modifies this flag if the user has an action on it.
    if (canExportToSeveralFiles) {
        preferences.setExportToSeveralFiles(exportToSeveralFiles);
    }

    return { exportAsSource, exportConfigurationFile, exportToSeveralFiles,
             baseLabel,
             useAddress ? address : OptionalInt(),
             sourceProfile.getSourceGeneratorConfiguration(),
             overriddenBinaryExtensionWithoutDot};
}

}   // namespace arkostracker
