#include "ToolZ80Profiler.h"

#include <juce_events/juce_events.h>

#include "../../utils/FileUtil.h"
#include "../../utils/MemoryBlockUtil.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/OptionalValue.h"
#include "../../utils/StreamUtil.h"
#include "../utils/CommandLineArgumentDescriptor.h"
#include "../utils/CommandLineParser.h"
#include "Z80SimpleEmulator.h"
#include "../../import/common/AmsdosHeaderUtil.h"

namespace arkostracker
{

/**
 * Example program:
    org #4000

    ;Hooks.
    jp Init     ;Start +0.
    jp Exec     ;Start +3.

Init
    ld hl,Music_Start
    call PLY_AKY_Init
    jp #ffff    ;Go back to the profiler.

Exec
    call PLY_AKY_Play
    jp #ffff    ;Go back to the profiler.

    include "resources/MusicBoulesEtBits_playerconfig.asm"
Music_Start
    include "resources/MusicBoulesEtBits.asm"

    include "PlayerAky.asm"



    And to test: /Z80Profiler input.bin out.txt
 */

int ToolZ80Profiler::initNops = 0;

int ToolZ80Profiler::execute(const int argc, char* argv[])      // NOLINT(*-avoid-c-arrays,clion-misra-cpp2008-3-1-3)
{
    constexpr auto maximumNopsDefault = 9999999;
    constexpr auto execCallsDefault = 10000;
    constexpr auto loadAddressDefault = 0x4000;
    constexpr auto initOffsetAddressDefault = 0;
    constexpr auto execOffsetAddressDefaultIfInitPresent = 3;
    constexpr auto execOffsetAddressDefaultIfInitAbsent = 0;
    constexpr auto stopAddressDefault = 0xffff;
    constexpr auto spAddressDefault = 0xffff;
    constexpr auto execOverheadNopsDefault = 0;
    constexpr unsigned char initADefault = 0;

    constexpr auto callOverheadInit = 7;        // 3 for Hook JP, 3 for the jp endAddress, 1 for the last NOP breakpoint.
    constexpr auto callOverheadExec = 7;        // 3 for Hook JP, 3 for the jp endAddress, 1 for the last NOP breakpoint.

    const auto guiInit = juce::ScopedJuceInitialiser_GUI { };       // To remove JUCE assertions being not initialized.

    // Creates the command line.
    const juce::String description = "Z80 Profiler runs a Z80 code and tells you how many cycles it took to complete.\nUsage: Z80Profiler "
                                     "<path to input Z80 binary file> <path to output report>";
    std::vector<CommandLineArgumentDescriptor*> descriptors;

    const auto descriptorParameterInput = std::make_unique<CommandLineArgumentDescriptor>(
        CommandLineArgumentDescriptor::buildArgumentWithDirectValue(juce::translate("<path to input Z80 binary file>"),
                                                                    juce::translate("Path and filename to the Z80 binary file to load."
                                                                                    " AMSDOS header is skipped, if present."),
                                                                    true));
    descriptors.push_back(descriptorParameterInput.get());

    const auto descriptorParameterOutput = std::make_unique<CommandLineArgumentDescriptor>(
        CommandLineArgumentDescriptor::buildArgumentWithDirectValue(juce::translate("<path to output CSV file>"),
                                                                    juce::translate("If present, path and filename to generate a report."),
                                                                    false));
    descriptors.push_back(descriptorParameterOutput.get());

    const auto optionLoadAddress = std::make_unique<Option>(Option::buildOption("l", "loadAddress"));
    const auto parameterLoadAddress = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorLoadAddress = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, sets where to load the binary in memory. " + juce::String(loadAddressDefault) + " is default."),
        *optionLoadAddress, false, *parameterLoadAddress));
    descriptors.push_back(descriptorLoadAddress.get());

    const auto optionInitOffsetAddress = std::make_unique<Option>(Option::buildOption("i", "initOffsetAddress"));
    const auto parameterInitOffsetAddress = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorInitOffsetAddress = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, sets where to call the init code from the load address. " + juce::String(initOffsetAddressDefault)
            + " is default. Set to -1 to skip init."),
        *optionInitOffsetAddress, false, *parameterInitOffsetAddress));
    descriptors.push_back(descriptorInitOffsetAddress.get());

    const auto optionExecOffsetAddress = std::make_unique<Option>(Option::buildOption("x", "execOffsetAddress"));
    const auto parameterExecOffsetAddress = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorExecOffsetAddress = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, sets where to call the exec code from the load address. "
            + juce::String(execOffsetAddressDefaultIfInitPresent) + " is default if init is present, else " + juce::String(execOffsetAddressDefaultIfInitAbsent) + "."),
        *optionExecOffsetAddress, false, *parameterExecOffsetAddress));
    descriptors.push_back(descriptorExecOffsetAddress.get());

    const auto optionStopAddress = std::make_unique<Option>(Option::buildOption("s", "stopAddress"));
    const auto parameterStopAddress = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorStopAddress = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, sets what address to jump to when your init/exec code have finished their job. "
            + displayHexNumber(stopAddressDefault) + " is default."),
        *optionStopAddress, false, *parameterStopAddress));
    descriptors.push_back(descriptorStopAddress.get());

    const auto optionSpAddress = std::make_unique<Option>(Option::buildLongOption("spAddress"));
    const auto parameterSpAddress = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorSpAddress = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, sets what the stack pointer (SP), before init and each exec. "
            + displayHexNumber(spAddressDefault) + " is default."),
        *optionSpAddress, false, *parameterSpAddress));
    descriptors.push_back(descriptorSpAddress.get());

    const auto optionMaximumNops = std::make_unique<Option>(Option::buildOption("n", "maximumNops"));
    const auto parameterMaximumNops = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorMaximumNops = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, indicates how long the init/exec can last, as a security. " + juce::String(maximumNopsDefault) + " is default."),
        *optionMaximumNops, false, *parameterMaximumNops));
    descriptors.push_back(descriptorMaximumNops.get());

    const auto optionExecCalls = std::make_unique<Option>(Option::buildOption("c", "execCalls"));
    const auto parameterExecCalls = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorExecCalls = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, indicates how many times the exec is called. " + juce::String(execCallsDefault) + " is default."),
        *optionExecCalls, false, *parameterExecCalls));
    descriptors.push_back(descriptorExecCalls.get());

    const auto optionInitA = std::make_unique<Option>(Option::buildOption("a", "initValueInA"));
    const auto parameterInitA = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorInitA = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, passed to the init code in the A register. Useful to indicate a subsong for example. " + juce::String(initADefault) + " is default."),
        *optionInitA, false, *parameterInitA));
    descriptors.push_back(descriptorInitA.get());

    const auto optionExecOverheadNops = std::make_unique<Option>(Option::buildOption("o", "execOverheadNops"));
    const auto parameterExecOverheadNops = std::make_unique<Parameter>(ParameterType::integer);
    const auto descriptorExecOverheadNops = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, decreases the output NOP count of each exec frame by this value. Useful if you add operations to call a subroutine for example. " + juce::String(execOverheadNopsDefault) + " is default."),
        *optionExecOverheadNops, false, *parameterExecOverheadNops));
    descriptors.push_back(descriptorExecOverheadNops.get());

    const auto optionRemoveC9FB = std::make_unique<Option>(Option::buildOption("r", "removeC9FB"));
    const auto descriptorRemoveC9FB = std::make_unique<CommandLineArgumentDescriptor>(CommandLineArgumentDescriptor::buildArgumentWithOption(
        juce::translate("If present, 0xC9FB (di : ret) will not be written in 0x38."),
        *optionRemoveC9FB, false));
    descriptors.push_back(descriptorRemoveC9FB.get());

    CommandLineParser parser(argc, argv, descriptors, description);
    const auto parseResult = parser.parse();
    if (parseResult == CommandLineParser::ParsingResult::parsingHelp) {
        return 0;               // Stops, no parameters.
    }
    if (parseResult == CommandLineParser::ParsingResult::parsingFailure) {
        return -1;              // Stops if parsing failed.
    }

    OptionalValue<juce::uint16> initOffsetAddress = initOffsetAddressDefault;
    if (parameterInitOffsetAddress->isPresent()) {
        const auto initOffsetAddressRaw = parameterInitOffsetAddress->getValueAsInteger();
        if (initOffsetAddressRaw < 0) {
            initOffsetAddress = { };
        } else {
            initOffsetAddress = static_cast<juce::uint16>(initOffsetAddressRaw);
        }
    }

    juce::uint16 execOffsetAddress = initOffsetAddress.isPresent() ? execOffsetAddressDefaultIfInitPresent : execOffsetAddressDefaultIfInitAbsent;
    if (parameterExecOffsetAddress->isPresent()) {
        execOffsetAddress = static_cast<juce::uint16>(parameterExecOffsetAddress->getValueAsInteger());
    }

    juce::uint16 loadAddress = loadAddressDefault;
    if (parameterLoadAddress->isPresent()) {
        loadAddress = static_cast<juce::uint16>(parameterLoadAddress->getValueAsInteger());
    }

    juce::uint16 stopAddress = stopAddressDefault;
    if (parameterStopAddress->isPresent()) {
        stopAddress = static_cast<juce::uint16>(parameterStopAddress->getValueAsInteger());
    }

    juce::uint16 spAddress = spAddressDefault;
    if (parameterSpAddress->isPresent()) {
        spAddress = static_cast<juce::uint16>(parameterSpAddress->getValueAsInteger());
    }

    auto maximumNops = maximumNopsDefault;
    if (parameterMaximumNops->isPresent()) {
        maximumNops = parameterMaximumNops->getValueAsInteger();
    }
    if (constexpr auto maximumNopsMinimum = 1; maximumNops < maximumNopsMinimum) {
        cerr("The maximum NOPs must be at least " + juce::String(maximumNopsMinimum) + ".");
        return -1;
    }

    auto execCalls = execCallsDefault;
    if (parameterExecCalls->isPresent()) {
        execCalls = parameterExecCalls->getValueAsInteger();
    }
    if (execCalls <= 0) {
        cerr("The execution call count must be at least 1!");
        return -1;
    }

    auto initA = initADefault;
    if (parameterInitA->isPresent()) {
        initA = static_cast<unsigned char>(parameterInitA->getValueAsInteger());
    }

    auto execOverheadNops = execOverheadNopsDefault;
    if (parameterExecOverheadNops->isPresent()) {
        execOverheadNops = parameterExecOverheadNops->getValueAsInteger();
    }

    auto putC9FB = !descriptorRemoveC9FB->isPresent();

    // Loads the input file.
    const auto inputFilePath = descriptorParameterInput->getDirectValue();
    const auto inputFile = FileUtil::getFileFromString(inputFilePath);
    if (!inputFile.existsAsFile()) {
        cerr("Input file is not found.");
        return -1;
    }
    const auto originalMemoryBlock = MemoryBlockUtil::fromFile(inputFile);
    if (originalMemoryBlock == nullptr) {
        cerr("Input file could not be loaded.");
        return -1;
    }
    // Removes the possible Amsdos header.
    const auto dataMemoryBlock = AmsdosHeaderUtil::removeHeaderIfNeeded(*originalMemoryBlock);

    Z80SimpleEmulator emulator;
    emulator.injectData(dataMemoryBlock, loadAddress);
    if (putC9FB) {
        constexpr auto rstAddress = 0x38;
        emulator.writeData(rstAddress, 0xfb);         // DI.
        emulator.writeData(rstAddress + 1, 0xc9);     // RET.
    }

    cout("Loaded the file in " + displayHexNumber(loadAddress));
    cout("Interruptions are disabled at init/every exec.");
    if (putC9FB) {
        cout("0xC9FB is put in 0x38.");
    } else {
        cout("0xC9FB is not put in 0x38.");
    }
    cout("");

    // Calls the init.
    if (initOffsetAddress.isPresent()) {
        const auto initAddress = static_cast<juce::uint16>(loadAddress + initOffsetAddress.getValue());
        cout("Calling the initialization code from address " + displayHexNumber(initAddress) + "...");
        // Sets A.
        emulator.z80.reg.pair.A = initA;

        const auto nopCount = std::max(0, emulator.runTill(initAddress, stopAddress, spAddress, maximumNops) - callOverheadInit);
        if (nopCount < 0) {
            return -1;
        }
        cout("Init code NOPs: " + juce::String(nopCount) + "\n");

        initNops = nopCount;
    }

    // Execs.
    const auto execAddress = static_cast<juce::uint16>(loadAddress + execOffsetAddress);
    auto minimumExecNops = 999999;
    auto maximumExecNops = 0;
    cout("Calling the exec code from address " + displayHexNumber(execAddress) + ", " + juce::String(execCalls) +" times...");
    std::vector<int> nopCounts;
    for (auto frameIndex = 0; frameIndex < execCalls; ++frameIndex) {
        const auto nopCount = std::max(0, emulator.runTill(execAddress, stopAddress, spAddress, maximumNops) - callOverheadExec - execOverheadNops);
        if (nopCount < 0) {
            return -1;
        }
        nopCounts.emplace_back(nopCount);

        if (nopCount < minimumExecNops) {
            minimumExecNops = nopCount;
        }
        if (nopCount > maximumExecNops) {
            maximumExecNops = nopCount;
        }

        if (frameIndex % 1000 == 0) {
            cout("Exec code iteration " + juce::String(frameIndex) + ". NOPs: " + juce::String(nopCount));
        }
    }
    cout("");

    // Calculates the average.
    auto averageExecNops = 0;
    auto totalExecNops = 0;
    for (const auto nopCount : nopCounts) {
        totalExecNops += nopCount;
    }
    averageExecNops = totalExecNops / execCalls;

    // Generates the report.
    if (descriptorParameterOutput->isPresent()) {
        cout(juce::translate("Generating report."));
        const auto outputFileString = descriptorParameterOutput->getDirectValue();
        const auto outputFile = FileUtil::getFileFromString(outputFileString);
        (void)outputFile.deleteFile();
        juce::FileOutputStream reportOutputStream(outputFile);

        StreamUtil::write(reportOutputStream, "Execution index,nop count");
        auto frameIndex = 0;
        for (const auto nopCount : nopCounts) {
            StreamUtil::write(reportOutputStream, juce::String(frameIndex) + "," + juce::String(nopCount));
            ++frameIndex;
        }

        cout("");
    }

    cout(juce::translate("Profiling done!"));
    cout(juce::translate("Minimum NOPs: " + juce::String(minimumExecNops)));
    cout(juce::translate("Maximum NOPs: " + juce::String(maximumExecNops)));
    cout(juce::translate("Average NOPs: " + juce::String(averageExecNops)));

    return 0;
}

int ToolZ80Profiler::getInitNops() noexcept
{
    return initNops;
}

void ToolZ80Profiler::cout(const juce::String& text) noexcept
{
    std::cout << text << '\n';
}

void ToolZ80Profiler::cerr(const juce::String& text) noexcept
{
    std::cerr << text << '\n';
}

juce::String ToolZ80Profiler::displayHexNumber(const int number) noexcept
{
    return NumberUtil::signedHexToStringWithPrefix(number, "0x", false, false);
}

}   // namespace arkostracker
