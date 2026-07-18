cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.vgmstreamplugin)

set(VGMSTREAM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vgmstream)

file(GLOB VGMSTREAM_SOURCES
    ${VGMSTREAM_DIR}/*.c
    ${VGMSTREAM_DIR}/base/*.c
    ${VGMSTREAM_DIR}/coding/*.c
    ${VGMSTREAM_DIR}/coding/libs/*.c
    ${VGMSTREAM_DIR}/layout/*.c
    ${VGMSTREAM_DIR}/meta/*.c
    ${VGMSTREAM_DIR}/util/*.c
)

add_library(vgmstreamplugin_objs OBJECT
    VGMStreamPlugin.cpp
    ${VGMSTREAM_SOURCES}
)

target_include_directories(vgmstreamplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${VGMSTREAM_DIR}
    ${VGMSTREAM_DIR}/base
    ${VGMSTREAM_DIR}/coding
    ${VGMSTREAM_DIR}/layout
    ${VGMSTREAM_DIR}/meta
    ${VGMSTREAM_DIR}/util
    /opt/homebrew/include
)

# Enable vgmstream's FFmpeg codec backend so ATRAC3 (.at3) / ATRAC3plus (.oma) and
# other ffmpeg-gated codecs decode. Uses the same system FFmpeg that ffmpegplugin
# links.
target_compile_definitions(vgmstreamplugin_objs PRIVATE VGM_USE_FFMPEG)

# Apply signed char, warnings suppression, and hidden visibility to the vgmstream sources
target_compile_options(vgmstreamplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden -fsigned-char -w)
set_target_properties(vgmstreamplugin_objs PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# Overwrite for our plugin C++ wrapper so it remains visible and exports the register function
set_source_files_properties(VGMStreamPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-std=c++17;-fvisibility=default;-fvisibility-inlines-hidden")

target_link_libraries(vgmstreamplugin_objs PRIVATE coreutils)

set(VGMSTREAM_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/vgmstreamplugin_combined.o)
add_custom_command(
    OUTPUT ${VGMSTREAM_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:vgmstreamplugin_objs> -o ${VGMSTREAM_COMBINED}
    DEPENDS vgmstreamplugin_objs $<TARGET_OBJECTS:vgmstreamplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing vgmstreamplugin symbols (ld -r)")
add_custom_target(vgmstreamplugin_combine DEPENDS ${VGMSTREAM_COMBINED})

add_library(vgmstreamplugin STATIC ${VGMSTREAM_COMBINED})
add_dependencies(vgmstreamplugin vgmstreamplugin_combine)
set_target_properties(vgmstreamplugin PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(vgmstreamplugin PRIVATE coreutils avcodec avformat avutil swresample)
if(NOT WIN32)
    target_link_libraries(vgmstreamplugin PRIVATE m)
endif()
