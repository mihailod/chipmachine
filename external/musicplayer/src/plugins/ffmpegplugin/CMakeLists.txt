project(cpp-mods.musicplayer.ffmpegplugin)

set(SOURCE_FILES FFMPEGPlugin.cpp)

# Define the target as a static library
add_library(ffmpegplugin STATIC ${SOURCE_FILES})

# FIXED: Corrected relative path jumps to match the internal musicplayer/apone layout
target_include_directories(ffmpegplugin PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../.. 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../apone/coreutils/src
    /opt/homebrew/include
)

# Inject required formatting macros
target_compile_options(ffmpegplugin PRIVATE "-include" "coreutils/format.h")

# Link coreutils and system FFmpeg libraries directly.
target_link_libraries(ffmpegplugin PRIVATE 
    coreutils 
    avcodec 
    avformat 
    avutil 
    swresample
)
