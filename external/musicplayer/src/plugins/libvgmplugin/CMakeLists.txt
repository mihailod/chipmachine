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

# Core selection (see CM_HAVE_LIBVGM_GPL_CORES in chipmachine/CMakeLists.txt for
# the full reasoning). libvgm enables every core when SNDDEV_SELECT is undefined;
# we define it and list chips + cores explicitly below, which is what lets the
# GPL-licensed MAME/Gens cores be left out. Defaults to ON so this tree still
# builds standalone / outside chipmachine.
if(NOT DEFINED CM_HAVE_LIBVGM_GPL_CORES)
    set(CM_HAVE_LIBVGM_GPL_CORES ON)
endif()

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
    # ym2413.c (MAME) and nukedopll.c are GPL-2.0+ AND unreachable -- emu2413 is
    # first in 2413intf.c's core list, so they were never selected. Dropped.
    ${LIBVGM_DIR}/emu/cores/emu2413.c
    ${LIBVGM_DIR}/emu/cores/2612intf.c
    # ym2612.c is the Gens core (no licence statement anywhere; Gens itself is
    # GPL-2) and is unreachable -- GPGX, which lives in fmopn.c, is first. Dropped.
    ${LIBVGM_DIR}/emu/cores/ym3438.c
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
    # ymf262.c (MAME OPL3) is GPL-2.0+ and unreachable -- 262intf.c lists AdLibEmu
    # first, "default, because it's better than MAME", and VGMPlayer additionally
    # hard-requests FCC_ADLE for the YMF262 linked to an OPL4. Dropped.
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
    # nes_apu.c (MAME) is GPL-2.0+ and unreachable -- nesintf.c lists the NSFPlay
    # cores above it. Dropped.
    ${LIBVGM_DIR}/emu/cores/mikey.c
    ${LIBVGM_DIR}/emu/cores/multipcm.c
    ${LIBVGM_DIR}/emu/cores/upd7759.c
    ${LIBVGM_DIR}/emu/cores/okim6258.c
    ${LIBVGM_DIR}/emu/cores/okim6295.c
    ${LIBVGM_DIR}/emu/cores/okiadpcm.c
    ${LIBVGM_DIR}/emu/cores/k051649.c
    ${LIBVGM_DIR}/emu/cores/k054539.c
    ${LIBVGM_DIR}/emu/cores/c6280intf.c
    ${LIBVGM_DIR}/emu/cores/c6280_mame.c
    # c6280_mame.c is the HuC6280 core mas uses; plus prefers Ootake (below).
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

# The two cores where the GPL one IS the active default, so leaving it out is an
# AUDIBLE change: plus keeps exactly what it has always used, mas falls back to a
# non-GPL core that is already compiled in every build.
#   ym2151.c     (MAME YM2151, GPL-2.0+) -> mas uses nukedopm.c   (LGPL-2.1+)
#   Ootake_PSG.c (Ootake HuC6280, GPL)   -> mas uses c6280_mame.c (BSD-3)
# Both swaps were A/B rendered against real game music before being accepted --
# Castlevania (Sharp X68000) for the YM2151 and Aldynes stage 1 (PC Engine) for
# the HuC6280 -- at RMS ratios of 1.00 and 0.98 with no clipping on either side.
# KNOWN CAVEAT, accepted deliberately: the MAME HuC6280 runs hot on
# noise/DDA-saturated material and clips where Ootake does not (BotB "Citizens of
# Luala" 1.49x, 785 clipped samples in 30s vs Ootake's 3). The cause is that
# libvgm's _CHIP_VOLUME table (player/vgmplayer.cpp) carries ONE volume per chip
# TYPE -- hand-corrected for K051649 and C140/C219 only -- so it is calibrated
# for whichever core is listed first, i.e. Ootake. A per-core correction there
# (about 0.7x for MAME) would remove the clipping; nobody has calibrated it yet.
# See CM_HAVE_LIBVGM_GPL_CORES in chipmachine/CMakeLists.txt.
if(CM_HAVE_LIBVGM_GPL_CORES)
    target_sources(libvgmplugin_objs PRIVATE
        ${LIBVGM_DIR}/emu/cores/ym2151.c
        ${LIBVGM_DIR}/emu/cores/Ootake_PSG.c)
endif()

target_include_directories(libvgmplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBVGM_DIR}
    ${ZLIB_INCLUDE_DIRS})

# VGM_LITTLE_ENDIAN: target is arm64/x86_64 LE. HAVE_STDINT_H / HAVE_ZLIB_H:
# libvgm feature probes. USE_ZLIB: FileLoader/MemoryLoader gzip path.
target_compile_definitions(libvgmplugin_objs PRIVATE
    VGM_LITTLE_ENDIAN HAVE_STDINT_H HAVE_ZLIB_H USE_ZLIB)

# SNDDEV_SELECT switches libvgm from "enable every chip and every core" to an
# explicit whitelist. The SNDDEV_ list below is libvgm's own default set from
# emu/SoundEmu.c, copied verbatim so no chip is lost by omission -- adding a chip
# on revendor means adding it here too. The EC_ list picks ONE core per chip
# family for the chips that ship several; a core named here MUST have its .c in
# the source list above, or the link fails.
target_compile_definitions(libvgmplugin_objs PRIVATE
    SNDDEV_SELECT
    # --- chips (verbatim from SoundEmu.c's no-SNDDEV_SELECT block) ---
    SNDDEV_SN76496 SNDDEV_YM2413 SNDDEV_YM2612 SNDDEV_YM2151 SNDDEV_SEGAPCM
    SNDDEV_RF5C68 SNDDEV_YM2203 SNDDEV_YM2608 SNDDEV_YM2610 SNDDEV_YM3812
    SNDDEV_YM3526 SNDDEV_Y8950 SNDDEV_YMF262 SNDDEV_YMF278B SNDDEV_YMZ280B
    SNDDEV_YMF271 SNDDEV_AY8910 SNDDEV_32X_PWM SNDDEV_GAMEBOY SNDDEV_NES_APU
    SNDDEV_YMW258 SNDDEV_UPD7759 SNDDEV_MSM6258 SNDDEV_MSM6295 SNDDEV_K051649
    SNDDEV_K054539 SNDDEV_C6280 SNDDEV_C140 SNDDEV_C219 SNDDEV_K053260
    SNDDEV_POKEY SNDDEV_QSOUND SNDDEV_SCSP SNDDEV_WSWAN SNDDEV_VBOY_VSU
    SNDDEV_SAA1099 SNDDEV_ES5503 SNDDEV_ES5506 SNDDEV_X1_010 SNDDEV_C352
    SNDDEV_GA20 SNDDEV_MIKEY SNDDEV_K007232 SNDDEV_K005289 SNDDEV_MSM5205
    SNDDEV_MSM5232 SNDDEV_BSMT2000 SNDDEV_ICS2115
    # --- cores, for the chips that have more than one ---
    EC_YM2413_EMU2413        # was: + MAME (GPL) + Nuked (GPL), both unreachable
    EC_YM2612_GPGX           # GPGX lives in fmopn.c and is the default
    EC_YM2612_NUKED          # ym3438.c, LGPL-2.1+
    EC_YM2151_NUKED          # nukedopm.c, LGPL-2.1+ (+ MAME below, plus only)
    EC_YM3812_ADLIBEMU       # LGPL-2.1; MAME's YM3812 path dropped. fmopl.c is
    EC_YM3812_NUKED          # still built -- YM3526/Y8950 are MAME-only.
    EC_YMF262_ADLIBEMU EC_YMF262_NUKED   # was: + MAME (GPL), unreachable
    EC_NES_NSFPLAY EC_NES_NSFP_FDS       # was: + MAME (GPL), unreachable
    EC_C6280_MAME            # BSD-3 (+ Ootake below, plus only)
    EC_RF5C68_MAME           # BSD-3, and VGMPlayer hard-requests FCC_GENS for
    EC_RF5C68_GENS           # RF5C164 -- scd_pcm.c cannot be dropped
    EC_QSOUND_MAME EC_QSOUND_CTR
    EC_SAA1099_MAME EC_SAA1099_VB        # NOT _NRS: that core is not vendored
    EC_AY8910_MAME EC_AY8910_EMU2149
    EC_SN76496_MAME EC_SN76496_MAXIM)    # MAME is hard-requested by VGMPlayer
if(CM_HAVE_LIBVGM_GPL_CORES)
    target_compile_definitions(libvgmplugin_objs PRIVATE
        EC_YM2151_MAME EC_C6280_OOTAKE)
endif()
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
