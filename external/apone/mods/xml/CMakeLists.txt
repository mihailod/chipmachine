cmake_minimum_required(VERSION 3.12.0)
project(cpp-mods.xml)

set(SOURCE_FILES tinyxml2.cpp)

add_library(xml ${SOURCE_FILES})
#target_link_libraries(json json-c)
