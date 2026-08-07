cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.dmfcrplugin)

# DefleMask .dmf (SEGA Genesis + Master System) -- a CLEAN-ROOM parser and
# sequencer written from DefleMask's own published documentation: the DMF_SPECS_0x11/12/13/15/16/18
# format documents and the effect tables in DefleMask's own tracker manual, all
# archived verbatim under spec/. No code, and no reading, from Furnace
# (tildearrow/furnace, GPL-2.0-or-later) -- which matters, because Furnace is
# the ONLY other engine that reads .dmf and it is what dmfcrplugin exists to
# replace in the Mac App Store build. See README.md.
#
# Ships in BOTH variants. In the plus build dmfplugin (Furnace) registers first
# and wins the extension, so plus playback is unchanged and remains available as
# an A/B reference; in mas, where dmfplugin is absent (CM_HAVE_DMF), this plugin
# is the only claimant. See plugin_register.cpp.
#
# Chip cores, both permissive:
#   * YM2612 -- Aaron Giles' ymfm (BSD-3, external/ymfm), the same vendored copy
#     libvgmplugin already uses for its OPN adapters. NOT Nuked-OPN2: that is
#     LGPL, which static linking into an App Store binary cannot satisfy.
#   * SN76489 -- sn76489.cpp, written here from the published hardware
#     description. libvgm's sn76496.c is BSD-3 and would have been fine on
#     licence, but it is welded to libvgm's device framework; the chip is four
#     counters and an LFSR, so writing it was cheaper than vendoring that.
#     Same reasoning as victrackerplugin's VIC-I core.

set(YMFM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ymfm)

find_package(ZLIB REQUIRED)   # .dmf payloads are zlib-wrapped

add_library(dmfcrplugin
    DMFCRPlugin.cpp
    DMFCRPlugin.h
    dmf_file.cpp
    dmf_file.h
    dmf_player.cpp
    dmf_player.h
    sn76489.cpp
    sn76489.h
    # ymfm: only the OPN family is needed for the YM2612. ymfm_opn.cpp includes
    # ymfm_fm.ipp and instantiates the fm_engine templates there, so nothing
    # else in this target may include that .ipp -- see the same note in
    # libvgmplugin/opn_ymfm.cpp.
    ${YMFM_DIR}/src/ymfm_opn.cpp
    ${YMFM_DIR}/src/ymfm_adpcm.cpp
    ${YMFM_DIR}/src/ymfm_ssg.cpp
    ${YMFM_DIR}/src/ymfm_pcm.cpp)

target_include_directories(dmfcrplugin PRIVATE ${YMFM_DIR}/src)
target_link_libraries(dmfcrplugin PRIVATE coreutils ZLIB::ZLIB)
