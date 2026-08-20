cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.pxtoneplugin)

include(FetchContent)

FetchContent_Declare(
    libpxtone
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libpxtone
)

FetchContent_MakeAvailable(libpxtone)

# libpxtone bundles stb_vorbis, whose memory-backed seek guards a huge offset
# with `stream_start + loc < stream_start`. Pointer overflow is UB, so clang 21
# folds that half to false and warns. It is upstream stb code and the other half
# of the same test (`stream_start + loc >= stream_end`) already rejects every
# offset we can produce, so silence it rather than diverge from upstream.
# Set here, not in libpxtone/CMakeLists.txt, so a revendor cannot drop it.
target_compile_options(libpxtone_static PRIVATE -Wno-tautological-compare)

add_library(pxtoneplugin PxTonePlugin.cpp PxTonePlugin.h)

target_link_libraries(pxtoneplugin PRIVATE libpxtone_static coreutils)
target_include_directories(pxtoneplugin PRIVATE ${libpxtone_SOURCE_DIR}/src)
