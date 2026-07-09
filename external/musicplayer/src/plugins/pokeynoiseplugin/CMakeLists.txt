cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.pokeynoiseplugin)

# Vendored ASAP (Another Slight Atari Player) 6502 + POKEY emulation core.
# https://github.com/epi/asap  (C) 2005-2023 Piotr Fusik, GPL.
add_library(pokeynoiseplugin PokeyNoisePlugin.cpp asap/asap.c PokeyNoisePlugin.h)

target_include_directories(pokeynoiseplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/asap)
target_link_libraries(pokeynoiseplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(pokeynoiseplugin PRIVATE m)
endif()
