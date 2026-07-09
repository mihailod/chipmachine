project(vgmstream)

add_definitions(-DVGM_USE_FFMPEG -DVGM_USE_VORBIS -DVGM_USE_ATRAC9 -DVGM_USE_MPEG -DVGM_USE_G7221)

include_directories("../../3rdparty/atrac9/C/src" "../../3rdparty/mpg123/src/libmpg123" "../../3rdparty/ffmpeg" "../../3rdparty")

file(GLOB SRCS_C "*.c" "base/*.c" "coding/*.c" "coding/libs/*.c" "layout/*.c" "meta/*.c" "util/*.c")
file(GLOB SRCS_H "*.h" "base/*.h" "coding/*.h" "coding/libs/*.h" "layout/*.h" "meta/*.h" "util/*.h")

add_library(vgmstream ${SRCS_H} ${SRCS_C})
set_property(TARGET vgmstream PROPERTY FOLDER "3rdparty")
