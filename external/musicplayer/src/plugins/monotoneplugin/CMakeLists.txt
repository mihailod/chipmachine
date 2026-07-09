cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.monotoneplugin)

# Vendored PTPlayer: a player for the Polytone (.pol) format that also loads the
# legacy MONOTONE (.mon) PC-speaker tracker format.
# https://github.com/prochazkaml/PTPlayer  (C) 2021-2025 Michal Prochazka, BSD-3.
add_library(monotoneplugin MonotonePlugin.cpp ptplayer/ptplayer.c MonotonePlugin.h)

target_include_directories(monotoneplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(monotoneplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(monotoneplugin PRIVATE m)
endif()
