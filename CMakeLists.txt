cmake_minimum_required(VERSION 3.15)

# Auto-detect macOS deployment target from host OS (can be overridden with -D)
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    execute_process(COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE _MACOS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "^[0-9]+\\.[0-9]+" _MACOS_MAJOR_MINOR "${_MACOS_VERSION}")
    if(_MACOS_MAJOR_MINOR)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "${_MACOS_MAJOR_MINOR}" CACHE STRING "macOS Deployment Target")
    endif()
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
# The exposure is 2,071 songs (0.28% of the catalog), dropped from the mas index
# by songFormatHasNoPlayer() in MusicDatabase.cpp. Content-probed, not guessed:
# ".dmf" is TWO unrelated formats and libopenmpt plays the other one, so only the
# rows whose format column names DefleMask are hidden -- modland "Deflemask"
# (1,807) + BotB "DefleMask" (263) + demozoo "NEC PC Engine" (1). The X-Tracker
# DDMF rows stay indexed and keep playing: "X-Tracker" (366), "DSMI Compact"
# (13), the generic "DMF" bucket (328, amp + modarchive -- sampled and 100%
# DDMF) and demozoo "Demoscene" (16, all 16 fetched and all DDMF).
#
# PARTIALLY REVERSIBLE, and meant to be reversed. A clean-room non-GPL .dmf
# player is a live follow-up project. The chip cores needed are already
# available under permissive licences (ymfm/BSD for YM2612 + YM2610, SameBoy/MIT
# for Game Boy, sn76496/BSD for SN76489, and the WTFPL cSID already vendored
# here for the C64 rows); what has to be written is the sequencer. By measured
# target share of the hidden pool -- Genesis 46%, Game Boy 16%, PC Engine 13%,
# Neo Geo 10%, NES 6%, C64 5%, SMS 3% -- Genesis alone would resurface ~950
# songs. When a target lands, DELETE the matching formatPlayer row rather than
# adding a second gate: the rows come straight back on the next reindex.
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
# victrackerplugin (.vt, VIC-20) needed NO variant gate, unlike the four above.
# Its GPL was internal rather than structural -- the plugin is ours, but it drove
# two GPLv2 engine cores (the omarandlorraine fake6502 fork, and a VIC-I sound
# core lifted from VICE's vic20sound.c). Both were replaced in place, by
# MyLittle6502 (public domain / CC0) and by a VIC-I written against the documented
# chip; the replacements were A/B'd by ear against the originals and shipped to
# both variants, so there is nothing to gate and all 11 .vt songs stay in the mas
# index. See the plugin's README.md.
message(STATUS "ChipMachine build variant: ${CM_VARIANT} (PROGRAM_NAME=\"${CM_PROGRAM_NAME}\", UADE=${CM_HAVE_UADE}, VICE=${CM_HAVE_VICE}, GOATTRACKER=${CM_HAVE_GOATTRACKER}, DMF=${CM_HAVE_DMF}, SC68=${CM_HAVE_SC68}, POKEYNOISE=${CM_HAVE_POKEYNOISE}, AO=${CM_HAVE_AO}, NDS=${CM_HAVE_NDS}, USF=${CM_HAVE_USF}, HT=${CM_HAVE_HT})")

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

# Define core plugin array
set(MUSICPLAYER_PLUGINS
    openmptplugin gmeplugin sndhplugin
    stsoundplugin adplugin mp3plugin hivelyplugin rsnplugin
    ayflyplugin mdxplugin s98plugin gsfplugin tedplugin v2plugin
    ffmpegplugin fmpplugin pxtoneplugin ptkplugin orgplugin sunvoxplugin eupplugin
    kssplugin quartetplugin wsrplugin zxtuneplugin bbsongplugin
    soundsmithplugin musxplugin cocoplugin mgtplugin medplugin sbstudioplugin maxtraxplugin sksplugin nedplugin sccmusixxplugin
    playerproplugin jxsplugin monotoneplugin mikmodplugin ixsplugin copplugin fnkplugin
    famitrackerplugin libvgmplugin vgmstreamplugin
    victrackerplugin klystrackplugin csidplugin musplugin
)
# GPL -- plus build only, see CM_HAVE_UADE above.
if(CM_HAVE_UADE)
    list(APPEND MUSICPLAYER_PLUGINS uadeplugin)
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
# UnExoticA collection ships its tunes in. The `lhasa` static-lib target is
# created by the zxtuneplugin subproject above.
get_filename_component(ZXTUNE_ROOT "${CMAKE_SOURCE_DIR}/external/zxtune" ABSOLUTE)
include_directories(${ZXTUNE_ROOT}/3rdparty/lhasa/lib/public)

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
        aoplugin v2plugin unrar coreutils lua crypto tedplugin gmeplugin
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
