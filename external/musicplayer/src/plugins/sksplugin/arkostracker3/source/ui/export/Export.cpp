#include "Export.h"

#include "akg/ExportAkgDialog.h"
#include "akm/ExportAkmDialog.h"
#include "aky/ExportAkyDialog.h"
#include "events/ExportEventsDialog.h"
#include "fap/ExportFapDialog.h"
#include "fap/ExportYmToFapDialog.h"
#include "instrumentToWav/ExportInstrumentToWavDialog.h"
#include "midi/ExportMidiDialog.h"
#include "mod/ExportModDialog.h"
#include "quickTesting/QuickExportAkgDialog.h"
#include "quickTesting/QuickTestingDialog.h"
#include "raw/ExportRawDialog.h"
#include "rawLinear/ExportRawLinearDialog.h"
#include "sample/ExportSamplesDialog.h"
#include "sfx/ExportSfxsDialog.h"
#include "txt/ExportTxtDialog.h"
#include "vgm/ExportVgmDialog.h"
#include "wav/ExportWavDialog.h"
#include "ym/ExportYmDialog.h"

namespace arkostracker 
{

Export::Export(MainController& pMainController) noexcept :
        mainController(pMainController)
{
}

void Export::openExportToAkg() noexcept
{
    openDialog<ExportAkgDialog>();
}

void Export::openExportToAky() noexcept
{
    openDialog<ExportAkyDialog>();
}

void Export::openExportToAkm() noexcept
{
    openDialog<ExportAkmDialog>();
}

void Export::openExportToFap() noexcept
{
    openDialog<ExportFapDialog>();
}

void Export::openExportToMidi() noexcept
{
    openDialog<ExportMidiDialog>();
}

void Export::openExportToMod() noexcept
{
    openDialog<ExportModDialog>();
}

void Export::openExportToText() noexcept
{
    openDialog<ExportTxtDialog>();
}

void Export::openExportToYm() noexcept
{
    openDialog<ExportYmDialog>();
}

void Export::openExportYmToFap() noexcept
{
    openDialog<ExportYmToFapDialog>();
}

void Export::openExportToVgm() noexcept
{
    openDialog<ExportVgmDialog>();
}

void Export::openExportToWav() noexcept
{
    openDialog<ExportWavDialog>();
}

void Export::openExportInstrumentToWav() noexcept
{
    openDialog<ExportInstrumentToWavDialog>();
}

void Export::openExportSfxs() noexcept
{
    openDialog<ExportSfxsDialog>();
}

void Export::openExportEvents() noexcept
{
    openDialog<ExportEventsDialog>();
}

void Export::openExportToRaw() noexcept
{
    openDialog<ExportRawDialog>();
}

void Export::openExportToRawLinear() noexcept
{
    openDialog<ExportRawLinearDialog>();
}

void Export::openExportSamples() noexcept
{
    openDialog<ExportSamplesDialog>();
}

void Export::closeDialog() noexcept
{
    exportDialog.reset();
}

void Export::openQuickExport() noexcept
{
    openDialog<QuickTestingDialog>();
}

void Export::openQuickExportToBasic() noexcept
{
    openDialog<QuickExportAkgDialog>();
}

}   // namespace arkostracker
