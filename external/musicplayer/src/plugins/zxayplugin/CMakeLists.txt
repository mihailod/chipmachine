cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.zxayplugin)

# ZX Spectrum AY tracker formats with no copyleft anywhere in the chain, so the
# Mac App Store build keeps them when Ayfly (GPL-2) is gated out. See
# players/PROVENANCE.md for where the Z80 replay routines come from.
add_library(zxayplugin
    ZXAYPlugin.cpp
    zx_ay_machine.cpp
    zxay_format.cpp
    zxay_source.cpp
    zxay_native.cpp
    zxay_loop.cpp
    zxay_stc.cpp
    zxay_stp.cpp
    zxay_sqt.cpp
    zxay_asc.cpp
    zxay_fxm.cpp
    zxay_dump.cpp
    ayumi/ayumi.c)

# We reuse the GME plugin's Z80 core (Z80_Cpu.{h,cpp} + blargg headers) rather
# than vendoring a second copy -- exactly as bbsongplugin and copplugin do.
# zx_ay_machine.cpp #includes its run body and the class methods resolve at
# final link against gmeplugin's Z80_Cpu.o.
target_include_directories(zxayplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../gmeplugin)

# .vtx is an LH5-packed AY register dump -- the same packing as the .ym files
# stsoundplugin already plays -- so it depacks with StSound's CLzhDepacker
# (BSD-2) instead of a second LZH implementation.
target_include_directories(zxayplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../stsoundplugin/StSoundLibrary)

target_link_libraries(zxayplugin PRIVATE coreutils stsoundplugin)
