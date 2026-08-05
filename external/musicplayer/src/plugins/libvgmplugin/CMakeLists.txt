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

# The legacy 32X PWM core is GPL. Never let it into a build that is meant to be
# free of GPL, however the developer A/B switch was left set.
if(CM_LIBVGM_LEGACY_PWM AND NOT CM_HAVE_LIBVGM_GPL_CORES)
    message(FATAL_ERROR
        "CM_LIBVGM_LEGACY_PWM builds the GPL Gens PWM core and cannot be used "
        "with CM_HAVE_LIBVGM_GPL_CORES=OFF (the Mac App Store variant).")
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
    # scd_pcm.c (Gens RF5C164) is plus-only -- see the variant block below.
    ${LIBVGM_DIR}/emu/cores/opnintf.c
    ${LIBVGM_DIR}/emu/cores/ymdeltat.c
    ${LIBVGM_DIR}/emu/cores/oplintf.c
    ${LIBVGM_DIR}/emu/cores/adlibemu_opl2.c
    # fmopl.c (MAME YM3526/Y8950/YM3812) is plus-only -- see the variant block
    # below. Its YM3812 path was already unreachable (AdLibEmu is listed first).
    ${LIBVGM_DIR}/emu/cores/262intf.c
    ${LIBVGM_DIR}/emu/cores/nukedopl3.c
    ${LIBVGM_DIR}/emu/cores/adlibemu_opl3.c
    # ymf262.c (MAME OPL3) is GPL-2.0+ and unreachable -- 262intf.c lists AdLibEmu
    # first, "default, because it's better than MAME", and VGMPlayer additionally
    # hard-requests FCC_ADLE for the YMF262 linked to an OPL4. Dropped.
    # ymf278b.c (openMSX/MAME OPL4 wave engine) is plus-only -- see below.
    ${LIBVGM_DIR}/emu/cores/ymz280b.c
    ${LIBVGM_DIR}/emu/cores/ymf271.c
    ${LIBVGM_DIR}/emu/cores/ayintf.c
    ${LIBVGM_DIR}/emu/cores/emu2149.c
    ${LIBVGM_DIR}/emu/cores/ay8910.c
    # pwm.c (Gens 32X PWM, GPL-2 by descent) is REPLACED in both variants by
    # chipmachine's own pwm_32x.c, written against the documented registers.
    # CM_LIBVGM_LEGACY_PWM builds the old GPL core instead. It exists only so a
    # future change to pwm_32x.c can be A/B'd against the core it replaced, with
    # that file as the single variable. It is refused for mas below.
    $<IF:$<BOOL:${CM_LIBVGM_LEGACY_PWM}>,${LIBVGM_DIR}/emu/cores/pwm.c,${CMAKE_CURRENT_SOURCE_DIR}/pwm_32x.c>
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
    # msm5232.c (MAME, GPL-2.0+) is plus-only -- see the variant block below.
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
        ${LIBVGM_DIR}/emu/cores/Ootake_PSG.c
        # fmopn.c holds the MAME OPN family AND the GPGX YM2612 core. mas builds
        # ymfm instead (see below); this keeps plus byte-identical.
        ${LIBVGM_DIR}/emu/cores/fmopn.c
        # fmopl.c is the MAME OPL file. Its YM3812 path is dead (AdLibEmu wins
        # that chip), but it is the ONLY core libvgm ships for the YM3526 and the
        # Y8950 -- mas builds ymfm for those two instead.
        ${LIBVGM_DIR}/emu/cores/fmopl.c
        # ymf278b.c is the OPL4 WAVE engine (the FM half is a separate linked
        # YMF262 device). mas builds ymfm's OPL4 wave engine instead, keeping the
        # same linked AdLibEmu YMF262.
        ${LIBVGM_DIR}/emu/cores/ymf278b.c
        # scd_pcm.c is the Gens RF5C164. libvgm prefers it for that chip, but
        # only as a DEFAULT in vgmplayer.cpp -- BSD-3 rf5c68.c (MAME) implements
        # the same device and is compiled in both variants, so mas uses that.
        # See the local patch at "case DEVID_RF5C68" in player/vgmplayer.cpp.
        ${LIBVGM_DIR}/emu/cores/scd_pcm.c
        # msm5232.c is UNREACHABLE from any VGM that exists, so mas simply drops
        # it and needs no replacement and no index gate. libvgm reads the chip's
        # clock from header offset 0xF4, which is one of its own extension slots:
        # the VGM spec has never defined a field there, and even a v1.71 header
        # ends at 0xD0. VGMRips has no MSM5232 pack at all. Kept in plus only so
        # the chip is there if the format ever grows into that range.
        ${LIBVGM_DIR}/emu/cores/msm5232.c)
endif()

# Three chip groups have NO permissive core in libvgm at all, only a GPL one:
#
#   YM2203 / YM2608 / YM2610   fmopn.c   (MAME, GPL-2.0+)
#   YM3526 / Y8950             fmopl.c   (MAME, GPL-2.0+)
#   YMF278B (OPL4 wave)        ymf278b.c (openMSX/MAME, GPL-2.0)
#
# Dropping them outright would lose all of NEC PC-98/PC-88 and Neo Geo (~600-700
# rows), every YM3526 and Y8950 log, and the wavetable half of every MSX
# MoonSound track. So opn_ymfm.cpp and opl_ymfm.cpp wrap Aaron Giles' ymfm
# (BSD-3, external/ymfm, vendored from upstream -- NOT from Furnace's modified
# copy) in libvgm's DEV_DEF interface. ymfm's OPM/OPZ/OPLL cores stay unused,
# because libvgm already has permissive or LGPL cores for those chips.
#
# plus keeps all three MAME files (unchanged audio, and fmopn.c still owns the
# GPGX YM2612 core); mas builds ymfm instead. opnintf.c/.h and oplintf.c/.h carry
# a local patch adding the EC_*_MAME / EC_*_YMFM selection -- grep
# "chipmachine local patch" there, re-apply on revendor, .orig copies sit next to
# them. ymf278b.c is a single-core file and needed no patch: opl_ymfm.cpp simply
# supplies the sndDev_YMF278B DEV_DECL in the build where that file is absent.
# The OPL slice is compiled in BOTH variants, because one of its chips is not a
# licence swap: the ymfm YM3526 is used everywhere. It was A/B'd against the MAME
# core on Karnov and Athena (SNK) and PREFERRED by ear on the one track where the
# two measurably differ -- Athena's "Theme of Titan", 0.71x RMS, which is the OPL
# rhythm/percussion mode that MAME and ymfm are known to render differently. So
# plus takes it too, and fmopl.c's YM3526 path joins its YM3812 path as dead code
# there. The OPN slice stays mas-only.
set(YMFM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ymfm)
set(YMFM_SOURCES
    opl_ymfm.cpp
    ${YMFM_DIR}/src/ymfm_opl.cpp
    ${YMFM_DIR}/src/ymfm_adpcm.cpp     # Y8950 ADPCM-B, and ymfm_opl.cpp needs it
    ${YMFM_DIR}/src/ymfm_pcm.cpp)      # YMF278B wavetable, likewise
if(NOT CM_HAVE_LIBVGM_GPL_CORES)
    list(APPEND YMFM_SOURCES
        opn_ymfm.cpp
        ${YMFM_DIR}/src/ymfm_opn.cpp
        ${YMFM_DIR}/src/ymfm_ssg.cpp)
endif()
target_sources(libvgmplugin_objs PRIVATE ${YMFM_SOURCES})
target_include_directories(libvgmplugin_objs PRIVATE ${YMFM_DIR}/src)
set_source_files_properties(${YMFM_SOURCES} PROPERTIES COMPILE_OPTIONS "-std=c++17")

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
    SNDDEV_BSMT2000 SNDDEV_ICS2115
    # NOTE: SNDDEV_MSM5232 is the ONE omission from SoundEmu.c's list, and it is
    # deliberate -- it is defined in the plus-only block below. Unlike the other
    # chips here, the MSM5232's DEV_DECL lives inside its GPL core file rather
    # than a separate *intf.c, so the chip and the core cannot be gated apart.
    # --- cores, for the chips that have more than one ---
    EC_YM2413_EMU2413        # was: + MAME (GPL) + Nuked (GPL), both unreachable
    EC_YM2612_NUKED          # ym3438.c, LGPL-2.1+ (GPGX lives in fmopn.c, so it
                             # is plus-only -- see the variant block below)
    EC_YM2151_NUKED          # nukedopm.c, LGPL-2.1+ (+ MAME below, plus only)
    EC_YM3812_ADLIBEMU       # LGPL-2.1; MAME's YM3812 path was already dead code
    EC_YM3812_NUKED          # (AdLibEmu is listed first in oplintf.c)
    EC_YM3526_YMFM           # BSD-3, BOTH variants -- preferred by ear over the
                             # MAME core, see the ymfm block below
    EC_YMF262_ADLIBEMU EC_YMF262_NUKED   # was: + MAME (GPL), unreachable
    EC_NES_NSFPLAY EC_NES_NSFP_FDS       # was: + MAME (GPL), unreachable
    EC_C6280_MAME            # BSD-3 (+ Ootake below, plus only)
    EC_RF5C68_MAME           # BSD-3; also serves the RF5C164 in mas, where the
                             # Gens core is absent (+ EC_RF5C68_GENS below, plus only)
    EC_QSOUND_MAME EC_QSOUND_CTR
    EC_SAA1099_MAME EC_SAA1099_VB        # NOT _NRS: that core is not vendored
    EC_AY8910_MAME EC_AY8910_EMU2149
    EC_SN76496_MAME EC_SN76496_MAXIM)    # MAME is hard-requested by VGMPlayer
if(CM_HAVE_LIBVGM_GPL_CORES)
    target_compile_definitions(libvgmplugin_objs PRIVATE
        EC_YM2151_MAME EC_C6280_OOTAKE
        # fmopn.c: the MAME OPN cores, and the GPGX YM2612 core that lives in the
        # same file and is libvgm's first choice for that chip.
        EC_YM2203_MAME EC_YM2608_MAME EC_YM2610_MAME EC_YM2612_GPGX
        # fmopl.c stays in plus for the Y8950 alone -- its YM3526 path is now
        # dead code here too (EC_YM3526_YMFM above wins for both variants).
        EC_Y8950_MAME
        # scd_pcm.c. This macro ALSO drives the RF5C164 default in vgmplayer.cpp.
        EC_RF5C68_GENS
        # msm5232.c holds its own DEV_DECL, so the chip goes with the core.
        SNDDEV_MSM5232)
else()
    target_compile_definitions(libvgmplugin_objs PRIVATE
        EC_YM2203_YMFM EC_YM2608_YMFM EC_YM2610_YMFM
        # The YM2608's rhythm section plays from an ADPCM-A ROM inside the chip;
        # without it every PC-98 track loses its drums. See opn_ymfm.cpp.
        OPN_YMFM_RHYTHM_ROM
        # The other two chips in opl_ymfm.cpp -- these ARE licence swaps, since
        # fmopl.c and ymf278b.c are GPL and cannot ship in mas. EC_YMF278B_YMFM
        # additionally makes opl_ymfm.cpp define sndDev_YMF278B, which normally
        # lives in the (absent) ymf278b.c.
        EC_Y8950_YMFM EC_YMF278B_YMFM)
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

# A/B probe for core swaps -- OFF by default, it is a developer tool. It links
# the OBJECT library directly (before the `ld -r` visibility pass), so it gets
# exactly this variant's source list and SNDDEV_/EC_ selection. See
# "Verifying a core swap" in README.md.
option(CM_BUILD_VGM_PROBE "Build the libvgm chip-core A/B probe" OFF)
if(CM_BUILD_VGM_PROBE)
    add_executable(vgm_core_probe vgm_core_probe.cpp $<TARGET_OBJECTS:libvgmplugin_objs>)
    target_include_directories(vgm_core_probe PRIVATE ${LIBVGM_DIR} ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(vgm_core_probe PRIVATE coreutils ZLIB::ZLIB)
    set_target_properties(vgm_core_probe PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
endif()
