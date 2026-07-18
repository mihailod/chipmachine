cmake_minimum_required(VERSION 3.15)
project(cpp-mods.musicplayer.orgplugin)

set(ORGANYA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/organya)

add_library(orgplugin OrgPlugin.cpp organya_impl.c OrgPlugin.h)

target_include_directories(orgplugin PRIVATE ${ORGANYA_DIR})
target_link_libraries(orgplugin PRIVATE coreutils)

if(NOT WIN32)
    target_link_libraries(orgplugin PRIVATE m)
endif()
