cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.dmfplugin)

# DefleMask .dmf player. Wraps a headless slice of the Furnace engine
# (tildearrow/furnace, GPLv2 -- see ${FURNACE_DIR}/LICENSE and README) driven
# directly through DivEngine::load()/nextBuf(); no GUI, SDL, or libsndfile. The
# vendored slice is exactly the source set Furnace's own `BUILD_GUI=OFF`
# headless target compiles, minus main.cpp/CLI and the sndfile-only sfWrapper.
#
# Furnace bundles its own chip cores (sn76496, ay8910, segapcm, NES apu, Nuked-*
# OPN/OPLL, emu2413, SID, ...) whose global C symbols collide with GME / s98 /
# famitracker. Same fix as goattracker/ned/mikmod: compile everything with
# hidden visibility and partial-link with `ld -r`, which demotes those private
# externs to true locals. Only DMFPlugin.cpp stays at default visibility so
# musix::DMFPlugin and dmfplugin_register remain the exported surface.

set(FURNACE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/furnace)

find_package(ZLIB REQUIRED)   # DivEngine::load inflates the zlib-wrapped .dmf

add_library(dmfplugin_objs OBJECT
    DMFPlugin.cpp
    DMFPlugin.h
    # No-op libsndfile stub: Furnace's DivFilePlayer/SFWrapper reference sndfile
    # only for external audio-file instruments (never used by .dmf), so we stub
    # the handful of symbols instead of vendoring libsndfile.
    sndfile_stub/sndfile_stub.c
    furnace_stubs.cpp   # reportError() etc. normally in Furnace's main.cpp
    # Furnace's bundled fmt, compiled (matches Furnace's own build; header-only
    # mode changes fmt::vsprintf overload resolution and does not compile).
    ${FURNACE_DIR}/extern/fmt/src/format.cc
    ${FURNACE_DIR}/extern/fmt/src/os.cc
    ${FURNACE_DIR}/src/engine/sfWrapper.cpp
    ${FURNACE_DIR}/extern/ESFMu/esfm.c
    ${FURNACE_DIR}/extern/ESFMu/esfm_registers.c
    ${FURNACE_DIR}/extern/Nuked-OPLL/opll.c
    ${FURNACE_DIR}/extern/Nuked-PSG/ympsg.c
    ${FURNACE_DIR}/extern/SAASound/src/SAAAmp.cpp
    ${FURNACE_DIR}/extern/SAASound/src/SAADevice.cpp
    ${FURNACE_DIR}/extern/SAASound/src/SAAEnv.cpp
    ${FURNACE_DIR}/extern/SAASound/src/SAAFreq.cpp
    ${FURNACE_DIR}/extern/SAASound/src/SAAImpl.cpp
    ${FURNACE_DIR}/extern/SAASound/src/SAANoise.cpp
    ${FURNACE_DIR}/extern/SAASound/src/SAASndC.cpp
    ${FURNACE_DIR}/extern/SAASound/src/SAASound.cpp
    ${FURNACE_DIR}/extern/YM2608-LLE/fmopna_2608.c
    ${FURNACE_DIR}/extern/YM2608-LLE/fmopna_2610.c
    ${FURNACE_DIR}/extern/YM2608-LLE/fmopna_2612.c
    ${FURNACE_DIR}/extern/YM3812-LLE/fmopl2.c
    ${FURNACE_DIR}/extern/YMF262-LLE/fmopl3.c
    ${FURNACE_DIR}/extern/YMF276-LLE/fmopn2.c
    ${FURNACE_DIR}/extern/adpcm-xq-s/adpcm-lib.c
    ${FURNACE_DIR}/extern/adpcm/bs_codec.c
    ${FURNACE_DIR}/extern/adpcm/oki_codec.c
    ${FURNACE_DIR}/extern/adpcm/yma_codec.c
    ${FURNACE_DIR}/extern/adpcm/ymb_codec.c
    ${FURNACE_DIR}/extern/adpcm/ymz_codec.c
    ${FURNACE_DIR}/extern/blip_buf/blip_buf.c
    ${FURNACE_DIR}/extern/emu2413/emu2413.c
    ${FURNACE_DIR}/extern/itcompress/compression.c
    ${FURNACE_DIR}/extern/macports-legacy-support/src/time.c
    ${FURNACE_DIR}/extern/opl/opl3.c
    ${FURNACE_DIR}/extern/opm/opm.c
    ${FURNACE_DIR}/extern/opn/ym3438.c
    ${FURNACE_DIR}/extern/pwrnoise/pwrnoise.c
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/core/vox/vox.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/es550x/es5504.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/es550x/es5505.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/es550x/es5506.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/es550x/es550x.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/es550x/es550x_alu.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/es550x/es550x_filter.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/k005289/k005289.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/k007232/k007232.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/k053260/k053260.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/msm6295/msm6295.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/n163/n163.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/scc/scc.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/vrcvi/vrcvi.cpp
    ${FURNACE_DIR}/extern/vgsound_emu-modified/vgsound_emu/src/x1_010/x1_010.cpp
    ${FURNACE_DIR}/src/audio/abstract.cpp
    ${FURNACE_DIR}/src/audio/midi.cpp
    ${FURNACE_DIR}/src/audio/pipe.cpp
    ${FURNACE_DIR}/src/baseutils.cpp
    ${FURNACE_DIR}/src/engine/assetDir.cpp
    ${FURNACE_DIR}/src/engine/brrUtils.c
    ${FURNACE_DIR}/src/engine/cmdStream.cpp
    ${FURNACE_DIR}/src/engine/cmdStreamOps.cpp
    ${FURNACE_DIR}/src/engine/config.cpp
    ${FURNACE_DIR}/src/engine/configEngine.cpp
    ${FURNACE_DIR}/src/engine/dispatchContainer.cpp
    ${FURNACE_DIR}/src/engine/effect/abstract.cpp
    ${FURNACE_DIR}/src/engine/effect/dummy.cpp
    ${FURNACE_DIR}/src/engine/engine.cpp
    ${FURNACE_DIR}/src/engine/export.cpp
    ${FURNACE_DIR}/src/engine/export/abstract.cpp
    ${FURNACE_DIR}/src/engine/export/amigaValidation.cpp
    ${FURNACE_DIR}/src/engine/export/grub.cpp
    ${FURNACE_DIR}/src/engine/export/ipod.cpp
    ${FURNACE_DIR}/src/engine/export/sapr.cpp
    ${FURNACE_DIR}/src/engine/export/tiuna.cpp
    ${FURNACE_DIR}/src/engine/export/zsm.cpp
    ${FURNACE_DIR}/src/engine/exportDef.cpp
    ${FURNACE_DIR}/src/engine/fileOps/dmf.cpp
    ${FURNACE_DIR}/src/engine/fileOps/fc.cpp
    ${FURNACE_DIR}/src/engine/fileOps/fileOpsCommon.cpp
    ${FURNACE_DIR}/src/engine/fileOps/ftm.cpp
    ${FURNACE_DIR}/src/engine/fileOps/fur.cpp
    ${FURNACE_DIR}/src/engine/fileOps/it.cpp
    ${FURNACE_DIR}/src/engine/fileOps/mod.cpp
    ${FURNACE_DIR}/src/engine/fileOps/p.cpp
    ${FURNACE_DIR}/src/engine/fileOps/p86.cpp
    ${FURNACE_DIR}/src/engine/fileOps/pdx.cpp
    ${FURNACE_DIR}/src/engine/fileOps/ppc.cpp
    ${FURNACE_DIR}/src/engine/fileOps/pps.cpp
    ${FURNACE_DIR}/src/engine/fileOps/pvi.cpp
    ${FURNACE_DIR}/src/engine/fileOps/pzi.cpp
    ${FURNACE_DIR}/src/engine/fileOps/s3m.cpp
    ${FURNACE_DIR}/src/engine/fileOps/text.cpp
    ${FURNACE_DIR}/src/engine/fileOps/tfm.cpp
    ${FURNACE_DIR}/src/engine/fileOps/xm.cpp
    ${FURNACE_DIR}/src/engine/fileOpsIns.cpp
    ${FURNACE_DIR}/src/engine/fileOpsSample.cpp
    ${FURNACE_DIR}/src/engine/filePlayer.cpp
    ${FURNACE_DIR}/src/engine/filter.cpp
    ${FURNACE_DIR}/src/engine/instrument.cpp
    ${FURNACE_DIR}/src/engine/legacySample.cpp
    ${FURNACE_DIR}/src/engine/macroInt.cpp
    ${FURNACE_DIR}/src/engine/pattern.cpp
    ${FURNACE_DIR}/src/engine/pitchTable.cpp
    ${FURNACE_DIR}/src/engine/platform/abstract.cpp
    ${FURNACE_DIR}/src/engine/platform/amiga.cpp
    ${FURNACE_DIR}/src/engine/platform/arcade.cpp
    ${FURNACE_DIR}/src/engine/platform/ay.cpp
    ${FURNACE_DIR}/src/engine/platform/ay8930.cpp
    ${FURNACE_DIR}/src/engine/platform/bifurcator.cpp
    ${FURNACE_DIR}/src/engine/platform/bubsyswsg.cpp
    ${FURNACE_DIR}/src/engine/platform/c140.cpp
    ${FURNACE_DIR}/src/engine/platform/c64.cpp
    ${FURNACE_DIR}/src/engine/platform/dave.cpp
    ${FURNACE_DIR}/src/engine/platform/dummy.cpp
    ${FURNACE_DIR}/src/engine/platform/es5506.cpp
    ${FURNACE_DIR}/src/engine/platform/esfm.cpp
    ${FURNACE_DIR}/src/engine/platform/fds.cpp
    ${FURNACE_DIR}/src/engine/platform/ga20.cpp
    ${FURNACE_DIR}/src/engine/platform/gb.cpp
    ${FURNACE_DIR}/src/engine/platform/gbadma.cpp
    ${FURNACE_DIR}/src/engine/platform/gbaminmod.cpp
    ${FURNACE_DIR}/src/engine/platform/genesis.cpp
    ${FURNACE_DIR}/src/engine/platform/genesisext.cpp
    ${FURNACE_DIR}/src/engine/platform/k007232.cpp
    ${FURNACE_DIR}/src/engine/platform/k053260.cpp
    ${FURNACE_DIR}/src/engine/platform/lynx.cpp
    ${FURNACE_DIR}/src/engine/platform/mmc5.cpp
    ${FURNACE_DIR}/src/engine/platform/msm5232.cpp
    ${FURNACE_DIR}/src/engine/platform/msm6258.cpp
    ${FURNACE_DIR}/src/engine/platform/msm6295.cpp
    ${FURNACE_DIR}/src/engine/platform/multipcm.cpp
    ${FURNACE_DIR}/src/engine/platform/n163.cpp
    ${FURNACE_DIR}/src/engine/platform/namcowsg.cpp
    ${FURNACE_DIR}/src/engine/platform/nds.cpp
    ${FURNACE_DIR}/src/engine/platform/nes.cpp
    ${FURNACE_DIR}/src/engine/platform/opl.cpp
    ${FURNACE_DIR}/src/engine/platform/oplAInterface.cpp
    ${FURNACE_DIR}/src/engine/platform/opll.cpp
    ${FURNACE_DIR}/src/engine/platform/pce.cpp
    ${FURNACE_DIR}/src/engine/platform/pcmdac.cpp
    ${FURNACE_DIR}/src/engine/platform/pcspkr.cpp
    ${FURNACE_DIR}/src/engine/platform/pet.cpp
    ${FURNACE_DIR}/src/engine/platform/pokemini.cpp
    ${FURNACE_DIR}/src/engine/platform/pokey.cpp
    ${FURNACE_DIR}/src/engine/platform/pong.cpp
    ${FURNACE_DIR}/src/engine/platform/powernoise.cpp
    ${FURNACE_DIR}/src/engine/platform/pv1000.cpp
    ${FURNACE_DIR}/src/engine/platform/qsound.cpp
    ${FURNACE_DIR}/src/engine/platform/rf5c68.cpp
    ${FURNACE_DIR}/src/engine/platform/saa.cpp
    ${FURNACE_DIR}/src/engine/platform/scc.cpp
    ${FURNACE_DIR}/src/engine/platform/scvtone.cpp
    ${FURNACE_DIR}/src/engine/platform/segapcm.cpp
    ${FURNACE_DIR}/src/engine/platform/sid2.cpp
    ${FURNACE_DIR}/src/engine/platform/sid3.cpp
    ${FURNACE_DIR}/src/engine/platform/sm8521.cpp
    ${FURNACE_DIR}/src/engine/platform/sms.cpp
    ${FURNACE_DIR}/src/engine/platform/snes.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/atomicssg/ssg.c
    ${FURNACE_DIR}/src/engine/platform/sound/ay8910.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c140_c219.c
    ${FURNACE_DIR}/src/engine/platform/sound/c64/envelope.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/extfilt.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/filter.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/pot.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/sid.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/version.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/voice.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave6581_PST.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave6581_PS_.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave6581_P_T.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave6581__ST.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave8580_PST.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave8580_PS_.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave8580_P_T.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64/wave8580__ST.cc
    ${FURNACE_DIR}/src/engine/platform/sound/c64_d/dsid.c
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/Dac.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/EnvelopeGenerator.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/ExternalFilter.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/Filter.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/Filter6581.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/Filter8580.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/FilterModelConfig.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/FilterModelConfig6581.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/FilterModelConfig8580.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/Integrator6581.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/Integrator8580.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/OpAmp.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/SID.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/Spline.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/WaveformCalculator.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/WaveformGenerator.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/array.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/c64_fp/resample/SincResampler.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/d65modified.c
    ${FURNACE_DIR}/src/engine/platform/sound/dave/dave.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ga20/iremga20.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/gb/apu.c
    ${FURNACE_DIR}/src/engine/platform/sound/gb/timing.c
    ${FURNACE_DIR}/src/engine/platform/sound/lynx/Mikey.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/namco.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nds.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes/apu.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes/fds.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes/mmc5.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/5e01_apu.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/5e01_dmc.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/nes_apu.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/nes_dmc.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/nes_fds.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/nes_mmc5.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/nes_n106.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/nes_nsfplay/nes_vrc6.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/oki/msm5232.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/oki/okim6258.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/pce_psg.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/pokey/AltASAP.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/pokey/mzpokeysnd.c
    ${FURNACE_DIR}/src/engine/platform/sound/qsound.c
    ${FURNACE_DIR}/src/engine/platform/sound/rf5c68.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/saa1099.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/segapcm.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/envelope.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/filter.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/sid.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/version.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/voice.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/wave.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/wave8580_PST.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/wave8580_PS_.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/wave8580_P_T.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid2/wave8580__ST.cc
    ${FURNACE_DIR}/src/engine/platform/sound/sid3.c
    ${FURNACE_DIR}/src/engine/platform/sound/sm8521.c
    ${FURNACE_DIR}/src/engine/platform/sound/sn76496.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/snes/SPC_DSP.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/su.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/supervision.c
    ${FURNACE_DIR}/src/engine/platform/sound/swan.c
    ${FURNACE_DIR}/src/engine/platform/sound/t6w28/T6W28_Apu.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ted-sound.c
    ${FURNACE_DIR}/src/engine/platform/sound/tia/Audio.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/tia/AudioChannel.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/upd1771.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/vera_pcm.c
    ${FURNACE_DIR}/src/engine/platform/sound/vera_psg.c
    # sound/vic20sound.c deliberately omitted: VICE-derived GPLv2, and its only
    # user (platform/vic20.cpp, also omitted) drives a .fur-only target that
    # DMFPlugin can never reach. See dispatchContainer.cpp's local patch.
    ${FURNACE_DIR}/src/engine/platform/sound/vsu.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymf278b/ymf278.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymfm/ymfm_adpcm.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymfm/ymfm_opl.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymfm/ymfm_opm.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymfm/ymfm_opn.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymfm/ymfm_opz.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymfm/ymfm_pcm.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymfm/ymfm_ssg.cpp
    ${FURNACE_DIR}/src/engine/platform/sound/ymz280b.cpp
    ${FURNACE_DIR}/src/engine/platform/su.cpp
    ${FURNACE_DIR}/src/engine/platform/supervision.cpp
    ${FURNACE_DIR}/src/engine/platform/swan.cpp
    ${FURNACE_DIR}/src/engine/platform/t6w28.cpp
    ${FURNACE_DIR}/src/engine/platform/ted.cpp
    ${FURNACE_DIR}/src/engine/platform/tia.cpp
    ${FURNACE_DIR}/src/engine/platform/tx81z.cpp
    ${FURNACE_DIR}/src/engine/platform/vb.cpp
    ${FURNACE_DIR}/src/engine/platform/vera.cpp
    # platform/vic20.cpp deliberately omitted -- see sound/vic20sound.c above.
    ${FURNACE_DIR}/src/engine/platform/vrc6.cpp
    ${FURNACE_DIR}/src/engine/platform/x1_010.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2203.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2203ext.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2608.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2608Interface.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2608ext.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2610.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2610Interface.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2610b.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2610bext.cpp
    ${FURNACE_DIR}/src/engine/platform/ym2610ext.cpp
    ${FURNACE_DIR}/src/engine/platform/ymz280b.cpp
    ${FURNACE_DIR}/src/engine/platform/zxbeeper.cpp
    ${FURNACE_DIR}/src/engine/platform/zxbeeperquadtone.cpp
    ${FURNACE_DIR}/src/engine/playback.cpp
    ${FURNACE_DIR}/src/engine/safeReader.cpp
    ${FURNACE_DIR}/src/engine/safeWriter.cpp
    ${FURNACE_DIR}/src/engine/sample.cpp
    ${FURNACE_DIR}/src/engine/song.cpp
    ${FURNACE_DIR}/src/engine/sysDef.cpp
    ${FURNACE_DIR}/src/engine/vgmOps.cpp
    ${FURNACE_DIR}/src/engine/wavOps.cpp
    ${FURNACE_DIR}/src/engine/waveSynth.cpp
    ${FURNACE_DIR}/src/engine/wavetable.cpp
    ${FURNACE_DIR}/src/engine/workPool.cpp
    ${FURNACE_DIR}/src/fileutils.cpp
    ${FURNACE_DIR}/src/fileutils/abstract.cpp
    ${FURNACE_DIR}/src/fileutils/cfile.cpp
    ${FURNACE_DIR}/src/log.cpp
    ${FURNACE_DIR}/src/timeutils.cpp
    ${FURNACE_DIR}/src/utfutils.cpp
)

# NOTE: coreutils is deliberately NOT on this target. coreutils pulls
# chipmachine's own (different-version) fmt, and mixing it with Furnace's
# bundled fmt (pulled by engine.h -> ta-log.h) does not compile. DMFPlugin.cpp
# therefore uses only std headers + Furnace's fmt. sndfile_stub/ shadows the
# real <sndfile.h>.
# BEFORE: chipmachine's build carries its own newer fmt via global
# include_directories(); Furnace's bundled fmt (v10.1) must win on this target's
# path, so prepend these ahead of any inherited fmt include.
target_include_directories(dmfplugin_objs BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sndfile_stub
    ${FURNACE_DIR}/src
    ${FURNACE_DIR}/src/engine
    ${FURNACE_DIR}/src/icon
    ${FURNACE_DIR}/extern/fmt/include
    ${FURNACE_DIR}/extern/blip_buf
    ${FURNACE_DIR}/extern/vgsound_emu-modified
    ${FURNACE_DIR}/extern/macports-legacy-support/include
    ${FURNACE_DIR}/extern/IconFontCppHeaders
    ${ZLIB_INCLUDE_DIRS})

# HAVE_DIRENT_TYPE: use dirent d_type (assetDir). FMT_HEADER_ONLY: pull fmt
# inline instead of linking Furnace's fmt::fmt lib target (not built here).
# HAVE_SNDFILE: Furnace declares SFWrapper members unconditionally, so its
# no-sndfile path does not actually compile; define it and satisfy the symbols
# with sndfile_stub/ instead (we never hit an audio-file/export code path).
target_compile_definitions(dmfplugin_objs PRIVATE HAVE_DIRENT_TYPE HAVE_SNDFILE)
target_compile_options(dmfplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden -Wno-deprecated-declarations)

# Furnace targets C++14; at C++17 its bundled fmt (10.1) picks the std::string_view
# vsprintf overload in log.cpp, which fmt 10.1 does not accept. Build the Furnace
# slice at C++14 (as upstream does)...
set_target_properties(dmfplugin_objs PROPERTIES CXX_STANDARD 14 CXX_STANDARD_REQUIRED ON)

# ...but DMFPlugin.cpp needs C++17 (chipplugin.h uses `static inline`), and stays
# at default visibility so musix::DMFPlugin + dmfplugin_register remain exported.
set_source_files_properties(DMFPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-std=c++17;-fvisibility=default;-fvisibility-inlines-hidden")

set(DMF_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/dmfplugin_combined.o)
add_custom_command(
    OUTPUT ${DMF_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:dmfplugin_objs> -o ${DMF_COMBINED}
    DEPENDS dmfplugin_objs $<TARGET_OBJECTS:dmfplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing dmfplugin symbols (ld -r)")
add_custom_target(dmfplugin_combine DEPENDS ${DMF_COMBINED})

add_library(dmfplugin STATIC ${DMF_COMBINED})
add_dependencies(dmfplugin dmfplugin_combine)
set_target_properties(dmfplugin PROPERTIES LINKER_LANGUAGE CXX)

find_package(Threads REQUIRED)
target_link_libraries(dmfplugin PRIVATE ZLIB::ZLIB Threads::Threads)
if(NOT WIN32)
    target_link_libraries(dmfplugin PRIVATE m)
endif()
