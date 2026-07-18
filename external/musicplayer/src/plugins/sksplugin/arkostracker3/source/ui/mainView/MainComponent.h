#pragma once

#include "../../controllers/MainController.h"
#include "../../controllers/observers/SongMetadataObserver.h"
#include "../components/LookAndFeelChanger.h"
#include "../containerArranger/GlobalArrangementController.h"
#include "../export/Export.h"
#include "../keyboard/MainApplicationCommandTarget.h"
#include "../lookAndFeel/CustomLookAndFeel.h"
#include "../setup/Setup.h"
#include "../songProperties/SongProperties.h"
#include "../splash/Splash.h"
#include "../subsongProperties/SubsongProperties.h"
#include "../toolbar/app/AppMenuBarModel.h"
#include "../utils/backgroundOperation/BackgroundOperation.h"
#include "CommandPerformer.h"

namespace arkostracker 
{

class MainToolbarController;

/** Class that knows how to exit the application. */
class Exiter
{
public:
    /** Constructor. */
    virtual ~Exiter() = default;

    /** Call this to exit the application. No prompt is performed, but some storing are done (layout, preferences, etc.). */
    virtual void exitApplication() = 0;
};

/** Holds all the Views of the program. */
class MainComponent final : public BoundedComponent,
                            public CommandPerformer,
                            public Exiter,
                            public StorageListener,                   // To receive some orders from objects wanting to restore a layout (the Main Toolbar Panel for example).
                            public juce::ApplicationCommandTarget,    // To gets the calls from the UI hierarchy, but will retransmit them to our specific object.
                            public LookAndFeelChanger,
                            public SongMetadataObserver,              // To be aware of the save/name change and show that in the title.
                            public juce::FileDragAndDropTarget        // To detect drag'n'drop of external files.
{
public:
    /**
     * Constructor.
     * @param applicationCommandManager the ApplicationCommandManager to use. It should be set-up with ALL the commands of all the panels and main Component.
     * @param song a possible loaded song, or null to use a default one.
     */
    MainComponent(std::unique_ptr<juce::ApplicationCommandManager> applicationCommandManager, std::unique_ptr<Song> song) noexcept;

    /** Called to exit the application, but the save state is checked to prompt the user if needed. */
    void tryToExitApplication() const noexcept;

    // Exiter method implementations.
    // =====================================
    void exitApplication() override;

    // Component method implementations.
    // =====================================
    void resized() override;

    // StorageListener method implementations.
    // ===================================================
    int getCurrentArrangementNumber() const override;
    void onRestoreArrangement(int arrangementNumber) override;

    // ApplicationCommandTarget method implementations.
    // ===================================================
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;

    // FileDragAndDropTarget method implementations.
    // ===================================================
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // LookAndFeelChanger method implementations.
    // ===================================================
    void setLookAndFeel(int themeId) override;
    void notifyLookAndFeelChange() override;

    // CommandPerformer method implementations.
    // ===================================================
    void openGeneralSetup() noexcept override;
    void openKeyboardMappingSetup() noexcept override;
    void openThemeEditor() noexcept override;
    void openPatternViewerSetup() noexcept override;
    void openAudioInterfaceSetup() noexcept override;
    void openOutputSetup() noexcept override;
    void openSourceProfileSetup() noexcept override;
    void openSerialSetup() noexcept override;
    void openExportToAkg() noexcept override;
    void openExportToAky() noexcept override;
    void openExportToAkm() noexcept override;
    void openExportToFap() noexcept override;
    void openExportToMidi() noexcept override;
    void openExportToMod() noexcept override;
    void openExportToText() noexcept override;
    void openExportToYm() noexcept override;
    void openExportYmToFap() noexcept override;
    void openExportToVgm() noexcept override;
    void openExportToWav() noexcept override;
    void openExportInstrumentToWav() noexcept override;
    void openExportSfxs() noexcept override;
    void openExportEvents() noexcept override;
    void openExportToRaw() noexcept override;
    void openExportToRawLinear() noexcept override;
    void openExportSamples() noexcept override;

    void openQuickExport() noexcept override;
    void openQuickExportToBasic() noexcept override;

    void openStreamedMusicAnalyzer() noexcept override;
    void openInstrumentOptimizer() noexcept override;
    void openErrorChecker() noexcept override;
    void rearrangePatterns(bool deleteUnused) noexcept override;
    void clearPatterns() noexcept override;
    void openAboutPage() noexcept override;

    void openPanel(PanelType panelType) noexcept override;
    void loadSong() noexcept override;
    void mergeSong() noexcept override;
    void newSong() noexcept override;
    void saveSong() noexcept override;
    void saveSongAs() noexcept override;
    void saveCopy() noexcept override;
    void clearRecentFiles() noexcept override;
    void loadInstrument() noexcept override;
    void editSongProperties() noexcept override;
    void openEditSubsongPanel(const Id& subsongId) noexcept override;
    void openAddNewSubsongPanel() noexcept override;
    void switchToSubsong(const Id& subsongId) noexcept override;

    // SongMetadataObserver method implementations.
    // ===================================================
    void onSongMetadataChanged(unsigned int what) override;

    // BoundedComponent method implementations.
    // ===================================================
    int getXInsideComponent() const noexcept override;
    int getYInsideComponent() const noexcept override;
    int getAvailableWidthInComponent() const noexcept override;
    int getAvailableHeightInComponent() const noexcept override;

private:
    static const int menuBarWidth;                                                          // How wide is the menu at the top.
    static const int menuBarHeight;                                                         // How high is the menu at the top.

    /** @return a default song to load. */
    static std::unique_ptr<Song> buildSong() noexcept;

    /** Sets the title from the song data. */
    void setTitleFromSongData() const noexcept;
    /** Closes the dialog. */
    void closeDialog() noexcept;

    /** Focused on a panel. Useful on app init, as none is focused at first. A slight delay is used, else it doesn't work :(. */
    void focusOnInitialPanelAfterDelay() const noexcept;

    /**
     * Called after a drag'n'drop has been performed, tried to load Instruments on a worker thread.
     * This is called on a UI thread.
     * @param files the files that we intended to load.
     * @param instruments the Instruments, or empty if none could be loaded.
     */
    void onFilesDroppedInstrumentReceived(const juce::StringArray& files, std::vector<std::unique_ptr<Instrument>> instruments) noexcept;

    std::unique_ptr<MainController> mainController;                                     // The one and only MainController.
    std::unique_ptr<GlobalArrangementController> globalArrangementController;           // Manages the layout of the UI.
    AppMenuBarModel menuBarModel;                                                       // Populates the menu bar.
    juce::MenuBarComponent menuBar;                                                     // The menu bar at the top.
    MainToolbarController& mainToolbarController;                                       // Shows the Play buttons, etc.

    std::unique_ptr<CustomLookAndFeel> customLookAndFeel;                               // Our look and feel.

    std::unique_ptr<juce::ApplicationCommandManager> applicationCommandManager;         // Manages the general commands.
    MainApplicationCommandTarget mainApplicationCommandTarget;                          // Holds all the commands.

    Setup setup;
    Export exporter;
    SongProperties songProperties;
    SubsongProperties subsongProperties;
    Splash splash;

    std::unique_ptr<ModalDialog> modalDialog;

    std::unique_ptr<BackgroundOperation<std::vector<std::unique_ptr<Instrument>>>> loadInstrumentOperation;
};

}   // namespace arkostracker