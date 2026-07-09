cmake_minimum_required(VERSION 3.12)

project(ThirdPartyFap)

set(TargetName ${PROJECT_NAME})

add_library(${TargetName})

target_compile_definitions(${TargetName} PRIVATE)

target_sources(${TargetName}
        PRIVATE
        FapCrunch.h
        FapCrunch.cpp
        Lzss.cpp
        Lzss.h
        Types.h
        YmData.cpp
        YmData.h
        YmLoad.cpp
        YmLoad.h
)