project(sseqplayer)

add_definitions(-D_LIBCPP_VERSION)

file(GLOB SRCS_CXX "*.cpp")
file(GLOB SRCS_H "*.h")

add_library(sseqplayer ${SRCS_H} ${SRCS_CXX})
set_property(TARGET sseqplayer PROPERTY FOLDER "3rdparty")
