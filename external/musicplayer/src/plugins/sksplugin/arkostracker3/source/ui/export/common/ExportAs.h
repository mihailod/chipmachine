#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../business/sourceProfile/SourceProfile.h"
#include "../../../utils/OptionalValue.h"
#include "../../components/EditText.h"

namespace arkostracker 
{

class PreferencesManager;

/**
 * Reusable Component showing the "export as" group.
 * It also has a bit of logic to auto-select the previous user's choice from the Preferences.
 *
 * Use the storeChanges method to store the changes into the Preferences.
 */
class ExportAs final : public juce::Component
{
public:
    static const int desiredHeight;                 // The height the client should be giving this Component.

    /** The data being stored. */
    class Result
    {
    public:
        /**
         * @param pOverriddenBinaryExtensionWithoutDot if present, overrides the returned binary extension. Useful to return "akg" instead of the generic "bin" for ex.
         */
        Result(bool pExportAsSource, bool pExportConfigurationFile, bool pExportToSeveralFiles,
               juce::String pBaseLabel, OptionalInt pAddress, SourceGeneratorConfiguration pConfiguration,
               juce::String pOverriddenBinaryExtensionWithoutDot) noexcept :
               exportAsSource(pExportAsSource),
               exportConfigurationFile(pExportConfigurationFile),
               exportToSeveralFiles(pExportToSeveralFiles),
               baseLabel(std::move(pBaseLabel)),
               address(pAddress),
               configuration(std::move(pConfiguration)),
               overriddenBinaryExtensionWithoutDot(std::move(pOverriddenBinaryExtensionWithoutDot))
        {
        }

        /**
         * @return the extension ("asm" for example, without the '.') to return the extension according to the source/binary choice.
         * It may return a generic binary extension, or a specific one (akg for example).
         */
        juce::String getExtension() const noexcept {
            return exportAsSource ? configuration.getSourceFileExtension() :
                // Uses the overridden binary extension, if present. Else, uses the generic one.
                overriddenBinaryExtensionWithoutDot.isEmpty() ? configuration.getBinaryFileExtension() : overriddenBinaryExtensionWithoutDot;
        }

        bool isExportAsSource() const noexcept
        {
            return exportAsSource;
        }

        bool getExportConfigurationFile() const noexcept
        {
            return exportConfigurationFile;
        }

        bool isExportToSeveralFiles() const noexcept
        {
            return exportToSeveralFiles;
        }

        const juce::String& getBaseLabel() const noexcept
        {
            return baseLabel;
        }

        const OptionalInt& getAddress() const noexcept
        {
            return address;
        }

        const SourceGeneratorConfiguration& getConfiguration() const noexcept
        {
            return configuration;
        }

    private:
        bool exportAsSource;
        bool exportConfigurationFile;
        bool exportToSeveralFiles;
        juce::String baseLabel;
        OptionalInt address;
        SourceGeneratorConfiguration configuration;
        juce::String overriddenBinaryExtensionWithoutDot;
    };

    /**
     * Constructor.
     * @param canExportToSeveralFiles true if the user can export to several files. False to have it greyed out.
     * @param overriddenBinaryExtensionWithoutDot if present, overrides the returned binary extension. Useful to return "akg" instead of the generic "bin" for ex.
     * @param canExportConfigurationFile false to disable the "export configuration file".
     */
    explicit ExportAs(bool canExportToSeveralFiles = true, juce::String overriddenBinaryExtensionWithoutDot = juce::String(),
        bool canExportConfigurationFile = true) noexcept;

    /** Stores the changes from the current UI into the Preferences. */
    Result storeChanges() noexcept;

    /**
     * Calls this method when the selected Subsong changes (in an other UI element, for example).
     * This will update the label prefix to the name of the Subsong.
     * @param subsongName the name of the Subsong.
     */
    void onSubsongChanged(const juce::String& subsongName) noexcept;

    // Component method implementations.
    // ====================================
    void resized() override;

private:
    static const int sourceLabelMaximumLength;

    /** Locates the views according to the current export (source or binary). */
    void locateAndSetUpDynamicViews() noexcept;

    /** Called when the Source Toggle is clicked. */
    void onSourceToggleSelected() noexcept;
    /** Called when the Binary Toggle is clicked. */
    void onBinaryToggleSelected() noexcept;

    /** @return a prefix from the given name. It will be trimmed of illegal characters. For example, the name of the subsong is a good candidate. */
    static juce::String extractLabelPrefix(const juce::String& name) noexcept;

    const bool canExportToSeveralFiles;
    juce::String overriddenBinaryExtensionWithoutDot;
    const bool canExportConfigurationFile;

    PreferencesManager& preferences;

    std::vector<SourceProfile> sourceProfiles;

    juce::TextEditor::LengthAndCharacterRestriction labelEditorRestriction;         // Restricts the label Editor.
    juce::TextEditor::LengthAndCharacterRestriction addressEditorRestriction;       // Restricts the address TextEditor.

    juce::GroupComponent group;

    juce::ToggleButton exportAsSourceToggle;
    juce::ToggleButton exportAsBinaryToggle;
    juce::ToggleButton exportToSeveralFilesToggle;
    juce::ToggleButton generateConfigurationFileToggle;

    juce::Label sourceProfileLabel;
    juce::ComboBox sourceProfileComboBox;

    juce::Label sourceLabelsPrefixIfSourceLabel;                    // Visible only for Source.
    EditText sourceLabelsPrefixIfSourceEditor;

    juce::ToggleButton encodeToAddressIfSourceToggle;               // Visible only for Source.
    juce::Label encodeToAddressIfBinaryLabel;
    EditText encodeToAddressEditor;                                 // For both.
};

}   // namespace arkostracker
