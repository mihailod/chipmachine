cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.goattrackerplugin)

# GoatTracker v2 .sng (C64 SID tracker, Lasse Oorni) player. Wraps GoatTracker's
# own playback core -- the gplay.c sequencer and the gsid.cpp reSID interface --
# driving the vendored reSID engine directly (no SDL / BME / editor). The .sng
# format loader (gt2_load.c) reproduces GoatTracker's modern GTS3/4/5 parse path
# from memory into the same global song arrays gplay.c consumes.

file(GLOB RESID_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resid/*.cpp)
# resid/version.cpp only exposes resid_version_string (= the autoconf VERSION
# macro), which nothing in the player uses and which we don't define. Drop it.
list(FILTER RESID_SOURCES EXCLUDE REGEX "version\\.cpp$")

# GoatTracker's engine and reSID export many generic, un-prefixed globals
# (pattern, instr, sid, sidreg, songname, sound_flush, ...) that collide with
# other vendored plugins (e.g. vicepluginbridge's sound_flush/sound_suspend).
# Same fix as mikmodplugin: compile everything with hidden visibility and
# partial-link with `ld -r`, which demotes those private externs to true local
# symbols. Only GoatTrackerPlugin.cpp stays at default visibility so
# musix::GoatTrackerPlugin and goattrackerplugin_register remain exported.
add_library(goattrackerplugin_objs OBJECT
    GoatTrackerPlugin.cpp
    gplay.c
    gsid.cpp
    gt2_load.c
    ${RESID_SOURCES}
    GoatTrackerPlugin.h
    gt2_engine.h
    gcommon.h
    gplay.h
    gsid.h
    gsong.h
    gsound.h)

# Vendored GoatTracker/reSID sources use sprintf(), which the macOS SDK marks
# deprecated. Silence it on the third-party sources only (not our own
# GoatTrackerPlugin.cpp) rather than patch them -- keeps revendoring clean.
set_source_files_properties(gplay.c gsid.cpp gt2_load.c ${RESID_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-Wno-deprecated-declarations")

target_include_directories(goattrackerplugin_objs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(goattrackerplugin_objs PRIVATE coreutils)
target_compile_options(goattrackerplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden)

# Keep the musix::GoatTrackerPlugin methods + goattrackerplugin_register exported
# -- the host and cmtest reference them directly.
set_source_files_properties(GoatTrackerPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=default;-fvisibility-inlines-hidden")

set(GT_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/goattrackerplugin_combined.o)
add_custom_command(
    OUTPUT ${GT_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:goattrackerplugin_objs> -o ${GT_COMBINED}
    DEPENDS goattrackerplugin_objs $<TARGET_OBJECTS:goattrackerplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing goattrackerplugin symbols (ld -r)")
add_custom_target(goattrackerplugin_combine DEPENDS ${GT_COMBINED})

add_library(goattrackerplugin STATIC ${GT_COMBINED})
add_dependencies(goattrackerplugin goattrackerplugin_combine)
set_target_properties(goattrackerplugin PROPERTIES LINKER_LANGUAGE CXX)

if(NOT WIN32)
    target_link_libraries(goattrackerplugin PRIVATE m)
endif()
