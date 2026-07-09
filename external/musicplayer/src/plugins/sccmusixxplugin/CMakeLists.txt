cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.sccmusixxplugin)

# SCC-Musixx .SNG (Konami SCC tracker music, MSX, Tyfoon-Software 1990) player.
# Runs the original SCC-MUSIXX replay routine (embedded REPLAY.BIN) on an
# embedded Z80 core, routing its SCC register writes into emu2212.
add_library(sccmusixxplugin
    SccMusixxPlugin.cpp
    scc_machine.cpp
    scc_emu.c
    SccMusixxPlugin.h
    scc_machine.h
    scc_emu_names.h
    replay_bin.h)

# emu2212 (the Konami SCC emulator) is vendored with libkss, a sibling repo.
get_filename_component(_libkss_default "${CMAKE_SOURCE_DIR}/external/libkss" ABSOLUTE)
set(LIBKSS_ROOT "${_libkss_default}" CACHE PATH "Where libkss is checked out")
if(NOT EXISTS "${LIBKSS_ROOT}/modules/emu2212/emu2212.c")
    message(FATAL_ERROR
        "sccmusixxplugin: ${LIBKSS_ROOT}/modules/emu2212/emu2212.c not found.\n"
        "Clone https://github.com/digital-sound-antiques/libkss next to "
        "chipmachine-as, or pass -DLIBKSS_ROOT=/path/to/libkss to cmake.")
endif()

# scc_emu.c #includes emu2212.c, and both scc_emu.c and scc_machine.cpp need
# emu2212.h on the include path. We reuse the GME plugin's Z80 core the same way
# bbsongplugin does: headers only, with Z80_Cpu.o resolved at final link.
target_include_directories(sccmusixxplugin PRIVATE
    ${LIBKSS_ROOT}/modules/emu2212
    ${CMAKE_CURRENT_SOURCE_DIR}/../gmeplugin)

target_link_libraries(sccmusixxplugin PRIVATE coreutils)
