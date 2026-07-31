cmake_minimum_required(VERSION 3.12)
project(cpp-mods)

# vicepluginbridge is GPLv2 and is not built at all in the mas variant -- see the
# CM_HAVE_VICE block in chipmachine/CMakeLists.txt. It stays out of
# MUSICPLAYER_PLUGINS (rather than being appended to it) because it has always
# been added and linked separately here.
if(CM_HAVE_VICE)
    add_subdirectory(vicepluginbridge)
endif()

foreach(PLUGIN ${MUSICPLAYER_PLUGINS})
    add_subdirectory(${PLUGIN})
endforeach()

add_library(plugins INTERFACE)
target_link_libraries(plugins INTERFACE ${MUSICPLAYER_PLUGINS})
if(CM_HAVE_VICE)
    target_link_libraries(plugins INTERFACE vicepluginbridge)
endif()
