cmake_minimum_required(VERSION 3.12)
project(cpp-mods)

add_subdirectory(vicepluginbridge)

foreach(PLUGIN ${MUSICPLAYER_PLUGINS})
    add_subdirectory(${PLUGIN})
endforeach()

add_library(plugins INTERFACE)
target_link_libraries(plugins INTERFACE ${MUSICPLAYER_PLUGINS} vicepluginbridge)
