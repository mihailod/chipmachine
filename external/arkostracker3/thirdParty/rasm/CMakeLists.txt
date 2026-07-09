cmake_minimum_required(VERSION 3.12)

project(ThirdPartyRasm)

set(TargetName ${PROJECT_NAME})

add_library(${TargetName})

target_compile_definitions(${TargetName} PRIVATE)

target_sources(${TargetName}
        PRIVATE
        minilib.h
        rasm.c
        rasm.h
)