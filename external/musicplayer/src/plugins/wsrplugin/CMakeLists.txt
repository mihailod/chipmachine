cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.wsrplugin)

# Self-contained in_wsr WonderSwan replayer (GPL-2.0+), vendored under in_wsr/.
# It is the same core Kodi's audiodecoder.wsr ships; only the public
# WSRPlayerSetUp() entry point is called from WSRPlugin.cpp.
set(IN_WSR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/in_wsr)

set(IN_WSR_SOURCES
    ${IN_WSR_ROOT}/wsr_player.c
    ${IN_WSR_ROOT}/ws_audio.c
    ${IN_WSR_ROOT}/ws_io.c
    ${IN_WSR_ROOT}/ws_memory.c
    ${IN_WSR_ROOT}/nec/nec.c
)

add_library(wsrplugin STATIC WSRPlugin.cpp ${IN_WSR_SOURCES})

target_include_directories(wsrplugin PRIVATE ${IN_WSR_ROOT})

# The in_wsr core keeps its whole machine state in file-scope globals with very
# generic names (ROM, ROMSize, SampleRate, sample_buffer, ws_timer, nec_reset,
# ...). Several of those collide with other vendored plugins (ndsplugin,
# mdxplugin, ...) and would break the final static link. Rename EVERY exported
# core symbol to a wsr__* prefix at compile time. These defines are applied to
# the whole target, so WSRPlugin.cpp (which calls WSRPlayerSetUp) rewrites in
# lock-step with the core and the wsr_player.h declaration. The list is the full
# set of global TEXT/DATA/BSS symbols emitted by the core
# (nm -g over the compiled objects); regenerate it if the core is updated.
foreach(_sym
    CPU_Count CPU_Cycles CPU_Run ChannelMuting Close_SampleData Close_WSR
    DMACycles Get_ChannelMuting Get_FirstSong Init_SampleData Init_WSR Load_WSR
    ROM ROMBank ROMSize Reset_WSR SampleRate Sample_Enable Sample_Length
    Sample_Offset Set_ChannelMuting Set_Frequency Update_SampleData Update_WSR
    WSRPlayerSetUp WS_Cpu_Clock WaveAdrs baseBank cpu_interrupt cpu_readmem20
    cpu_readport cpu_writemem20 cpu_writeport initialIoValue mul_div nec_Cycles
    nec_ICount nec_execute nec_exit nec_get_reg nec_getcycles nec_instruction
    nec_int nec_reset nec_set_reg nec_yield romAddressMask romSize sample_buffer
    seg_prefix ws_audio_done ws_audio_init ws_audio_port_read ws_audio_port_write
    ws_audio_process ws_audio_reset ws_audio_sounddma ws_audio_update
    ws_internalRam ws_ioRam ws_io_done ws_io_init ws_io_reset ws_memory_done
    ws_memory_init ws_memory_reset ws_rom ws_staticRam ws_timer ws_timer_count
    ws_timer_min ws_timer_pending ws_timer_reset ws_timer_set ws_timer_update)
    target_compile_definitions(wsrplugin PRIVATE "${_sym}=wsr__${_sym}")
endforeach()

# The vendored C is third-party; build it quietly and keep its symbols out of
# any shared-object export table (mirrors kssplugin). Only WSRPlugin.cpp needs
# default visibility to export wsrplugin_register.
set_source_files_properties(${IN_WSR_SOURCES} PROPERTIES
    COMPILE_FLAGS "-w -fvisibility=hidden")

target_link_libraries(wsrplugin PRIVATE coreutils)
