cmake_minimum_required(VERSION 3.15)

# macOS deployment target. PINNED, deliberately -- do not go back to deriving it
# from the build host's `sw_vers`.
#
# This used to auto-detect from the host, which made the shipped minimum OS an
# accident of whichever Mac ran the build (a machine on 26.5 produced a binary
# no 26.4 user could launch), and made builds unreproducible across machines.
#
# 26.0 rather than the host's version because every dylib in
# Contents/Frameworks (libav*, libssl/libcrypto, libglfw, libGLEW, libfreetype,
# libpng, libmpg123) is itself built for 26.0. Check with:
#     otool -l Contents/Frameworks/libavformat.62.dylib | grep -A4 LC_BUILD_VERSION
#
# To be precise about what this number does and does not guarantee: minos is a
# COMPILE-time contract, not a hard runtime gate. dyld is largely permissive, so
# a build targeting 26.0 can in fact launch on much older macOS if no code path
# happens to touch a newer API -- this has been observed working on an old Mac
# with LSMinimumSystemVersion hand-edited to 11.0.
#
# That is luck, not support. Targeting 26.0 tells the compiler every API up to
# 26.0 exists, so it emits NO availability diagnostics and strong-links freely;
# the first call into a post-11.0 API -- from this tree, from any of the ~20
# vendored deps, or from a future ffmpeg/glfw bump -- becomes a launch-time
# "Symbol not found" crash on those systems with no build-time warning at all.
# We also have no older hardware to test on.
#
# So 26.0 is an HONEST floor, not a physical one. Genuinely supporting older
# macOS means lowering this AND rebuilding the vendored dylibs at the same
# target, so the compiler enforces availability -- then testing on that OS.
# Lowering this line alone only converts a tested claim into an untested one.
#
# package_app.sh reads LSMinimumSystemVersion back off the built binary, so the
# Info.plist follows this value automatically -- there is no second place to
# update. Override for experiments with -DCMAKE_OSX_DEPLOYMENT_TARGET=...
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "26.0" CACHE STRING "macOS Deployment Target")
endif()

project(chipmachine)

# ---------------------------------------------------------------------------
# Build variant (App Store vs full/GitHub) -- see variants.conf
# ---------------------------------------------------------------------------
# CM_VARIANT selects the product identity and feature set:
#   plus (default) -- full build, includes YouTube/yt-dlp, Developer ID / GitHub.
#   mas            -- Mac App Store build: defines CM_MAS, which gates out the
#                     YouTube plugin (see src/main.cpp) and the YouTube catalog
#                     rows (see lua/db.lua via the cm_is_mas flag).
# Names come from variants.conf so there is ONE source of truth shared with
# package_app.sh. Configure a MAS build in its own dir, e.g.:
#   cmake -S chipmachine -B build-mas -DCMAKE_BUILD_TYPE=Release -DCM_VARIANT=mas
set(CM_VARIANT "plus" CACHE STRING "Build variant: plus (full/GitHub) | mas (Mac App Store)")
set_property(CACHE CM_VARIANT PROPERTY STRINGS plus mas)

if(NOT CM_VARIANT STREQUAL "plus" AND NOT CM_VARIANT STREQUAL "mas")
    message(FATAL_ERROR "CM_VARIANT must be 'plus' or 'mas', got '${CM_VARIANT}'")
endif()

# Pull the compile-time PROGRAM_NAME for this variant out of variants.conf
# (<VARIANT>_PROGRAM_NAME="..."), so renaming a product means editing only that
# file. Fail loudly if the key is missing rather than silently mis-naming.
string(TOUPPER "${CM_VARIANT}" _CM_VARIANT_UC)
set(CM_VARIANTS_CONF "${CMAKE_CURRENT_SOURCE_DIR}/variants.conf")
set(CM_PROGRAM_NAME "")
if(EXISTS "${CM_VARIANTS_CONF}")
    file(STRINGS "${CM_VARIANTS_CONF}" _CM_CONF_LINES
         REGEX "^${_CM_VARIANT_UC}_PROGRAM_NAME=")
    foreach(_line IN LISTS _CM_CONF_LINES)
        if(_line MATCHES "^${_CM_VARIANT_UC}_PROGRAM_NAME=\"(.*)\"")
            set(CM_PROGRAM_NAME "${CMAKE_MATCH_1}")
        endif()
    endforeach()
endif()
if(CM_PROGRAM_NAME STREQUAL "")
    message(FATAL_ERROR
        "Could not read ${_CM_VARIANT_UC}_PROGRAM_NAME from ${CM_VARIANTS_CONF}")
endif()

# Compile definitions applied to every target that shows the product name or
# registers the YouTube plugin (chipmachine, cm). cmtest is intentionally
# excluded so the test suite always exercises the full feature set.
set(CM_VARIANT_DEFS "PROGRAM_NAME=\"${CM_PROGRAM_NAME}\"")
if(CM_VARIANT STREQUAL "mas")
    list(APPEND CM_VARIANT_DEFS CM_MAS)
endif()

# UADE is GPLv2. The App Store distribution terms are incompatible with it (the
# same reason the whole app is Developer-ID-only outside the MAS variant), so
# the mas build must not merely disable UADE -- the code must be ABSENT from the
# binary. uadeplugin is therefore dropped from MUSICPLAYER_PLUGINS below, which
# means the uadeplugin CMake target does not exist at all in a mas build tree
# and nothing may reference uadeplugin_register() or <uadeplugin/UADEPlugin.h>.
#
# This is a SEPARATE flag from CM_MAS on purpose. CM_MAS carries the product
# identity and the YouTube gating and is deliberately withheld from cmtest;
# CM_NO_UADE is a hard statement about which targets got linked, so it must
# reach every target that compiles against the plugin list -- cmtest included,
# or the test binary would not link in a mas tree.
#
# Formats only UADE plays therefore Skip in the mas build. That set is kept as
# small as possible by OpenMPTPlugin's content probe (see openmptCanDecode),
# which routes everything libopenmpt can decode here regardless of filename.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_UADE OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_UADE)
else()
    set(CM_HAVE_UADE ON)
endif()

# VICE is GPLv2, and the same App Store reasoning applies: vicepluginbridge must
# be ABSENT from the mas binary, not merely disabled. It follows CM_HAVE_UADE's
# pattern exactly -- CM_NO_VICE reaches every target compiling against the plugin
# list (cmtest included) because it states which targets were linked.
#
# Unlike UADE, though, VICE's MAIN job is gone rather than lost: ordinary SID
# (.sid/.rsid) is now played by csidplugin (Hermit's cSID, WTFPL) in BOTH
# variants, at 0.958-1.000 spectral cosine against VICE/reSID. In the plus build
# vicepluginbridge is retained for the two things cSID cannot do -- Compute!
# Sidplayer .mus/.str, and self-IRQ RSIDs (play=$0000), of which only ~2 in 14
# are audible under cSID (see VicePlugin::canHandle). In the mas build those
# .mus/.str tunes are played instead by musplugin -- a clean-room Compute!
# Sidplayer sequencer over cSID, registered after vicepluginbridge so the plus
# build keeps using VICE and is bit-for-bit unchanged. The self-IRQ RSIDs have no
# mas fallback and stay with cSID, which reports the silent ones unplayable so
# they Skip.
#
# Dropping VICE also drops its data/c64 KERNAL/BASIC/chargen ROM images, which
# are Commodore-copyrighted and a separate App Store problem from the licence.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_VICE OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_VICE)
else()
    set(CM_HAVE_VICE ON)
endif()
# GoatTracker is GPLv2 twice over and the same App Store reasoning applies a
# third time: goattrackerplugin vendors GoatTracker's own gplay.c playroutine and
# gsong.c loader (Lasse Oorni, GPL-2) on top of reSID (Dag Lem, GPL-2), so it
# must be ABSENT from the mas binary. Follows CM_HAVE_UADE's pattern exactly.
#
# Unlike UADE and VICE there is no fallback and none is worth building. Swapping
# reSID for the already-vendored cSID (WTFPL) would not help -- the GT2
# playroutine is the other half of the licence problem -- so a mas fallback means
# clean-rooming the whole sequencer. The .sng file format is documented in
# GoatTracker's goattrk2.txt, but the playroutine semantics (wave/pulse/filter
# tables, arpeggio, hard-restart/ADSR-bug timing) are not, and a hand-written
# parser has already been tried once and got the version-gated fixups wrong (see
# memory [[goattracker-sng-support]]). The exposure is 104 songs -- modland
# "GoatTracker" (90) + "GoatTracker 2" (14), the only GoatTracker-format .sng in
# the whole catalog -- so they are dropped from the mas index instead, by
# songFormatHasNoPlayer() in MusicDatabase.cpp. Every OTHER .sng format
# (SCC-Musixx, Richard Joseph, Sam Coupe, Ad Lib, ...) has its own player and is
# unaffected.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_GOATTRACKER OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_GOATTRACKER)
else()
    set(CM_HAVE_GOATTRACKER ON)
endif()
# DefleMask .dmf is the fourth and largest case. dmfplugin drives a headless
# slice of the Furnace engine (tildearrow, GPL-2.0-or-later), and unlike the
# three above the GPL here is STRUCTURAL rather than a swappable core: 76 of 77
# files under furnace/src/engine, all 80 platform/DivPlatform*.cpp wrappers, and
# several sound cores (reSID, reSIDfp, sid2, nes, pce_psg, tia, vsu, pokey,
# dave, lynx, ted-sound, ymf278b) carry the GPL header. There is no file to
# remove and no second implementation to swap in -- DefleMask itself is closed
# source and Furnace is the only engine that reads .dmf -- so the whole plugin
# must be ABSENT from the mas binary. Follows CM_HAVE_UADE's pattern exactly.
#
# dmfplugin is also the SOLE source of reSIDfp in the mas link (183 symbols in
# libdmfplugin.a; verified with nm over every static lib in build-mas), so this
# gate is what finally clears reSIDfp out of the App Store build -- GoatTracker's
# reSID was already gone via CM_HAVE_GOATTRACKER.
#
# The original exposure was 2,071 songs (0.28% of the catalog). Content-probed,
# not guessed: ".dmf" is TWO unrelated formats and libopenmpt plays the other
# one, so only the rows whose format column names DefleMask were hidden --
# modland "Deflemask" (1,807) + BotB "DefleMask" (263) + demozoo "NEC PC Engine"
# (1). The X-Tracker DDMF rows stay indexed and keep playing: "X-Tracker" (366),
# "DSMI Compact" (13), the generic "DMF" bucket (328, amp + modarchive --
# sampled and 100% DDMF) and demozoo "Demoscene" (16, all 16 fetched and all
# DDMF).
#
# BEING REVERSED, one chip target at a time. dmfcrplugin now ships a CLEAN-ROOM
# .dmf player in BOTH variants (see its README and LEGAL), written from
# DefleMask's own published specs and manual on ymfm (BSD-3) plus an SN76489
# written from the documented hardware. It covers SEGA Genesis and SEGA Master
# System, bringing the exposure down from 2,071 to 1,375: 598 Genesis + 98 SMS
# rows are back in the mas index.
#
# The remaining targets, by measured share of what is still hidden: Game Boy
# ~340 (SameBoy/MIT available), PC Engine ~275 (furnace's pce_psg is GPL, needs
# a replacement), Neo Geo ~200 (ymfm again -- it is a YM2610, so the FM half is
# largely what the Genesis path already does), NES ~125 (NSFplay-family cores
# exist), C64 ~110 (the WTFPL cSID is already vendored here).
#
# NOTE the resurfacing mechanism CHANGED with dmfcrplugin. Deleting a
# formatPlayer row is no longer the way: the format column reads "Deflemask" for
# all eight DefleMask systems, so a name-keyed rule can only say all-or-nothing.
# Which rows survive is now the measured allow-list data/misc/dmfcr_playable.txt,
# consulted by songDefleMaskUnplayable() in MusicDatabase.cpp. When a new target
# lands, widen the player's canHandle() and regenerate that list (tools/dmfcheck).
#
# dmfplugin still registers FIRST in plugin_register.cpp, so the plus build
# keeps playing every .dmf through Furnace and stays byte-identical -- which
# also keeps a reference implementation on hand for A/B listening.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_DMF OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_DMF)
else()
    set(CM_HAVE_DMF ON)
endif()
# sc68plugin is the fifth case, and the first where the REPLACEMENT landed in the
# same change as the gate. libsc68 + emu68 + file68 + unice68 are GPL-3 (Benjamin
# Gerard), so the plugin must be ABSENT from the mas binary. Follows
# CM_HAVE_UADE's pattern exactly.
#
# Most of what it played does NOT go away. sc68plugin owned 7,976 rows, and 6,079
# of them (76%) are ".sndh" -- those are now decoded by sndhplugin, built in BOTH
# variants on AtariAudio (Arnaud Carre, MIT: Musashi 68000 + a from-scratch
# YM2149 + MFP timers + STE DMA DAC + a public-domain ICE! depacker). AtariAudio
# is the engine behind Arnaud Carre's own SNDH archive player, and he wrote the
# original StSound this project already ships. sndhplugin declares priority 2, so
# in the plus build it takes .sndh first and libsc68 remains reachable as a
# fallback for anything it declines -- that build can only get better, never
# worse.
#
# What IS lost is ".sc68" itself: 1,894 rows. That container dispatches to the 95
# prebuilt 68k replay binaries in data/sc68/Replay/, which are sc68 project build
# outputs under the same GPL-3 and have no permissive equivalent -- there is no
# second implementation to swap in and no spec to clean-room against. Those rows
# are dropped from the mas index by songHasNoPlayer() in MusicDatabase.cpp.
#
# The 100 AdLib ".snd" rows are unaffected: SC68Plugin only ever content-gated
# that extension, and AdPlug claims them in both variants.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_SC68 OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_SC68)
else()
    set(CM_HAVE_SC68 ON)
endif()
# pokeynoiseplugin is the sixth case, and by far the cheapest. It drives ASAP
# (Another Slight Atari Player, GPL-2, vendored at pokeynoiseplugin/asap/), so it
# must be ABSENT from the mas binary. Follows CM_HAVE_UADE's pattern exactly.
#
# It claims two extensions and only loses one of them. ".sap" -- the 6,617-song
# ASMA corpus -- is ALSO claimed by gmeplugin (LGPL, in both variants), which
# already wins it on registration order, so Atari 8-bit POKEY music is unaffected
# by this gate. What goes is ".pn": the 17 modland
# "PokeyNoise/<artist>/pn.<game>" tunes, which nothing else claims.
#
# Those 17 cost almost nothing, and that was MEASURED rather than assumed: 16 of
# them already exist in the catalog as a ".sap" with the same title AND the same
# composer -- Ballblazer, Behind Jaggi Lines! (pn.bjl, the working title of
# Rescue on Fractalus), Boulder Dash, International Karate, Jet Set Willy,
# Warhawk, Draconus, Zybex, four David Whittakers, Schreckenstein, Space
# Lobsters, Whistlers Brothers and Feud. Every one keeps playing in mas through
# GME. The single possible loss is "pn.tetris", whose composer is unknown and
# whose three same-titled .sap entries are by unrelated authors.
#
# No formatPlayer entry is needed: ".pn" has no other claimer, so the plain
# extension test in songHasNoPlayer() drops the rows on its own -- via
# nameHasPlayer()'s modland prefix-before-dot branch, since these paths carry the
# format BEFORE the dot ("pn.draconus") and have no usable suffix.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_POKEYNOISE OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_POKEYNOISE)
else()
    set(CM_HAVE_POKEYNOISE ON)
endif()
# mdxplugin drives mdxmini (BouKiCHi's mdxplay derivative, Daisuke Nagano), which
# is GPL-2 at the package level -- COPYING is the GPL-2 text and mdxmini.txt says
# in as many words that linking to it, statically or dynamically, imposes GPL2.
# Only src/mdxmini.c carries the boilerplate header, so a per-file audit
# understates this badly; there is no header-stripping argument available.
# The plugin must therefore be ABSENT from the mas binary. Follows
# CM_HAVE_UADE's pattern exactly.
#
# This one is a clean, total loss of 6,913 rows, with no partial rescue: it is
# the sole claimer of ".mdx" (rsnplugin lists the extension only to hand archive
# members off to whoever claims them), every row ends in ".mdx" with format
# "MDX", and there are no MULTI: groups and no ".pdx" rows. So the plain
# extension test in songHasNoPlayer() drops all of them and NO formatPlayer
# entry is needed -- unlike mus/dmf/sc68, which needed format-name keys.
#
# There is no replacement to swap in, and this was checked properly rather than
# assumed. Every other MDX player is either GPL (mdxtools/mdx2vgm are GPL-3;
# webMDX is mdxmini compiled to JS) or worse: GAMDX/MXDRVg looks like the answer
# because GORRY licenses his own code Apache-2.0, but its actual driver engine is
# a DISASSEMBLY of the proprietary 1988 mxdrv.x, under a licence its own
# distributor says he cannot locate. That is a worse App Store risk than GPL, not
# a better one. Running the real mxdrv.x under the MIT Musashi already vendored
# for sndhplugin hits the same wall -- it means shipping the proprietary driver.
#
# A clean-room OPM-only engine was prototyped (cmdx, see
# plugins/mdxcrplugin/cmdx/, not wired into either build) on the measured fact
# that 69.3% of the corpus never references a .pdx sample bank and so can never
# start a PCM8 voice. It reached only ~3.7% median register-stream fidelity
# against the reference and is SHELVED. If it is ever finished, this gate
# becomes conditional on the MDX/MDX-PCM split rather than being removed.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_MDX OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_MDX)
else()
    set(CM_HAVE_MDX ON)
endif()
# famitrackerplugin drives the vendored FamiTracker CX slice
# (external/famitracker-cx), whose engine -- the APU, the channel handlers, the
# document loader and the instrument model alike -- is jsr's FamiTracker under
# GPL-2 or later. PROVENANCE.md's "mixed GPL-2 / New BSD" is true of the upstream
# package as a whole, but every file we actually compile that matters carries the
# GPL boilerplate; only blargg's Blip_Buffer (LGPL) and emu2413 (zlib-style) are
# permissive. So the plugin must be ABSENT from the mas binary, following
# CM_HAVE_UADE's pattern.
#
# Cost: 1,597 rows (960 modland "FamiTracker/", 637 Battle of the Bits, plus 2
# scene.org rows filed under "Demoscene"). This one NEEDS formatPlayer entries in
# MusicDatabase.cpp -- unlike CM_HAVE_MDX, ".ftm" is NOT sole-claimed: libopenmpt
# advertises it for the unrelated Atari "Face The Music" format, so "ftm" is in
# playableExtensions() in every build and the plain extension test would keep all
# 1,597 rows visible and unplayable. The 95 "Face The Music" rows are OpenMPT's
# and must keep playing in mas -- which is exactly why the keys are (extension,
# format) pairs.
#
# Rerouting was measured and rejected: only 42 of the 1,597 rows have a
# title+composer twin anywhere else in the catalog (39 of those against .nsf),
# and several of those are coincidental title collisions. This is a scene corpus
# of originals, not game rips with NSF counterparts.
#
# No replacement exists to swap in, and this was checked rather than assumed.
# Every other engine that reads .ftm is GPL (0CC-FamiTracker, Dn-FamiTracker,
# famitracker-cx itself, Furnace -- which is already gated out of mas for .dmf).
# FamiStudio is MIT but is C#, imports "official 0.4.6 only", and renders through
# its own engine: its own documentation calls the conversion lossy (slides
# "a lossy process", VRC7 pitch/vibrato "will likely not sound like FamiTracker",
# pitch+arpeggio envelopes wrong, "every other effect will be ignored").
#
# What a replacement WOULD look like, if this is ever revisited: the chip half is
# already solved and permissively licensed. The NSFPlay/xgm cores vendored under
# dmfplugin/furnace/src/engine/platform/sound/nes_nsfplay/ (nes_apu, nes_dmc,
# nes_fds, nes_mmc5, nes_n106, nes_vrc6; "You can reuse these source code
# freely") cover every chip FamiTracker uses bar VRC7 (emu2413, already in tree
# three times) and Sunsoft 5B (emu2149 in libkss, or GME's Nes_Fme7_Apu), and are
# independently vendorable out of the GPL Furnace tree the way Musashi and
# StSound were. Only the driver would have to be written -- ~4,000 lines against
# the documented module format, with famitracker-cx in the plus build as a
# register-trace oracle (the mdxtrace pattern). Measured corpus shape for
# phasing: 2A03-only 42%, +VRC6 69%, +MMC5 76%, +FDS 80%, +VRC7 89%.
# Such an engine would also be a candidate to REPLACE famitracker-cx in plus,
# because those cores can drive the N163 and Sunsoft 5B channels that upstream
# never wired: 152 files, plus 21 saved by FamiTracker 0.5.0-beta/0CC (module
# version 0x450, rejected by the FILE_VER gate) and 2 Dn-FamiTracker files whose
# magic canHandle declines -- 175 files, 11% of the corpus, that NEITHER variant
# can play today. Until any of that exists, this is a clean loss in mas.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_FAMITRACKER OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_FAMITRACKER)
else()
    set(CM_HAVE_FAMITRACKER ON)
endif()
# maxtraxplugin's decoder is ScummVM's, and it is GPL-3 OR LATER -- the strictest
# licence in the tree. Both vendored files carry the boilerplate
# (plugins/maxtraxplugin/maxtrax/maxtrax.{h,cpp}, the loader + sequencer, and
# paula.{h,cpp}, the 4-voice Amiga mixer; 1,812 lines together). Only our own
# wrapper, compat.h and the split-set logic are outside it. So the plugin must be
# ABSENT from the mas binary, following CM_HAVE_UADE's pattern.
#
# Cost: 93 rows, and it is a clean total loss -- 82 modland "MaxTrax/" files and
# 11 UnExoticA game rips (7 of them MULTI: groups). NO formatPlayer entry is
# needed: ".mxtx" is sole-claimed (vgmstreamplugin lists it only in
# excluded_extensions, i.e. it declines it), so the plain extension test in
# songHasNoPlayer() drops the modland rows, and the UnExoticA rows -- prefix-form
# "mxtx.<song>", whose pathExtension is the song name ("agro", "mx", "eng",
# "mx00", "badinfo", "trx"), none of them claimed by anything in either build --
# fall to nameHasPlayer()'s leading-token branch on "mxtx". MULTI: groups are
# judged on their first member and every one of those is an "mxtx." name.
#
# Rerouting was measured and rejected: ZERO of the 93 have a title+composer twin
# anywhere else in the catalog (5 share a title alone, all coincidental). The
# corpus does duplicate itself, though -- modland and UnExoticA both carry
# Darkseed, A-Train, Dune II and Kyrandia -- so the count overstates the number
# of distinct tunes lost.
#
# No replacement exists to swap in, and this was checked rather than assumed.
# ScummVM's is the ONLY native implementation of the format anywhere: there is no
# libmaxtrax, nothing on GitHub, and rePlayer does not support MaxTrax. Every
# other player is 68k assembly -- Wanted Team's EaglePlayer (EP_MaxTrax.lha /
# SRC_MaxTrax.lzx on wt.exotica.org.uk), which states no licence at all and needs
# an Amiga to run. UADE cannot help either, in this or any build: its
# amifilemagic.c detects the "MXTX" magic but upstream has no MaxTrax eagleplayer,
# so the detection is vestigial. And unlike .sndh or .sc68, a .mxtx file is pure
# data -- the replayer is not in it -- so running the original driver under the
# MIT Musashi core would mean shipping that unlicensed binary.
#
# What a replacement WOULD look like, if this is ever revisited: the Paula half is
# easy and we have done its like twice (the wsrplugin machine, victrackerplugin's
# VIC-I), with OpenMPT's BSD soundlib/Paula.{h,cpp} already in tree as a
# permissive reference. The blocker is the ~1,050-line sequencer, for which there
# is NO published spec -- David 'Talin' Joiner and Joe Pearce's Music-X-derived
# format was never documented publicly. That leaves ScummVM's code (GPL, so it
# taints a clean-room) or the Wanted Team assembly (an unlicensed reconstruction,
# and maxtrax.h's own comments show ScummVM was ported from exactly that
# assembler source) -- the same shape as the GAMDX disassembly rejected for
# CM_HAVE_MDX above, for 93 rows instead of 6,913. Verification would be the easy
# part: 93 files and the plus build as an audio oracle.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_MAXTRAX OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_MAXTRAX)
else()
    set(CM_HAVE_MAXTRAX ON)
endif()
# libvgmplugin is the exception to the pattern above: the plugin is NOT gated
# (it is the only decoder for the 1,341-row OPL Archive and much of VGMRips), but
# libvgm bundles SEVERAL cores per chip and some of them are GPL. libvgm is built
# for exactly this -- every core sits behind an EC_<CHIP>_<CORE> macro and every
# chip behind SNDDEV_<CHIP>, enabled wholesale only when SNDDEV_SELECT is undefined
# ("if not asked to select certain sound devices, just include everything").
# libvgmplugin now defines SNDDEV_SELECT and picks cores explicitly.
#
# Five GPL cores were dropped from BOTH variants because they are unreachable
# dead code -- libvgm's own preference order already puts a permissive core
# first, so they were compiled in and never selected: ym2413.c + nukedopll.c
# (emu2413 is the default), ymf262.c (AdLibEmu is, per libvgm's own comment,
# "default, because it's better than MAME"), nes_apu.c (NSFPlay is), ym2612.c
# (the GPGX core in fmopn.c is). Dropping them changes no audio in either build.
#
# THIS flag covers the two cases where the GPL core IS the active default, so
# removing it is an audible change. plus keeps exactly what it has always used;
# mas swaps to a non-GPL core that every build already compiles:
#   * ym2151.c     -- MAME YM2151. mas uses nukedopm.c (Nuked OPM, LGPL-2.1):
#                     more accurate, measurably slower. Affects the X68000 corpus
#                     (202 VGMRips rows) and ~40% of libvgm-routed BotB VGMs.
#   * Ootake_PSG.c -- Ootake HuC6280. mas uses c6280_mame.c (BSD-3). Affects PC
#                     Engine (234 VGMRips rows, ~36% of routed BotB).
# Both accepted only after an A/B render against real game music -- Castlevania
# (Sharp X68000) and Aldynes stage 1 (PC Engine) -- at RMS ratios of 1.00 and
# 0.98 with no clipping on either side.
#
# KNOWN CAVEAT on the HuC6280 swap, accepted deliberately: the MAME core runs hot
# on noise/DDA-saturated material and clips where Ootake does not (BotB "Citizens
# of Luala" 1.49x, 785 clipped samples in 30s vs Ootake's 3; another rip 1.29x,
# 238 vs 0). Cause: libvgm's _CHIP_VOLUME table (player/vgmplayer.cpp) carries
# ONE volume per chip TYPE, hand-corrected for K051649 and C140/C219 only, so it
# is calibrated for whichever core is listed FIRST -- Ootake. A per-core
# correction there (about 0.7x for MAME) would remove the clipping; nobody has
# calibrated it yet, and ordinary game music is unaffected.
#
# The OPN family (fmopn.c -- YM2203/YM2608/YM2610, ~600-700 rows including all of
# PC-98, PC-88 and Neo Geo) was the biggest of these and is now SOLVED: mas builds
# Aaron Giles' ymfm (BSD-3, external/ymfm, vendored from UPSTREAM rather than from
# the modified copy in dmfplugin's GPL Furnace tree) through the adapter at
# libvgmplugin/opn_ymfm.cpp. plus still builds fmopn.c, so its audio is unchanged.
#
# What this does NOT fix: fmopl.c (YM3526 + Y8950 are MAME-only in libvgm; the
# YM3812 path is not), ymf278b.c (OPL4), msm5232.c, pwm.c, vsu.c and scd_pcm.c
# (RF5C164 -- VGMPlayer hard-requests FCC_GENS for it, so it cannot simply be
# dropped) have NO alternative core and are still compiled into mas. They are
# declared as open items in LEGAL. ymfm also implements YM3526/Y8950 and YMF278B,
# so extending the same adapter would retire fmopl.c and ymf278b.c too.
#
# NOTE on the failure mode, which is why cores are NOT dropped wholesale: when a
# core is missing, libvgm's VGMPlayer does `devInf->devDef = NULL; continue;` --
# the file still loads and plays with that chip SILENT. There is no load error to
# catch and nothing the index gate can see, so a dropped core is worse than a
# dropped plugin: it produces quietly wrong audio instead of a clean Skip.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_LIBVGM_GPL_CORES OFF)
else()
    set(CM_HAVE_LIBVGM_GPL_CORES ON)
endif()
# PlayStation audio is the seventh case, and the only one that started as a
# COPYRIGHT problem rather than a licence one.
#
# heplugin (Highly Experimental, kode54) used to play all four PSF extensions,
# and it could not initialise at all without a real Sony PlayStation 2 BIOS
# image: HEPlugin::fromFile() opened data/hebios.bin, fed it to
# bios_set_image(), and returned nullptr if it was not there. That file was
# 512 KB of Sony Computer Entertainment firmware -- it contains the literal
# string "Copyright 1999-2002 (C) Sony Computer Entertainment Inc." -- and it
# was shipping inside BOTH bundles, unmentioned in LEGAL. Nothing about a free
# app, a GPL argument or an App Store guideline is relevant to that; it is
# simply not ours to redistribute. It was DELETED from the tree (both copies:
# data/hebios.bin and an unreferenced duplicate under
# external/musicplayer/data/), which removes it from both bundles, since data/
# is copied wholesale by package_app.sh.
#
# heplugin itself was then DELETED too, and has no gate here for that reason.
# It carried NO licence statement at all, in-tree or upstream (LEGAL used to
# claim "zlib License" for it, which was wrong -- the zlib hits were
# `#include <zlib.h>`), and with the BIOS gone its fromFile() returned nullptr
# for every file in every variant. An unlicensed engine that cannot decode
# anything is pure liability, so nothing is kept to gate.
#
# The replacement was already in this repo, compiled and unreachable. aoplugin's
# AOSDK builds eng_psf/{eng_psf.c,eng_psf2.c,psx.c,psx_hw.c}, and psx_hw.c is an
# HLE implementation -- it emulates the PS1 BIOS A0/B0/C0 vectors and the PS2
# IOP kernel in software, so it needs NO BIOS image. Those engines were dead
# code only because AOPlugin::supported_ext did not list the PSF extensions. It
# does now, so ".psf/.minipsf/.psf2/.minipsf2" keep playing in the plus build.
# Verified over ~100 modland rips with `cm --dump-metadata`: every file Highly
# Experimental could load, AOSDK loads. (The handful that fail fail under BOTH
# engines, and for a reason that is neither engine's fault -- their "_lib2" tags
# name companions with underscores that modland stores with spaces.)
#
# aoplugin cannot go to the App Store, so it IS gated. Per aosdk/license.txt
# (which is explicit, file by file):
#   * BSD-3 (R. Belmont / Richard Bannister) -- corlett.c, eng_psf.c, eng_psf2.c,
#     eng_spu.c, psx_hw.c, eng_qsf.c, eng_ssf.c, sat_hw.c.
#   * MAME 1997-2008 (Nicola Salmoria and the MAME team) -- psx.c and cpuintrf.h
#     for PS1, the whole Musashi 68000 + SCSP for Saturn, and z80.c + qsound.c +
#     kabuki.c for QSound. That licence says outright "Redistributions may not be
#     sold, nor may they be used in a commercial product or activity", which no
#     App Store submission can satisfy.
#   * GPL-2 (Pete Bernert) -- eng_psf/peops/ and eng_psf/peops2/, the SPU and
#     SPU2 cores. Same reasoning as the six gates above.
# The MAME and GPL parts are the emulator cores themselves, one per target, so
# there is nothing to remove and no permissive substitute vendored here; the
# plugin must be ABSENT from the mas binary. Follows CM_HAVE_UADE's pattern.
#
# Exposure in mas: 685 rows. Measured as a SET DIFFERENCE between a completed
# plus index and a completed mas index (every row in mas is also in plus, so the
# difference is exactly what the gates hide) -- not as a before/after total,
# which is unreliable here because a --forcedbreindex rewrites the song table
# collection by collection and an interim COUNT(*) is meaningless:
#   .minipsf   353 | .psf 137 | .psf2 131 | .minipsf2 47   = 668 PSF rows
#   .spu         9 | .miniqsf 8                            =  17 AO-only rows
# ".ssf"/".minissf" (136 rows) are NOT lost: HTPlugin declares priority 1 and
# already owns them in both variants -- see [[plugin-order-stable-sort]].
# The 11 modland "SoundFactory" rows that carry a ".psf" suffix are not lost
# either; they are an Amiga format, they were always UADE's, and AOPlugin
# declines them on the "/soundfactory" path check.
#
# All FOUR extensions need formatPlayer rows in songFormatHasNoPlayer(). ".psf"
# because vgmstream also advertises it, so the plain extension test would keep
# those 137 rows; the other three because nameHasPlayer() also accepts a
# modland prefix-before-dot, and a first reindex with only the ".psf" row left
# exactly one straggler behind ("bgm.psf2" -- "bgm" is an MSX extension
# kssplugin claims). Same trap the three ".sc68" stragglers sprang.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_AO OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_AO)
else()
    set(CM_HAVE_AO ON)
endif()
# The last three console cores, gated together on 2026-08-01 after a licence
# review that found no permissive replacement for any of them. All three are
# "possible to clean-room, wildly disproportionate to the songs recovered".
#
# ndsplugin -- vio2sf, whose NDS core is derived from DeSmuME (GPL-2). Playing a
# .2sf means emulating BOTH DS CPUs (ARM7TDMI + ARM9/ARMv5TE) plus the 16-channel
# SPU. GBATEK documents the SPU and memory map exhaustively and mGBA (MPL-2.0)
# would supply an ARM7TDMI, but no permissive ARMv5TE core exists and melonDS is
# GPL-3. 2,040 songs -- the largest of the three, and still not worth an emulator.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_NDS OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_NDS)
else()
    set(CM_HAVE_NDS ON)
endif()
# usfplugin -- lazyusf2, a near-rewrite of Mupen64Plus (GPL-2). This is the one
# case with a STRUCTURAL blocker rather than merely a large one: a .usf is a
# Project 64 SAVESTATE, an undocumented emulator-private memory layout, not a
# published hardware format. A clean-room needs a spec to work from and there
# isn't one, so this is the single entry on the list written off outright.
# 184 songs.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_USF OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_USF)
else()
    set(CM_HAVE_USF ON)
endif()
# htplugin -- Neill Corlett's SegaCore as maintained in kode54/Highly_Theoretical,
# which carries GPL-3 at the repo root AND in Core/COPYING.txt. Note the licence
# files were never vendored here, so the in-tree copy looks unlicensed except for
# an old non-commercial Musashi header -- do not be misled by that again; the
# upstream repo is the source of truth. The CPU halves are actually solvable
# (MIT Musashi is already vendored for sndhplugin, and mGBA's MPL-2.0 ARM7TDMI is
# close kin to the Dreamcast's ARM7DI); what is not is SCSP/AICA, a 32/64-channel
# Yamaha part with a real DSP. 273 songs.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_HT OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_HT)
else()
    set(CM_HAVE_HT ON)
endif()
# ayflyplugin is the eleventh case, and the first where the replacement covers
# the WHOLE format list rather than part of it.
#
# Ayfly (Deryabin Andrew, GPL-2-or-later) plus its z80ex CPU core (Boo-boo,
# GPL-2, containing FUSE code) owned 67,305 songs -- ZX Spectrum AY trackers,
# far and away the largest single format family in the catalog and the largest
# unresolved copyleft dependency in the App Store binary. It must be ABSENT
# from the mas binary. Follows CM_HAVE_UADE's pattern exactly.
#
# Nothing is lost. zxayplugin is built in BOTH variants and covers every
# extension Ayfly claimed, by three routes (see its players/PROVENANCE.md):
#   * the tracker's ORIGINAL ZX Spectrum Z80 replay routine, run on the GME Z80
#     core into Ayumi (Peter Sovietov, MIT) -- .pt1/.pt2/.pt3/.psc/.sqt. The
#     routines are Sergey Bulba's published players, used under his attribution
#     grant, the same footing on which this project already ships the .fxm and
#     .amad players.
#   * a sequencer written from the published format description where no
#     redistributable player exists -- .stc (and its .zxs/.st13 aliases), .asc,
#     .stp/.stp2, .fxm/.amad.
#   * no player at all for the recorded register streams, .vtx and .psg.
# ".vt2" -- which Ayfly could NOT play, a measured 0 of 551 rows -- is picked up
# by sksplugin's Arkos Tracker 3 importer (MIT), so this change makes the mas
# index LARGER here than the plus index was.
#
# zxayplugin is registered AFTER ayflyplugin in plugin_register.cpp, so in the
# plus build -- where both exist and claim the same extensions -- Ayfly wins
# every tie on registration order and that build is bit-for-bit unchanged. Same
# arrangement as musplugin behind vicepluginbridge. No formatPlayer rows are
# needed in songFormatHasNoPlayer() because no extension loses its claimant.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_AYFLY OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_AYFLY)
else()
    set(CM_HAVE_AYFLY ON)
endif()
# zxtuneplugin is the twelfth case, and the smallest by songs -- but it is the
# last GPL-3 in the App Store binary. Vitamin/CAIG's ZXTune is GPL-3 (djdron's
# CMake fork ships an LICENSE.md claiming LGPL-3; upstream is GPL and the
# vendored sources carry GPL headers), so it must be ABSENT from the mas
# binary. Follows CM_HAVE_UADE's pattern exactly.
#
# It owned 569 rows across six ZX Spectrum formats, and 378 of them -- every
# one that is actually AY music -- are now played in BOTH variants by
# zxayplugin, which reclaimed them once the ZX AY machine built for the Ayfly
# replacement existed (see CM_HAVE_AYFLY):
#   .psm 188  Pro Sound Maker -- sequencer, from Bulba's AY_Emul
#   .ftc  88  Fast Tracker    -- Bulba's own MONS4D disassembly, run on the Z80
#   .st11 59  Sound Tracker 1.1, the UNCOMPILED form -- no player at all: it is
#             compiled to the ordinary .stc layout on load and handed to the
#             Sound Tracker player this plugin already had, which is what "ST
#             COMPILE" did on the Spectrum. See zxay_st11.cpp.
#   .gtr  43  Global Tracker  -- sequencer, from Bulba's AY_Emul
# All four were A/B'd against ZXTune (cmtest "[.zxtab]"): spectral cosine
# 0.80-0.98, envelope correlation 0.72-0.98, zero lag across 35 fixtures --
# i.e. the same music, differing only by chip model, which is as close as two
# AY emulations get.
#
# The mas variant loses the remaining 191 rows, dropped from its index to
# match. Both are out of scope because they are not AY music at all:
#   .tfe  148  TFM Music Maker -- TurboSound FM, i.e. a pair of YM2203s. The ZX
#              AY machine cannot help; it would need an OPN core plus an
#              undocumented sequencer.
#   .chi   43  Chip Tracker -- 4-channel DIGITAL sample playback, not the AY.
#              Same story: different hardware, no published format.
if(CM_VARIANT STREQUAL "mas")
    set(CM_HAVE_ZXTUNE OFF)
    list(APPEND CM_VARIANT_DEFS CM_NO_ZXTUNE)
else()
    set(CM_HAVE_ZXTUNE ON)
endif()
# victrackerplugin (.vt, VIC-20) needed NO variant gate, unlike the four above.
# Its GPL was internal rather than structural -- the plugin is ours, but it drove
# two GPLv2 engine cores (the omarandlorraine fake6502 fork, and a VIC-I sound
# core lifted from VICE's vic20sound.c). Both were replaced in place, by
# MyLittle6502 (public domain / CC0) and by a VIC-I written against the documented
# chip; the replacements were A/B'd by ear against the originals and shipped to
# both variants, so there is nothing to gate and all 11 .vt songs stay in the mas
# index. See the plugin's README.md.
message(STATUS "ChipMachine build variant: ${CM_VARIANT} (PROGRAM_NAME=\"${CM_PROGRAM_NAME}\", UADE=${CM_HAVE_UADE}, VICE=${CM_HAVE_VICE}, GOATTRACKER=${CM_HAVE_GOATTRACKER}, DMF=${CM_HAVE_DMF}, SC68=${CM_HAVE_SC68}, POKEYNOISE=${CM_HAVE_POKEYNOISE}, MDX=${CM_HAVE_MDX}, FAMITRACKER=${CM_HAVE_FAMITRACKER}, MAXTRAX=${CM_HAVE_MAXTRAX}, LIBVGM_GPL_CORES=${CM_HAVE_LIBVGM_GPL_CORES}, AO=${CM_HAVE_AO}, NDS=${CM_HAVE_NDS}, USF=${CM_HAVE_USF}, HT=${CM_HAVE_HT}, AYFLY=${CM_HAVE_AYFLY}, ZXTUNE=${CM_HAVE_ZXTUNE})")

# ---------------------------------------------------------------------------
# macOS & Architecture Initialization (Must run BEFORE target definitions)
# ---------------------------------------------------------------------------
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    list(APPEND CMAKE_FRAMEWORK_PATH "/opt/homebrew/Frameworks")

    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)

    # Vendored LGPLv2.1 FFmpeg (decode-only, App-Store compliant). Prepended so
    # the libav* headers AND dylibs resolve here instead of Homebrew's GPL build
    # -- fully decouples the ffmpeg dependency from /opt/homebrew. Sonames match
    # (avcodec.62/avformat.62/avutil.60/swresample.6), so the bare avcodec/...
    # link names below resolve to the vendored libs. The dylib ids are
    # @executable_path/... so they resolve inside the .app bundle; for running
    # from the build tree they are copied next to each binary (POST_BUILD below).
    # See external/ffmpeg-lgpl/README.md for provenance + license.
    set(FFMPEG_LGPL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/ffmpeg-lgpl")
    if(EXISTS "${FFMPEG_LGPL_DIR}/lib/libavcodec.62.dylib")
        include_directories(BEFORE "${FFMPEG_LGPL_DIR}/include")
        link_directories(BEFORE "${FFMPEG_LGPL_DIR}/lib")
        set(FFMPEG_LGPL_LIBS
            "${FFMPEG_LGPL_DIR}/lib/libavcodec.62.dylib"
            "${FFMPEG_LGPL_DIR}/lib/libavformat.62.dylib"
            "${FFMPEG_LGPL_DIR}/lib/libavutil.60.dylib"
            "${FFMPEG_LGPL_DIR}/lib/libswresample.6.dylib")
    else()
        message(WARNING "Vendored LGPL FFmpeg not found at ${FFMPEG_LGPL_DIR}/lib "
                        "-- falling back to Homebrew's (GPL) ffmpeg. Build the "
                        "vendored libs via external/ffmpeg-lgpl/build_lgpl_ffmpeg.sh.")
    endif()

    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    endif()
endif()

# Global overrides
set(ZLIB_LIBRARIES z CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# FORCE GLOBAL SILENCING OF INTERFACE OVERRIDE WARNINGS (FIXES RSNPLUGIN LEAK)
if(APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-inconsistent-missing-override)
endif()

# Compiler Cache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Subproject Paths
set(AP1_ROOT external/apone CACHE PATH "Where apone is checked out")
set(MUSICPLAYER external/musicplayer CACHE PATH "Where musicplayer is checked out")
get_filename_component(VICE_ROOT "${CMAKE_SOURCE_DIR}/external/vice310" ABSOLUTE)
set(VICE_ROOT ${VICE_ROOT} CACHE PATH "Where vice is checked out")

include(${AP1_ROOT}/cmake/Utils.cmake)

# Platform specific flag adjustments
if(RASPBERRYPI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
endif()

# ---------------------------------------------------------------------------
# Unified Warning Profiles & AppleClang Tweaks
# ---------------------------------------------------------------------------
set(GCC_CLANG_WARNINGS -Wall -Wextra -Wnon-virtual-dtor -pedantic)

if(APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND GCC_CLANG_WARNINGS -Wno-deprecated-declarations -Wno-string-compare -Wno-pointer-sign -Wno-inconsistent-missing-override)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    string(APPEND CMAKE_CXX_FLAGS " -fno-common")
    string(APPEND CMAKE_C_FLAGS " -fno-common")
    add_link_options("-Wl,-no_warn_duplicate_libraries")
endif()

set(GCC_WARNINGS
    -Wduplicated-branches
    -Wduplicated-cond
    -Wlogical-op
    -Wnull-dereference
    -Wcast-align
    -Wno-missing-field-initializers)

set(MSVC_WARNINGS /W4)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(WARNINGS ${WARNINGS} ${GCC_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(WARNINGS ${WARNINGS} ${CLANG_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(WARNINGS ${WARNINGS} ${MSVC_WARNINGS})
endif()

add_library(Warnings INTERFACE)
target_compile_options(Warnings INTERFACE ${WARNINGS})

if(NOT WINDOWS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -funsigned-char")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -funsigned-char")

    # Keep the developer's absolute source paths OUT of the shipped binary.
    #
    # CMake compiles with absolute paths, so every __FILE__ baked in by an
    # assert/LOGD macro lands in __TEXT as a full
    # /Users/<you>/Documents/chipmachine-as/... string. Those survive `strip`
    # (they are ordinary string constants, not debug info), so the packaging
    # step alone cannot remove them -- it has to be done here, at compile time.
    #
    # -ffile-prefix-map rewrites the prefix in BOTH places it appears:
    #   * __FILE__ / __BASE_FILE__          (the -fmacro-prefix-map half)
    #   * DWARF and the linker's debug map  (the -fdebug-prefix-map half)
    # and, as a side effect that matters here, the source location clang embeds
    # in __PRETTY_FUNCTION__ for lambdas -- which is how sol3's ctti type names
    # were leaking paths of their own.
    #
    # The replacement is a SYNTHETIC ABSOLUTE ROOT ("/chipmachine-as/..."), not
    # a relative path, and the mapped-from key deliberately has NO trailing
    # slash. That is a hard requirement, not a style choice:
    # external/zxtune/include/source_location.h turns __FILE__ into a template
    # parameter pack and static_asserts that the first character is '/' (it
    # trims SOURCES_ROOT, which this project never defines, so ROOT is empty and
    # the check lands on __FILE__[0]). Mapping to a bare relative path makes
    # every zxtune TU fail to compile with "SOURCES_ROOT should not end with
    # slash". Keeping a leading slash satisfies it and still drops the user name.
    #
    # Paths therefore read "/chipmachine-as/chipmachine/src/main.cpp" -- absolute
    # and unambiguous in logs and asserts, but with nothing machine-specific.
    #
    # The build dir needs its own entry only when it sits outside the repo; an
    # in-tree build/ or build-mas/ is already covered by the repo-root mapping.
    #
    # Verify after a build with:
    #   strings -a build-mas/chipmachine | grep -c "$HOME"     # expect 0
    get_filename_component(CM_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        string(APPEND CMAKE_CXX_FLAGS
               " -ffile-prefix-map=${CM_REPO_ROOT}=/chipmachine-as")
        string(APPEND CMAKE_C_FLAGS
               " -ffile-prefix-map=${CM_REPO_ROOT}=/chipmachine-as")
        string(FIND "${CMAKE_BINARY_DIR}" "${CM_REPO_ROOT}/" _CM_BUILD_IN_TREE)
        if(NOT _CM_BUILD_IN_TREE EQUAL 0 AND
           NOT CMAKE_BINARY_DIR STREQUAL CM_REPO_ROOT)
            string(APPEND CMAKE_CXX_FLAGS
                   " -ffile-prefix-map=${CMAKE_BINARY_DIR}=/chipmachine-as/build")
            string(APPEND CMAKE_C_FLAGS
                   " -ffile-prefix-map=${CMAKE_BINARY_DIR}=/chipmachine-as/build")
        endif()
    endif()

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")
endif()

# ---------------------------------------------------------------------------
# Subdirectories & Dependency Layout (Processed early)
# ---------------------------------------------------------------------------
add_subdirectory(external/lua SYSTEM)
add_subdirectory(external/sol2 SYSTEM)

set(CORE_MODULES coreutils audioplayer archive webutils sqlite3 xml bbsutils crypto)
set(GUI_MODULES grappix fft)
set(APONE_MODULES ${GUI_MODULES} ${CORE_MODULES})

add_subdirectory(${AP1_ROOT}/mods ap1mods)

# apone vendors the SQLite 3.27 amalgamation, whose SQL round() compares a double
# against LARGEST_INT64-1; that constant is not representable as a double, so
# clang 21 warns that the bound silently becomes 2^63. Nothing in chipmachine
# calls SQL round(), and patching a 200k-line amalgamation would be lost on the
# next apone revendor, so silence it here rather than in apone's own CMakeLists.
if(TARGET sqlite3 AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(sqlite3 PRIVATE -Wno-implicit-const-int-float-conversion)
endif()

# Define core plugin array
set(MUSICPLAYER_PLUGINS
    openmptplugin gmeplugin sndhplugin
    stsoundplugin adplugin mp3plugin hivelyplugin rsnplugin
    zxayplugin s98plugin gsfplugin tedcrplugin v2plugin
    ffmpegplugin fmpplugin pxtoneplugin ptkplugin orgplugin sunvoxplugin eupplugin
    kssplugin quartetplugin wsrplugin bbsongplugin
    soundsmithplugin musxplugin cocoplugin mgtplugin medplugin sbstudioplugin sksplugin nedplugin sccmusixxplugin
    playerproplugin jxsplugin monotoneplugin mikmodplugin ixsplugin copplugin fnkplugin
    libvgmplugin vgmstreamplugin
    victrackerplugin klystrackplugin csidplugin musplugin dmfcrplugin
)
# GPL -- plus build only, see CM_HAVE_UADE above.
if(CM_HAVE_UADE)
    list(APPEND MUSICPLAYER_PLUGINS uadeplugin)
endif()
# GPL-2 (Ayfly + z80ex) -- plus build only, see CM_HAVE_AYFLY above. zxayplugin
# covers the same formats in both variants.
if(CM_HAVE_AYFLY)
    list(APPEND MUSICPLAYER_PLUGINS ayflyplugin)
endif()
# GPL-3 (ZXTune) -- plus build only, see CM_HAVE_ZXTUNE above. zxayplugin covers
# .psm/.ftc/.gtr in both variants; .st11/.tfe/.chi are lost in mas.
if(CM_HAVE_ZXTUNE)
    list(APPEND MUSICPLAYER_PLUGINS zxtuneplugin)
endif()
# GPL (GoatTracker + reSID) -- plus build only, see CM_HAVE_GOATTRACKER above.
if(CM_HAVE_GOATTRACKER)
    list(APPEND MUSICPLAYER_PLUGINS goattrackerplugin)
endif()
# GPL (the whole vendored Furnace engine) -- plus build only, see CM_HAVE_DMF above.
if(CM_HAVE_DMF)
    list(APPEND MUSICPLAYER_PLUGINS dmfplugin)
endif()
# GPL-3 (libsc68 + emu68 + file68 + unice68) -- plus build only, see CM_HAVE_SC68
# above. .sndh is handled by sndhplugin in both variants; only .sc68 needs this.
if(CM_HAVE_SC68)
    list(APPEND MUSICPLAYER_PLUGINS sc68plugin)
endif()
# GPL-2 (ASAP) -- plus build only, see CM_HAVE_POKEYNOISE above. Costs .pn only;
# .sap stays with gmeplugin in both variants.
if(CM_HAVE_POKEYNOISE)
    list(APPEND MUSICPLAYER_PLUGINS pokeynoiseplugin)
endif()
# GPL-2 (mdxmini) -- plus build only, see CM_HAVE_MDX above. Costs all 6,913
# ".mdx" rows; nothing else claims the extension.
if(CM_HAVE_MDX)
    list(APPEND MUSICPLAYER_PLUGINS mdxplugin)
endif()
# GPL-3+ (ScummVM MaxTrax + Paula) -- plus build only, see CM_HAVE_MAXTRAX above.
# Costs all 93 ".mxtx" rows; nothing else claims the extension.
if(CM_HAVE_MAXTRAX)
    list(APPEND MUSICPLAYER_PLUGINS maxtraxplugin)
endif()
# GPL-2+ (FamiTracker CX / jsr's FamiTracker engine) -- plus build only, see
# CM_HAVE_FAMITRACKER above. Costs 1,597 .ftm rows in mas; the 95 Atari "Face The
# Music" .ftm rows stay with OpenMPT in both variants.
if(CM_HAVE_FAMITRACKER)
    list(APPEND MUSICPLAYER_PLUGINS famitrackerplugin)
endif()
# MAME 1997-2008 (non-commercial) + GPL-2 peops -- plus build only, see
# CM_HAVE_AO above. This is what plays PSF/PSF2 now that the Sony BIOS is gone.
if(CM_HAVE_AO)
    list(APPEND MUSICPLAYER_PLUGINS aoplugin)
endif()
# GPL-2 (DeSmuME-derived vio2sf) -- plus build only, see CM_HAVE_NDS above.
if(CM_HAVE_NDS)
    list(APPEND MUSICPLAYER_PLUGINS ndsplugin)
endif()
# GPL-2 (lazyusf2 / Mupen64Plus) -- plus build only, see CM_HAVE_USF above.
if(CM_HAVE_USF)
    list(APPEND MUSICPLAYER_PLUGINS usfplugin)
endif()
# GPL-3 (Highly Theoretical SegaCore) -- plus build only, see CM_HAVE_HT above.
if(CM_HAVE_HT)
    list(APPEND MUSICPLAYER_PLUGINS htplugin)
endif()
# Empty in a mas tree, where the vicepluginbridge target does not exist at all.
if(CM_HAVE_VICE)
    set(CM_VICE_LIB vicepluginbridge)
else()
    set(CM_VICE_LIB "")
endif()
add_subdirectory(${MUSICPLAYER}/src/plugins plugins)
add_subdirectory(${MUSICPLAYER}/src/psf psf)

include_directories(${AP1_ROOT}/mods/grappix ${MUSICPLAYER}/plugins)

# Put src/ on the header search path so platform-native sources living in
# subdirectories (e.g. src/macnative/*.mm) can include the shared app headers
# ("ChipMachine.h", "version.h", ...) with plain quotes, exactly as the
# top-level src/*.cpp files do by same-directory resolution.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# liblhasa (vendored under zxtune) decompresses the .lha archives that the
# UnExoticA collection ships its tunes in. It is ISC-licensed (Simon Howard)
# and only HAPPENS to live inside the zxtune tree -- it is not part of ZXTune
# and is needed in BOTH variants. In a plus tree the `lhasa` static-lib target
# is created by the zxtuneplugin subproject; in a mas tree that subproject does
# not exist (see CM_HAVE_ZXTUNE), so the target has to be created here instead.
get_filename_component(ZXTUNE_ROOT "${CMAKE_SOURCE_DIR}/external/zxtune" ABSOLUTE)
include_directories(${ZXTUNE_ROOT}/3rdparty/lhasa/lib/public)
if(NOT CM_HAVE_ZXTUNE AND NOT TARGET lhasa)
    add_subdirectory(${ZXTUNE_ROOT}/3rdparty/lhasa lhasa_standalone)
endif()

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif()

add_library(musicplayer INTERFACE)
add_library(plugin_register STATIC src/plugin_register.cpp)
# register_plugins() calls uadeplugin_register() unless CM_NO_UADE is set, and
# the symbol only exists when the uadeplugin target was built (see CM_HAVE_UADE).
if(NOT CM_HAVE_UADE)
    target_compile_definitions(plugin_register PRIVATE CM_NO_UADE)
endif()
# Same for ayflyplugin_register() -- see CM_HAVE_AYFLY above.
if(NOT CM_HAVE_AYFLY)
    target_compile_definitions(plugin_register PRIVATE CM_NO_AYFLY)
endif()
# Same for zxtuneplugin_register() -- see CM_HAVE_ZXTUNE above.
if(NOT CM_HAVE_ZXTUNE)
    target_compile_definitions(plugin_register PRIVATE CM_NO_ZXTUNE)
endif()
# Same for vicepluginbridge_register() -- see CM_HAVE_VICE above.
if(NOT CM_HAVE_VICE)
    target_compile_definitions(plugin_register PRIVATE CM_NO_VICE)
    # csidplugin also needs it: with VICE present it hands self-IRQ RSIDs back to
    # VICE, and without VICE it must keep them (see CSIDPlugin::canHandle).
    target_compile_definitions(csidplugin PRIVATE CM_NO_VICE)
endif()
# Same for goattrackerplugin_register() -- see CM_HAVE_GOATTRACKER above.
if(NOT CM_HAVE_GOATTRACKER)
    target_compile_definitions(plugin_register PRIVATE CM_NO_GOATTRACKER)
endif()
# Same for dmfplugin_register() -- see CM_HAVE_DMF above.
if(NOT CM_HAVE_DMF)
    target_compile_definitions(plugin_register PRIVATE CM_NO_DMF)
endif()
# Same for sc68plugin_register() -- see CM_HAVE_SC68 above.
if(NOT CM_HAVE_SC68)
    target_compile_definitions(plugin_register PRIVATE CM_NO_SC68)
endif()
# Same for pokeynoiseplugin_register() -- see CM_HAVE_POKEYNOISE above.
if(NOT CM_HAVE_POKEYNOISE)
    target_compile_definitions(plugin_register PRIVATE CM_NO_POKEYNOISE)
endif()
# Same for mdxplugin_register() -- see CM_HAVE_MDX above.
if(NOT CM_HAVE_MDX)
    target_compile_definitions(plugin_register PRIVATE CM_NO_MDX)
endif()
# Same for famitrackerplugin_register() -- see CM_HAVE_FAMITRACKER above.
if(NOT CM_HAVE_FAMITRACKER)
    target_compile_definitions(plugin_register PRIVATE CM_NO_FAMITRACKER)
endif()
# Same for maxtraxplugin_register() -- see CM_HAVE_MAXTRAX above.
if(NOT CM_HAVE_MAXTRAX)
    target_compile_definitions(plugin_register PRIVATE CM_NO_MAXTRAX)
endif()
# Same for aoplugin_register() -- see CM_HAVE_AO above.
if(NOT CM_HAVE_AO)
    target_compile_definitions(plugin_register PRIVATE CM_NO_AO)
endif()
# Same for ndsplugin_register() / usfplugin_register() / htplugin_register().
if(NOT CM_HAVE_NDS)
    target_compile_definitions(plugin_register PRIVATE CM_NO_NDS)
endif()
if(NOT CM_HAVE_USF)
    target_compile_definitions(plugin_register PRIVATE CM_NO_USF)
endif()
if(NOT CM_HAVE_HT)
    target_compile_definitions(plugin_register PRIVATE CM_NO_HT)
endif()
target_link_libraries(musicplayer INTERFACE psf plugin_register)
target_include_directories(musicplayer INTERFACE ${MUSICPLAYER}/..)

if(APPLE)
    set(APPLE_FRAMEWORKS 
        "-framework CoreFoundation" 
        "-framework IOKit" 
        "-framework Cocoa" 
        "-framework AudioToolbox" 
        "-framework AudioUnit"
        "-framework Foundation"
        "-framework AppKit"
    )
endif()

# Source file layout declarations
set(MAIN_FILES
    src/MusicDatabase.cpp
    src/GZPlugin.cpp
    src/LhaArchive.cpp
    src/MusicPlayer.cpp
    src/MusicPlayerList.cpp
    src/RemoteLoader.cpp
    src/SearchIndex.cpp
    src/SongFileIdentifier.cpp
    src/state_machine.cpp
    src/youtube.cpp
    src/textmode.cpp
    src/macnative/CheckForUpdate.mm
    src/macnative/NativeDialogs.mm
    src/macnative/FileOpenHandler.mm
)

set(GUI_FILES
    src/ChipMachine.cpp
    src/MusicBars.cpp
    src/ChipMachine_config.cpp
    src/ChipMachine_keys.cpp
    src/ChipMachine_commands.cpp
    src/textmode.cpp
    demofx/Transitions.cpp
)

set(SOURCE_FILES ${SOURCE_FILES} src/version.h src/TextField.h src/TextListView.h src/CueSheet.h
    src/Dialog.h src/LineEdit.h src/Icons.h src/SongInfo.h src/SongInfoField.h src/ChipInterface.h)

file(GLOB DATA_FILES data/*.txt)
file(GLOB LUA_FILES lua/*.lua)
file(GLOB DOC_FILES *.md)
file(GLOB SCRIPT_FILES scripts/*)

add_custom_target(chipmachine.data_files SOURCES ${DATA_FILES})
add_custom_target(chipmachine.lua_files SOURCES ${LUA_FILES})
add_custom_target(chipmachine.doc_files SOURCES ${DOC_FILES})
add_custom_target(chipmachine.script_files SOURCES ${SCRIPT_FILES})

# ---------------------------------------------------------------------------
# Targets Execution definitions (Direct file-level dependency tracking)
# ---------------------------------------------------------------------------

if(APPLE)
    add_executable(chipmachine MACOSX_BUNDLE ${MAIN_FILES} ${GUI_FILES} src/main.cpp)

    # Existing: copy the compiled binary next to the build dir for direct invocation
    add_custom_command(TARGET chipmachine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:chipmachine>
            ${CMAKE_CURRENT_BINARY_DIR})

else()
    add_executable(chipmachine ${MAIN_FILES} ${GUI_FILES} src/main.cpp)
endif()

target_link_libraries(chipmachine PRIVATE
    musicplayer
    plugin_register
    ${CORE_MODULES}
    ${GUI_MODULES}
    ${MUSICPLAYER_PLUGINS}
    lhasa
    ${CM_VICE_LIB}
    sol
    ${APPLE_FRAMEWORKS}
    avcodec
    avformat
    avutil
    swresample
)
target_compile_definitions(chipmachine PRIVATE -DAPP_NAME=chipmachine)
target_compile_definitions(chipmachine PRIVATE ${CM_VARIANT_DEFS})

if(NOT WIN32)
    add_executable(cm ${MAIN_FILES} src/textmode.cpp src/main.cpp)
    target_compile_definitions(cm PUBLIC TEXTMODE_ONLY)
    target_compile_definitions(cm PRIVATE ${CM_VARIANT_DEFS})
    target_link_libraries(cm PRIVATE
        musicplayer
        ${CORE_MODULES}
        ${MUSICPLAYER_PLUGINS}
        lhasa
        ${CM_VICE_LIB}
        sol
        ${APPLE_FRAMEWORKS}
        avcodec
        avformat
        avutil
        swresample
        )
endif()

add_executable(mksonglist scripts/mksonglist/mksonglist.cpp src/SongFileIdentifier.cpp)
# sndhplugin, not sc68plugin: SongFileIdentifier::parseSndh has to depack
# ICE!-packed SNDH files before it can read their tags, and it now does that
# through musix::sndh::iceDepack (public-domain ice_24.c, vendored with
# AtariAudio) instead of unice68 (GPL-3, and absent from a mas tree).
target_link_libraries(mksonglist PRIVATE coreutils archive mp3plugin sndhplugin)
# ...and the include root that makes <musicplayer/src/plugins/...> resolve. The
# app targets inherit it from the `musicplayer` INTERFACE library; mksonglist
# does not link that, so it needs the path directly.
target_include_directories(mksonglist PRIVATE ${MUSICPLAYER}/..)

# cmtest is built for the plus tree only. test.cpp has ~20 hard references to
# musix::UADEPlugin (routing, prefix-collision and companion-file cases that are
# specifically ABOUT UADE), and in a mas tree that target does not exist. The
# suite is variant-independent otherwise -- mas is plus minus UADE and YouTube,
# and every line of shared code is covered by the plus run -- so run the tests
# from the plus build tree (./cmtest in build/) and configure -DCM_VARIANT=mas
# purely to produce the App Store binary.
if(CM_HAVE_UADE)
add_executable(cmtest testmain.cpp test.cpp test_opna_rhythm.cpp ${MAIN_FILES})
target_link_libraries(cmtest PRIVATE
    plugin_register
    ${CORE_MODULES}
    ${MUSICPLAYER_PLUGINS}
    lhasa
    vicepluginbridge
    musicplayer
    sol
    ${APPLE_FRAMEWORKS}
    avcodec
    avformat
    avutil
    swresample
)
target_include_directories(cmtest PRIVATE ${APONE}/mods
    ${MUSICPLAYER}/src/plugins/s98plugin/m_s98/device/fmgen)
target_compile_definitions(cmtest PRIVATE -DTESTING)
else()
    message(STATUS "cmtest: not built for the mas variant (test.cpp depends on "
                   "uadeplugin); run the suite from the plus build tree")
endif()

# The SunVox engine is a prebuilt shared library that the plugin dlopen()s at
# runtime from next to the executable. Copy it beside each binary that can play
# music so SunVoxPlugin can find it (see SUNVOX_RUNTIME_LIB in sunvoxplugin).
if(SUNVOX_RUNTIME_LIB)
    foreach(_sv_target chipmachine cm cmtest)
        if(TARGET ${_sv_target})
            add_custom_command(TARGET ${_sv_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SUNVOX_RUNTIME_LIB}
                    $<TARGET_FILE_DIR:${_sv_target}>)
        endif()
    endforeach()
endif()

# The vendored LGPL libav dylibs have @executable_path install ids, so a binary
# run from the build tree needs them sitting next to it. Copy them beside each
# playable target (the packaged .app gets them in Contents/MacOS/ via
# package_app.sh). Same pattern as the SunVox runtime lib above.
if(APPLE AND FFMPEG_LGPL_LIBS)
    foreach(_ff_target chipmachine cm cmtest)
        if(TARGET ${_ff_target})
            add_custom_command(TARGET ${_ff_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${FFMPEG_LGPL_LIBS}
                    $<TARGET_FILE_DIR:${_ff_target}>)
        endif()
    endforeach()
endif()

# --- EXTERNAL STUFF COMPILER WARNING SUPPRESSION ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(SILENT_TARGETS
        vicepluginbridge ndsplugin usfplugin adplugin csidplugin
        aoplugin v2plugin unrar coreutils lua crypto tedcrplugin gmeplugin
        openmptplugin sc68plugin ayflyplugin mdxplugin s98plugin ffmpegplugin rsnplugin fmpplugin
    )
    if(CM_HAVE_UADE)
        list(APPEND SILENT_TARGETS uadeplugin)
    endif()
    # sndhplugin and gsfplugin silence their vendored engines (AtariAudio, mGBA)
    # per-file in their own CMakeLists, so that our SNDHPlugin.cpp /
    # GSFPlugin.cpp stay strict; neither is listed here. sc68plugin is, and the
    # loop below already skips targets that do not exist -- which is what a mas
    # tree has. Note also that -w here would not reach gsfplugin anyway: it is
    # an `ld -r` wrapper around gsfplugin_objs, which compiles nothing itself.
    foreach(target_item ${SILENT_TARGETS})
        if(TARGET ${target_item})
            target_compile_options(${target_item} PRIVATE -w)
        endif()
    endforeach()
endif()

target_compile_options(adplugin PRIVATE -fsigned-char)

# ---------------------------------------------------------------------------
# Bundle install() setup — used by cmake --install and as a reference layout.
# Note: package_app.sh is the authoritative release packager; this block
# keeps `cmake --install` consistent and documents the expected asset layout.
# ---------------------------------------------------------------------------
if(APPLE)
    set(APPS ${CMAKE_CURRENT_BINARY_DIR}/Chipmachine.app)
    set(DIRS "")
    install(CODE "
       file(COPY \"\${CMAKE_SOURCE_DIR}/lua\"  DESTINATION Chipmachine.app/Contents/Resources/)
       file(COPY \"\${CMAKE_SOURCE_DIR}/data\" DESTINATION Chipmachine.app/Contents/Resources/)
       file(MAKE_DIRECTORY Chipmachine.app/Contents/Resources/bin)
       file(COPY \"\${CMAKE_SOURCE_DIR}/Info.plist\" DESTINATION Chipmachine.app/Contents/)

       # NB: no music/Console or music/hvtc step here. db.lua VERSION 129 and 130
       # un-bundled HVTC and NSFE -- no music may ship inside the app for Mac App
       # Store submission. Their tracks are fetched on demand (hvtc from
       # plus4world, nsfe from an archive.org ZIP), like hvsc. Only music/projectay
       # is still bundled, by package_app.sh.

       include(BundleUtilities)
       fixup_bundle(\"\${APPS}\" \"\" \"\$\\\{DIRS\}\")
       file(COPY \"\${CMAKE_SOURCE_DIR}/bin/ffmpeg\" DESTINATION Chipmachine.app/Contents/Resources/bin/)
       " COMPONENT Runtime)
endif()

# ---------------------------------------------------------------------------
# Build timer: print total wall-clock time at the very end of a `ninja` build.
#
# `build_timer_start` has no dependencies, so Ninja schedules it in the first
# wave and it stamps the start epoch. `build_timer_end` depends on every app
# target, so it runs last and prints the elapsed time. Both are ALL targets and
# always out of date (no file outputs), so a default `ninja` always prints a
# timing line -- meaning it no longer short-circuits to "no work to do". Doing it
# silently-on-no-op is only possible with a shell wrapper around ninja, which we
# deliberately avoid; this always-print tradeoff is accepted. Neither target is a
# dependency OF the app targets, so this never forces a relink.
#
# Note: a targeted build (e.g. `ninja chipmachine` / `cmake --build --target
# chipmachine`, as package_app.sh uses) does NOT pull in these ALL targets, so
# no line prints there -- package_app.sh does its own end-to-end timing instead.
# ---------------------------------------------------------------------------
if(UNIX)
    set(_BUILD_TIMER_STAMP "${CMAKE_BINARY_DIR}/.build_start_epoch")
    add_custom_target(build_timer_start ALL
        COMMAND sh -c "date +%s > '${_BUILD_TIMER_STAMP}'"
        COMMENT "Build timer started"
        VERBATIM)
    add_custom_target(build_timer_end ALL
        COMMAND sh -c "if [ -f '${_BUILD_TIMER_STAMP}' ]; then _s=$(cat '${_BUILD_TIMER_STAMP}'); _e=$(date +%s); _d=$((_e - _s)); printf '=== Build finished in %dm %02ds ===\\n' $((_d / 60)) $((_d % 60)); fi"
        COMMENT "Build time"
        VERBATIM)
    # end must run AFTER start (to read a fresh stamp) and after every app target
    # (so it is genuinely last). start has no deps, so it runs in the first wave.
    add_dependencies(build_timer_end build_timer_start)
    foreach(_timed_target chipmachine cm cmtest mksonglist)
        if(TARGET ${_timed_target})
            add_dependencies(build_timer_end ${_timed_target})
        endif()
    endforeach()
endif()
