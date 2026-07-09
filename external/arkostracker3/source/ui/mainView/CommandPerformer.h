#pragma once

#include "../containerArranger/PanelType.h"

namespace arkostracker 
{

/** Abstract class to perform commands (from the menus for example). */
class CommandPerformer
{
public:
    /** Destructor. */
    virtual ~CommandPerformer() = default;

    /** Opens the General setup. */
    virtual void openGeneralSetup() noexcept = 0;
    /** Opens the setup for keyboard mapping. */
    virtual void openKeyboardMappingSetup() noexcept = 0;
    /** Opens the Theme editor. */
    virtual void openThemeEditor() noexcept = 0;
    /** Opens the Pattern Viewer setup. */
    virtual void openPatternViewerSetup() noexcept = 0;
    /** Opens the Audio Interface setup. */
    virtual void openAudioInterfaceSetup() noexcept = 0;
    /** Opens the Output (mix) setup. */
    virtual void openOutputSetup() noexcept = 0;
    /** Opens the Source Profile setup. */
    virtual void openSourceProfileSetup() noexcept = 0;
    /** Opens the setup for the Serial. */
    virtual void openSerialSetup() noexcept = 0;

    /** Opens the Export to AKG dialog. */
    virtual void openExportToAkg() noexcept = 0;
    /** Opens the Export to AKY dialog. */
    virtual void openExportToAky() noexcept = 0;
    /** Opens the Export to AKM dialog. */
    virtual void openExportToAkm() noexcept = 0;
    /** Opens the Export to FAP dialog. */
    virtual void openExportToFap() noexcept = 0;
    /** Opens the Export to MIDI dialog. */
    virtual void openExportToMidi() noexcept = 0;
    /** Opens the Export to MOD dialog. */
    virtual void openExportToMod() noexcept = 0;
    /** Opens the Export to text dialog. */
    virtual void openExportToText() noexcept = 0;
    /** Opens the Export to YM dialog. */
    virtual void openExportToYm() noexcept = 0;
    /** Opens the Export YM to FAP dialog. */
    virtual void openExportYmToFap() noexcept = 0;
    /** Opens the Export to VGM dialog. */
    virtual void openExportToVgm() noexcept = 0;
    /** Opens the Export to WAV dialog. */
    virtual void openExportToWav() noexcept = 0;
    /** Opens the Export instrument to WAV dialog. */
    virtual void openExportInstrumentToWav() noexcept = 0;
    /** Opens the Export sfxs dialog. */
    virtual void openExportSfxs() noexcept = 0;
    /** Opens the Export events dialog. */
    virtual void openExportEvents() noexcept = 0;
    /** Opens the Export to RAW dialog. */
    virtual void openExportToRaw() noexcept = 0;
    /** Opens the Export to RAW Linear dialog. */
    virtual void openExportToRawLinear() noexcept = 0;
    /** Opens the Export samples dialog. */
    virtual void openExportSamples() noexcept = 0;

    /** Opens the Quick Export as a DSK or SNA. */
    virtual void openQuickExport() noexcept = 0;
    /** Opens the Quick Export to AKG Basic, in DSK. */
    virtual void openQuickExportToBasic() noexcept = 0;

    /** Opens the Streamed Music Analyzer. */
    virtual void openStreamedMusicAnalyzer() noexcept = 0;
    /** Opens the instrument optimizer. */
    virtual void openInstrumentOptimizer() noexcept = 0;
    /** Opens the error checker. */
    virtual void openErrorChecker() noexcept = 0;
    /** Rearranges the patterns. */
    virtual void rearrangePatterns(bool deleteUnused) noexcept = 0;
    /** Clears the patterns. */
    virtual void clearPatterns() noexcept = 0;

    /** Opens the About page. */
    virtual void openAboutPage() noexcept = 0;

    /**
     * Opens the panel which type is given. If not found, tries to open another workspace.
     * @param panelType the panel type.
     */
    virtual void openPanel(PanelType panelType) noexcept = 0;

    /** Loads a song, checking if the current one isn't modified, etc. */
    virtual void loadSong() noexcept = 0;
    /** Merges a song. This involves checking if the current one isn't modified, etc. as a new song is actually created. */
    virtual void mergeSong() noexcept = 0;
    /** Creates a new song, checking if the current one isn't modified, etc. */
    virtual void newSong() noexcept = 0;
    /** Saves the song, opening a picker if needed, etc. */
    virtual void saveSong() noexcept = 0;
    /** Saves the song, opening a picker, etc. */
    virtual void saveSongAs() noexcept = 0;
    /** Saves a copy of the song, if a path is present. */
    virtual void saveCopy() noexcept = 0;
    /** Clears the list of recent files. */
    virtual void clearRecentFiles() noexcept = 0;
    /** Loads an instrument. */
    virtual void loadInstrument() noexcept = 0;

    /** Opens the page to edit the Song properties. */
    virtual void editSongProperties() noexcept = 0;

    /**
     * Opens the panel to edit a Subsong.
     * @param subsongId the subsong id. It must be valid.
     */
    virtual void openEditSubsongPanel(const Id& subsongId) noexcept = 0;

    /** Opens the panel to create a new Subsong. */
    virtual void openAddNewSubsongPanel() noexcept = 0;

    /**
     * Selects a Subsong. This switches to it, which will notify all the UI.
     * @param subsongId the subsong id. It must be valid.
     */
    virtual void switchToSubsong(const Id& subsongId) noexcept = 0;
};

}   // namespace arkostracker

