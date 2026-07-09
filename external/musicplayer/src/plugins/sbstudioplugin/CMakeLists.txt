cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.sbstudioplugin C CXX)

# SBStudio (.pac) is decoded by a vendored copy of Thomas Pfaff's libpac
# (ISC-licensed ANSI C, http://libpac.sourceforge.net/). libpac decodes a module
# straight to interleaved PCM, so there is no shared engine to reuse -- we just
# compile the library's ~10 source files alongside the C++ glue.
file(GLOB LIBPAC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/libpac/*.c)

# NDEBUG turns off libpac's debug fprintf() in pac_init() (and the asserts).
set_source_files_properties(${LIBPAC_SOURCES} PROPERTIES COMPILE_DEFINITIONS "NDEBUG")

add_library(sbstudioplugin STATIC SBStudioPlugin.cpp SBStudioPlugin.h ${LIBPAC_SOURCES})
target_include_directories(sbstudioplugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libpac)
target_link_libraries(sbstudioplugin PRIVATE coreutils)
