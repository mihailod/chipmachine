cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.victrackerplugin)

# VIC-TRACKER (.vt) player -- the modland "Vic-Tracker" corpus: Commodore VIC-20
# music in Daniel Kahlin's tracker format. We run the tracker's own 6502 replay
# routine (player.asm, BSD, vendored pre-assembled as vtplayer_bin.h) on the
# fake6502 core (GPLv2) and route its VIC sound-register writes into the VIC-I
# sound core lifted from VICE's vic20sound.c (GPLv2, vic_sound.c). See
# vt_machine.cpp and victracker/ for the vendored player sources + rebuild script.

add_library(victrackerplugin
    VTPlugin.cpp
    vt_machine.cpp
    fake6502.c
    vic_sound.c
    VTPlugin.h
    vt_machine.h
    vic_sound.h
    vtplayer_bin.h)

# fake6502 selects its opcode variant at compile time; the VIC-20 is an NMOS 6502.
set_source_files_properties(fake6502.c PROPERTIES COMPILE_DEFINITIONS NMOS6502)

target_link_libraries(victrackerplugin PRIVATE coreutils)
