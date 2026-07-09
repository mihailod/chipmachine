cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.cocoplugin C CXX)

# Coconizer (.coco) is another Acorn Archimedes libxmp format. We do NOT build a
# second libxmp slice: this plugin compiles only the unique coco_load.c loader
# (plus the C++ glue) and links against musxplugin, which already provides the
# shared libxmp objects (control/load/sample/voltable/arch_vol_table/...). That
# keeps exactly one libxmp copy in the binary -- a second full slice here would
# collide on every shared symbol at final link.
set(XMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../zxtune/3rdparty/xmp)

set(COCO_LOADER ${XMP_DIR}/src/loaders/coco_load.c)

# Same trimmed build config the musxplugin slice uses.
set_source_files_properties(${COCO_LOADER} PROPERTIES COMPILE_DEFINITIONS
    "BUILDING_STATIC;NO_VERSIONED_SYMBOLS;DECRUNCH_MAX=0;NO_PROWIZARD;NO_EXTERNALFILES;NO_COMPOSITE_LOADER;PATH_MAX=1024")

add_library(cocoplugin STATIC CocoPlugin.cpp CocoPlugin.h ${COCO_LOADER})
target_include_directories(cocoplugin PRIVATE ${XMP_DIR}/include ${XMP_DIR}/src)
# musxplugin supplies the rest of the libxmp slice (and coreutils via its own
# link). PUBLIC nothing extra needed.
target_link_libraries(cocoplugin PRIVATE coreutils musxplugin)
