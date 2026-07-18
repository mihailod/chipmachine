cmake_minimum_required(VERSION 3.17)
project(cpp-mods.musicplayer.sksplugin)

# Vendored Arkos Tracker 3 (MIT), repo-root /arkostracker3. We compile the same
# non-GUI "BaseExport" slice that AT3's headless command-line tools use (song
# import for every AY/YM format incl. STarKos .sks, the SongPlayer engine and
# the PsgStreamGenerator software AY/YM renderer), and drive it from
# SksPlugin.cpp exactly like AT3's SongToWav tool -- minus the WAV writer.
set(AT3_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/arkostracker3)

# ---------------------------------------------------------------------------
# JUCE 7.0.1.2 (Julien Névo's fork, patched for Xcode 16 / macOS Tahoe), the
# framework AT3 is built on. Vendored in-repo at ${AT3_ROOT}/JUCE (trimmed to
# modules + the CMake/juceaide build infra). Only the non-GUI modules are used
# (juce_core, juce_events, juce_data_structures, juce_audio_basics).
# ---------------------------------------------------------------------------
if(NOT TARGET juce::juce_core)
    add_subdirectory(${AT3_ROOT}/JUCE at3_juce)
endif()

# ---------------------------------------------------------------------------
# AT3 third-party + business-logic static libs, reusing the upstream
# CMakeLists verbatim (relative source paths resolve inside the vendored tree).
# rasm/lzh first as BaseExport/BaseCli link them.
# ---------------------------------------------------------------------------
add_subdirectory(${AT3_ROOT}/thirdParty/rasm                    at3_rasm)
add_subdirectory(${AT3_ROOT}/thirdParty/lzh                     at3_lzh)
add_subdirectory(${AT3_ROOT}/source/commandLineTools/baseCli    at3_basecli)
add_subdirectory(${AT3_ROOT}/source/commandLineTools/baseExport at3_baseexport)

add_library(sksplugin STATIC
    SksPlugin.cpp
    SksPlugin.h
)

target_include_directories(sksplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${AT3_ROOT}/source
)

target_link_libraries(sksplugin PRIVATE
    BaseExport
    coreutils
    juce::juce_audio_basics
    juce::juce_events
    juce::juce_core
)

# The AT3 / third-party sources are vendored verbatim and the plugin glue only
# trips warnings inside upstream headers (coreutils, chipplugin, JUCE). Silence
# the lot, as the other vendored plugins (ptk/zxtune/kss) do, so they don't
# drown the build log. -w is applied last so it overrides the project's global
# warning flags on these targets.
foreach(_at3_target BaseExport BaseCli ThirdPartyRasm ThirdPartyLzh)
    if(TARGET ${_at3_target})
        target_compile_options(${_at3_target} PRIVATE -w)
    endif()
endforeach()
target_compile_options(sksplugin PRIVATE -w)
