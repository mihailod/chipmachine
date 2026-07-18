cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.ixsplugin)

# Ixalance (.ixs) player. Wraps "webixs" -- Juergen Wothke's native C++
# reimplementation of Shortcut Software's lost IXS player (reverse-engineered
# with Ghidra), vendored at chipmachine-as root /webixs. The format synthesizes
# and zlib-compresses its own wavetable data, hence the zlib dependency.
#
# webixs is CC BY-NC-SA 4.0 (NonCommercial) -- see webixs/LICENSE.
set(IXS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/webixs)

add_library(ixsplugin STATIC
    IXSPlugin.cpp
    IXSPlugin.h
    # webixs core
    ${IXS_ROOT}/PlayerIXS.cpp
    ${IXS_ROOT}/PlayerCore.cpp
    ${IXS_ROOT}/Module.cpp
    ${IXS_ROOT}/MixerBase.cpp
    ${IXS_ROOT}/Mixer1.cpp
    ${IXS_ROOT}/Mixer2.cpp
    ${IXS_ROOT}/Mixer3.cpp
    ${IXS_ROOT}/Mixer4.cpp
    ${IXS_ROOT}/WaveGen.cpp
    ${IXS_ROOT}/Packer.cpp
    ${IXS_ROOT}/FileMap.cpp
    ${IXS_ROOT}/FileMapSFXI.cpp
    ${IXS_ROOT}/FileSFXI.cpp
    ${IXS_ROOT}/asmEmu.cpp
    # embedded wavetable resources (plain C -- compiled as C by extension)
    ${IXS_ROOT}/WaveLibrary.c
)

target_include_directories(ixsplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IXS_ROOT}
    ${ZLIB_INCLUDE_DIRS}
)

# LINUX = "modern non-Emscripten, non-Win32 build": compiles out the original's
# Windows audio device + worker-thread paths and exposes the pull-style render
# API (genAudio / getAudioBuffer / isSongEnd) the plugin drives.
target_compile_definitions(ixsplugin PRIVATE LINUX)

# -w silences the (large) decompiler-generated warning output.
# -fsigned-char is REQUIRED: the umbrella project builds with -funsigned-char,
# but this Ghidra-decompiled code relies on signed `char` semantics (e.g. signed
# sample/index math in WaveGen). Without it the synth indexes out of bounds and
# crashes -- same trap as playerproplugin.
target_compile_options(ixsplugin PRIVATE -w -fsigned-char)

target_link_libraries(ixsplugin PRIVATE coreutils ${ZLIB_LIBRARIES})

if(NOT WIN32)
    target_link_libraries(ixsplugin PRIVATE m)
endif()
