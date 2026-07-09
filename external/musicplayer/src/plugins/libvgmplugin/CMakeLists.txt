cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.libvgmplugin)

# OPL-family VGM/VGZ player. Wraps ValleyBell's libvgm (the same vendored tree
# zxtune ships under zxtune/3rdparty/vgm, MIT/BSD -- see its LICENSE), driven
# through PlayerA + VGMPlayer. GME's Vgm_Emu only knows PSG/YM2413/YM2612 and
# plays OPL2 (YM3812) logs silent -- or aborts on them (Blip_Buffer assertion) --
# so OPL-carrying VGMs route here instead. The source list mirrors libvgm's own
# CMake `vgm` target (all chip cores, so SoundEmu's device table resolves), plus
# the gzip-capable FileLoader/MemoryLoader compiled with USE_ZLIB (reads .vgz
# directly, no external gunzip).
#
# libvgm bundles chip cores (ay8910, ym2612, ym2413, sn76489, emu2413, nes_apu,
# gb, ...) whose global C symbols collide with GME / s98 / famitracker / kss.
# Same fix as dmf/goattracker/ned/mikmod: compile the slice with hidden
# visibility and partial-link with `ld -r`, demoting those externs to true
# locals. Only LibVGMPlugin.cpp stays at default visibility so musix::LibVGMPlugin
# and libvgmplugin_register remain the exported surface.

set(LIBVGM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../zxtune/3rdparty/vgm)

find_package(ZLIB REQUIRED)

add_library(libvgmplugin_objs OBJECT
    LibVGMPlugin.cpp
    # --- libvgm infrastructure + all chip cores (mirrors libvgm's `vgm` target) ---
    ${LIBVGM_DIR}/emu/SoundEmu.c
    ${LIBVGM_DIR}/emu/Resampler.c
    ${LIBVGM_DIR}/emu/panning.c
    ${LIBVGM_DIR}/emu/dac_control.c
    ${LIBVGM_DIR}/emu/logging_stub.c
    ${LIBVGM_DIR}/utils/DataLoader.c
    ${LIBVGM_DIR}/utils/StrUtils-CPConv_Stub.c
    ${LIBVGM_DIR}/player/helper.c
    ${LIBVGM_DIR}/player/dblk_compr.c
    ${LIBVGM_DIR}/emu/cores/sn764intf.c
    ${LIBVGM_DIR}/emu/cores/sn76496.c
    ${LIBVGM_DIR}/emu/cores/sn76489.c
    ${LIBVGM_DIR}/emu/cores/2413intf.c
    ${LIBVGM_DIR}/emu/cores/ym2413.c
    ${LIBVGM_DIR}/emu/cores/nukedopll.c
    ${LIBVGM_DIR}/emu/cores/emu2413.c
    ${LIBVGM_DIR}/emu/cores/2612intf.c
    ${LIBVGM_DIR}/emu/cores/ym2612.c
    ${LIBVGM_DIR}/emu/cores/ym3438.c
    ${LIBVGM_DIR}/emu/cores/ym2151.c
    ${LIBVGM_DIR}/emu/cores/2151intf.c
    ${LIBVGM_DIR}/emu/cores/nukedopm.c
    ${LIBVGM_DIR}/emu/cores/segapcm.c
    ${LIBVGM_DIR}/emu/cores/rf5cintf.c
    ${LIBVGM_DIR}/emu/cores/rf5c68.c
    ${LIBVGM_DIR}/emu/cores/scd_pcm.c
    ${LIBVGM_DIR}/emu/cores/opnintf.c
    ${LIBVGM_DIR}/emu/cores/fmopn.c
    ${LIBVGM_DIR}/emu/cores/ymdeltat.c
    ${LIBVGM_DIR}/emu/cores/oplintf.c
    ${LIBVGM_DIR}/emu/cores/adlibemu_opl2.c
    ${LIBVGM_DIR}/emu/cores/fmopl.c
    ${LIBVGM_DIR}/emu/cores/262intf.c
    ${LIBVGM_DIR}/emu/cores/nukedopl3.c
    ${LIBVGM_DIR}/emu/cores/adlibemu_opl3.c
    ${LIBVGM_DIR}/emu/cores/ymf262.c
    ${LIBVGM_DIR}/emu/cores/ymf278b.c
    ${LIBVGM_DIR}/emu/cores/ymz280b.c
    ${LIBVGM_DIR}/emu/cores/ymf271.c
    ${LIBVGM_DIR}/emu/cores/ayintf.c
    ${LIBVGM_DIR}/emu/cores/emu2149.c
    ${LIBVGM_DIR}/emu/cores/ay8910.c
    ${LIBVGM_DIR}/emu/cores/pwm.c
    ${LIBVGM_DIR}/emu/cores/gb.c
    ${LIBVGM_DIR}/emu/cores/nesintf.c
    ${LIBVGM_DIR}/emu/cores/np_nes_apu.c
    ${LIBVGM_DIR}/emu/cores/np_nes_dmc.c
    ${LIBVGM_DIR}/emu/cores/np_nes_fds.c
    ${LIBVGM_DIR}/emu/cores/nes_apu.c
    ${LIBVGM_DIR}/emu/cores/mikey.c
    ${LIBVGM_DIR}/emu/cores/multipcm.c
    ${LIBVGM_DIR}/emu/cores/upd7759.c
    ${LIBVGM_DIR}/emu/cores/okim6258.c
    ${LIBVGM_DIR}/emu/cores/okim6295.c
    ${LIBVGM_DIR}/emu/cores/okiadpcm.c
    ${LIBVGM_DIR}/emu/cores/k051649.c
    ${LIBVGM_DIR}/emu/cores/k054539.c
    ${LIBVGM_DIR}/emu/cores/c6280intf.c
    ${LIBVGM_DIR}/emu/cores/Ootake_PSG.c
    ${LIBVGM_DIR}/emu/cores/c6280_mame.c
    ${LIBVGM_DIR}/emu/cores/c140.c
    ${LIBVGM_DIR}/emu/cores/c219.c
    ${LIBVGM_DIR}/emu/cores/k053260.c
    ${LIBVGM_DIR}/emu/cores/pokey.c
    ${LIBVGM_DIR}/emu/cores/qsoundintf.c
    ${LIBVGM_DIR}/emu/cores/qsound_ctr.c
    ${LIBVGM_DIR}/emu/cores/qsound_mame.c
    ${LIBVGM_DIR}/emu/cores/scsp.c
    ${LIBVGM_DIR}/emu/cores/scspdsp.c
    ${LIBVGM_DIR}/emu/cores/ws_audio.c
    ${LIBVGM_DIR}/emu/cores/vsu.c
    ${LIBVGM_DIR}/emu/cores/saaintf.c
    ${LIBVGM_DIR}/emu/cores/saa1099_vb.c
    ${LIBVGM_DIR}/emu/cores/saa1099_mame.c
    ${LIBVGM_DIR}/emu/cores/es5503.c
    ${LIBVGM_DIR}/emu/cores/es5506.c
    ${LIBVGM_DIR}/emu/cores/x1_010.c
    ${LIBVGM_DIR}/emu/cores/c352.c
    ${LIBVGM_DIR}/emu/cores/iremga20.c
    ${LIBVGM_DIR}/emu/cores/bsmt2000.c
    ${LIBVGM_DIR}/emu/cores/ics2115.c
    ${LIBVGM_DIR}/emu/cores/k005289.c
    ${LIBVGM_DIR}/emu/cores/k007232.c
    ${LIBVGM_DIR}/emu/cores/msm5205.c
    ${LIBVGM_DIR}/emu/cores/msm5232.c
    # gzip-capable loaders (USE_ZLIB -> reads .vgz transparently)
    ${LIBVGM_DIR}/utils/FileLoader.c
    ${LIBVGM_DIR}/utils/MemoryLoader.c
    # player engine (only VGM is registered at runtime)
    ${LIBVGM_DIR}/player/playerbase.cpp
    ${LIBVGM_DIR}/player/playera.cpp
    ${LIBVGM_DIR}/player/vgmplayer.cpp
    ${LIBVGM_DIR}/player/vgmplayer_cmdhandler.cpp
)

target_include_directories(libvgmplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBVGM_DIR}
    ${ZLIB_INCLUDE_DIRS})

# VGM_LITTLE_ENDIAN: target is arm64/x86_64 LE. HAVE_STDINT_H / HAVE_ZLIB_H:
# libvgm feature probes. USE_ZLIB: FileLoader/MemoryLoader gzip path.
target_compile_definitions(libvgmplugin_objs PRIVATE
    VGM_LITTLE_ENDIAN HAVE_STDINT_H HAVE_ZLIB_H USE_ZLIB)
target_compile_options(libvgmplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden -w)
set_target_properties(libvgmplugin_objs PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

# LibVGMPlugin.cpp is the exported surface: keep it at default visibility so
# musix::LibVGMPlugin + libvgmplugin_register survive the hidden-visibility pass.
# It needs coreutils (utils::path_extension) and C++17 (chipplugin.h static inline).
set_source_files_properties(LibVGMPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-std=c++17;-fvisibility=default;-fvisibility-inlines-hidden")
# Propagate coreutils' INTERFACE include dir to the object compile (utils.h).
target_link_libraries(libvgmplugin_objs PRIVATE coreutils)

set(LIBVGM_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/libvgmplugin_combined.o)
add_custom_command(
    OUTPUT ${LIBVGM_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:libvgmplugin_objs> -o ${LIBVGM_COMBINED}
    DEPENDS libvgmplugin_objs $<TARGET_OBJECTS:libvgmplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing libvgmplugin symbols (ld -r)")
add_custom_target(libvgmplugin_combine DEPENDS ${LIBVGM_COMBINED})

add_library(libvgmplugin STATIC ${LIBVGM_COMBINED})
add_dependencies(libvgmplugin libvgmplugin_combine)
set_target_properties(libvgmplugin PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(libvgmplugin PRIVATE coreutils ZLIB::ZLIB)
if(NOT WIN32)
    target_link_libraries(libvgmplugin PRIVATE m)
endif()
