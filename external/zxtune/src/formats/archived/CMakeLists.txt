project(libformats_archived)

file(GLOB SRCS_CXX "*.cpp" "multitrack/*.cpp")
file(GLOB SRCS_H "*.h")

add_library(formats_archived ${SRCS_CXX} ${SRCS_H})
set_property(TARGET formats_archived PROPERTY FOLDER "formats")
