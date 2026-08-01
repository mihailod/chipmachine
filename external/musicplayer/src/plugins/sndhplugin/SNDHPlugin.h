#pragma once

#include "../../chipplugin.h"

namespace musix {

// Atari ST/STE SNDH player built on AtariAudio (Arnaud Carre / Leonard-Oxygene,
// MIT), vendored under atariaudio/. AtariAudio is a self-contained ST audio
// machine: Musashi 68000 (Karl Stenerud, MIT), a from-scratch YM2149, the MFP
// 68901 timers and the STE DMA DAC, plus an ICE! depacker (Hans Wessels, public
// domain). No external dependencies.
//
// This REPLACES libsc68 as the .sndh decoder in BOTH build variants -- libsc68 is
// GPL-3 and cannot ship on the Mac App Store, and .sndh is 76% of what it played
// (6,079 of 7,976 rows). SC68Plugin still exists in the plus build and still
// claims ".sndh" as a lower-priority fallback, so anything AtariAudio rejects is
// handed on to libsc68 there exactly as before; in the mas build SC68Plugin is
// not linked at all. See ../sc68plugin/ and the CM_HAVE_SC68 block in
// chipmachine/CMakeLists.txt.
//
// ".sc68" itself does NOT move here. That container leans on the 95 prebuilt
// GPL-3 replay binaries in data/sc68/Replay/, which have no permissive
// equivalent, so those 1,894 rows stay plus-only and are dropped from the mas
// index by songHasNoPlayer() in MusicDatabase.cpp.
class SNDHPlugin : public ChipPlugin
{
public:
    // Names the ENGINE, not the machine -- this string is what the TAB plugin
    // filter screen shows, and there it sits next to StSound and (in the plus
    // build) SC68, which are also Atari ST. "AtariAudio" is the useful
    // distinction; "Atari ST" would not be. The per-song `format` metadata is a
    // separate string and stays "SNDH (Atari ST)", which describes the file
    // format rather than the decoder.
    //
    // cmtest derives its fixture directory from this name -- rename
    // testmus/sndh (atariaudio)/ in step if it ever changes again.
    std::string name() const override { return "SNDH (AtariAudio)"; }
    bool canHandle(const std::string& name) override;
    std::set<std::string> getSupportedExtensions() const override;
    ChipPlayer* fromFile(const std::string& fileName) override;

    // Above SC68Plugin's 1 (which is itself above AdPlug's 0 for the shared
    // ".snd" extension). Ordering is what makes AtariAudio the .sndh decoder in
    // the plus build while leaving libsc68 reachable as a fallback -- plugins are
    // sorted by priority and MusicPlayer::fromFile() walks every claimer until
    // one actually returns a player.
    int priority() override { return 2; }
};

} // namespace musix
