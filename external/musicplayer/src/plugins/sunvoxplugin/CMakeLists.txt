cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.sunvoxplugin)

set(SUNVOX_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sunvox_lib)

add_library(sunvoxplugin SunVoxPlugin.cpp SunVoxPlugin.h)

target_include_directories(sunvoxplugin PRIVATE ${SUNVOX_LIB_DIR})

# The SunVox engine is dlopen()ed at runtime, so we only need libdl here, not a
# link-time dependency on the prebuilt shared library.
target_link_libraries(sunvoxplugin PRIVATE coreutils ${CMAKE_DL_LIBS})

# Expose the runtime library path so executables can copy it next to themselves.
set(SUNVOX_RUNTIME_LIB ${SUNVOX_LIB_DIR}/sunvox.dylib
    CACHE INTERNAL "SunVox runtime shared library")
