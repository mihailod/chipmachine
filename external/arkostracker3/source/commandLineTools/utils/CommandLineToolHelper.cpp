#include "CommandLineToolHelper.h"

#include <iostream>

#include "../../business/serialization/sourceProfile/SourceProfileSerializer.h"
#include "../../business/sourceProfile/SourceProfileValidator.h"
#include "../../import/loader/SongLoader.h"
#include "../../reader/ym/YmReader.h"
#include "../../utils/FileUtil.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/StringUtil.h"
#include "../../utils/ZipHelper.h"
#include "CommandLineParser.h"

namespace arkostracker
{

const juce::String CommandLineToolHelper::profileZ80String("z80");                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::profile68000String("68000");            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::profile6502AcmeString("6502acme");      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::profile6502MadsString("6502mads");      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String CommandLineToolHelper::machineCpc("cpc");                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machinePcw("pcw");                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineSpectrum("spectrum");            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineSpecNext("specNext");            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machinePentagon("pentagon");            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineMsx("msx");                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineSvi("svi");                      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineSt("st");                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineXe("xe");                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineXl("xl");                        // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineApple2("apple2");                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineOric("oric");                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String CommandLineToolHelper::machineVectrex("vectrex");              // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String CommandLineToolHelper::machineAll =                            // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
        machineCpc + "/"
        + machinePcw + "/"
        + machineSpectrum + "/"
        + machineSpecNext + "/"
        + machinePentagon + "/"
        + machineMsx + "/"
        + machineSvi + "/"
        + machineSt + "/"
        + machineXe + "/"
        + machineXl + "/"
        + machineApple2 + "/"
        + machineOric + "/"
        + machineVectrex;

juce::ScopedJuceInitialiser_GUI CommandLineToolHelper::initJuce() noexcept
{
    return juce::ScopedJuceInitialiser_GUI { };       // To remove JUCE assertions being not initialized.
}

juce::String CommandLineToolHelper::getDescriptionForExportAs(const bool allowExportPlayerConfig, const bool showExportAsBinary) noexcept
{
    auto string = showExportAsBinary ? juce::String("[--exportAsBinary] [--encodingAddress <address>] ") : "";
    if (allowExportPlayerConfig) {
        string += "[--exportAsBinary] [--encodingAddress <address>] ";
    }

    return string += "[--sourceProfile <z80/68000/6502acme/6502mads>] [--customSourceProfileFile <path to xml file>]";
}

juce::String CommandLineToolHelper::getDescriptionForExportSeveralFiles() noexcept
{
    return "[-esf]";
}

juce::String CommandLineToolHelper::getDescriptionForSampleExport(const bool canDisableSamples, const bool canIgnoreUselessAndGenerateIndexTable) noexcept
{
    auto beginning = canDisableSamples ? juce::String("[-smd] ") : juce::String();
    if (canIgnoreUselessAndGenerateIndexTable) {
        beginning += "--seu --sdg ";
    }
    return beginning + "[-seu] [-sdg] [-sms] [-sma <amplitude>] [-smo <offset>] [-smpl <padding length>] [-smfo <fadeout length>] [-smmin] [-smonly]";
}

juce::String CommandLineToolHelper::getDescriptionForSubsongParameter() noexcept
{
    return R"([-s <subsong number>])";
}

juce::String CommandLineToolHelper::getDescriptionForSubsongsParameter() noexcept
{
    return R"([-s <1,2,3...>])";
}

juce::String CommandLineToolHelper::getDescriptionForcedPsgFrequencyParameter() noexcept
{
    return juce::String("[--forcedPsgFrequency <psg frequency in Hz, or the machine: ")
        + machineAll + ">]";
}

juce::String CommandLineToolHelper::getDescriptionForTreatWarningAsError() noexcept
{
    return R"([--treatWarningAsError])";
}

void CommandLineToolHelper::declareExportSamplesParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors, const bool canDisableSamples,
    const bool canIgnoreUselessAndGenerateIndexTable) noexcept
{
    if (canDisableSamples) {
        optionExportSamplesDisabled = std::make_unique<Option>(Option::buildOption("smd", "sampleExportDisabled"));
        descriptorOptionExportSamplesDisabled = std::make_unique<CommandLineArgumentDescriptor>(
                CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, no samples are exported."), *optionExportSamplesDisabled, false));
        descriptors.push_back(descriptorOptionExportSamplesDisabled.get());
    }

    if (canIgnoreUselessAndGenerateIndexTable) {
        optionExportUnusedSamples = std::make_unique<Option>(Option::buildOption("seu", "sampleExportUnused"));
        descriptorOptionExportUnusedSamples = std::make_unique<CommandLineArgumentDescriptor>(
                CommandLineArgumentDescriptor::buildArgumentWithOption(
                    juce::translate("If present, unused samples are exported."), *optionExportUnusedSamples, false));
        descriptors.push_back(descriptorOptionExportUnusedSamples.get());

        optionDontGenerateIndexTableForSamples = std::make_unique<Option>(Option::buildOption("sdg", "sampleDontGenerateIndexTable"));
        descriptorOptionDontGenerateIndexTableForSamples = std::make_unique<CommandLineArgumentDescriptor>(
                CommandLineArgumentDescriptor::buildArgumentWithOption(
                    juce::translate("If present, the index table is not generated."), *optionDontGenerateIndexTableForSamples, false));
        descriptors.push_back(descriptorOptionDontGenerateIndexTableForSamples.get());
    }

    optionExportSamplesAmplitude = std::make_unique<Option>(Option::buildOption("sma", "sampleExportAmplitude"));
    parameterExportSamplesAmplitude = std::make_unique<Parameter>(ParameterType::integer);
    descriptorOptionExportSamplesAmplitude = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("Sets the amplitude of each value of the samples. 256 by default."),
                                                                   *optionExportSamplesAmplitude, false, *parameterExportSamplesAmplitude));
    descriptors.push_back(descriptorOptionExportSamplesAmplitude.get());

    optionExportSamplesOffset = std::make_unique<Option>(Option::buildOption("smo", "sampleExportOffset"));
    parameterExportSamplesOffset = std::make_unique<Parameter>(ParameterType::integer);
    descriptorOptionExportSamplesOffset = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the samples are added an offset."),
                                                                   *optionExportSamplesOffset, false, *parameterExportSamplesOffset));
    descriptors.push_back(descriptorOptionExportSamplesOffset.get());

    optionExportSamplesPaddingLength = std::make_unique<Option>(Option::buildOption("smpl", "sampleExportPaddingLength"));
    parameterExportSamplesPaddingLength = std::make_unique<Parameter>(ParameterType::integer);
    descriptorOptionExportSamplesPaddingLength = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present and >0, a padding is added at the end of every sample."),
                                                                   *optionExportSamplesPaddingLength, false, *parameterExportSamplesPaddingLength));
    descriptors.push_back(descriptorOptionExportSamplesPaddingLength.get());

    optionExportSamplesFadeOutLength = std::make_unique<Option>(Option::buildOption("smfo", "sampleExportFadeOutLength"));
    parameterExportSamplesFadeOutLength = std::make_unique<Parameter>(ParameterType::integer);
    descriptorOptionExportSamplesFadeOutLength = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present and >0, the last bytes of the non-looping samples are faded out."),
                                                                   *optionExportSamplesFadeOutLength, false, *parameterExportSamplesFadeOutLength));
    descriptors.push_back(descriptorOptionExportSamplesFadeOutLength.get());

    optionExportSamplesPaddingAndFadeToMinValue = std::make_unique<Option>(Option::buildOption("smmin", "sampleExportPaddingAndFadeToMinValue"));
    descriptorOptionExportSamplesPaddingAndFadeToMinValue = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, the sample padding and fade-out target 0, instead of middle value."),
                                                                   *optionExportSamplesPaddingAndFadeToMinValue, false));
    descriptors.push_back(descriptorOptionExportSamplesPaddingAndFadeToMinValue.get());

    optionExportSamplesExportOnlyUsedLength = std::make_unique<Option>(Option::buildOption("smonly", "sampleExportOnlyUsedLength"));
    descriptorOptionExportSamplesExportOnlyUsedLength = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, export only the used length of the samples."),
                                                                   *optionExportSamplesExportOnlyUsedLength, false));
    descriptors.push_back(descriptorOptionExportSamplesExportOnlyUsedLength.get());
}

void CommandLineToolHelper::declareTreatWarningAsError(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept
{
    optionTreatWarningAsError = std::make_unique<Option>(Option::buildOption("twe", "treatWarningAsError"));
    descriptorTreatWarningAsError = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, all the warnings are considered errors (security)."),
                                                                   *optionTreatWarningAsError, false));
    descriptors.push_back(descriptorTreatWarningAsError.get());
}

void CommandLineToolHelper::declareExportAsParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors, const bool allowExportPlayerConfig,
    const bool showExportAsBinary) noexcept
{
    if (showExportAsBinary) {
        optionExportAsBinary = std::make_unique<Option>(Option::buildOption("bin", "exportAsBinary"));
        descriptorOptionExportAsBinary = std::make_unique<CommandLineArgumentDescriptor>(
                CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, exports as a binary file. If not, exports as source (default)."),
                                                                       *optionExportAsBinary, false));
        descriptors.push_back(descriptorOptionExportAsBinary.get());

        optionEncodingAddress = std::make_unique<Option>(Option::buildOption("adr", "encodingAddress"));
        parameterEncodingAddress = std::make_unique<Parameter>(ParameterType::integer);
        descriptorOptionEncodingAddress = std::make_unique<CommandLineArgumentDescriptor>(
                CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, encodes the file to this address (may be hex (0xa000 for example)). "
                                                                                       "Mandatory if encoding as binary."), *optionEncodingAddress, false, *parameterEncodingAddress));
        descriptors.push_back(descriptorOptionEncodingAddress.get());
    }

    optionLabelPrefix = std::make_unique<Option>(Option::buildLongOption("labelPrefix"));
    parameterLabelPrefix = std::make_unique<Parameter>(ParameterType::string);
    descriptorOptionLabelPrefix = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("Optional, but useful for source generation. Indicates a prefix to all the labels."),
                                                                   *optionLabelPrefix, false, *parameterLabelPrefix));
    descriptors.push_back(descriptorOptionLabelPrefix.get());

    if (allowExportPlayerConfig) {
        optionExportPlayerConfiguration = std::make_unique<Option>(Option::buildLongOption("exportPlayerConfig"));
        descriptorOptionExportPlayerConfiguration = std::make_unique<CommandLineArgumentDescriptor>(
                CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("Exports a player configuration source file, if present."),
                                                                       *optionExportPlayerConfiguration, false));
        descriptors.push_back(descriptorOptionExportPlayerConfiguration.get());
    }

    // Source profile options.
    optionSourceProfile = std::make_unique<Option>(Option::buildLongOption("sourceProfile"));       // Do not use "sp" as a shortcut, already used!
    parameterSourceProfile = std::make_unique<Parameter>(ParameterType::string);
    descriptorOptionSourceProfile = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("When generating the sources, indicates what source profile to use (among " + profileZ80String
                                                                             + ", " + profile68000String + ", " + profile6502AcmeString + ", " + profile6502MadsString
                                                                             + "). Default is z80. If the export is binary, z80 must be chosen."),
                                                                   *optionSourceProfile, false, *parameterSourceProfile));
    descriptors.push_back(descriptorOptionSourceProfile.get());

    optionCustomSourceProfile = std::make_unique<Option>(Option::buildLongOption("customSourceProfileFile"));
    parameterCustomSourceProfile = std::make_unique<Parameter>(ParameterType::string);
    descriptorCustomOptionSourceProfile = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("Path to a source profile data file. Use File > Setup > Source Profile >"
                                                                                   " Export to generate a profile to load."),
                                                                   *optionCustomSourceProfile, false, *parameterCustomSourceProfile));
    descriptors.push_back(descriptorCustomOptionSourceProfile.get());
}

void CommandLineToolHelper::declareExportSeveralFilesParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept
{
    optionExportSeveralFiles = std::make_unique<Option>(Option::buildOption("esf", "exportSeveralFiles"));
    descriptorOptionExportSeveralFiles = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, each subsong will be encoded in its own file."),
                                                                   *optionExportSeveralFiles, false));
    descriptors.push_back(descriptorOptionExportSeveralFiles.get());
}

void CommandLineToolHelper::declareSubsongParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept
{
    optionSubsong = std::make_unique<Option>(Option::buildOption("s", "subsong"));
    parameterSubsong = std::make_unique<Parameter>(ParameterType::integer);
    descriptorOptionSubsong = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("The subsong number (>=1). 1 is default."), *optionSubsong, false, *parameterSubsong));

    descriptors.push_back(descriptorOptionSubsong.get());
}

void CommandLineToolHelper::declareSubsongsParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept
{
    // The same objects as for the "subsong" above are used.
    optionSubsong = std::make_unique<Option>(Option::buildOption("s", "subsongs"));
    parameterSubsong = std::make_unique<Parameter>(ParameterType::string);
    descriptorOptionSubsong = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("The subsong to export from their number (>=1), comma separated. Example: \"1,3,5\". By default, all subsongs are exported."),
                *optionSubsong, false, *parameterSubsong));

    descriptors.push_back(descriptorOptionSubsong.get());
}

void CommandLineToolHelper::declareForcedPsgFrequencyParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept
{
    optionForcedPsgFrequency = std::make_unique<Option>(Option::buildLongOption("forcedPsgFrequency"));
    parameterForcedPsgFrequency = std::make_unique<Parameter>(ParameterType::string);
    descriptorForcedPsgFrequency = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("If present, forces the output PSG frequency, in Hz (1000000 for CPC for example), "
                                                                                   "or type the machine: " + machineAll + "."), *optionForcedPsgFrequency,
                false, *parameterForcedPsgFrequency));

    descriptors.push_back(descriptorForcedPsgFrequency.get());
}

void CommandLineToolHelper::declareInputSongParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors, bool mandatory) noexcept
{
    descriptorParameterInput = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithDirectValue(juce::translate("<path to input song>"),
                                                                        juce::translate("Path and filename to the song to load."),
                                                                        mandatory));

    descriptors.push_back(descriptorParameterInput.get());
}

void CommandLineToolHelper::declarePsgParameter(std::vector<CommandLineArgumentDescriptor*>& descriptors) noexcept
{
    optionPsg = std::make_unique<Option>(Option::buildOption("p", "psg"));
    parameterPsg = std::make_unique<Parameter>(ParameterType::integer);
    descriptorOptionPsg = std::make_unique<CommandLineArgumentDescriptor>(
            CommandLineArgumentDescriptor::buildArgumentWithOption(juce::translate("The PSG number (>=1). 1 is default."), *optionPsg, false, *parameterPsg));

    descriptors.push_back(descriptorOptionPsg.get());
}

bool CommandLineToolHelper::encodeAsBinary() const noexcept
{
    return (descriptorOptionExportAsBinary != nullptr) && descriptorOptionExportAsBinary->isPresent();
}

OptionalInt CommandLineToolHelper::getEncodingAddress() const noexcept
{
    return ((parameterEncodingAddress != nullptr) && parameterEncodingAddress->isPresent()) ? parameterEncodingAddress->getValueAsInteger() : OptionalInt();
}

juce::String CommandLineToolHelper::getLabelPrefix() const noexcept
{
    return parameterLabelPrefix->isPresent() ? parameterLabelPrefix->getValueAsString() : juce::String();
}

OptionalInt CommandLineToolHelper::getForcedPsgFrequency() const noexcept
{
    if ((parameterForcedPsgFrequency == nullptr) || !parameterForcedPsgFrequency->isPresent()) {
        return { };
    }

    // It can be either a number of a String.
    const auto frequencyString = parameterForcedPsgFrequency->getValueAsString();
    OptionalInt forcedPsgFrequency;
    if ((frequencyString == machineCpc) || (frequencyString == machinePcw)) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyCPC;
    } else if (frequencyString == machineSpectrum) {
        forcedPsgFrequency = PsgFrequency::psgFrequencySpectrum;
    } else if (frequencyString == machineSpecNext) {
        forcedPsgFrequency = PsgFrequency::psgFrequencySpecNext;
    } else if (frequencyString == machinePentagon) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyPentagon128K;
    } else if ((frequencyString == machineMsx) || (frequencyString == machineSvi)) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyMsxAndSvi;
    } else if (frequencyString == machineSt) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyAtariST;
    } else if ((frequencyString == machineXe) || (frequencyString == machineXl)) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyAtariXEXL;
    } else if (frequencyString == machineApple2) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyApple2;
    } else if (frequencyString == machineOric) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyOric;
    } else if (frequencyString == machineVectrex) {
        forcedPsgFrequency = PsgFrequency::psgFrequencyVectrex;
    }

    // Maybe a number?
    if (forcedPsgFrequency.isAbsent()) {
        auto success = false;
        forcedPsgFrequency = parameterForcedPsgFrequency->getValueAsIntegerWithFailureTest(success);
        if (!success) {
            forcedPsgFrequency = 0;
        }
    }

    return forcedPsgFrequency;
}

bool CommandLineToolHelper::exportPlayerConfiguration() const noexcept
{
    return (descriptorOptionExportPlayerConfiguration != nullptr) && descriptorOptionExportPlayerConfiguration->isPresent();
}

bool CommandLineToolHelper::isExportToSeveralFiles() const noexcept
{
    return descriptorOptionExportSeveralFiles->isPresent();
}

bool CommandLineToolHelper::isWarningTreatedAsError() const noexcept
{
    return descriptorTreatWarningAsError->isPresent();
}

SampleEncoderFlags CommandLineToolHelper::getSampleEncoderFlags() const noexcept
{
    const auto amplitude = parameterExportSamplesAmplitude->isPresent() ? parameterExportSamplesAmplitude->getValueAsInteger() : 256;

    return SampleEncoderFlags(
            (descriptorOptionExportSamplesDisabled == nullptr) || !descriptorOptionExportSamplesDisabled->isPresent(),
            amplitude,
            parameterExportSamplesOffset->getValueAsInteger(),
            parameterExportSamplesPaddingLength->getValueAsInteger(),
            parameterExportSamplesFadeOutLength->getValueAsInteger(),
            descriptorOptionExportSamplesPaddingAndFadeToMinValue->isPresent(),
            descriptorOptionExportSamplesExportOnlyUsedLength->isPresent(),
            (descriptorOptionExportUnusedSamples == nullptr) || !descriptorOptionExportUnusedSamples->isPresent(),
            (descriptorOptionDontGenerateIndexTableForSamples == nullptr) || !descriptorOptionDontGenerateIndexTableForSamples->isPresent()
    );
}

bool CommandLineToolHelper::checkSubsongIdAndPsgIndexAbsentElseDisplayErr() const noexcept
{
    if (isSubsongIdPresent()) {
        cerr(juce::translate("The subsong index makes no sense in this context. Please remove this parameter."));
        return false;
    }
    if (isPsgIndexPresent()) {
        cerr(juce::translate("The PSG index is useless in this context. Please remove this parameter."));
        return false;
    }

    return true;
}

bool CommandLineToolHelper::checkExportSamplesValidityAndDisplayErr() const noexcept
{
    const auto flags = getSampleEncoderFlags();

    if ((flags.getOffset() < 0) || (flags.getOffset() > 255)) {
        cerr("The sample export offset must be >=0 and <=255.");
        return false;
    }

    if ((flags.getPaddingLength() < 0) || (flags.getPaddingLength() > 32768)) {
        cerr("The sample export padding length must be >=0 and <=32768.");
        return false;
    }

    if ((flags.getAmplitude() < 2) || (flags.getAmplitude() > 256)) {
        cerr("The sample amplitude must be >=2 and <=256.");
        return false;
    }

    return true;
}

bool CommandLineToolHelper::checkTargetPsgFrequencyAndDisplayErr() const noexcept
{
    if (const auto targetPsgFrequency = getForcedPsgFrequency(); targetPsgFrequency.isPresent()) {
        if ((targetPsgFrequency.getValue() < PsgFrequency::minimumPsgFrequency) || (targetPsgFrequency.getValue() > PsgFrequency::maximumPsgFrequency)) {
            cerr(juce::translate("The forced PSG frequency is invalid. It must be a valid machine, or included from ") + juce::String(PsgFrequency::minimumPsgFrequency)
                + " to " + juce::String(PsgFrequency::maximumPsgFrequency) + ".");
            return false;
        }
    }

    return true;
}

juce::String CommandLineToolHelper::getInputSong() const noexcept
{
    return descriptorParameterInput->getDirectValue();
}

bool CommandLineToolHelper::isSubsongIdPresent() const noexcept
{
    return (parameterSubsong != nullptr) && parameterSubsong->isPresent();
}

bool CommandLineToolHelper::isPsgIndexPresent() const noexcept
{
    return (parameterPsg != nullptr) && parameterPsg->isPresent();
}

OptionalId CommandLineToolHelper::getSubsongIdOrWriteError(const Song& song) const noexcept
{
    auto success = true;
    auto userSubsongIndex = 1;      // Index entered the user starts at 1.

    // Reads the possible Subsong index.
    if (parameterSubsong->isPresent()) {
        userSubsongIndex = parameterSubsong->getValueAsIntegerWithFailureTest(success);
    }
    const auto subsongIndex = userSubsongIndex - 1;
    success = success && (subsongIndex >= 0);

    auto subsongId = success ? song.getSubsongId(subsongIndex) : OptionalId();
    if (subsongId.isAbsent()) {
        cerr(juce::translate("The subsong index must be >0 and within bounds."));
        return { };
    }

    return subsongId;
}

std::vector<Id> CommandLineToolHelper::getSubsongIdsOrWriteError(const Song& song) const noexcept
{
    // Anything present? If not, returns all the Subsongs.
    if (!parameterSubsong->isPresent()) {
        return song.getSubsongIds();
    }

    std::vector<Id> subsongIds;

    const auto splits = StringUtil::split(parameterSubsong->getValueAsString(), ",");
    auto success = false;
    std::set<int> userSubsongIndexes;
    for (const auto& split : splits) {
        const auto userSubsongIndex = StringUtil::stringToInt(split, success);
        if (!success) {
            cerr(juce::translate("The subsong index is invalid: ") + juce::String(split) + ".");
            return { };
        }

        // Don't allow already added subsong indexes.
        if (userSubsongIndexes.find(userSubsongIndex) != userSubsongIndexes.cend()) {
            cerr(juce::translate("The subsong index has already been added: ") + juce::String(userSubsongIndex) + ".");
            return { };
        }
        userSubsongIndexes.insert(userSubsongIndex);

        const auto subsongIndex = userSubsongIndex - 1;
        const auto subsongId = song.getSubsongId(subsongIndex);
        if (subsongId.isAbsent()) {
            cerr(juce::translate("The subsong index \"") + juce::String(userSubsongIndex)+ juce::translate("\" must be >0 and within bounds."));
            return { };
        }

        subsongIds.push_back(subsongId.getValueRef());
    }

    // Security, shouldn't happen.
    if (subsongIds.empty()) {
        cerr(juce::translate("Invalid subsong indexes."));
        jassertfalse;
        return { };
    }

    return subsongIds;
}

OptionalInt CommandLineToolHelper::getPsgIndexOrWriteError(const Song& song, const Id& subsongId) const noexcept
{
    if (!song.doesSubsongExist(subsongId)) {
        cerr(juce::translate("The subsong doesn't exist."));
        return { };
    }

    auto psgNumber = 1;         // User PSG starts at 1.
    // Reads the possible PSG index.
    if (parameterPsg->isPresent()) {
        bool success;       // NOLINT(*-init-variables)
        psgNumber = parameterPsg->getValueAsIntegerWithFailureTest(success);
        if ((!success) || (psgNumber < 1)) {
            cerr(juce::translate("The PSG number must be >0."));
            return { };
        }
    }
    const auto psgIndex = psgNumber - 1;

    // How many PSGs in this Song?
    const auto psgCount = song.getPsgCount(subsongId);
    if (psgIndex >= psgCount) {
        cerr(juce::translate("There are only " + juce::String(psgCount) + " PSGs in this subsong."));
        return { };
    }

    return psgIndex;
}

void CommandLineToolHelper::cout(const juce::String& text) noexcept
{
    std::cout << text << '\n';
}

void CommandLineToolHelper::cerr(const juce::String& text) noexcept
{
    std::cerr << text << '\n';
}

std::unique_ptr<Song> CommandLineToolHelper::loadSongOrWriteError(const juce::File& inputFile) noexcept
{
    SongLoader songLoader;
    const auto result = songLoader.loadSong(inputFile);

    switch (result->status) {
        case SongLoader::ImportStatus::fileNotFound:
            cerr(juce::translate("The file does not exist."));
            break;
        case SongLoader::ImportStatus::loadingFailed: [[fallthrough]];
        case SongLoader::ImportStatus::decodingFailure: [[fallthrough]];
        case SongLoader::ImportStatus::noMatchingFormat:
            cerr(juce::translate("Unknown format for the song!"));
            break;
        case SongLoader::ImportStatus::configurationRequired:
            cerr(juce::translate("This is a bug. Contact the author right away!"));
            break;
        case SongLoader::ImportStatus::ok:
            break;
    }

    return std::move(result->song);
}

std::unique_ptr<juce::OutputStream> CommandLineToolHelper::createFileOutputStream(const juce::String& outputFileString) noexcept
{
    auto outputFile = FileUtil::getFileFromString(outputFileString);
    (void)outputFile.deleteFile();

    return std::make_unique<juce::FileOutputStream>(outputFile);
}

bool CommandLineToolHelper::checkExportAsValidityAndDisplayErr(const bool saveToBinary, const OptionalInt orgAddress) noexcept
{
    if (saveToBinary && orgAddress.isAbsent()) {
        cerr(juce::translate("The encoding address must be declared if saving to binary."));
        return false;
    }

    return true;
}

std::pair<bool, std::shared_ptr<Song>> CommandLineToolHelper::parseAndGetSong(
        const int argc, char* argv[], const std::vector<CommandLineArgumentDescriptor*>& descriptors,  // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
        const juce::String& description) const noexcept
{
    auto result = parseAndGetSong(argc, argv, descriptors, description, false);
    return { std::get<0>(result), std::get<1>(result) };
}

std::tuple<bool, std::shared_ptr<Song>, std::unique_ptr<juce::MemoryBlock>> CommandLineToolHelper::parseAndGetSong(
    const int argc, char* argv[],
    const std::vector<CommandLineArgumentDescriptor*>& descriptors, const juce::String& description, const bool tryToLoadYm) const noexcept
{
    CommandLineParser parser(argc, argv, descriptors, description);
    const auto parseResult = parser.parse();
    if (parseResult == CommandLineParser::ParsingResult::parsingHelp) {
        return { true, nullptr, nullptr };               // Stops, no parameters.
    }
    if (parseResult == CommandLineParser::ParsingResult::parsingFailure) {
        return { false, nullptr, nullptr };              // Stops if parsing failed.
    }

    const auto inputFileString = getInputSong();
    const auto inputFile = FileUtil::getFileFromString(inputFileString);

    // Maybe a YM?
    if (tryToLoadYm) {
        if (const auto fileMemoryBlock = MemoryBlockUtil::fromFile(inputFile); fileMemoryBlock != nullptr) {
            const auto ymMemoryBlock = ZipHelper::unzip(*fileMemoryBlock);
            auto ymInputStream = juce::MemoryInputStream(ymMemoryBlock, false);
            if (YmReader ymReader(ymInputStream); ymReader.checkFormat()) {
                return { true, nullptr, std::make_unique<juce::MemoryBlock>(ymMemoryBlock) };
            }
        }
    }

    // Loads the Song. It may fail.
    auto songUniquePtr = loadSongOrWriteError(inputFile);
    if (songUniquePtr != nullptr) {
        return { true, std::move(songUniquePtr), nullptr };
    }

    return { false, nullptr, nullptr };
}

bool CommandLineToolHelper::displayAndCheckStatusReport(const ErrorReport& errorReport, const bool treatWarningAsError) noexcept
{
    for (auto lineIndex = 0, lineCount = errorReport.getLineCount(); lineIndex < lineCount; ++lineIndex) {
        const auto& statusReportLine = errorReport.getLine(lineIndex);
        const auto level = statusReportLine.getLevel();
        const auto isError = (level == ReportLevel::error);
        auto text = isError ? juce::translate("Error") : juce::translate("Warning");

        const auto lineNumber = statusReportLine.getLineNumber();
        if (lineNumber.isPresent()) {
            text += " at line " + juce::String(lineNumber.getValue());
        }

        text += ": ";
        text += statusReportLine.getText();

        if (isError) {
            cerr(text);
        } else {
            cout(text);
        }
    }

    if (!errorReport.isOk()) {
        cerr(juce::translate("Errors found, stopping."));
        return false;
    }
    if (treatWarningAsError && (errorReport.getWarningCount() > 0)) {
        cerr(juce::translate("Warnings found and are considered errors, stopping."));
        return false;
    }

    return true;
}

std::unique_ptr<SourceGeneratorConfiguration> CommandLineToolHelper::getSourceConfigurationOrDisplayErr() const noexcept
{
    // If source profile and custom, error.
    if (parameterSourceProfile->isPresent() && (parameterCustomSourceProfile->isPresent())) {
        cerr(juce::translate("You must not declare both source profile and a custom one."));
        return nullptr;
    }

    // Is the source profile, if any, valid?
    if (parameterSourceProfile->isPresent()) {
        const auto profile = parameterSourceProfile->getValueAsString();
        if (profileZ80String == profile) {
            return std::make_unique<SourceGeneratorConfiguration>(SourceGeneratorConfiguration::buildZ80());
        }
        if (profile68000String == profile) {
            return std::make_unique<SourceGeneratorConfiguration>(SourceGeneratorConfiguration::build68000());
        }
        if (profile6502AcmeString == profile) {
            return std::make_unique<SourceGeneratorConfiguration>(SourceGeneratorConfiguration::build6502Acme());
        }
        if (profile6502MadsString == profile) {
            return std::make_unique<SourceGeneratorConfiguration>(SourceGeneratorConfiguration::build6502Mads());
        }
        cerr(juce::translate("Unknown source profile: " + profile + "."));
        return nullptr;
    }

    // Reads the possible custom source profile.
    if (parameterCustomSourceProfile->isPresent()) {
        return readSourceProfileFromFileOrDisplayErr(parameterCustomSourceProfile->getValueAsString());
    }

    // Defaults to Z80.
    return std::make_unique<SourceGeneratorConfiguration>(SourceGeneratorConfiguration::buildZ80());
}

std::unique_ptr<SourceGeneratorConfiguration> CommandLineToolHelper::readSourceProfileFromFileOrDisplayErr(const juce::String& inputFilePath) noexcept
{
    const auto inputFile = FileUtil::getFileFromString(inputFilePath);
    if (!inputFile.existsAsFile()) {
        cerr(juce::translate("The input file does not exist."));
        return nullptr;
    }

    // Deserializes the file.
    juce::XmlDocument document(inputFile);
    auto sourceProfileNode = document.getDocumentElement();
    if (sourceProfileNode == nullptr) {
        cerr(juce::translate("The profile format is invalid."));
        return nullptr;
    }

    const auto sourceProfile = SourceProfileSerializer::deserializeOne(*sourceProfileNode);

    // Checks it.
    const auto validationResult = SourceProfileValidator::validate(sourceProfile);
    if (validationResult.empty()) {
        return std::make_unique<SourceGeneratorConfiguration>(sourceProfile.getSourceGeneratorConfiguration());
    }

    // Errors.
    cerr(juce::translate("Errors were found in the source profile:"));
    for (const auto& error : validationResult) {
        // Gets a displayable location and error.
        const auto locationName = SourceProfileValidator::locationToDisplayableLocation(error.first);
        const auto errorMessage = SourceProfileValidator::errorToDisplayableError(error.second);
        cerr(juce::translate("In ") + locationName + juce::translate(": ") + errorMessage);
    }

    return nullptr;
}

}   // namespace arkostracker
