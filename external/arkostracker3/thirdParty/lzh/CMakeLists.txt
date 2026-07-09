cmake_minimum_required(VERSION 3.12)

project(ThirdPartyLzh)

set(TargetName ${PROJECT_NAME})

add_library(${TargetName})

target_compile_definitions(${TargetName} PRIVATE)

target_sources(${TargetName}
        PRIVATE
        LzhStSound.cpp
        LzhStSound.h
)