cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.gmeplugin)

file(GLOB SOURCE_FILES gme/*.cpp)

# gme/emu2413.c is the real YM2413 (OPLL) FM core, vendored from libkss's
# emu2413 (Digital Sound Antiques). The glob above only picks up .cpp, so add
# it explicitly. Ym2413_Emu.cpp wraps it to give SMS FM (.sgc) and VGM/VGZ FM.
add_library(gmeplugin GMEPlugin.cpp ${SOURCE_FILES} gme/emu2413.c)
target_compile_definitions(gmeplugin PRIVATE
    BLARGG_LITTLE_ENDIAN HAVE_ZLIB_H VGM_YM2612_NUKED)
#VGM_YM2612_GENS VGM_YM2612_MAME)
target_compile_options(gmeplugin PRIVATE -fwrapv)
target_link_libraries(gmeplugin PRIVATE coreutils ${ZLIB_LIBRARIES})
target_include_directories(gmeplugin PRIVATE psf ${ZLIB_INCLUDE_DIRS})

# s98plugin and famitracker vendor the SAME Okazaki emu2413 (OPLL) core under
# the stock global OPLL_* names, so all three static libs would collide at
# final link. Rename ours to GME_OPLL_* across the core and its wrapper (both
# translation units rewrite together), and hide the core's symbols. This is the
# same technique kssplugin uses with its KSS_ prefix.
set(_gme_opll_renames
    OPLL_new=GME_OPLL_new OPLL_delete=GME_OPLL_delete OPLL_reset=GME_OPLL_reset
    OPLL_resetPatch=GME_OPLL_resetPatch OPLL_writeReg=GME_OPLL_writeReg
    OPLL_writeIO=GME_OPLL_writeIO OPLL_calc=GME_OPLL_calc
    OPLL_calcStereo=GME_OPLL_calcStereo OPLL_setMask=GME_OPLL_setMask
    OPLL_toggleMask=GME_OPLL_toggleMask OPLL_copyPatch=GME_OPLL_copyPatch
    OPLL_forceRefresh=GME_OPLL_forceRefresh
    OPLL_getDefaultPatch=GME_OPLL_getDefaultPatch OPLL_setPatch=GME_OPLL_setPatch
    OPLL_setRate=GME_OPLL_setRate OPLL_setQuality=GME_OPLL_setQuality
    OPLL_setChipType=GME_OPLL_setChipType OPLL_setPan=GME_OPLL_setPan
    OPLL_setPanFine=GME_OPLL_setPanFine OPLL_dumpToPatch=GME_OPLL_dumpToPatch
    OPLL_patchToDump=GME_OPLL_patchToDump
    OPLL_RateConv_new=GME_OPLL_RateConv_new
    OPLL_RateConv_delete=GME_OPLL_RateConv_delete
    OPLL_RateConv_reset=GME_OPLL_RateConv_reset
    OPLL_RateConv_getData=GME_OPLL_RateConv_getData
    OPLL_RateConv_putData=GME_OPLL_RateConv_putData)
set_source_files_properties(gme/emu2413.c PROPERTIES
    COMPILE_FLAGS "-w -fvisibility=hidden"
    COMPILE_DEFINITIONS "${_gme_opll_renames}")
set_source_files_properties(gme/Ym2413_Emu.cpp PROPERTIES
    COMPILE_DEFINITIONS "${_gme_opll_renames}")

