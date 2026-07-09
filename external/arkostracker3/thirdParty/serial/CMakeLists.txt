cmake_minimum_required(VERSION 3.12)

project(ThirdPartySerial)

set(TargetName ${PROJECT_NAME})

add_library(${TargetName})

target_compile_definitions(${TargetName} PRIVATE)

target_sources(${TargetName}
        PRIVATE
        src/impl/list_ports/list_ports_linux.cc
        src/impl/list_ports/list_ports_osx.cc
        src/impl/list_ports/list_ports_win.cc
        src/impl/unix.cc
        src/impl/win.cc
        src/serial.cc
        include/impl/unix.h
        include/impl/win.h
        include/serial.h
        include/v8stdint.h
)