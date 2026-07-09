cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.famitrackerplugin)

# FamiTracker (.ftm) player. The decode engine is a vendored, boost-free slice of
# nukep's cross-platform "FamiTracker CX" (a fork of jsr's FamiTracker), kept at
# repo-root famitracker-cx/ (a sibling of chipmachine/ and musicplayer/, like
# jaytrax/ and playerpro/). See famitracker-cx/PROVENANCE.md.
#
# Only the headless engine is built: famitracker-core/ (document loader + sound
# driver), its APU/ (NES 2A03 + VRC6/VRC7/MMC5/FDS + N106), and a minimal
# boost-free core/ shim (io + a thread-free soundsink). The GUI, ncurses/console
# frontends, dynamic sink loading and WAV export are dropped.
get_filename_component(_fami_default
    "${CMAKE_SOURCE_DIR}/external/famitracker-cx" ABSOLUTE)
set(FAMITRACKER_ROOT "${_fami_default}" CACHE PATH "Where the famitracker-cx slice lives")

if(NOT EXISTS "${FAMITRACKER_ROOT}/famitracker-core/SoundGen.cpp")
    message(FATAL_ERROR
        "famitrackerplugin: FamiTracker CX engine slice not found at ${FAMITRACKER_ROOT}\n"
        "Expected the vendored famitracker-cx/ directory next to musicplayer/.")
endif()

set(FC ${FAMITRACKER_ROOT}/famitracker-core)
set(FAMI_SOURCES
    ${FC}/exceptions.cpp
    ${FC}/SoundGen.cpp
    ${FC}/App.cpp
    ${FC}/ChannelHandler.cpp
    ${FC}/Channels2A03.cpp
    ${FC}/ChannelsFDS.cpp
    ${FC}/ChannelsMMC5.cpp
    ${FC}/ChannelsVRC6.cpp
    ${FC}/ChannelsVRC7.cpp
    ${FC}/ChannelMap.cpp
    ${FC}/Document.cpp
    ${FC}/FtmDocument.cpp
    ${FC}/Instrument.cpp
    ${FC}/Instrument2A03.cpp
    ${FC}/InstrumentFDS.cpp
    ${FC}/InstrumentVRC6.cpp
    ${FC}/InstrumentVRC7.cpp
    ${FC}/PatternData.cpp
    ${FC}/Sequence.cpp
    ${FC}/TrackerChannel.cpp
    ${FC}/TrackerController.cpp
    ${FC}/Settings.cpp
    # APU (NES sound emulation)
    ${FC}/APU/Blip_Buffer/Blip_Buffer.cpp
    ${FC}/APU/APU.cpp
    ${FC}/APU/Mixer.cpp
    ${FC}/APU/DPCM.cpp
    ${FC}/APU/Noise.cpp
    ${FC}/APU/Square.cpp
    ${FC}/APU/Triangle.cpp
    ${FC}/APU/FDS.cpp
    ${FC}/APU/MMC5.cpp
    ${FC}/APU/N106.cpp
    ${FC}/APU/VRC6.cpp
    ${FC}/APU/VRC7.cpp
    ${FC}/APU/emu2413.c
    ${FC}/APU/FDSSound.cpp
    # boost-free core shim
    ${FAMITRACKER_ROOT}/core/io.cpp
    ${FAMITRACKER_ROOT}/core/soundsink.cpp)

# gmeplugin and nedplugin already vendor blargg's Blip_Buffer in the global
# namespace, and FamiTracker CX's APU/emulation uses many generic global names
# that collide with other trackers. Build everything with hidden visibility (->
# Mach-O private externs), then partial-link with `ld -r` to demote those to true
# locals. Only FamiTrackerPlugin.cpp is compiled at default visibility, so
# famitrackerplugin_register() and the musix::FamiTrackerPlugin methods remain the
# only exported symbols -- nothing collides at the final link. (Same approach as
# nedplugin / mikmodplugin.)
add_library(famitrackerplugin_objs OBJECT
    FamiTrackerPlugin.cpp
    FamiTrackerPlugin.h
    ${FAMI_SOURCES})

target_include_directories(famitrackerplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FAMITRACKER_ROOT}
    ${FC})

target_link_libraries(famitrackerplugin_objs PRIVATE coreutils)

# -w silences the heavy upstream warning output (the FamiTracker source predates
# many modern warnings); matches the other vendored-engine plugins.
target_compile_options(famitrackerplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden -w)

set_source_files_properties(FamiTrackerPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=default;-fvisibility-inlines-hidden")

set(FAMI_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/famitrackerplugin_combined.o)
add_custom_command(
    OUTPUT ${FAMI_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:famitrackerplugin_objs> -o ${FAMI_COMBINED}
    DEPENDS famitrackerplugin_objs $<TARGET_OBJECTS:famitrackerplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing famitrackerplugin symbols (ld -r)")
add_custom_target(famitrackerplugin_combine DEPENDS ${FAMI_COMBINED})

add_library(famitrackerplugin STATIC ${FAMI_COMBINED})
add_dependencies(famitrackerplugin famitrackerplugin_combine)
set_target_properties(famitrackerplugin PROPERTIES LINKER_LANGUAGE CXX)

if(NOT WIN32)
    target_link_libraries(famitrackerplugin PRIVATE m)
endif()
