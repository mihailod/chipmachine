cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.playerproplugin)

# PlayerPRO (Macintosh tracker, Antoine Rosset) player. The decode engine is the
# public-domain MADDriver, vendored as a minimal slice at repo-root playerpro/
# (a sibling of chipmachine/ and musicplayer/, like eupmini/ and libkss/).
get_filename_component(_playerpro_default
    "${CMAKE_SOURCE_DIR}/external/playerpro" ABSOLUTE)
set(PLAYERPRO_ROOT "${_playerpro_default}" CACHE PATH "Where the playerpro slice lives")

if(NOT EXISTS "${PLAYERPRO_ROOT}/MADDriver.c")
    message(FATAL_ERROR
        "playerproplugin: PlayerPRO MADDriver slice not found at ${PLAYERPRO_ROOT}\n"
        "Expected the vendored playerpro/ directory next to musicplayer/.")
endif()

# The MADDriver software-synth core + the MAD-FG format loader + our two glue
# units (static plugin registry replacing the dlopen scanner, and inert stubs
# for the Mac live-audio/Finder symbols the __APPLE__ build references). See
# playerpro/PROVENANCE.md.
set(PLAYERPRO_SOURCES
    ${PLAYERPRO_ROOT}/DelayOutPut.c
    ${PLAYERPRO_ROOT}/Effects.c
    ${PLAYERPRO_ROOT}/MainDriver.c
    ${PLAYERPRO_ROOT}/FileUtils.c
    ${PLAYERPRO_ROOT}/Interrupt.c
    ${PLAYERPRO_ROOT}/MADDriver.c
    ${PLAYERPRO_ROOT}/MyDebugStr.c
    ${PLAYERPRO_ROOT}/OutPut8.c
    ${PLAYERPRO_ROOT}/realft.c
    ${PLAYERPRO_ROOT}/TickRemover.c
    ${PLAYERPRO_ROOT}/stub-VSTPlugIn.c
    ${PLAYERPRO_ROOT}/MIDI-Hardware-Stub.c
    ${PLAYERPRO_ROOT}/MADfg.c
    ${PLAYERPRO_ROOT}/PPEmbeddedPlugs.c
    ${PLAYERPRO_ROOT}/PPStubs.c
)

add_library(playerproplugin STATIC
    PlayerProPlugin.cpp
    PlayerProRender.c
    ${PLAYERPRO_SOURCES})

# EMBEDPLUGS makes the MAD-FG loader compile in as mainMADfg() (no dlopen);
# PPRO_PORTABLE_PLUG redirects the engine's framework-style includes to the
# vendored headers. The bridge unit needs the same so its engine includes match.
# -fsigned-char is REQUIRED: the MADDriver engine is classic C that reads 8-bit
# PCM sample data through plain `char`, assuming it is signed (-128..127). The
# umbrella project builds with -funsigned-char, under which those samples become
# 0..255 -- a DC offset + doubled amplitude that renders every tune badly
# clipped and garbled. Forcing signed char here (the flag wins as it comes last)
# restores correct playback. Explicit `unsigned char`/MADByte uses are unaffected.
set_source_files_properties(${PLAYERPRO_SOURCES} PlayerProRender.c PROPERTIES
    COMPILE_FLAGS "-w -fvisibility=hidden -fsigned-char")
target_compile_definitions(playerproplugin PRIVATE EMBEDPLUGS=1 PPRO_PORTABLE_PLUG)

target_include_directories(playerproplugin PRIVATE ${PLAYERPRO_ROOT})

target_link_libraries(playerproplugin PRIVATE coreutils)

# The engine keeps all its IO/struct code on its well-tested __APPLE__ paths,
# which use CoreFoundation (CFURL/CFString). No live-audio frameworks are
# needed -- the CoreAudio backend is stubbed out (PPStubs.c).
if(APPLE)
    target_link_libraries(playerproplugin PRIVATE "-framework CoreFoundation")
endif()
