project(libvgm)

set(SRCS_C
	"emu/SoundEmu.c" "emu/Resampler.c" "emu/panning.c" "emu/dac_control.c" "emu/logging_stub.c"
	"utils/DataLoader.c" "utils/StrUtils-CPConv_Stub.c"
	"player/helper.c" "player/dblk_compr.c"
#mame default
	"emu/cores/sn764intf.c" "emu/cores/sn76496.c" "emu/cores/sn76489.c"
	"emu/cores/2413intf.c" "emu/cores/ym2413.c" "emu/cores/nukedopll.c" "emu/cores/emu2413.c"
#gpgx default
	"emu/cores/2612intf.c" "emu/cores/ym2612.c" "emu/cores/ym3438.c"
	"emu/cores/ym2151.c" "emu/cores/2151intf.c" "emu/cores/nukedopm.c"
	"emu/cores/segapcm.c"
#both cores used
	"emu/cores/rf5cintf.c" "emu/cores/rf5c68.c" "emu/cores/scd_pcm.c"
	"emu/cores/opnintf.c" "emu/cores/fmopn.c" "emu/cores/ymdeltat.c"
#adlibemu default
	"emu/cores/oplintf.c" "emu/cores/adlibemu_opl2.c" "emu/cores/fmopl.c"
#nuke default, adlib for 278b
	"emu/cores/262intf.c" "emu/cores/nukedopl3.c" "emu/cores/adlibemu_opl3.c" "emu/cores/ymf262.c"
	"emu/cores/ymf278b.c"
	"emu/cores/ymz280b.c"
	"emu/cores/ymf271.c"
#fcc_emu default
	"emu/cores/ayintf.c" "emu/cores/emu2149.c" "emu/cores/ay8910.c"
	"emu/cores/pwm.c"
	"emu/cores/gb.c"
#nsfp default with fds
	"emu/cores/nesintf.c" "emu/cores/np_nes_apu.c" "emu/cores/np_nes_dmc.c" "emu/cores/np_nes_fds.c" "emu/cores/nes_apu.c"
	"emu/cores/mikey.c" "emu/cores/multipcm.c"
	"emu/cores/upd7759.c"
	"emu/cores/okim6258.c"
	"emu/cores/okim6295.c" "emu/cores/okiadpcm.c"
	"emu/cores/k051649.c"
	"emu/cores/k054539.c"
#ootk default
	"emu/cores/c6280intf.c" "emu/cores/Ootake_PSG.c" "emu/cores/c6280_mame.c"
	"emu/cores/c140.c"
	"emu/cores/c219.c"
	"emu/cores/k053260.c"
	"emu/cores/pokey.c"
#ctr default
	"emu/cores/qsoundintf.c" "emu/cores/qsound_ctr.c" "emu/cores/qsound_mame.c"
	"emu/cores/scsp.c" "emu/cores/scspdsp.c"
	"emu/cores/ws_audio.c"
	"emu/cores/vsu.c"
#vbel default
	"emu/cores/saaintf.c" "emu/cores/saa1099_vb.c" "emu/cores/saa1099_mame.c"
	"emu/cores/es5503.c"
	"emu/cores/es5506.c"
	"emu/cores/x1_010.c"
	"emu/cores/c352.c"
	"emu/cores/iremga20.c"
	"emu/cores/bsmt2000.c"
	"emu/cores/ics2115.c"
	"emu/cores/k005289.c"
	"emu/cores/k007232.c"
	"emu/cores/msm5205.c"
	"emu/cores/msm5232.c"
)

set(SRCS_C_CXX "player/playera.cpp" "player/playerbase.cpp" "player/s98player.cpp" "player/vgmplayer.cpp" "player/vgmplayer_cmdhandler.cpp")

file(GLOB SRCS_H "emu/*.h" "utils/*.h" "player/*.h" "emu/cores/*.h")

add_definitions(-DHAVE_STDINT_H -DVGM_LITTLE_ENDIAN)

add_library(vgm ${SRCS_H} ${SRCS_C} ${SRCS_C_CXX})
set_property(TARGET vgm PROPERTY FOLDER "3rdparty")
