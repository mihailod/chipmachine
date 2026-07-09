cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.medplugin C CXX)

# Old MED (.med, magic "MED\x02".."MED\x04") is another libxmp format. We do NOT
# build a second libxmp slice: this plugin compiles only the unique med2/med3/
# med4_load.c loaders (plus the C++ glue) and links against musxplugin, which
# already provides the shared libxmp objects (control/load/sample/med_extras/
# mmd_common/...). That keeps exactly one libxmp copy in the binary -- a second
# full slice here would collide on every shared symbol at final link (same
# arrangement as cocoplugin/mgtplugin).
set(XMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../zxtune/3rdparty/xmp)

set(MED_LOADERS
    ${XMP_DIR}/src/loaders/med2_load.c
    ${XMP_DIR}/src/loaders/med3_load.c
    ${XMP_DIR}/src/loaders/med4_load.c
)

# Same trimmed build config the musxplugin slice uses.
set_source_files_properties(${MED_LOADERS} PROPERTIES COMPILE_DEFINITIONS
    "BUILDING_STATIC;NO_VERSIONED_SYMBOLS;DECRUNCH_MAX=0;NO_PROWIZARD;NO_EXTERNALFILES;NO_COMPOSITE_LOADER;PATH_MAX=1024")

add_library(medplugin STATIC MedPlugin.cpp MedPlugin.h ${MED_LOADERS})
target_include_directories(medplugin PRIVATE ${XMP_DIR}/include ${XMP_DIR}/src)
# musxplugin supplies the rest of the libxmp slice.
target_link_libraries(medplugin PRIVATE coreutils musxplugin)
