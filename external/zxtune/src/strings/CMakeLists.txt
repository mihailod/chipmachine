project(libstrings)

file(GLOB SRCS_CXX "src/*.cpp" "../../3rdparty/fmt/src/format.cc")
file(GLOB SRCS_H "*.h" "src/*.h")

include_directories("../../3rdparty/fmt/include")

add_library(strings ${SRCS_CXX} ${SRCS_H})
