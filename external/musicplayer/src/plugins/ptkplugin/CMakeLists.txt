cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.ptkplugin)

# Vendored ProTrekkr tracker/replayer (in-plugin at ./protrekkr).
set(PTK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/protrekkr)
set(PTK_REPLAY ${PTK_ROOT}/release/ptk/replay/lib)

# The .ptk/.ntk on-disk format is a zlib-compressed container; only Load_Ptk in
# the full tracker file loaders can read it (the replay lib's Ptk_InitModule only
# accepts the raw, already-decompressed .ptp image). So we compile the file
# loaders together with the replayer, in the in-memory BZR2 + __WINAMP__
# integration mode (same as BZRPlayer / the Winamp plugin), and provide the
# tracker glue globals in PTKPlugin.cpp.
add_library(ptkplugin STATIC
    PTKPlugin.cpp
    PTKPlugin.h
    # replayer
    ${PTK_REPLAY}/replay.cpp
    ${PTK_REPLAY}/synth.cpp
    ${PTK_REPLAY}/tb_303.cpp
    ${PTK_REPLAY}/wavpack.cpp
    ${PTK_REPLAY}/endianness.cpp
    ${PTK_REPLAY}/samples_unpack.cpp
    # file loaders
    ${PTK_ROOT}/src/editors/patterns_blocks.cpp
    ${PTK_ROOT}/src/files/files.cpp
    ${PTK_ROOT}/src/files/303s.cpp
    ${PTK_ROOT}/src/files/reverbs.cpp
    ${PTK_ROOT}/src/files/synths.cpp
)

target_include_directories(ptkplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PTK_REPLAY}
    ${PTK_REPLAY}/include
    ${PTK_REPLAY}/sounddriver/include
    ${PTK_ROOT}/src/editors/include
    ${PTK_ROOT}/src/files/include
    ${PTK_ROOT}/src/include
    ${ZLIB_INCLUDE_DIRS}
)

# BZR2            - in-memory CustomFile integration (no real file/audio I/O)
# __STAND_ALONE__ - strip the tracker UI / scope / vumeter bookkeeping
# __WINAMP__      - strip the remaining Status_Box() UI calls in the loaders and
#                   make Ptk_InitDriver() set the song up without an audio driver
target_compile_definitions(ptkplugin PRIVATE BZR2 __STAND_ALONE__ __WINAMP__)

# -w silences the (large) upstream warning output.
target_compile_options(ptkplugin PRIVATE -w)

# The GSM/MP3/AT3 sample decoders in samples_unpack.cpp need the Windows ACM
# codec API; disable just those three for this one file (PTKPlugin.cpp provides
# no-op stubs for their entry points). See ptk_no_acm_codecs.h.
set_source_files_properties(${PTK_REPLAY}/samples_unpack.cpp PROPERTIES
    COMPILE_OPTIONS "-include;${CMAKE_CURRENT_SOURCE_DIR}/ptk_no_acm_codecs.h")

target_link_libraries(ptkplugin PRIVATE coreutils ${ZLIB_LIBRARIES})
