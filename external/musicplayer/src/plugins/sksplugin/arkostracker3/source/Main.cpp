#include "ui/mainView/MainDocumentWindow.h"

#include "../../ProjectInfo.h"          // Generated file in the build folder.
#include "app/preferences/PreferencesManager.h"
#include "commandLineTools/utils/CommandLineParser.h"
#include "commandLineTools/utils/CommandLineToolHelper.h"
#include "controllers/SongControllerNoOp.h"
#include "ui/editorWithBars/view/BarEditorCommands.h"
#include "ui/linker/LinkerPanelAreaCommands.h"
#include "ui/linker/controller/LinkerControllerNoOp.h"
#include "ui/lists/ListCommands.h"
#include "ui/lists/OperationValidityProvider.h"
#include "ui/patternViewer/controller/PatternViewerControllerNoOp.h"
#include "ui/patternViewer/view/PatternViewerCommands.h"
#include "ui/testArea/view/TestAreaCommands.h"
#include "utils/MainControllerNoOp.h"

namespace arkostracker 
{

/** Startup class for Arkos Tracker 3. */
class ArkosTracker3Application final : public juce::JUCEApplication
{
public:
    ArkosTracker3Application() :
        mainWindow()
    {
    }

    const juce::String getApplicationName() override // NOLINT(readability-const-return-type)
    {
        return projectinfo::applicationName;
    }

    const juce::String getApplicationVersion() override // NOLINT(readability-const-return-type)
    {
        return projectinfo::applicationVersion;
    }

    bool moreThanOneInstanceAllowed() override
    {
        return true;
    }

    // ==============================================================================
    void initialise(const juce::String& /*commandLine*/) override
    {
        juce::SystemStats::setApplicationCrashHandler(crashHandler);

        // Reads the command line, possibly loading an input song.
        auto [mustContinue, songToLoad] = manageCommandLine();

        if (!mustContinue) {
            systemRequestedQuit();
        } else {
            // Creates and show the main window.
            mainWindow = std::make_unique<MainDocumentWindow>(getApplicationName());
            mainWindow->showMainWindow(createMainApplicationCommandManager(), std::move(songToLoad));
        }
    }

    void shutdown() override
    {
        // Add your application's shutdown code here.
    }

    // ==============================================================================
    void systemRequestedQuit() override
    {
        // This is called when the app is being asked to quit: you can ignore this
        // request and let the app carry on running, or call quit() to allow the app to close.
        quit();
    }

    void anotherInstanceStarted (const juce::String& /*commandLine*/) override
    {
        // When another instance of the app is launched while this one is running,
        // this method is invoked, and the commandLine parameter tells you what
        // the other instance's command-line arguments were.
    }

private:
    /**
     * Manages the command line which line is given.
     * @return the possible song to load, or nullptr in any other case, and a boolean indicating if the app should continue (true) or stop (false).
     */
    static std::pair<bool, std::unique_ptr<Song>> manageCommandLine()
    {
        const auto commandLineStringArray = getCommandLineParameterArray();

        // Starts parsing.
        std::vector<CommandLineArgumentDescriptor*> descriptors;
        CommandLineToolHelper commandLineToolHelper;
        commandLineToolHelper.declareInputSongParameter(descriptors, false);

        CommandLineParser parser(commandLineStringArray, descriptors, "Arkos Tracker 3.\n"
                                     "Usage: ArkosTracker3 <path to song to load>");
        switch (parser.parse(false, true)) {
            case CommandLineParser::ParsingResult::parsingOk:
            case CommandLineParser::ParsingResult::parsingFailure:
                break;
            case CommandLineParser::ParsingResult::parsingHelp:
                return { false, nullptr };
        }
        const auto songPath = commandLineToolHelper.getInputSong();
        if (songPath.isEmpty()) {
            return { true, nullptr };       // Continue even if the song couldn't be opened.
        }

        auto song = CommandLineToolHelper::loadSongOrWriteError(songPath);
        return { true, std::move(song) };     // Continue even if the song couldn't be opened.
    }

    /** @return the ApplicationCommandManager to use. It should be set-up with ALL the commands of all the panels and main Component. */
    static std::unique_ptr<juce::ApplicationCommandManager> createMainApplicationCommandManager()
    {
        auto applicationCommandManager = std::make_unique<juce::ApplicationCommandManager>();

        const auto& preferences = PreferencesManager::getInstance();

        // Registers ALL the commands from the top-level generic commands, as well as all the panels.
        // This is necessary, else the Commands will be shown in the Setup only when the Panel is instantiated on first time!
        MainControllerNoOp noOpMainController;
        SongControllerNoOp noOpSongController;
        PatternViewerControllerNoOp noOpPatternController;
        LinkerControllerNoOp noOpLinkerController(noOpSongController);
        const OperationValidityProviderNoOp operationValidityProviderNoOp;

        MainApplicationCommands mainApplicationCommands(noOpMainController);
        TestAreaCommands testAreaCommands;
        ListCommands listCommands(operationValidityProviderNoOp);
        LinkerPanelAreaCommands linkerPanelAreaCommands(noOpLinkerController);
        PatternViewerCommands patternViewerCommands(noOpPatternController);
        BarEditorCommands barEditorCommands;
        VirtualKeyboardCommands virtualKeyboardCommands(preferences.getVirtualKeyboardLayout());
        applicationCommandManager->registerAllCommandsForTarget(&mainApplicationCommands);
        applicationCommandManager->registerAllCommandsForTarget(&testAreaCommands);
        applicationCommandManager->registerAllCommandsForTarget(&listCommands);
        applicationCommandManager->registerAllCommandsForTarget(&linkerPanelAreaCommands);
        applicationCommandManager->registerAllCommandsForTarget(&patternViewerCommands);
        applicationCommandManager->registerAllCommandsForTarget(&barEditorCommands);
        applicationCommandManager->registerAllCommandsForTarget(&virtualKeyboardCommands);

        // Is there an existing mapping? If yes, use it. It will merge with the existing commands.
        const auto xmlElement(preferences.getKeyMapping());
        if (xmlElement != nullptr) {
            applicationCommandManager->getKeyMappings()->restoreFromXml(*xmlElement);
        }

        return applicationCommandManager;
    }

    /** Called if a crash happened. */
    static void crashHandler(void* /*param*/) {
        std::cerr << juce::SystemStats::getStackBacktrace();

        // Creates the logger folder. On Windows, it will be thus in the same folder as the Properties because the name is the same.
        const auto& applicationName = PreferencesManager::getApplicationName();
        const auto crashFileBaseFolder(juce::FileLogger::getSystemLogFileFolder().getFullPathName() + juce::File::getSeparatorString() + applicationName);
        const juce::File crashFileFolder(crashFileBaseFolder);
        if (!crashFileFolder.exists()) {
            const auto& result = crashFileFolder.createDirectory();
            if (!result.wasOk()) {
                std::cerr << "Unable to create a file logger at " << crashFileBaseFolder;
                return;
            }
        }

#if JUCE_DEBUG
        std::cout << "Log file: " << crashFileFolder.getFullPathName() << '\n';
#endif

        // Creates the Logger file.
        const auto crashFileLogger = juce::FileLogger::createDefaultAppLogger(
                crashFileFolder.getFullPathName(), "crashlog.txt", "Please report this to the author of Arkos Tracker. He may be interested.");
        if (crashFileLogger == nullptr) {
            std::cerr << "Unable to create a file logger.";
            return;
        }

        crashFileLogger->logMessage("CPU model: " + juce::SystemStats::getCpuModel());
        crashFileLogger->logMessage("CPU vendor: " + juce::SystemStats::getCpuVendor());
        crashFileLogger->logMessage("Device: " + juce::SystemStats::getDeviceDescription());
        crashFileLogger->logMessage("OS: " + juce::SystemStats::getOperatingSystemName());
        crashFileLogger->logMessage("Physical CPU count: " + juce::String(juce::SystemStats::getNumPhysicalCpus()));
        crashFileLogger->logMessage("CPU count: " + juce::String(juce::SystemStats::getNumCpus()));
        crashFileLogger->logMessage("");
        crashFileLogger->logMessage(juce::SystemStats::getStackBacktrace());
    }

    std::unique_ptr<MainDocumentWindow> mainWindow{};                 // The main window of the application.
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (ArkosTracker3Application)                // NOLINT(*)

}   // namespace arkostracker
