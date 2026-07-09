project(libogg)

include_directories("../../3rdparty")

file(GLOB SRCS_C "src/*.c")
file(GLOB SRCS_H "src/*.h" "*.h")

add_library(ogg ${SRCS_H} ${SRCS_C})
set_property(TARGET ogg PROPERTY FOLDER "3rdparty")
