cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.musplugin)

# Compute!'s Sidplayer (.mus / .str) -- a CLEAN-ROOM sequencer written from the
# two freely-distributed format write-ups in the Compute's Gazette SID Collection
# (MUS_format_A.txt by Peter Weighill, MUS_format_B.txt by Dick Thornton). No
# code from libsidplayfp / sidplayfp / JSIDPlay2 (all GPL) or from any
# disassembly of the original 1980s player was consulted -- see mus.c and
# README.md. Permissively licensable, which is the point: it lets the Mac App
# Store build play these formats, where the GPL VICE bridge cannot ship.
#
# The SID chip emulation is cSID's (WTFPL), reached through the register-level
# API in csidplugin -- hence the link dependency.
add_library(musplugin
    MusPlugin.cpp
    mus.c
    MusPlugin.h
    mus.h)

target_link_libraries(musplugin PRIVATE coreutils csidplugin)
