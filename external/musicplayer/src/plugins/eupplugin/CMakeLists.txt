cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.eupplugin)

# eupmini lives as a sibling repo inside chipmachine-as/, like 98fmplayer.
#   chipmachine-as/
#     chipmachine/   <- CMAKE_SOURCE_DIR
#     musicplayer/
#     eupmini/       <- here
get_filename_component(_eupmini_default
    "${CMAKE_SOURCE_DIR}/external/eupmini" ABSOLUTE)

set(EUPMINI_ROOT "${_eupmini_default}" CACHE PATH "Where eupmini is checked out")

if(NOT EXISTS "${EUPMINI_ROOT}/eupplayer.cpp")
    message(FATAL_ERROR
        "eupplugin: eupmini not found at ${EUPMINI_ROOT}\n"
        "Clone https://github.com/gzaffin/eupmini next to chipmachine-as "
        "or pass -DEUPMINI_ROOT=/path/to/eupmini to cmake.")
endif()

# eupmini's own sources (the CLI eupplay.cpp and the mksintbl.c generator are
# intentionally excluded; this plugin provides the host integration instead).
set(SOURCE_FILES
    EUPPlugin.cpp
    ${EUPMINI_ROOT}/eupplayer.cpp
    ${EUPMINI_ROOT}/eupplayer_townsEmulator.cpp
    ${EUPMINI_ROOT}/eupplayer_townsInternal.cpp
    ${EUPMINI_ROOT}/sintbl.cpp
)

add_library(eupplugin STATIC ${SOURCE_FILES})

# eupmini is C++; its headers are not warning-clean, keep them quiet.
set_source_files_properties(
    ${EUPMINI_ROOT}/eupplayer.cpp
    ${EUPMINI_ROOT}/eupplayer_townsEmulator.cpp
    ${EUPMINI_ROOT}/eupplayer_townsInternal.cpp
    ${EUPMINI_ROOT}/sintbl.cpp
    PROPERTIES COMPILE_FLAGS "-w"
)

target_include_directories(eupplugin PRIVATE ${EUPMINI_ROOT})

target_link_libraries(eupplugin PRIVATE coreutils)
