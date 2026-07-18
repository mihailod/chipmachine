cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.klystrackplugin)

set(KLYSTRON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/klystron)

# libksnd + the klystron `snd` (cyd) synth core. Built SDL-free: we drive it via
# KSND_CreatePlayerUnregistered()/KSND_FillBuffer(), so with NOSDL_MIXER and
# without USESDLMUTEXES/USESDL_RWOPS the only SDL surface left is a handful of
# integer types and endian macros, supplied by klystron/shim/.
file(GLOB KLYSTRON_SND_SOURCES ${KLYSTRON_DIR}/snd/*.c)

add_library(klystrackplugin
    KlystrackPlugin.cpp
    KlystrackPlugin.h
    ${KLYSTRON_DIR}/lib/ksnd.c
    ${KLYSTRON_SND_SOURCES})

target_compile_definitions(klystrackplugin PRIVATE NOSDL_MIXER)

# shim/ must precede the real snd/ headers so klystron's `#include "SDL.h"`
# resolves to the shim, not any system SDL.
target_include_directories(klystrackplugin PRIVATE
    ${KLYSTRON_DIR}/shim
    ${KLYSTRON_DIR}
    ${KLYSTRON_DIR}/snd
    ${KLYSTRON_DIR}/lib)

# klystron's C sources are gnu99 and rely on type-punning through unions/pointers.
set_source_files_properties(
    ${KLYSTRON_DIR}/lib/ksnd.c ${KLYSTRON_SND_SOURCES}
    PROPERTIES COMPILE_FLAGS "-std=gnu99 -Wno-strict-aliasing")

target_link_libraries(klystrackplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(klystrackplugin PRIVATE m)
endif()
