#pragma once

#include <juce_events/juce_events.h>

#include <vector>

#include "../../export/samples/SampleEncoderFlags.h"
#include "../../export/sourceGenerator/SourceGeneratorConfiguration.h"
#include "../../song/Song.h"
#include "../../utils/ErrorReport.h"
#include "../../utils/task/Task.h"
#include "CommandLineArgumentDescriptor.h"

namespace arkostracker
{

class CommandLineToolHelper
{
public:

    /** The possible source profiles. */
    enum class SourceProfile : uint8_t
    {
        z80,
        m68000,
        m6502Acme,
        m6502Mads
    };

    /** Constructor. */
    CommandLineToolHelper() noexcept = default;

    /**
     * Initializes JUCE. This is useful to avoid assertions.
     * @return an object that must be alive during the whole process.
     */
    static juce::ScopedJuceInitialiser_GUI initJuce() noexcept;

    /**
     * @return the description to show for the "export as" choices (sources or asm). Can be directly added to the tool description line.
     * @param allowExportPlayerConfig false to remove the player config, useless in some exports.
     * @param showExportAsBinary true to show the "export as binary" option, along with related options.
     */
    static juce::String getDescriptionForExportAs(bool allowExportPlayerConfig = true, bool showExportAsBinary = true) noexcept;

    /** @return the description to show for exporting several files. Can be directly added to the tool description line. */
    static juce::String getDescriptionForExportSeveralFiles() noexcept;

    /** @return the description to show for the "treat warning as error" option. Can be directly added to the tool description line. */
    static juce::String getDescriptionForTreatWarningAsError() noexcept;

    /**
     * @return the description to show to exporting sample. Can be directly added to the tool description line.
     * @param canDisableSamples true to be able to disable the samples.
     * @param canIgnoreUselessAndGenerateIndexTable true to be add the options to ignore useless samples, and generated the index table.
     */
    static juce::String getDescriptionForSampleExport(bool canDisableSamples, bool canIgnoreUselessAndGenerateIndexTable = false) noexcept;

    /**
     * Declares the optional "export samples" parameter in the given Descriptors.
     * @param descriptors the descriptors.
     * @param canDisableSamples true to be able to disable the samples. This only affects the display.
     * @param canIgnoreUselessAndGenerateIndexTable true to be add the options to ignore useless samples, and generated the index table.
     */
    void declareExportSamplesParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors, bool canDisableSamples,
        bool canIgnoreUselessAndGenerateIndexTable = false) noexcept;


    /**
     * Declares the optional "treat warning as error" parameter in the given Descriptors.
     * @param descriptors the descriptors.
     */
    void declareTreatWarningAsError(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept;

    /**
     * Declares the optional "export as" parameter (sources or asm) in the given Descriptors.
     * @param descriptors the descriptors.
     * @param allowExportPlayerConfig false to remove the player config, useless in some exports.
     * @param showExportAsBinary true to show the "export as binary" option, along with related options.
     */
    void declareExportAsParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors, bool allowExportPlayerConfig = true, bool showExportAsBinary = true) noexcept;

    /**
     * Declares the optional "export several files" parameter in the given Descriptors.
     * @param descriptors the descriptors.
     */
    void declareExportSeveralFilesParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept;

    /** @return the description to show for the subsong, to be directly added to the tool description line. */
    static juce::String getDescriptionForSubsongParameter() noexcept;
    /** @return the description to show for the subsongs, to be directly added to the tool description line. */
    static juce::String getDescriptionForSubsongsParameter() noexcept;

    /** @return the description of the "force psg frequency", to be directly added to the tool description line. */
    static juce::String getDescriptionForcedPsgFrequencyParameter() noexcept;

    /**
     * Declares the optional Subsong parameter in the given Descriptors (only one subsong).
     * @param descriptors the descriptors.
     */
    void declareSubsongParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept;
    /**
     * Declares the optional Subsongs parameter in the given Descriptors (the subsongs to be exported).
     * @param descriptors the descriptors.
     */
    void declareSubsongsParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept;

    /**
     * Declares the required input Song in the given Descriptors.
     * @param descriptors the descriptors.
     * @param mandatory true if the input song is mandatory.
     */
    void declareInputSongParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors, bool mandatory = true) noexcept;

    /**
     * Declares the optional PSG parameter in the given Descriptors.
     * @param descriptors the descriptors.
     */
    void declarePsgParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept;

    /**
     * Declares the optional Force Psg Frequency parameter in the given Descriptors.
     * @param descriptors the descriptors.
     */
    void declareForcedPsgFrequencyParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept;

    /** @return the input Song file. Call this after the parsing has been done. */
    juce::String getInputSong() const noexcept;

    /** @return true if the Subsong Id is given in parameter. */
    bool isSubsongIdPresent() const noexcept;
    /** @return true if the PSG index is given in parameter. */
    bool isPsgIndexPresent() const noexcept;

    /**
     * @return the Subsong Id, or the first id if the user hasn't entered an index, or empty if invalid (<0 or too high).
     * In case of error, an error is displayed.
     * @param song the Song.
     */
    OptionalId getSubsongIdOrWriteError(const Song& song) const noexcept;

    /**
     * @return the Subsong Ids, or the first id if the user hasn't entered an index, or empty if invalid (wrong parsing, wrong indexes).
     * In case of error, an error is displayed.
     * @param song the Song.
     */
    std::vector<Id> getSubsongIdsOrWriteError(const Song& song) const noexcept;

    /**
     * @return the Psg index from the parameters, or 0 of the user hasn't entered an index, or empty of invalid (<0 or too high).
     * @param song the Song.
     * @param subsongId the subsong ID the PSG is related to.
     */
    OptionalInt getPsgIndexOrWriteError(const Song& song, const Id& subsongId) const noexcept;

    /** @return true if the export must be encoded as a binary file. False for source. */
    bool encodeAsBinary() const noexcept;
    /** @return the possible encoding address, or empty if none has been set. Its validity must have been checked before. */
    OptionalInt getEncodingAddress() const noexcept;
    /** @return the possible Label prefix. May be empty. */
    juce::String getLabelPrefix() const noexcept;

    /** @return true if the user indicated that the player configuration must be exported. */
    bool exportPlayerConfiguration() const noexcept;

    /** @return true if the user indicated that the export is done in several files. */
    bool isExportToSeveralFiles() const noexcept;

    /** @return true if the user indicated that a warning is treated as an error. */
    bool isWarningTreatedAsError() const noexcept;

    /** Checks that the subsong ID and psg index are absent. If present, returns false and writes an error message. */
    bool checkSubsongIdAndPsgIndexAbsentElseDisplayErr() const noexcept;

    /** Checks that the "export samples" data is valid. In these cases, returns false and writes an error message. */
    bool checkExportSamplesValidityAndDisplayErr() const noexcept;

    /** Checks that the "forced PSG frequency" is valid, if present. If not returns false and writes an error message. */
    bool checkTargetPsgFrequencyAndDisplayErr() const noexcept;

    /** @return the flags about the sample export. They may not be valid, this can be checked with the check method. */
    SampleEncoderFlags getSampleEncoderFlags() const noexcept;

    /**
     * Gets or reads the source generator (Z80, etc. or custom). Displays an error if needed.
     * @return the source generator configuration, or nullptr if a problem occurred.
     */
    std::unique_ptr<SourceGeneratorConfiguration> getSourceConfigurationOrDisplayErr() const noexcept;

    /** @return the possibly entered forced PSG frequency, in Hz, or 0 if invalid. */
    OptionalInt getForcedPsgFrequency() const noexcept;

    /**
     * Displays a line just like std::cout does, but adds a carriage-return.
     * @param text the text to display.
     */
    static void cout(const juce::String& text) noexcept;

    /**
     * Displays a line just like std::cerr does, but adds a carriage-return.
     * @param text the text to display.
     */
    static void cerr(const juce::String& text) noexcept;

    /**
     * Loads the Song from all the possible format. In case of error, displays an error. In case of warnings, displays the messages.
     * @param inputFile the input file to load. May not exist.
     * @return the Song, or nullptr if none could be loaded.
     */
    static std::unique_ptr<Song> loadSongOrWriteError(const juce::File& inputFile) noexcept;

    /**
     * @return an output stream from the File path that is given. The existing file is deleted.
     * @param outputFileString the full path to the output file.
     */
    static std::unique_ptr<juce::OutputStream> createFileOutputStream(const juce::String& outputFileString) noexcept;

    /**
     * @return true if the parameters are valid, else output an error and return false.
     * @param saveToBinary true if the user wants to save as binary.
     * @param orgAddress the possible org address.
     */
    static bool checkExportAsValidityAndDisplayErr(bool saveToBinary, OptionalInt orgAddress) noexcept;

    /**
     * Parses the parameters and loads the input song. Errors are directly displayed. Use the other method if wanted to try with a YM.
     * @param argc the parameters count.
     * @param argv the raw parameters of the CLI.
     * @param descriptors the descriptors.
     * @param description the description to display.
     * @return the song, or nullptr if the CLI must stop. True if success. Note that there can be a success yet not song (help for example).
     * In this case, CLI must return 0.
     */
    std::pair<bool, std::shared_ptr<Song>> parseAndGetSong(int argc, char* argv[],  // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
                                                          const std::vector<CommandLineArgumentDescriptor*>& descriptors,
                                                          const juce::String& description) const noexcept;

    /**
     * Parses the parameters and loads the input song OR an YM (zipped or not). Errors are directly displayed.
     * @param argc the parameters count.
     * @param argv the raw parameters of the CLI.
     * @param descriptors the descriptors.
     * @param description the description to display.
     * @param tryToLoadYm true to try to load an YM (zipped or not). False if not interested.
     * @return the song or YM, or nullptr if the CLI must stop. True if success. Note that there can be a success yet not song (help for example).
     * In this case, CLI must return 0.
     */
    std::tuple<bool, std::shared_ptr<Song>, std::unique_ptr<juce::MemoryBlock>> parseAndGetSong(int argc, char* argv[],  // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
                                                          const std::vector<CommandLineArgumentDescriptor*>& descriptors,
                                                          const juce::String& description, bool tryToLoadYm) const noexcept;

    /**
     * Displays the possible warnings/errors from the given Status Report, if it exists.
     * @param errorReport the possible report. If absent, considered a success.
     * @param treatWarningAsError the user option. If true if warnings are found, a failure is returned.
     * @return true if success, false if errors are found, or warnings if treatWarningAsError is set to true.
     */
    static bool displayAndCheckStatusReport(const ErrorReport& errorReport, bool treatWarningAsError) noexcept;

    /**
     * @return a source configuration read from a file (exported from the AT3). Nullptr in case of error, and displays an error.
     * @param inputFilePath the file path. May not exist.
     */
    static std::unique_ptr<SourceGeneratorConfiguration> readSourceProfileFromFileOrDisplayErr(const juce::String& inputFilePath) noexcept;

    /**
     * Performs the given task, displays a message according to the success/failure, and returns 0 (success) or -1 (failure).
     * This is intended to be used at the end of the CLI code, WITHOUT any need of the return type (the task itself too care of writing in
     * an output file for example).
     * @param task the task to perform.
     * @param successMessage the message displayed if everything went fine.
     * @param failureMessage the message displayed if the task failed.
     * @return 0 if everything went fine, else -1.
     */
    template<typename RESULT>
    static int performTaskWithNoResult(Task<RESULT>& task, const juce::String& successMessage, const juce::String& failureMessage) noexcept
    {
        const auto result = task.performTask();

        if (result.first) {
            cout(successMessage);
            return 0;
        }

        // Failure.
        cerr(failureMessage);
        return -1;
    }

private:
    static const juce::String profileZ80String;                                               // String typed by the user for the Z80 profile.
    static const juce::String profile68000String;                                             // String typed by the user for the 68000 profile.
    static const juce::String profile6502AcmeString;                                          // String typed by the user for the 6502 Acme profile.
    static const juce::String profile6502MadsString;                                          // String typed by the user for the 6502 Mads profile.

    static const juce::String machineCpc;
    static const juce::String machinePcw;
    static const juce::String machineSpectrum;
    static const juce::String machineSpecNext;
    static const juce::String machinePentagon;
    static const juce::String machineMsx;
    static const juce::String machineSvi;
    static const juce::String machineSt;
    static const juce::String machineXe;
    static const juce::String machineXl;
    static const juce::String machineApple2;
    static const juce::String machineOric;
    static const juce::String machineVectrex;

    static const juce::String machineAll;

    std::unique_ptr<Option> optionSubsong;
    std::unique_ptr<Parameter> parameterSubsong;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionSubsong;             // The Subsong(s) option in the parameters.

    std::unique_ptr<Option> optionPsg;
    std::unique_ptr<Parameter> parameterPsg;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionPsg;                 // The PSG option in the parameters.

    std::unique_ptr<CommandLineArgumentDescriptor> descriptorParameterInput;            // The input song in the parameters.

    std::unique_ptr<Option> optionExportAsBinary;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportAsBinary;

    std::unique_ptr<Option> optionEncodingAddress;
    std::unique_ptr<Parameter> parameterEncodingAddress;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionEncodingAddress;

    std::unique_ptr<Option> optionExportSeveralFiles;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSeveralFiles;

    std::unique_ptr<Option> optionLabelPrefix;
    std::unique_ptr<Parameter> parameterLabelPrefix;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionLabelPrefix;

    std::unique_ptr<Option> optionSourceProfile;
    std::unique_ptr<Parameter> parameterSourceProfile;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionSourceProfile;

    std::unique_ptr<Option> optionCustomSourceProfile;
    std::unique_ptr<Parameter> parameterCustomSourceProfile;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorCustomOptionSourceProfile;

    std::unique_ptr<Option> optionExportPlayerConfiguration;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportPlayerConfiguration;

    std::unique_ptr<Option> optionTreatWarningAsError;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorTreatWarningAsError;

    std::unique_ptr<Option> optionExportSamplesDisabled;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSamplesDisabled;

    std::unique_ptr<Option> optionExportSamplesAmplitude;
    std::unique_ptr<Parameter> parameterExportSamplesAmplitude;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSamplesAmplitude;

    std::unique_ptr<Option> optionExportSamplesOffset;
    std::unique_ptr<Parameter> parameterExportSamplesOffset;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSamplesOffset;

    std::unique_ptr<Option> optionExportSamplesPaddingLength;
    std::unique_ptr<Parameter> parameterExportSamplesPaddingLength;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSamplesPaddingLength;

    std::unique_ptr<Option> optionExportSamplesFadeOutLength;
    std::unique_ptr<Parameter> parameterExportSamplesFadeOutLength;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSamplesFadeOutLength;

    std::unique_ptr<Option> optionExportSamplesPaddingAndFadeToMinValue;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSamplesPaddingAndFadeToMinValue;

    std::unique_ptr<Option> optionExportSamplesExportOnlyUsedLength;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportSamplesExportOnlyUsedLength;

    std::unique_ptr<Option> optionExportUnusedSamples;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionExportUnusedSamples;

    std::unique_ptr<Option> optionDontGenerateIndexTableForSamples;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorOptionDontGenerateIndexTableForSamples;

    std::unique_ptr<Option> optionForcedPsgFrequency;
    std::unique_ptr<Parameter> parameterForcedPsgFrequency;
    std::unique_ptr<CommandLineArgumentDescriptor> descriptorForcedPsgFrequency;
};

}   // namespace arkostracker
