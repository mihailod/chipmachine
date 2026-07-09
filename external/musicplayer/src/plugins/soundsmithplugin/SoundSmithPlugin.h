#pragma once

#include "../../chipplugin.h"

namespace musix {

// Plays Apple IIgs SoundSmith music. SoundSmith (Huibert Aalbers, 1989) was the
// dominant IIgs tracker; its tunes drive the Ensoniq 5503 "DOC" digital
// oscillator chip. On Modland each tune is a PAIR: a bare-named song file
// (patterns/orders) plus a "<song>.W" wavebank holding the 64KB of DOC sound
// RAM and the instrument table. canHandle() identifies the song by its header
// structure (the leading signature varies per editor build, so it is not a
// usable magic); getSecondaryFiles() asks the host to fetch the ".W" companion next
// to it. We emulate the DOC in-process (ported from Sean Kasun's BSD player) and
// render at the chip's native 26320 Hz. NOT routed through UADE.
class SoundSmithPlugin : public ChipPlugin
{
public:
    std::string name() const override { return "SoundSmith"; }
    bool canHandle(const std::string& name) override;
    std::vector<std::string> getSecondaryFiles(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;
};

} // namespace musix
