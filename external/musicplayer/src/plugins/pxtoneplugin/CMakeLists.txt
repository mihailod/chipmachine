cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.pxtoneplugin)

include(FetchContent)

FetchContent_Declare(
    libpxtone
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libpxtone
)

FetchContent_MakeAvailable(libpxtone)

add_library(pxtoneplugin PxTonePlugin.cpp PxTonePlugin.h)

target_link_libraries(pxtoneplugin PRIVATE libpxtone_static coreutils)
target_include_directories(pxtoneplugin PRIVATE ${libpxtone_SOURCE_DIR}/src)
