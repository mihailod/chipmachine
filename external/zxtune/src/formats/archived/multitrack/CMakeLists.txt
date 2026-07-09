project(libformats_archived_multitrack)

file(GLOB SRCS_CXX "*.cpp")
file(GLOB SRCS_H "*.h")

add_library(formats_archived_multitrack ${SRCS_CXX} ${SRCS_H})
set_property(TARGET formats_archived_multitrack PROPERTY FOLDER "formats")
