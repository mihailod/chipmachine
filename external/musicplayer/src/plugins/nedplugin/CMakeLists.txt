cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.nedplugin)

# NerdTracker II (.ned) playback.
#
#   * ned/ned_engine.c  -- player/loader core lifted from the "NerdTracker 2 SDL
#                          port" v1.0 by thefox, itself a port of NerdTracker II
#                          by Michel Iwaniec ("Bananmos").
#   * ned/apuwrap.c, ned/blarggAPU.cpp -- the port's glue onto the APU.
#   * ned/Nes_Snd_Emu-0.1.7/ -- blargg's (Shay Green) NES 2A03 sound emulation,
#                               LGPL; we compile only the subset Simple_Apu_PAL
#                               needs.
set(NES_APU ned/Nes_Snd_Emu-0.1.7)

# gmeplugin already vendors the same blargg Nes_Snd_Emu classes (Nes_Apu,
# Blip_Buffer, ...) in the global namespace, and the .ned engine uses generic
# global names (inst, SampleName, ...) that collide with other trackers. We
# build our objects with hidden visibility (-> Mach-O private externs) and then
# partial-link them with `ld -r`, which demotes every private extern to a true
# local symbol. Only nedplugin_register() keeps default visibility, so it stays
# the single exported symbol and nothing collides at the final link.
add_library(nedplugin_objs OBJECT
    NEDPlugin.cpp
    NEDPlugin.h
    ned/ned_engine.c
    ned/apuwrap.c
    ned/file.c
    ned/blarggAPU.cpp
    ${NES_APU}/Simple_Apu_PAL.cpp
    ${NES_APU}/nes_apu/Nes_Apu.cpp
    ${NES_APU}/nes_apu/Nes_Oscs.cpp
    ${NES_APU}/nes_apu/Blip_Buffer.cpp
    ${NES_APU}/nes_apu/apu_snapshot.cpp)

target_include_directories(nedplugin_objs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ned
    ${CMAKE_CURRENT_SOURCE_DIR}/${NES_APU})

target_link_libraries(nedplugin_objs PRIVATE coreutils)
# -w silences the (large) upstream warning output: blargg's Nes_Snd_Emu uses the
# assert(("msg", cond)) idiom (-Wunused-value) and an intentional ULONG_MAX>>n
# narrowing, and thefox's ned_engine.c has if((a==b))/if(x=...) style. None are
# bugs; matches the other vendored-engine plugins (ixsplugin, ptkplugin).
target_compile_options(nedplugin_objs PRIVATE
    -fvisibility=hidden -fvisibility-inlines-hidden -w)

# NEDPlugin.cpp must keep its musix::NEDPlugin methods exported -- the host (and
# cmtest) reference the class directly. Only the vendored engine/APU symbols
# need hiding, so compile this one TU at default visibility.
set_source_files_properties(NEDPlugin.cpp PROPERTIES
    COMPILE_OPTIONS "-fvisibility=default;-fvisibility-inlines-hidden")

# Collapse the objects into one relocatable object with all symbols localized
# except nedplugin_register (`ld -r` localizes private externs by default).
set(NED_COMBINED ${CMAKE_CURRENT_BINARY_DIR}/nedplugin_combined.o)
add_custom_command(
    OUTPUT ${NED_COMBINED}
    COMMAND ld -r $<TARGET_OBJECTS:nedplugin_objs> -o ${NED_COMBINED}
    DEPENDS nedplugin_objs $<TARGET_OBJECTS:nedplugin_objs>
    COMMAND_EXPAND_LISTS
    VERBATIM
    COMMENT "Localizing nedplugin symbols (ld -r)")
add_custom_target(nedplugin_combine DEPENDS ${NED_COMBINED})

add_library(nedplugin STATIC ${NED_COMBINED})
add_dependencies(nedplugin nedplugin_combine)
set_target_properties(nedplugin PROPERTIES LINKER_LANGUAGE CXX)

if(NOT WIN32)
    target_link_libraries(nedplugin PRIVATE m)
endif()
