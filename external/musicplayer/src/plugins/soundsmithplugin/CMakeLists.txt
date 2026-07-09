cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.soundsmithplugin)

# Apple IIgs SoundSmith player. Self-contained C++ port of Sean Kasun's
# BSD-licensed JavaScript player (vendored at repo-root soundsmith/); emulates
# the Ensoniq 5503 "DOC" in-process, no external library.
add_library(soundsmithplugin SoundSmithPlugin.cpp SoundSmithPlugin.h)

target_link_libraries(soundsmithplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(soundsmithplugin PRIVATE m)
endif()
