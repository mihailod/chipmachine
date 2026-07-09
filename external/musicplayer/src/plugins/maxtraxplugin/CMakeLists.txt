cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.maxtraxplugin)

# Vendored MaxTrax + Paula player from ScummVM (audio/mods/maxtrax.{h,cpp},
# audio/mods/paula.{h,cpp}), GPLv3. maxtrax/compat.h is a thin shim replacing
# the handful of ScummVM dependencies so the two files build standalone.
# poc_main.cpp is a standalone decode harness and is intentionally NOT built.
add_library(maxtraxplugin
    MaxTraxPlugin.cpp
    maxtrax/maxtrax.cpp
    maxtrax/paula.cpp
    MaxTraxPlugin.h)

target_include_directories(maxtraxplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/maxtrax)
target_link_libraries(maxtraxplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(maxtraxplugin PRIVATE m)
endif()
