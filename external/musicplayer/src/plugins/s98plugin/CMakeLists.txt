cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.s98plugin)


# The OPL/OPL2/OPL3 devices come from ymfm (BSD-3), not from MAME. MAME's
# fmopl.c and ymf262.c are GPL-2.0+ -- Jarek Burczynski did not agree to the
# 2016 BSD-3 relicensing -- and were the only GPL code in this plugin. They are
# replaced by m_s98/device/s98_ymfm.cpp; s98mame.cpp and m_s98/device/mame/ are
# gone.
#
# The swap is in BOTH variants, not just mas, because it is a correctness fix
# rather than only a licence swap. m_s98.cpp encodes a port-1 write as
# 0x100|reg, which for OPL3 is the second register bank; s98mame.cpp passed that
# to YMF262Write() as an 8-bit address, so every bank-1 write landed in bank 0.
# Registers 0x105 (OPL3 enable) and 0x104 (4-operator mode) were therefore never
# reachable and all nine OPL3 songs played as plain 2-op OPL2. On the OPL2 side
# the two cores agree to within 1.6% RMS, so nothing regresses there.
#
# ymfm is compiled again here rather than shared with libvgmplugin: that target
# merges its objects with `ld -r`, which turns its ymfm template instantiations
# into private externs. They are invisible to other targets, and by the same
# token this second copy cannot collide with them.
set(YMFM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../../ymfm)
set(S98_YMFM_SOURCES
    m_s98/device/s98_ymfm.cpp
    ${YMFM_DIR}/src/ymfm_opl.cpp
    ${YMFM_DIR}/src/ymfm_adpcm.cpp     # ymfm_opl.cpp needs both of these even
    ${YMFM_DIR}/src/ymfm_pcm.cpp)      # though Y8950/OPL4 are unused here

set(SOURCE_FILES
    m_s98/device/s98fmgen.cpp
    m_s98/device/s98opll.cpp
    m_s98/device/s98sng.cpp
    m_s98/device/emu2413/emu2413.c
    m_s98/device/s_logtbl.c
    m_s98/device/s_sng.c
    m_s98/m_s98.cpp
	m_s98/device/fmgen/file.cpp
    m_s98/device/fmgen/fmgen.cpp
    m_s98/device/fmgen/fmtimer.cpp
    m_s98/device/fmgen/opm.cpp
    m_s98/device/fmgen/opna.cpp
    m_s98/device/fmgen/opna_rhythm_rom.cpp
    m_s98/device/fmgen/psg.cpp
    ${S98_YMFM_SOURCES}
)

add_library(s98plugin S98Plugin.cpp ${SOURCE_FILES})
target_link_libraries(s98plugin PRIVATE coreutils ${ZLIB_LIBRARIES})
if(NOT WIN32)
    target_compile_options(s98plugin PRIVATE -Wno-incompatible-function-pointer-types -Wno-incompatible-pointer-types)
endif()
set_source_files_properties(${S98_YMFM_SOURCES} PROPERTIES COMPILE_OPTIONS "-std=c++17")
target_include_directories(s98plugin PRIVATE m_s98 ${YMFM_DIR}/src ${ZLIB_INCLUDE_DIRS})
#set_source_files_properties(m_s98/device/s_sng.c PROPERTIES COMPILE_FLAGS -Wno-incompatible-pointer-types)

