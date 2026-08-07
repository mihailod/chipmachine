cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.tedcrplugin)

# Commodore 264 series (C16 / 116 / Plus/4) TED music, `.prg`. Clean-room: our
# own TED sound core (ted_sound.c, written against a measured specification) and
# our own cut-down 264 machine (ted_machine.cpp) on the MyLittle6502 CPU core
# (pd6502.h, public domain / CC0). No Commodore ROM images -- the handful of
# KERNAL entry points the corpus calls are re-implemented in a synthetic high
# page. See README.md for the derivation and the differential results.
#
# Nothing here is GPL, so the plugin builds identically in both variants -- which
# is the whole point, since it replaces tedplay.

add_library(tedcrplugin
    TEDCRPlugin.cpp
    ted_machine.cpp
    ted_cpu.c
    ted_sound.c
    TEDCRPlugin.h
    ted_machine.h
    ted_cpu.h
    ted_sound.h
    pd6502.h)

target_link_libraries(tedcrplugin PRIVATE coreutils)
