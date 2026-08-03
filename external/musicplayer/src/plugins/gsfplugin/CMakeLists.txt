cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.gsfplugin)

# Game Boy Advance .gsf/.minigsf. The engine is mGBA (Jeffrey Pfau / endrift and
# contributors), MPL-2.0, vendored verbatim under mgba/ -- see README.md. It
# replaced a vendored VisualBoyAdvance (GPL-2-or-later) on 2026-08-01, which is
# what lets the App Store build keep all 1,246 GSF rows instead of gating them.
# Nothing here is copyleft-restricted, so the plugin builds identically in both
# variants and needs no CM_HAVE_* switch.
set(MGBA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/mgba)

# ---------------------------------------------------------------------------
# mGBA feature macros.
#
# READ THIS BEFORE TOUCHING THE LIST. Several of these change the LAYOUT of
# `struct mCore`, not just which code is compiled:
#
#   ENABLE_VFS + ENABLE_DIRECTORIES  -> adds `struct mDirectorySet dirs`
#   MINIMAL_CORE                     -> removes `struct mInputMap inputMap`
#   ENABLE_DEBUGGERS                 -> pulls in mgba/debugger/debugger.h
#
# sizeof(struct mCore) is 1352 with none of them and 2432 with the set below,
# and `init`/`reset` move from offset 704 to 1784. If a translation unit that
# includes <mgba/core/core.h> disagrees with the compiled library about these,
# GBACoreCreate() returns a perfectly valid pointer whose function pointers read
# as garbage, and the first virtual call jumps into hyperspace. That failure
# looks exactly like an ABI/LTO problem and is not one -- it cost a whole spike
# session (see data-notbundled/gsf-mgba-spike/README.md).
#
# The defences are: (a) ONE list, applied to the whole object library, so the
# engine and GSFPlugin.cpp cannot diverge; (b) never compile anything that
# includes mGBA headers outside this target.
#
# The set mirrors what mGBA's own CMake produces for a static, GBA-only,
# frontend-less build, minus ENABLE_DEBUGGERS/ENABLE_SCRIPTING (no debugger or
# Lua in a music player) and minus USE_MINIZIP (a GSF is never a zip; dropping
# it is what lets us leave mGBA's 24 MB third-party/zlib tree unvendored).
set(MGBA_DEFINES
    BUILD_STATIC
    M_CORE_GBA
    ENABLE_VFS
    ENABLE_VFS_FD
    ENABLE_DIRECTORIES
    USE_ZLIB
    HAVE_CRC32)

# Platform-detected feature macros. These do NOT affect struct layout -- they
# only tell mGBA which libc functions exist so it can skip its own fallbacks.
# Getting one wrong is a loud link error (missing symbol or duplicate symbol),
# never silent misbehaviour. Mirrors mGBA's find_function()/check_include_files.
include(CheckSymbolExists)
include(CheckIncludeFiles)
set(_mgba_probe_defs -D_DARWIN_C_SOURCE -D_GNU_SOURCE)
set(CMAKE_REQUIRED_DEFINITIONS ${_mgba_probe_defs})
foreach(_fn strdup strndup strlcpy vasprintf localtime_r realpath futimens futimes)
    string(TOUPPER "HAVE_${_fn}" _have)
    check_symbol_exists(${_fn}
        "stdio.h;stdlib.h;string.h;time.h;sys/time.h;sys/stat.h" ${_have})
    if(${_have})
        list(APPEND MGBA_DEFINES ${_have})
    endif()
endforeach()
foreach(_fn setlocale newlocale freelocale uselocale)
    string(TOUPPER "HAVE_${_fn}" _have)
    check_symbol_exists(${_fn} "locale.h;xlocale.h" ${_have})
    if(${_have})
        list(APPEND MGBA_DEFINES ${_have} HAVE_LOCALE)
    endif()
endforeach()
check_include_files("xlocale.h" HAVE_XLOCALE)
if(HAVE_XLOCALE)
    list(APPEND MGBA_DEFINES HAVE_XLOCALE)
endif()
check_symbol_exists(snprintf_l "stdio.h;xlocale.h" HAVE_SNPRINTF_L)
if(HAVE_SNPRINTF_L)
    list(APPEND MGBA_DEFINES HAVE_SNPRINTF_L)
endif()
check_symbol_exists(strtof_l "stdlib.h;xlocale.h" HAVE_STRTOF_L)
if(HAVE_STRTOF_L)
    list(APPEND MGBA_DEFINES HAVE_STRTOF_L)
endif()
unset(CMAKE_REQUIRED_DEFINITIONS)
list(REMOVE_DUPLICATES MGBA_DEFINES)
if(APPLE)
    list(APPEND MGBA_DEFINES _DARWIN_C_SOURCE)
endif()

# Threading: mGBA's core/thread.c is compiled because mCoreThread types are
# referenced from headers, but this plugin never starts an mCoreThread -- it
# drives core->runFrame() synchronously from the audio thread.
find_package(Threads)
if(CMAKE_USE_PTHREADS_INIT)
    list(APPEND MGBA_DEFINES USE_PTHREADS HAVE_PTHREAD_CREATE)
    if(APPLE)
        list(APPEND MGBA_DEFINES HAVE_PTHREAD_SETNAME_NP)
    endif()
endif()

# ---------------------------------------------------------------------------
# The mGBA sources a GBA-only, headless, static build needs. This is exactly the
# file set mGBA's own CMake compiles for
#   -DM_CORE_GBA=ON -DM_CORE_GB=OFF -DENABLE_DEBUGGERS=OFF -DENABLE_SCRIPTING=OFF
#   -DBUILD_STATIC=ON -DBUILD_{QT,SDL,GL,GLES2,GLES3}=OFF -DUSE_*=OFF
# minus the three minizip files (USE_MINIZIP dropped, see above) and minus the
# generated version.c (we supply mgba-version.c instead).
#
# To re-derive after a resync: configure upstream with those flags plus
# -DCMAKE_EXPORT_COMPILE_COMMANDS=ON and read the file list out of
# compile_commands.json. Do not prune by hand -- several files that look like
# frontend-only code (renderers/gl.c, util/image/png-io.c) compile to nothing
# under these defines but are still referenced by the video-backend vtables.
set(MGBA_SOURCES
    ${MGBA_DIR}/src/arm/arm.c
    ${MGBA_DIR}/src/arm/decoder-arm.c
    ${MGBA_DIR}/src/arm/decoder-thumb.c
    ${MGBA_DIR}/src/arm/decoder.c
    ${MGBA_DIR}/src/arm/isa-arm.c
    ${MGBA_DIR}/src/arm/isa-thumb.c

    ${MGBA_DIR}/src/core/bitmap-cache.c
    ${MGBA_DIR}/src/core/cache-set.c
    ${MGBA_DIR}/src/core/cheats.c
    ${MGBA_DIR}/src/core/config.c
    ${MGBA_DIR}/src/core/core.c
    ${MGBA_DIR}/src/core/directories.c
    ${MGBA_DIR}/src/core/input.c
    ${MGBA_DIR}/src/core/interface.c
    ${MGBA_DIR}/src/core/library.c
    ${MGBA_DIR}/src/core/lockstep.c
    ${MGBA_DIR}/src/core/log.c
    ${MGBA_DIR}/src/core/map-cache.c
    ${MGBA_DIR}/src/core/mem-search.c
    ${MGBA_DIR}/src/core/rewind.c
    ${MGBA_DIR}/src/core/serialize.c
    ${MGBA_DIR}/src/core/sync.c
    ${MGBA_DIR}/src/core/thread.c
    ${MGBA_DIR}/src/core/tile-cache.c
    ${MGBA_DIR}/src/core/timing.c

    ${MGBA_DIR}/src/feature/commandline.c
    ${MGBA_DIR}/src/feature/proxy-backend.c
    ${MGBA_DIR}/src/feature/thread-proxy.c
    ${MGBA_DIR}/src/feature/updater.c
    ${MGBA_DIR}/src/feature/video-backend.c
    ${MGBA_DIR}/src/feature/video-logger.c

    # The GBA's four PSG channels ARE the Game Boy's; gba/audio.c drives
    # GBAudio from src/gb even when M_CORE_GB is off.
    ${MGBA_DIR}/src/gb/audio.c

    ${MGBA_DIR}/src/gba/audio.c
    ${MGBA_DIR}/src/gba/bios.c
    ${MGBA_DIR}/src/gba/cheats.c
    ${MGBA_DIR}/src/gba/core.c
    ${MGBA_DIR}/src/gba/dma.c
    ${MGBA_DIR}/src/gba/gba.c
    ${MGBA_DIR}/src/gba/hle-bios.c
    ${MGBA_DIR}/src/gba/input.c
    ${MGBA_DIR}/src/gba/io.c
    ${MGBA_DIR}/src/gba/memory.c
    ${MGBA_DIR}/src/gba/overrides.c
    ${MGBA_DIR}/src/gba/savedata.c
    ${MGBA_DIR}/src/gba/serialize.c
    ${MGBA_DIR}/src/gba/sharkport.c
    ${MGBA_DIR}/src/gba/sio.c
    ${MGBA_DIR}/src/gba/timer.c
    ${MGBA_DIR}/src/gba/video.c
    ${MGBA_DIR}/src/gba/cart/ereader.c
    ${MGBA_DIR}/src/gba/cart/gpio.c
    ${MGBA_DIR}/src/gba/cart/matrix.c
    ${MGBA_DIR}/src/gba/cart/unlicensed.c
    ${MGBA_DIR}/src/gba/cart/vfame.c
    ${MGBA_DIR}/src/gba/cheats/codebreaker.c
    ${MGBA_DIR}/src/gba/cheats/gameshark.c
    ${MGBA_DIR}/src/gba/cheats/parv3.c
    ${MGBA_DIR}/src/gba/extra/battlechip.c
    ${MGBA_DIR}/src/gba/extra/proxy.c
    ${MGBA_DIR}/src/gba/renderers/cache-set.c
    ${MGBA_DIR}/src/gba/renderers/common.c
    ${MGBA_DIR}/src/gba/renderers/gl.c
    ${MGBA_DIR}/src/gba/renderers/software-bg.c
    ${MGBA_DIR}/src/gba/renderers/software-mode0.c
    ${MGBA_DIR}/src/gba/renderers/software-obj.c
    ${MGBA_DIR}/src/gba/renderers/video-software.c
    ${MGBA_DIR}/src/gba/sio/dolphin.c
    ${MGBA_DIR}/src/gba/sio/gbp.c
    ${MGBA_DIR}/src/gba/sio/lockstep.c

    ${MGBA_DIR}/src/platform/posix/memory.c
    ${MGBA_DIR}/src/third-party/inih/ini.c

    ${MGBA_DIR}/src/util/audio-buffer.c
    ${MGBA_DIR}/src/util/audio-resampler.c
    ${MGBA_DIR}/src/util/circle-buffer.c
    ${MGBA_DIR}/src/util/configuration.c
    ${MGBA_DIR}/src/util/convolve.c
    ${MGBA_DIR}/src/util/crc32.c
    ${MGBA_DIR}/src/util/elf-read.c
    ${MGBA_DIR}/src/util/formatting.c
    ${MGBA_DIR}/src/util/gbk-table.c
    ${MGBA_DIR}/src/util/geometry.c
    ${MGBA_DIR}/src/util/hash.c
    ${MGBA_DIR}/src/util/image.c
    ${MGBA_DIR}/src/util/interpolator.c
    ${MGBA_DIR}/src/util/md5.c
    ${MGBA_DIR}/src/util/patch-fast.c
    ${MGBA_DIR}/src/util/patch-ips.c
    ${MGBA_DIR}/src/util/patch-ups.c
    ${MGBA_DIR}/src/util/patch.c
    ${MGBA_DIR}/src/util/ring-fifo.c
    ${MGBA_DIR}/src/util/sfo.c
    ${MGBA_DIR}/src/util/sha1.c
    ${MGBA_DIR}/src/util/string.c
    ${MGBA_DIR}/src/util/table.c
    ${MGBA_DIR}/src/util/text-codec.c
    ${MGBA_DIR}/src/util/vector.c
    ${MGBA_DIR}/src/util/vfs.c
    ${MGBA_DIR}/src/util/image/export.c
    ${MGBA_DIR}/src/util/image/font.c
    ${MGBA_DIR}/src/util/image/png-io.c
    ${MGBA_DIR}/src/util/vfs/vfs-dirent.c
    ${MGBA_DIR}/src/util/vfs/vfs-fd.c
    ${MGBA_DIR}/src/util/vfs/vfs-fifo.c
    ${MGBA_DIR}/src/util/vfs/vfs-mem.c
    # vfs-zip.c is deliberately absent: it has no "no zip backend" branch and
    # hard-includes minizip/zip.h once USE_LIBZIP is off. Nothing reaches it --
    # a GSF is a raw PSF container, and archive handling in this app happens a
    # layer above the plugins.

    ${CMAKE_CURRENT_SOURCE_DIR}/mgba-version.c)

add_library(gsfplugin_objs OBJECT
    GSFPlugin.cpp GSFPlugin.h
    GsfRom.cpp GsfRom.h
    ${MGBA_SOURCES})

target_include_directories(gsfplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${MGBA_DIR}/include
    ${MGBA_DIR}/src          # third-party/inih/ini.h is included path-relative
    ${ZLIB_INCLUDE_DIRS}
    ..)

# One define list for engine AND wrapper -- see the layout note at the top.
target_compile_definitions(gsfplugin_objs PRIVATE ${MGBA_DEFINES})

target_link_libraries(gsfplugin_objs PRIVATE psf coreutils)

# NDEBUG is required, not tidiness. This project's Release build does not define
# it (see the vendored-assert note in the libxmp plugins), and mGBA asserts
# liberally on states a malformed rip can reach -- an out-of-range DMA in
# gba/dma.c, an unhandled BIOS SWI in gba/bios.c. A single bad file in a
# 1,246-file corpus would abort() the whole app instead of skipping one song.
# Genuine load failures still come back through loadROM()'s return value, which
# is what GSFPlayer keys on.
#
# -w: the engine is third-party, don't police its warnings (this is why
# gsfplugin no longer appears in SILENT_TARGETS at the top level -- our own
# GSFPlugin.cpp/GsfRom.cpp stay strict).
set_source_files_properties(${MGBA_SOURCES} PROPERTIES
    COMPILE_DEFINITIONS "NDEBUG"
    COMPILE_OPTIONS "-w")

# mGBA puts 1,186 C symbols at global scope, many of them names any other
# vendored engine could plausibly pick: TableInit, HashTableInit, VFileOpen,
# md5Init, sha1Init, ini_parse, ARMRun, ARMReset. Checked 2026-08-01 against
# every other .a in the build and NOTHING collides today -- this is prophylaxis,
# not a fix, and it is cheap: build hidden and partial-link with `ld -r` so all
# of that becomes local, the same treatment mikmodplugin, sndhplugin and
# vgmstreamplugin already get. It also means adding an ARM-based engine later
# (a 2SF core, say) cannot break the GSF build.
target_compile_options(gsfplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden)

# Our own wrappers must stay exported: gsfplugin_register() is called from
# plugin_register.cpp and musix::GSFPlugin is referenced by cmtest.
set_source_files_properties(GSFPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=default;-fvisibility-inlines-hidden")

set(GSF_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/gsfplugin_combined.o)
add_custom_command(
    OUTPUT ${GSF_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:gsfplugin_objs> -o ${GSF_COMBINED}
    DEPENDS gsfplugin_objs $<TARGET_OBJECTS:gsfplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing gsfplugin/mGBA symbols (ld -r)")
add_custom_target(gsfplugin_combine DEPENDS ${GSF_COMBINED})

add_library(gsfplugin STATIC ${GSF_COMBINED})
add_dependencies(gsfplugin gsfplugin_combine)
set_target_properties(gsfplugin PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(gsfplugin PUBLIC psf coreutils ${ZLIB_LIBRARIES})
if(APPLE)
    # core/config.c's mCoreConfigDirectory() resolves ~/Library/Application
    # Support through CFPreferences on Darwin. Nothing in the decode path calls
    # it, but it is compiled in and has to resolve.
    target_link_libraries(gsfplugin PUBLIC "-framework CoreFoundation")
endif()
