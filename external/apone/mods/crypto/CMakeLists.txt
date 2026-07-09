cmake_minimum_required(VERSION 3.12.0)
project(cpp-mods.crypto)

set(SOURCE_FILES sha256.cpp md5.cpp solar-md5.c)

add_library(crypto ${SOURCE_FILES})
