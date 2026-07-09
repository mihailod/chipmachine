#pragma once

// FAC SoundTracker (MSX) ".MUS" -> KSS conversion. See FacMus2Kss.cpp for the
// format notes and attribution (rePlayer / Wothke webnez / FAC's own routine).

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace musix {
namespace fac {

// Reads a companion file (a drumkit ".SM1"/".SM2") given its plain name into
// `out`; returns false if it can't be found/read. Bound by the caller to the
// song's directory.
using CompanionReader =
    std::function<bool(const std::string& name, std::vector<uint8_t>& out)>;

// A FAC ".MUS" is exactly this size (7-byte MSX BSAVE header + 0x4000 page).
constexpr size_t kFacMusSize = 0x4007;

// True if `data`/`size` looks like a FAC SoundTracker ".MUS" song: an MSX BSAVE
// image (0xFE, load 0x8000) of the expected length. Content gate for canHandle.
bool IsFacMus(const uint8_t* data, size_t size);

// The drumkit base name a drummed song references (e.g. "DRUMKIT1"); empty for
// a "NO DRUMS" song or a non-FAC/short buffer. Used to surface SM1/SM2 as
// secondary files.
std::string FacDrumkitBaseName(const uint8_t* data, size_t size);

// Convert a whole ".MUS" file (`input`/`size`, including the BSAVE header) into
// a KSS image in `kssOut`, fetching the SM1/SM2 drumkits via `openCompanion`
// when the song uses drums. Fills `title`/`artist`. Returns false if it isn't a
// convertible FAC song or a required drumkit companion is missing.
bool FacMusToKss(const uint8_t* input, size_t size,
                 const CompanionReader& openCompanion,
                 std::vector<uint8_t>& kssOut, std::string& title,
                 std::string& artist);

} // namespace fac
} // namespace musix
