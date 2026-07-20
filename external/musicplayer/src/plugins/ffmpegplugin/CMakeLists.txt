project(cpp-mods.musicplayer.ffmpegplugin)

set(SOURCE_FILES FFMPEGPlugin.cpp)

# Define the target as a static library
add_library(ffmpegplugin STATIC ${SOURCE_FILES})

# coreutils headers (exec.h/format.h/etc.) come from the single vendored apone via
# the linked `coreutils` target's interface include (external/apone/mods). The old
# musicplayer/apone include dir was removed as part of the apone dedup.
# Vendored LGPL FFmpeg headers first (decouples from Homebrew's GPL ffmpeg);
# /opt/homebrew/include kept only as a fallback for older checkouts.
target_include_directories(ffmpegplugin BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ffmpeg-lgpl/include
)
target_include_directories(ffmpegplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
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
