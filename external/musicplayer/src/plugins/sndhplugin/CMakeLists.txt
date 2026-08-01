cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.sndhplugin)

# Atari ST/STE SNDH player. The engine is AtariAudio v1.01 by Arnaud Carre
# (Leonard/Oxygene), vendored verbatim under atariaudio/ -- MIT, and with it the
# Musashi 68000 core (Karl Stenerud, MIT) and Hans Wessels' public-domain ICE!
# depacker. Nothing here is GPL, so the plugin builds identically in both
# variants; it is what lets the App Store build keep the 6,079 .sndh rows that
# GPL-3 libsc68 used to own. See SNDHPlugin.h and README.md.
set(ATARIAUDIO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/atariaudio)

set(ATARIAUDIO_SOURCES
    ${ATARIAUDIO_DIR}/AtariMachine.cpp
    ${ATARIAUDIO_DIR}/Mk68901.cpp
    ${ATARIAUDIO_DIR}/SndhFile.cpp
    ${ATARIAUDIO_DIR}/SteDac.cpp
    ${ATARIAUDIO_DIR}/ym2149c.cpp
    ${ATARIAUDIO_DIR}/external/ice_24.c
    # m68kops.c is Musashi's PREBUILT opcode table, committed upstream -- do NOT
    # add m68kmake.c, the generator that produces it (it has its own main()).
    # m68kdasm.c is the disassembler and is unused. m68kfpu.c is likewise not
    # listed, but do not delete it either: m68kcpu.c `#include`s it directly at
    # line 49 and will not compile without the file present.
    ${ATARIAUDIO_DIR}/external/Musashi/m68kcpu.c
    ${ATARIAUDIO_DIR}/external/Musashi/m68kops.c)

# NDEBUG is REQUIRED here, not a tidiness flag. This project's Release build does
# not define it (see the vendored-assert note in the libxmp plugins), and
# AtariAudio asserts on input it does not like rather than returning an error:
# unsupported GEMDOS/XBIOS calls, unknown TRAP #n, and illegal opcodes all hit
# assert(false) in AtariMachine.cpp. A single odd .sndh in a 6,079-file corpus
# would otherwise abort() the whole app instead of skipping one song.
#
# The engine still reports genuine load failures through SndhFile::Load's return
# value, which is what SNDHPlugin::fromFile actually keys on.
set_source_files_properties(${ATARIAUDIO_SOURCES} PROPERTIES
    COMPILE_DEFINITIONS "NDEBUG")

add_library(sndhplugin_objs OBJECT
    SNDHPlugin.cpp SNDHPlugin.h
    IceDepack.cpp IceDepack.h
    ${ATARIAUDIO_SOURCES})

target_include_directories(sndhplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ATARIAUDIO_DIR})

target_link_libraries(sndhplugin_objs PRIVATE coreutils)

# aoplugin's Saturn core (aosdk/eng_ssf) is ALSO a Musashi, and exports the same
# un-prefixed C entry points at global scope -- verified with
# `nm -g libaoplugin.a`: _m68k_init, _m68k_execute, _m68k_read_memory_8 and
# friends are all T there. Linking a second copy as-is is a duplicate-symbol
# error. Build the whole plugin with hidden visibility and partial-link it with
# `ld -r`, which demotes those private externs to true local symbols -- the same
# treatment mikmodplugin needs for libmikmod's un-prefixed helpers.
target_compile_options(sndhplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden)

# -w: the vendored engine is third-party; don't police its warnings.
#
# -include stddef.h: ym2149c.h (and its siblings) use bare NULL in default
# argument values while only including <stdint.h>, so they rely on a transitive
# NULL that libc++ does not provide here. Forcing the declaration in from the
# build keeps the vendored sources byte-identical to upstream -- there is no
# patch to re-apply when AtariAudio is re-synced.
set_source_files_properties(${ATARIAUDIO_SOURCES} PROPERTIES
    COMPILE_OPTIONS "-w;-include;stddef.h")

# Our own wrappers must stay exported: sndhplugin_register() is called from
# plugin_register.cpp, musix::SNDHPlugin is referenced by cmtest, and
# musix::sndh::iceDepack is what SongFileIdentifier::parseSndh calls now that it
# no longer reaches into GPL-3 unice68.
set_source_files_properties(SNDHPlugin.cpp IceDepack.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=default;-fvisibility-inlines-hidden")

set(SNDH_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/sndhplugin_combined.o)
add_custom_command(
    OUTPUT ${SNDH_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:sndhplugin_objs> -o ${SNDH_COMBINED}
    DEPENDS sndhplugin_objs $<TARGET_OBJECTS:sndhplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing sndhplugin symbols (ld -r)")
add_custom_target(sndhplugin_combine DEPENDS ${SNDH_COMBINED})

add_library(sndhplugin STATIC ${SNDH_COMBINED})
add_dependencies(sndhplugin sndhplugin_combine)
set_target_properties(sndhplugin PROPERTIES LINKER_LANGUAGE CXX)
