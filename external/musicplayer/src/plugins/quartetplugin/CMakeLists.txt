cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.quartetplugin)

set(ZZ_SRC ${CMAKE_CURRENT_SOURCE_DIR}/zingzong/src)

set(ZZ_LIB_SOURCES
    ${ZZ_SRC}/zz_init.c
    ${ZZ_SRC}/zz_core.c
    ${ZZ_SRC}/mix_none.c
    ${ZZ_SRC}/mix_lerp.c
    ${ZZ_SRC}/mix_qerp.c
    ${ZZ_SRC}/mix_soxr.c
    ${ZZ_SRC}/mix_srate.c
    ${ZZ_SRC}/mix_help.c
    ${ZZ_SRC}/mix_test.c
    ${ZZ_SRC}/zz_play.c
    ${ZZ_SRC}/zz_log.c
    ${ZZ_SRC}/zz_load.c
    ${ZZ_SRC}/zz_bin.c
    ${ZZ_SRC}/zz_mem.c
    ${ZZ_SRC}/zz_str.c
    ${ZZ_SRC}/zz_vfs.c
    ${ZZ_SRC}/zz_mixers.c
    ${ZZ_SRC}/vfs_file.c
    ${ZZ_SRC}/vfs_ice.c
)

add_library(quartetplugin QuartetPlugin.cpp ${ZZ_LIB_SOURCES})

target_include_directories(quartetplugin PRIVATE ${ZZ_SRC})
target_compile_definitions(quartetplugin PRIVATE 
    PACKAGE_NAME="zingzong"
    PACKAGE_VERSION="1.1.0"
    NO_AO=1
)

target_link_libraries(quartetplugin PRIVATE coreutils)
