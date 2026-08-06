LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# NOTE: this file is not part of the chipmachine build (CMakeLists.txt is) and
# is kept only as upstream inherited it. The OPL devices now come from ymfm --
# see device/s98_ymfm.cpp and the note in ../CMakeLists.txt -- so the MAME
# fmopl/ymf262 entries are gone and the ymfm sources would have to be added
# here for this makefile to work again.
MY_FMGEN_SRC = ./device/fmgen/file.cpp \
./device/fmgen/fmgen.cpp \
./device/fmgen/fmtimer.cpp \
./device/fmgen/opm.cpp \
./device/fmgen/opna.cpp \
./device/fmgen/opna_rhythm_rom.cpp \
./device/fmgen/psg.cpp

MY_M_S98_SRC = ./device/s98fmgen.cpp \
./device/s98_ymfm.cpp \
./device/s98opll.cpp \
./device/s98sng.cpp \
./device/emu2413/emu2413.c \
./device/s_logtbl.c \
./device/s_sng.c \
./m_s98.cpp

LOCAL_MODULE    := m_s98
LOCAL_SRC_FILES := jni.cpp $(MY_M_S98_SRC) $(MY_FMGEN_SRC)
LOCAL_CFLAGS    += -DUSE_ZLIB -I$(LOCAL_PATH)/m_s98 -I.. -I$(LOCAL_PATH)/m_s98/device/fmgen
LOCAL_LDLIBS	+= -lm -lz -llog
LOCAL_ARM_MODE	:= arm

include $(BUILD_SHARED_LIBRARY)
