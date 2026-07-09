cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.fmpplugin)

# 98fmplayer lives as a sibling repo inside chipmachine-as/, just like
# apone and musicplayer.  Standard layout:
#   chipmachine-as/
#     chipmachine/   <- CMAKE_SOURCE_DIR
#     musicplayer/
#     apone/
#     98fmplayer/    <- here
#     build/
get_filename_component(_fmp98_default
    "${CMAKE_SOURCE_DIR}/external/98fmplayer" ABSOLUTE)

set(FMP98_ROOT "${_fmp98_default}" CACHE PATH "Where 98fmplayer is checked out")

if(NOT EXISTS "${FMP98_ROOT}/fmdriver/fmdriver_fmp.c")
    message(FATAL_ERROR
        "fmpplugin: 98fmplayer not found at ${FMP98_ROOT}\n"
        "Clone https://github.com/mistydemeo/98fmplayer next to chipmachine-as "
        "or pass -DFMP98_ROOT=/path/to/98fmplayer to cmake.")
endif()

set(FMGEN_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/../s98plugin/m_s98/device/fmgen")
set(FMGEN_ROOT    "${CMAKE_CURRENT_SOURCE_DIR}/../s98plugin/m_s98/device")

set(SOURCE_FILES
    FMPPlugin.cpp
    ${FMP98_ROOT}/fmdriver/fmdriver_fmp.c
    ${FMP98_ROOT}/fmdriver/fmdriver_common.c
    ${FMP98_ROOT}/fmdriver/ppz8.c
)

add_library(fmpplugin STATIC ${SOURCE_FILES})

set_source_files_properties(
    ${FMP98_ROOT}/fmdriver/fmdriver_fmp.c
    ${FMP98_ROOT}/fmdriver/fmdriver_common.c
    ${FMP98_ROOT}/fmdriver/ppz8.c
    PROPERTIES COMPILE_FLAGS "-std=c11"
)

target_include_directories(fmpplugin PRIVATE
    ${FMP98_ROOT}/fmdriver
    ${FMP98_ROOT}
    ${FMGEN_INCLUDE}
    ${FMGEN_ROOT}
    ${CMAKE_CURRENT_SOURCE_DIR}/../s98plugin/m_s98
)

target_link_libraries(fmpplugin PRIVATE coreutils s98plugin)
