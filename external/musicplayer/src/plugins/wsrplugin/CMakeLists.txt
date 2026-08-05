cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.wsrplugin)

# WonderSwan / WonderSwan Color music (.wsr), played by emulating the machine:
#
#   v30mz/  NEC V30MZ CPU, vendored from ares (ISC) -- see its PROVENANCE.md
#   wswan/  the machine (memory map, bank registers, blank timers, interrupts,
#           general DMA) and the APU, both written for this project from the
#           WSdev Wiki -- see wswan/README.md
#
# This replaces the in_wsr replayer that used to live here (GPL-2.0+, cut from
# OSWAN 0.70), which was the last GPL code in this plugin. ws_apu.c is ALSO
# compiled into libvgmplugin, which plays the same chip inside VGM logs; that
# slice is partial-linked with `ld -r`, so the duplicate is local to it.

set(WSR_V30MZ ${CMAKE_CURRENT_SOURCE_DIR}/v30mz)
set(WSR_WSWAN ${CMAKE_CURRENT_SOURCE_DIR}/wswan)

# v30mz.cpp #includes every other .cpp in that directory, ares-style, so it is
# the only translation unit here.
set(WSR_CORE_SOURCES
    ${WSR_V30MZ}/v30mz.cpp
    ${WSR_V30MZ}/v30mz_selftest.cpp
    ${WSR_WSWAN}/ws_machine.cpp
    ${WSR_WSWAN}/ws_apu.c
)

add_library(wsrplugin STATIC WSRPlugin.cpp ${WSR_CORE_SOURCES})

set_property(TARGET wsrplugin PROPERTY CXX_STANDARD 17)
set_property(TARGET wsrplugin PROPERTY CXX_STANDARD_REQUIRED ON)

# ares' core is third-party; build it quietly. Everything here lives in the
# `ares` or `wswan` namespace (or is static), so there are no generic C globals
# to rename the way the old in_wsr core needed -- but keep the symbols hidden so
# they cannot clash with the other WonderSwan copy inside libvgmplugin.
set_source_files_properties(${WSR_CORE_SOURCES} PROPERTIES
    COMPILE_FLAGS "-w -fvisibility=hidden")

target_link_libraries(wsrplugin PRIVATE coreutils)
