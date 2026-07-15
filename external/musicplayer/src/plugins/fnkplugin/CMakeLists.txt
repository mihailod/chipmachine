cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.fnkplugin C CXX)

# We reuse the libxmp source vendored under zxtune, but compile our own minimal
# single-loader slice (fnk_loader only) into this plugin. We deliberately do
# NOT add_subdirectory the shared zxtune/3rdparty/xmp build, so chipmachine's
# zxtuneplugin is untouched and there is no duplicate "xmp" target. (Same shape
# as musxplugin/cocoplugin/mgtplugin -- see musxplugin/README.md.)
set(XMP_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../zxtune/3rdparty/xmp)

set(XMP_SRCS
    ${XMP_DIR}/src/control.c ${XMP_DIR}/src/dataio.c ${XMP_DIR}/src/effects.c
    ${XMP_DIR}/src/envelope.c ${XMP_DIR}/src/extras.c ${XMP_DIR}/src/filter.c
    ${XMP_DIR}/src/fnmatch.c ${XMP_DIR}/src/format.c ${XMP_DIR}/src/hio.c
    ${XMP_DIR}/src/hmn_extras.c ${XMP_DIR}/src/lfo.c ${XMP_DIR}/src/load.c
    ${XMP_DIR}/src/load_helpers.c ${XMP_DIR}/src/md5.c ${XMP_DIR}/src/med_extras.c
    ${XMP_DIR}/src/misc.c ${XMP_DIR}/src/mixer.c ${XMP_DIR}/src/mix_all.c
    ${XMP_DIR}/src/period.c ${XMP_DIR}/src/player.c ${XMP_DIR}/src/read_event.c
    ${XMP_DIR}/src/scan.c ${XMP_DIR}/src/smix.c ${XMP_DIR}/src/virtual.c
    ${XMP_DIR}/src/loaders/common.c ${XMP_DIR}/src/loaders/iff.c
    ${XMP_DIR}/src/loaders/sample.c ${XMP_DIR}/src/loaders/mmd_common.c
    ${XMP_DIR}/src/loaders/voltable.c ${XMP_DIR}/src/loaders/fnk_load.c
)

# Same trimmed build config zxtune uses (no prowizard / external files /
# composite auto-detect table / depackers); keeps the slice tiny.
set_source_files_properties(${XMP_SRCS} PROPERTIES COMPILE_DEFINITIONS
    "BUILDING_STATIC;NO_VERSIONED_SYMBOLS;DECRUNCH_MAX=0;NO_PROWIZARD;NO_EXTERNALFILES;NO_COMPOSITE_LOADER;PATH_MAX=1024")

add_library(fnkplugin STATIC FnkPlugin.cpp FnkPlugin.h ${XMP_SRCS})
target_include_directories(fnkplugin PRIVATE ${XMP_DIR}/include ${XMP_DIR}/src)
target_link_libraries(fnkplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(fnkplugin PRIVATE m)
endif()
