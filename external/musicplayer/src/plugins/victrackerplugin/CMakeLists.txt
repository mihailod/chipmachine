cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.victrackerplugin)

# VIC-TRACKER (.vt) player -- the modland "Vic-Tracker" corpus: Commodore VIC-20
# music in Daniel Kahlin's tracker format. We run the tracker's own 6502 replay
# routine (player.asm, BSD, vendored pre-assembled as vtplayer_bin.h) on the
# MyLittle6502 CPU core (pd6502.h, public domain / CC0) and route its VIC
# sound-register writes into our own VIC-I sound core (vic_sound.c). See
# vt_machine.cpp and victracker/ for the vendored player sources + rebuild script.
#
# Nothing here is GPL, so the plugin builds identically in both variants -- see
# README.md for what the two cores replaced and why.

add_library(victrackerplugin
    VTPlugin.cpp
    vt_machine.cpp
    vt_cpu.c
    vic_sound.c
    VTPlugin.h
    vt_machine.h
    vt_cpu.h
    pd6502.h
    vic_sound.h
    vtplayer_bin.h)

target_link_libraries(victrackerplugin PRIVATE coreutils)
