#pragma once

#include <memory>

#include "../components/dialogs/ModalDialog.h"

namespace arkostracker 
{

class MainController;

/**
 * Manages the export pages.
 * We consider only one window can be opened at the same time.
 */
class Export
{
public:
    /**
     * Constructor.
     * @param mainController the Main Controller.
     */
    explicit Export(MainController& mainController) noexcept;

    /** Opens the Export to AKG page. */
    void openExportToAkg() noexcept;
    /** Opens the Export to AKY page. */
    void openExportToAky() noexcept;
    /** Opens the Export to AKM page. */
    void openExportToAkm() noexcept;
    /** Opens the Export to FAP page. */
    void openExportToFap() noexcept;
    /** Opens the Export to MIDI page. */
    void openExportToMidi() noexcept;
    /** Opens the Export to MOD page. */
    void openExportToMod() noexcept;
    /** Opens the Export to text page. */
    void openExportToText() noexcept;
    /** Opens the Export to YM page. */
    void openExportToYm() noexcept;
    /** Opens the Export YM to FAP page. */
    void openExportYmToFap() noexcept;
    /** Opens the Export to VGM page. */
    void openExportToVgm() noexcept;
    /** Opens the Export to WAV page. */
    void openExportToWav() noexcept;
    /** Opens the Export instrument to WAV page. */
    void openExportInstrumentToWav() noexcept;
    /** Opens the Export sfxs page. */
    void openExportSfxs() noexcept;
    /** Opens the Export events page. */
    void openExportEvents() noexcept;
    /** Opens the Export to RAW page. */
    void openExportToRaw() noexcept;
    /** Opens the Export to RAW Linear page. */
    void openExportToRawLinear() noexcept;
    /** Opens the Export samples page. */
    void openExportSamples() noexcept;

    /** Opens the Quick Export as DSK or SNA page. */
    void openQuickExport() noexcept;
    /** Opens the Quick Export to AKG Basic, in DSK. */
    void openQuickExportToBasic() noexcept;

private:
    /** Closes the dialog. */
    void closeDialog() noexcept;

    /** Opens the dialog which type is given. */
    template<class DIALOG>
    void openDialog()
    {
        jassert(exportDialog == nullptr);               // Already present?
        exportDialog = std::make_unique<DIALOG>(mainController, [&] { closeDialog(); });
    }

    MainController& mainController;
    std::unique_ptr<ModalDialog> exportDialog;
};

}   // namespace arkostracker
