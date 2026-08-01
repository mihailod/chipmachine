#pragma once

#include <cstdint>
#include <vector>

namespace musix::sndh {

// ICE! 2.4 depacking, exported for callers OUTSIDE the decoder.
//
// Most SNDH files in the archive are ICE!-packed, so anything that wants to read
// their tags -- not just play them -- has to depack first. That used to mean
// calling unice68_* from sc68plugin, which is GPL-3 and is therefore not linked
// in the mas build at all (see CM_HAVE_SC68 in chipmachine/CMakeLists.txt). This
// forwards to the public-domain ice_24.c that AtariAudio already vendors, so
// SongFileIdentifier::parseSndh works identically in both variants.
//
// Kept as a separate translation unit at default visibility on purpose:
// everything else in this plugin is partial-linked to local symbols by `ld -r`
// (see CMakeLists.txt), which would otherwise make ice_24.c unreachable from
// outside the archive.

// True when `data` starts with a valid ICE! 2.4 header.
bool isIcePacked(const uint8_t* data, size_t size);

// Depacks an ICE! 2.4 buffer. Returns the depacked bytes, or an empty vector if
// the header is invalid or the depacker disagreed with the advertised size.
std::vector<uint8_t> iceDepack(const uint8_t* data, size_t size);

} // namespace musix::sndh
