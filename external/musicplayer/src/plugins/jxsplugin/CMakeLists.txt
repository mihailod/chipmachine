cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.jxsplugin)

# JayTrax (.jxs) player. The decode engine is the public C port of Reinier
# "Rhino" van Vliet's own JayTrax/CrossX replayer, vendored as a minimal slice
# at repo-root jaytrax/ (a sibling of chipmachine/ and musicplayer/, like
# playerpro/ and eupmini/). See jaytrax/PROVENANCE.md.
get_filename_component(_jaytrax_default
    "${CMAKE_SOURCE_DIR}/external/jaytrax" ABSOLUTE)
set(JAYTRAX_ROOT "${_jaytrax_default}" CACHE PATH "Where the jaytrax slice lives")

if(NOT EXISTS "${JAYTRAX_ROOT}/lib_oldjaytrax/jaytrax.c")
    message(FATAL_ERROR
        "jxsplugin: JayTrax replayer slice not found at ${JAYTRAX_ROOT}\n"
        "Expected the vendored jaytrax/ directory next to musicplayer/.")
endif()

set(JAYTRAX_SOURCES
    ${JAYTRAX_ROOT}/lib_oldjaytrax/jaytrax.c
    ${JAYTRAX_ROOT}/lib_oldjaytrax/jxs.c
    ${JAYTRAX_ROOT}/lib_oldjaytrax/mixcore.c)

add_library(jxsplugin STATIC
    JxsPlugin.cpp
    JxsPlugin.h
    ${JAYTRAX_SOURCES})

# Upstream C compiles cleanly but emits the usual third-party warnings; keep it
# quiet and hide its symbols so the global C namespace stays uncluttered.
set_source_files_properties(${JAYTRAX_SOURCES} PROPERTIES
    COMPILE_FLAGS "-w -fvisibility=hidden")

target_include_directories(jxsplugin PRIVATE ${JAYTRAX_ROOT}/lib_oldjaytrax)

target_link_libraries(jxsplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(jxsplugin PRIVATE m)
endif()
