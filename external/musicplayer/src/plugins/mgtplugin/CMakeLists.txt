cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.mgtplugin C CXX)

# Megatracker (.mgt) is another libxmp format. We do NOT build a second libxmp
# slice: this plugin compiles only the unique mgt_load.c loader (plus the C++
# glue) and links against musxplugin, which already provides the shared libxmp
# objects (control/load/sample/voltable/...). That keeps exactly one libxmp copy
# in the binary -- a second full slice here would collide on every shared symbol
# at final link (same arrangement as cocoplugin).
set(XMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../zxtune/3rdparty/xmp)

set(MGT_LOADER ${XMP_DIR}/src/loaders/mgt_load.c)

# Same trimmed build config the musxplugin slice uses.
set_source_files_properties(${MGT_LOADER} PROPERTIES COMPILE_DEFINITIONS
    "BUILDING_STATIC;NO_VERSIONED_SYMBOLS;DECRUNCH_MAX=0;NO_PROWIZARD;NO_EXTERNALFILES;NO_COMPOSITE_LOADER;PATH_MAX=1024")

add_library(mgtplugin STATIC MgtPlugin.cpp MgtPlugin.h ${MGT_LOADER})
target_include_directories(mgtplugin PRIVATE ${XMP_DIR}/include ${XMP_DIR}/src)
# musxplugin supplies the rest of the libxmp slice.
target_link_libraries(mgtplugin PRIVATE coreutils musxplugin)
