#pragma once

namespace arkostracker
{

/** The context of the operation, to know what folder to open. */
enum class FolderContext : unsigned char
{
    loadOrSaveSong,
    loadOrSaveInstrument,
    loadOrSaveArpeggio,
    loadOrSavePitch,
    /** All export will use the same folder, should be enough. */
    anyExport,
    /** Any other use. */
    other,

    last = other
};

}   // namespace arkostracker
