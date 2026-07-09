cmake_minimum_required(VERSION 3.1.0)
project(chipmachine)

if(MSVC) 
	set(STDFLAG /std:c++17)
	set(FSLIB "")
elseif(APPLE)
	set(STDFLAG -std=c++17)
	set(FSLIB c++experimental)
else()
    set(STDFLAG -std=c++17)
	set(FSLIB stdc++fs)
endif()

# Define core source files for the testing executable
set(TESTING_SOURCES
    mods/coreutils/newfile.t.cpp
    mods/luainterpreter/luainterpreter.t.cpp
    mods/luainterpreter/luainterpreter.cpp
)

# If compiling on macOS, add the Objective-C++ bridging file to the build
if(APPLE)
    list(APPEND TESTING_SOURCES mods/coreutils/environment_apple.mm)
endif()

add_executable(testing ${TESTING_SOURCES})
target_link_libraries(testing ${FSLIB} pthread)
target_include_directories(testing PRIVATE mods)
target_compile_definitions(testing PRIVATE -DTESTING)
