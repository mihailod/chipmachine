#include "MainComponent.h"

#include "../../app/preferences/PreferencesManager.h"
#include "../../controllers/MainControllerImpl.h"
#include "../../utils/NumberUtil.h"
#include "../about/AboutDialog.h"
#include "../lookAndFeel/LookAndFeelFactory.h"
#include "../tools/errorChecker/ErrorCheckerDialog.h"
#include "../tools/instrumentOptimizer/InstrumentOptimizerDialog.h"
#include "../tools/streamedMusicAnalyzer/StreamedMusicAnalyzer.h"

namespace arkostracker 
{

const int MainComponent::menuBarWidth = 250;
const int MainComponent::menuBarHeight = 25;

MainComponent::MainComponent(std::unique_ptr<juce::ApplicationCommandManager> paramApplicationCommandManager, std::unique_ptr<Song> pSong) noexcept :
        mainController(std::make_unique<MainControllerImpl>(
            (pSong != nullptr) ? std::move(pSong) : buildSong(), *this, *this)),
        globalArrangementController(),
        menuBarModel(*this, *mainController),
        menuBar(&menuBarModel),
        mainToolbarController(mainController->getMainToolbarControllerInstance()),

        customLookAndFeel(std::make_unique<CustomLookAndFeel>()),       // Uses latest saved values.

        applicationCommandManager(std::move(paramApplicationCommandManager)),
        mainApplicationCommandTarget(*mainController, *this),

        setup(*mainController, *this),
        exporter(*mainController),
        songProperties(*mainController),
        subsongProperties(*mainController),
        splash(*this, *mainController),

        modalDialog(),
        loadInstrumentOperation()
{
    mainController->setCommandManager(*applicationCommandManager);               // Not fond of this circular-reference, but no choice...

    juce::LookAndFeel::setDefaultLookAndFeel(customLookAndFeel.get());

    // Sets up the menu bar.
    addAndMakeVisible(menuBar);
    mainToolbarController.onParentViewCreated(*this);

    // Creates the controllers.
    globalArrangementController = std::make_unique<GlobalArrangementController>(*mainController, *this, menuBarHeight);
    mainController->setPanelSearcher(globalArrangementController.get());        // Same lifetime, no need to unregister it.

    // This top-component will send any key pressed to the KeyPressMappingSet, which will send it to the ALREADY registered component.
    addKeyListener(applicationCommandManager->getKeyMappings());        // See https://docs.juce.com/master/classKeyPressMappingSet.html#details.

    // Sets the "last selected folders" from the Preferences to a convenient singleton.
    const auto& preferencesManager = PreferencesManager::getInstance();
    preferencesManager.readStoredFoldersAndFillLastFolderStorage();

    // Registers the Player Controller to the midi events. Same lifecycle, no need to unregister them.
    auto& playerController = mainController->getPlayerController();
    mainController->getAudioController().getMidiController().getMidiObservers().addObserver(&playerController);

    // Observes the song metadata for the title bar text.
    mainController->getSongController().getSongMetadataObservers().addObserver(this);

    // Shows the splash, if needed.
    splash.onSplashClosed = [&] { focusOnInitialPanelAfterDelay(); };
#if JUCE_WINDOWS
    // Strange, on Windows, a small delay must be added, else the Splash is shown behind.
    juce::Timer::callAfterDelay(10, [&] {
#endif
        if (const auto splashShown = splash.showSplashIfPossible(); !splashShown) {
            // No splash: focuses on a panel. A bit hackish, but various callbacks didn't work, the view was never ready :(.
            focusOnInitialPanelAfterDelay();
        }
#if JUCE_WINDOWS
    });
#endif

    // Slight hack to select the first Instrument.
    const auto firstInstrumentId = mainController->getSongController().getSong()->getFirstRealInstrumentId();
    mainController->setSelectedInstrumentId(firstInstrumentId, false);
}

void MainComponent::resized()
{
    menuBar.setBounds(0, 0, menuBarWidth, menuBarHeight);

    const auto mainToolBarX = menuBar.getRight();
    const auto mainToolBarWidth = getWidth() - mainToolBarX;
    mainToolbarController.onParentViewResized(mainToolBarX, 0, mainToolBarWidth, menuBarHeight);
}

void MainComponent::tryToExitApplication() const noexcept
{
    mainController->tryToExitApplication();
}

void MainComponent::exitApplication()
{
    // NOTE: not everything is saved here (like the DocumentWindow size, which is handled by itself, as it has the data).

    // Saves the recent folders.
    auto& preferencesManager = PreferencesManager::getInstance();
    preferencesManager.saveRecentFolders();

    // Bye bye!
    juce::JUCEApplicationBase::quit();
}


// StorageListener method implementations.
// ===================================================

void MainComponent::onRestoreArrangement(const int arrangementNumber)
{
    globalArrangementController->onRestoreArrangement(arrangementNumber);
}

int MainComponent::getCurrentArrangementNumber() const
{
    // Called too early directly on construction of the controllers (MainToolbarController)...
    if (globalArrangementController == nullptr) {
        return 0;
    }
    return globalArrangementController->getCurrentArrangementNumber();
}


// ApplicationCommandTarget method implementations.
// ===================================================

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget()
{
    return mainApplicationCommandTarget.getNextCommandTarget();
}

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& commands)
{
    mainApplicationCommandTarget.getAllCommands(commands);
}

void MainComponent::getCommandInfo(const juce::CommandID commandID, juce::ApplicationCommandInfo& result)
{
    mainApplicationCommandTarget.getCommandInfo(commandID, result);
}

bool MainComponent::perform(const ApplicationCommandTarget::InvocationInfo& info)
{
    return mainApplicationCommandTarget.perform(info);
}


// FileDragAndDropTarget method implementations.
// ===================================================

bool MainComponent::isInterestedInFileDrag(const juce::StringArray& /*files*/)
{
    // By default, we receive all the files. Maybe several instruments, or one song only.
    return true;
}

void MainComponent::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (loadInstrumentOperation != nullptr) {
        jassertfalse;           // Operation hasn't been cleared?
        return;
    }

    // First, tries to load the files as Instruments. Creates a thread for that.
    loadInstrumentOperation = std::make_unique<BackgroundOperation<std::vector<std::unique_ptr<Instrument>>>>([&, files] (std::vector<std::unique_ptr<Instrument>> instruments) {
        onFilesDroppedInstrumentReceived(files, std::move(instruments));
    }, [files] {
        std::vector<std::unique_ptr<Instrument>> instruments;
        for (const auto& file : files) {
            // Tries to load as many instruments as possible.
            if (auto instrument = LoadInstrument::load(file); instrument != nullptr) {
                instruments.push_back(std::move(instrument));
            }
        }
        return instruments;
    });
    loadInstrumentOperation->performOperation();
}

void MainComponent::onFilesDroppedInstrumentReceived(const juce::StringArray& files, std::vector<std::unique_ptr<Instrument>> instruments) noexcept
{
    // Was it an instrument? If yes, adds it to the Song.
    if (!instruments.empty()) {
        auto& songController = mainController->getSongController();
        songController.insertInstruments(-1, std::move(instruments));
    } else if (files.size() == 1) {
        // No instruments loaded. If it's a Song, there must be only one file.
        const auto& fileName = files[0];
        const auto file = juce::File(fileName);

        // Tries to load as a Song.
        mainController->loadSong(fileName);
    }

    loadInstrumentOperation.reset();
}


// LookAndFeelChanger method implementations.
// ===================================================

void MainComponent::setLookAndFeel(int themeId)
{
    customLookAndFeel = std::make_unique<CustomLookAndFeel>(themeId);
    juce::LookAndFeel::setDefaultLookAndFeel(customLookAndFeel.get());
}

void MainComponent::notifyLookAndFeelChange()
{
    // Notifies the parent, it will encompass all the changes (change of color of its border or title, for example).
    // It *would* be better to have the MainDocumentWindow as a reference, but since we know it won't change...
    getParentComponent()->sendLookAndFeelChange();
}


// ===================================================

std::unique_ptr<Song> MainComponent::buildSong() noexcept
{
    return Song::buildDefaultSong(true);
}


// CommandPerformer method implementations.
// ===================================================

void MainComponent::openGeneralSetup() noexcept
{
    setup.openGeneralSetupDialog();
}

void MainComponent::openKeyboardMappingSetup() noexcept
{
    setup.openSetupKeyboardMappingDialog();
}

void MainComponent::openThemeEditor() noexcept
{
    setup.openThemeEditor();
}

void MainComponent::openPatternViewerSetup() noexcept
{
    setup.openPatternViewerSetup();
}

void MainComponent::openAudioInterfaceSetup() noexcept
{
    setup.openAudioInterfaceSetup();
}

void MainComponent::openOutputSetup() noexcept
{
    setup.openOutputSetup();
}

void MainComponent::openSourceProfileSetup() noexcept
{
    setup.openSourceProfileSetup();
}

void MainComponent::openSerialSetup() noexcept
{
    setup.openSerialSetup();
}

void MainComponent::openExportToAkg() noexcept
{
    exporter.openExportToAkg();
}

void MainComponent::openExportToAky() noexcept
{
    exporter.openExportToAky();
}

void MainComponent::openExportToAkm() noexcept
{
    exporter.openExportToAkm();
}

void MainComponent::openExportToFap() noexcept
{
    exporter.openExportToFap();
}

void MainComponent::openExportToMidi() noexcept
{
    exporter.openExportToMidi();
}

void MainComponent::openExportToMod() noexcept
{
    exporter.openExportToMod();
}

void MainComponent::openExportToText() noexcept
{
    exporter.openExportToText();
}

void MainComponent::openExportToYm() noexcept
{
    exporter.openExportToYm();
}

void MainComponent::openExportYmToFap() noexcept
{
    exporter.openExportYmToFap();
}

void MainComponent::openExportToVgm() noexcept
{
    exporter.openExportToVgm();
}

void MainComponent::openExportToWav() noexcept
{
    exporter.openExportToWav();
}

void MainComponent::openExportInstrumentToWav() noexcept
{
    exporter.openExportInstrumentToWav();
}

void MainComponent::openExportSfxs() noexcept
{
    exporter.openExportSfxs();
}

void MainComponent::openExportEvents() noexcept
{
    exporter.openExportEvents();
}

void MainComponent::openExportToRaw() noexcept
{
    exporter.openExportToRaw();
}

void MainComponent::openExportToRawLinear() noexcept
{
    exporter.openExportToRawLinear();
}

void MainComponent::openExportSamples() noexcept
{
    exporter.openExportSamples();
}

void MainComponent::openQuickExport() noexcept
{
    exporter.openQuickExport();
}

void MainComponent::openQuickExportToBasic() noexcept
{
    exporter.openQuickExportToBasic();
}

void MainComponent::openStreamedMusicAnalyzer() noexcept
{
    mainController->stopPlaying(false);

    jassert(modalDialog == nullptr);               // Already present?
    modalDialog = std::make_unique<StreamedMusicAnalyzer>(*mainController, [&] { closeDialog(); });
}

void MainComponent::openInstrumentOptimizer() noexcept
{
    mainController->stopPlaying(false);

    jassert(modalDialog == nullptr);               // Already present?
    modalDialog = std::make_unique<InstrumentOptimizerDialog>(mainController->getSongController(), [&] { closeDialog(); });
}

void MainComponent::openErrorChecker() noexcept
{
    jassert(modalDialog == nullptr);               // Already present?
    modalDialog = std::make_unique<ErrorCheckerDialog>(*mainController, [&] { closeDialog(); });
}

void MainComponent::rearrangePatterns(const bool deleteUnused) noexcept
{
    auto& songController = mainController->getSongController();
    songController.rearrangePatterns(songController.getCurrentSubsongId(), deleteUnused);
}

void MainComponent::clearPatterns() noexcept
{
    auto& songController = mainController->getSongController();
    songController.clearPatterns(songController.getCurrentSubsongId());
}

void MainComponent::openAboutPage() noexcept
{
    jassert(modalDialog == nullptr);               // Already present?
    modalDialog = std::make_unique<AboutDialog>([&] { closeDialog(); });
}

void MainComponent::openPanel(const PanelType panelType) noexcept
{
    const auto found = globalArrangementController->searchAndOpenPanel(panelType);
    jassert(found); (void)found;     // Not found in this workspace. TO IMPROVE try to find it in another one? May be dangerous because of linking.
}

void MainComponent::loadSong() noexcept
{
    mainController->loadSong(juce::String());
}

void MainComponent::mergeSong() noexcept
{
    mainController->mergeSong();
}

void MainComponent::clearRecentFiles() noexcept
{
    mainController->clearRecentFiles();
}

void MainComponent::newSong() noexcept
{
    mainController->newSong();
}

void MainComponent::saveSong() noexcept
{
    mainController->saveSong(false);
}

void MainComponent::saveSongAs() noexcept
{
    mainController->saveSong(true);
}

void MainComponent::saveCopy() noexcept
{
    mainController->saveCopy();
}

void MainComponent::loadInstrument() noexcept
{
    mainController->loadInstrument();
}

void MainComponent::editSongProperties() noexcept
{
    songProperties.openSongProperties();
}

void MainComponent::openEditSubsongPanel(const Id& subsongId) noexcept
{
    subsongProperties.openSubsongProperties(subsongId);
}

void MainComponent::openAddNewSubsongPanel() noexcept
{
    subsongProperties.openAddNewSubsongPanel();
}


// SongMetadataObserver method implementations.
// ===================================================

void MainComponent::onSongMetadataChanged(const unsigned int what)
{
    // Only interested in save state, path and title change.
    if (!NumberUtil::isBitPresent(what, SongMetadataObserver::What::savedPath | SongMetadataObserver::What::name | SongMetadataObserver::What::modified)) {
        return;
    }

    setTitleFromSongData();
}


// ===================================================

void MainComponent::setTitleFromSongData() const noexcept
{
    // Writes "Arkos Tracker X - <Song name>[*] - <path>"
    const auto applicationName = juce::JUCEApplication::getInstance()->getApplicationName();
    const auto originalSongName = mainController->getSongController().getSong()->getTitle();
    const auto path = mainController->getSongPath();

    const auto isModified = mainController->isSongModified();
    const auto modificationFlag = isModified ? "*" : juce::String();

    // Uses a default song name if none is provided.
    const auto songName = originalSongName.isEmpty() ? juce::translate("Untitled") : originalSongName;

    // Builds the final title.
    auto title = applicationName + " - " + songName + modificationFlag;
    if (path.isNotEmpty()) {
        title += " - " + path;
    }

    // Changes the top level text.
    auto* document = getTopLevelComponent();
    jassert(document != nullptr);
    if (document != nullptr) {
        document->setName(title);
    }
}

void MainComponent::switchToSubsong(const Id& subsongId) noexcept
{
    mainController->switchToSubsong(subsongId);
}

void MainComponent::closeDialog() noexcept
{
    modalDialog.reset();
}

void MainComponent::focusOnInitialPanelAfterDelay() const noexcept
{
    juce::Timer::callAfterDelay(400, [&] {
        mainController->searchAndOpenPanel(PanelType::patternViewer);
    });
}


// BoundedComponent method implementations.
// ===================================================

int MainComponent::getXInsideComponent() const noexcept
{
    return 0;
}

int MainComponent::getYInsideComponent() const noexcept
{
    return 0;
}

int MainComponent::getAvailableWidthInComponent() const noexcept
{
    return getWidth();
}

int MainComponent::getAvailableHeightInComponent() const noexcept
{
    return getHeight();
}

}   // namespace arkostracker
