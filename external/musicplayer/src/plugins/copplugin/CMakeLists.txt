cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.copplugin)

# Sam Coupe "COP" player (the modland "Sam Coupe COP" corpus): SAM Coupé music
# for the Philips SAA1099. Runs the song's embedded Z80 replay routine (or the
# shared E-Tracker replayer, vendored as etracker_bin.h) on the GME Z80 core,
# routing its SAA1099 port writes into Dave Hooper's SAASound (vendored, BSD-3).
#
# The load/patch/calling convention follows Christopher O'Neill's SCPlayer (MIT);
# we drive the commercial-clean GME Z80 core instead of SCPlayer's Z80 emulator.

set(SAASOUND_DIR ${CMAKE_CURRENT_SOURCE_DIR}/saasound)

add_library(copplugin
    CopPlugin.cpp
    cop_machine.cpp
    CopPlugin.h
    cop_machine.h
    etracker_bin.h
    ${SAASOUND_DIR}/src/SAAAmp.cpp
    ${SAASOUND_DIR}/src/SAADevice.cpp
    ${SAASOUND_DIR}/src/SAAEnv.cpp
    ${SAASOUND_DIR}/src/SAAFreq.cpp
    ${SAASOUND_DIR}/src/SAAImpl.cpp
    ${SAASOUND_DIR}/src/SAANoise.cpp
    ${SAASOUND_DIR}/src/SAASound.cpp
    ${SAASOUND_DIR}/src/SAASndC.cpp)

# SAASound headers (include/ for the public API, src/ for its internals), plus
# the GME plugin's headers for the Z80 core (Z80_Cpu.o is resolved at final
# link, the same way bbsong/sccmusixx reuse it).
target_include_directories(copplugin PRIVATE
    ${SAASOUND_DIR}/include
    ${SAASOUND_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../gmeplugin)

target_link_libraries(copplugin PRIVATE coreutils)
