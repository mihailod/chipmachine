cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.kssplugin)

# libkss lives as a sibling repo inside chipmachine-as/, like eupmini/organya.
#   chipmachine-as/
#     chipmachine/   <- CMAKE_SOURCE_DIR (or musicplayer/ for the standalone build)
#     musicplayer/
#     libkss/        <- here
get_filename_component(_libkss_default
    "${CMAKE_SOURCE_DIR}/external/libkss" ABSOLUTE)

set(LIBKSS_ROOT "${_libkss_default}" CACHE PATH "Where libkss is checked out")

if(NOT EXISTS "${LIBKSS_ROOT}/src/kssplay.c")
    message(FATAL_ERROR
        "kssplugin: libkss not found at ${LIBKSS_ROOT}\n"
        "Clone https://github.com/digital-sound-antiques/libkss (with "
        "--recursive, so modules/ is populated) next to chipmachine-as, "
        "or pass -DLIBKSS_ROOT=/path/to/libkss to cmake.")
endif()

if(NOT EXISTS "${LIBKSS_ROOT}/modules/drivers/mgsdrv.h")
    message(FATAL_ERROR
        "kssplugin: ${LIBKSS_ROOT}/modules/drivers/mgsdrv.h is missing.\n"
        "Run 'git submodule update --init --recursive' inside libkss.")
endif()

# libkss core + every format converter (KSS_bin2kss links all of them) + the
# chip cores the VM references. kss2vgm.c (VGM exporter) is intentionally left out.
set(LIBKSS_SOURCES
    ${LIBKSS_ROOT}/src/kssplay.c
    ${LIBKSS_ROOT}/src/kss/kss.c
    ${LIBKSS_ROOT}/src/kss/kssload.c
    ${LIBKSS_ROOT}/src/kss/mgs2kss.c
    ${LIBKSS_ROOT}/src/kss/bgm2kss.c
    ${LIBKSS_ROOT}/src/kss/mbm2kss.c
    ${LIBKSS_ROOT}/src/kss/mpk2kss.c
    ${LIBKSS_ROOT}/src/kss/opx2kss.c
    ${LIBKSS_ROOT}/src/kss/kss2kss.c
    ${LIBKSS_ROOT}/src/filters/dc_filter.c
    ${LIBKSS_ROOT}/src/filters/filter.c
    ${LIBKSS_ROOT}/src/filters/rc_filter.c
    ${LIBKSS_ROOT}/src/rconv/psg_rconv.c
    ${LIBKSS_ROOT}/src/vm/detect.c
    ${LIBKSS_ROOT}/src/vm/mmap.c
    ${LIBKSS_ROOT}/src/vm/vm.c
    ${LIBKSS_ROOT}/modules/emu2149/emu2149.c
    ${LIBKSS_ROOT}/modules/emu2212/emu2212.c
    ${LIBKSS_ROOT}/modules/emu2413/emu2413.c
    ${LIBKSS_ROOT}/modules/emu8950/emu8950.c
    ${LIBKSS_ROOT}/modules/emu8950/emuadpcm.c
    ${LIBKSS_ROOT}/modules/emu76489/emu76489.c
    ${LIBKSS_ROOT}/modules/kmz80/kmdmg.c
    ${LIBKSS_ROOT}/modules/kmz80/kmevent.c
    ${LIBKSS_ROOT}/modules/kmz80/kmr800.c
    ${LIBKSS_ROOT}/modules/kmz80/kmz80.c
    ${LIBKSS_ROOT}/modules/kmz80/kmz80c.c
    ${LIBKSS_ROOT}/modules/kmz80/kmz80t.c
)

add_library(kssplugin STATIC KSSPlugin.cpp FacMus2Kss.cpp ${LIBKSS_SOURCES})

# Embed the MGSDRV (.mgs), KINROU5 (.bgm), MPK (.mpk), OPX4KSS (.opx) and MBR143
# (.mbm) driver blobs. MoonBlaster 1.4 (MBR143) drives PSG + MSX-MUSIC (YM2413)
# + MSX-AUDIO (Y8950 ADPCM) -- all emulated here. It is NOT the OPL4/Moonsound
# "MoonBlaster Wave" variant; the Modland MoonBlaster collection is the 1.4
# format, so no OPL4 core is needed.

# s98plugin vendors its own copy of the same Okazaki emu2413 (OPLL) core, so the
# two static libs collide on the global _OPLL_* symbols at final link. Rename
# libkss's copy with a KSS_ prefix across the whole plugin compilation
# (definitions, header decls and call sites all rewrite together).
foreach(_sym
    OPLL_new OPLL_delete OPLL_reset OPLL_resetPatch OPLL_writeReg OPLL_writeIO
    OPLL_calc OPLL_calcStereo OPLL_setMask OPLL_toggleMask OPLL_copyPatch
    OPLL_forceRefresh OPLL_getDefaultPatch OPLL_setPatch OPLL_setRate
    OPLL_setQuality OPLL_setChipType OPLL_setPan OPLL_setPanFine
    OPLL_dumpToPatch OPLL_patchToDump
    OPLL_RateConv_new OPLL_RateConv_delete OPLL_RateConv_reset
    OPLL_RateConv_getData OPLL_RateConv_putData)
    target_compile_definitions(kssplugin PRIVATE "${_sym}=KSS_${_sym}")
endforeach()

# libkss resolves "emu2413/emu2413.h" and "drivers/mgsdrv.h" against modules/,
# and "kss/kss.h" etc. against src/.
target_include_directories(kssplugin PRIVATE
    ${LIBKSS_ROOT}/src
    ${LIBKSS_ROOT}/modules
)

# libkss's C is built -O3 -Wall upstream; keep it quiet under our flags.
# -fvisibility=hidden makes the vendored chip cores (emu2413, emu2149, ...)
# private to this archive. s98plugin vendors the SAME emu2413 OPLL core, so
# without this the two libs collide on _OPLL_* at final link. Only KSSPlugin.cpp
# (default visibility) needs to export kssplugin_register; everything libkss is
# called solely from within this plugin.
set_source_files_properties(${LIBKSS_SOURCES} PROPERTIES
    COMPILE_FLAGS "-w -fvisibility=hidden")

target_link_libraries(kssplugin PRIVATE coreutils)
