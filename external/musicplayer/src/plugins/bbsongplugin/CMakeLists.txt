cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.bbsongplugin)

# Beepola .bbsong (ZX Spectrum 1-bit beeper) player. Parses the .bbsong format,
# packs the song into a beeper engine's data layout, and runs the engine's Z80
# player on an embedded Z80 core while sampling the speaker (port 0xFE).
add_library(bbsongplugin
    BBSongPlugin.cpp
    bbsong_parser.cpp
    z80_machine.cpp
    z80asm.cpp
    phaser1_pack.cpp
    musicbox_pack.cpp
    musicstudio_pack.cpp
    sfx_pack.cpp
    BBSongPlugin.h
    bbsong_parser.h
    z80_machine.h
    z80asm.h
    phaser1_pack.h
    musicbox_pack.h
    musicstudio_pack.h
    sfx_pack.h
    sfx_player.h)

# We reuse the GME plugin's Z80 core (Z80_Cpu.{h,cpp} + blargg headers) rather
# than vendoring a second copy: z80_machine.cpp #includes its run body, and the
# class methods resolve at final link against gmeplugin's Z80_Cpu.o (which gme's
# own SGC player already pulls in). We only need its headers on the include path.
target_include_directories(bbsongplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../gmeplugin)

target_link_libraries(bbsongplugin PRIVATE coreutils)
