cmake_minimum_required(VERSION 2.8.4)
project(examples)

add_definitions(-DAPP_NAME=examples)

get_filename_component(AP1_ROOT .. ABSOLUTE)
#set(AP1_ROOT ..)

include(${AP1_ROOT}/cmake/Utils.cmake)
include_directories(${AP1_ROOT}/mods)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g")

set(APONE_MODULES
	audioplayer
	coreutils
	grappix
)
add_subdirectory(${AP1_ROOT}/mods ap1mods)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

add_subdirectory(simple)
add_subdirectory(demo)
add_subdirectory(snake)
