cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.mikmodplugin)

# Vendored slice of libmikmod 3.3.13 (LGPL-2.1+, https://mikmod.sourceforge.net/)
# Just enough to load and render MikMod UNITRK / UNIMOD modules (.uni):
# the player core + software mixer + the UNI loader + the null driver + the
# generic depackers. The OS sound drivers and the other format loaders are
# intentionally omitted (we register only drv_nos and load_uni at runtime).
set(MIKMOD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libmikmod)

set(MIKMOD_SOURCES
    ${MIKMOD_DIR}/playercode/mdriver.c
    ${MIKMOD_DIR}/playercode/mdulaw.c
    ${MIKMOD_DIR}/playercode/mloader.c
    ${MIKMOD_DIR}/playercode/mlutil.c
    ${MIKMOD_DIR}/playercode/mplayer.c
    ${MIKMOD_DIR}/playercode/munitrk.c
    ${MIKMOD_DIR}/playercode/npertab.c
    ${MIKMOD_DIR}/playercode/sloader.c
    ${MIKMOD_DIR}/playercode/virtch.c
    ${MIKMOD_DIR}/playercode/virtch2.c
    ${MIKMOD_DIR}/playercode/virtch_common.c
    ${MIKMOD_DIR}/mmio/mmalloc.c
    ${MIKMOD_DIR}/mmio/mmerror.c
    ${MIKMOD_DIR}/mmio/mmio.c
    ${MIKMOD_DIR}/loaders/load_uni.c
    ${MIKMOD_DIR}/drivers/drv_nos.c
    ${MIKMOD_DIR}/depackers/mmcmp.c
    ${MIKMOD_DIR}/depackers/pp20.c
    ${MIKMOD_DIR}/depackers/s404.c
    ${MIKMOD_DIR}/depackers/xpk.c
    ${MIKMOD_DIR}/posix/strcasecmp.c)

# libmikmod exposes a host of generic, un-prefixed internal helpers
# (getfrequency, getlinearperiod, ...) that collide with other vendored
# trackers (e.g. playerproplugin). Build everything with hidden visibility and
# partial-link with `ld -r`, which demotes the resulting private externs to true
# local symbols. MIKMODAPI is empty unless SYM_VISIBILITY is defined, so hiding
# applies to the whole library; only MikModPlugin.cpp stays at default
# visibility so musix::MikModPlugin and mikmodplugin_register remain exported.
add_library(mikmodplugin_objs OBJECT MikModPlugin.cpp MikModPlugin.h ${MIKMOD_SOURCES})

set_source_files_properties(${MIKMOD_SOURCES} PROPERTIES
    COMPILE_DEFINITIONS "HAVE_CONFIG_H;MIKMOD_STATIC")

target_include_directories(mikmodplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MIKMOD_DIR}
    ${MIKMOD_DIR}/include)

target_link_libraries(mikmodplugin_objs PRIVATE coreutils)
target_compile_options(mikmodplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden)

# Keep the musix::MikModPlugin methods exported -- the host and cmtest reference
# the class directly. Only the vendored libmikmod symbols need hiding.
set_source_files_properties(MikModPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=default;-fvisibility-inlines-hidden")

set(MIKMOD_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/mikmodplugin_combined.o)
add_custom_command(
    OUTPUT ${MIKMOD_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:mikmodplugin_objs> -o ${MIKMOD_COMBINED}
    DEPENDS mikmodplugin_objs $<TARGET_OBJECTS:mikmodplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing mikmodplugin symbols (ld -r)")
add_custom_target(mikmodplugin_combine DEPENDS ${MIKMOD_COMBINED})

add_library(mikmodplugin STATIC ${MIKMOD_COMBINED})
add_dependencies(mikmodplugin mikmodplugin_combine)
set_target_properties(mikmodplugin PROPERTIES LINKER_LANGUAGE CXX)

if(NOT WIN32)
    target_link_libraries(mikmodplugin PRIVATE m)
endif()
