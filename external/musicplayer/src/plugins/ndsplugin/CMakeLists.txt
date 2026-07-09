cmake_minimum_required(VERSION 3.12 )
project(cpp-mods.musicplayer.ndsplugin)

# Emulation core is the maintained kode54/Snowhill reentrant vio2sf
# (desmume/state.c + resampler). GPU emulation is gone (audio-only); SPU is now
# C++ (SPU.cpp). vio2sf.c is the PSF/_lib loader glue (see that file).
set(SOURCE_FILES
    NDSPlugin.cpp
    nds/vio2sf/vio2sf.c
    nds/vio2sf/desmume/arm_instructions.c
    nds/vio2sf/desmume/armcpu.c
    nds/vio2sf/desmume/bios.c
    nds/vio2sf/desmume/cp15.c
    nds/vio2sf/desmume/FIFO.c
    nds/vio2sf/desmume/isqrt.c
    nds/vio2sf/desmume/matrix.c
    nds/vio2sf/desmume/mc.c
    nds/vio2sf/desmume/MMU.c
    nds/vio2sf/desmume/NDSSystem.c
    nds/vio2sf/desmume/resampler.c
    nds/vio2sf/desmume/SPU.cpp
    nds/vio2sf/desmume/state.c
    nds/vio2sf/desmume/thumb_instructions.c
)

add_library(ndsplugin ${SOURCE_FILES})
target_include_directories(ndsplugin PRIVATE nds/vio2sf nds/vio2sf/desmume nds)
target_link_libraries(ndsplugin PRIVATE psf coreutils)
if(NOT WIN32)
    target_compile_options(ndsplugin PRIVATE -Wno-incompatible-pointer-types)
endif()
# NO_ASM: use the resampler's portable C path (no x86 SSE). Little-endian only
# (the core keys big-endian off WORDS_BIGENDIAN, which we leave undefined).
target_compile_definitions(ndsplugin PRIVATE -DNO_ASM)

