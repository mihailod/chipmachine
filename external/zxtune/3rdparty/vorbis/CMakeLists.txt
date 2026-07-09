project(libvorbis)

add_definitions(-Dalloca=malloc)
include_directories("../../3rdparty")

set(SRCS_C "lib/bitrate.c" "lib/block.c" "lib/codebook.c" "lib/envelope.c" "lib/floor0.c" "lib/floor1.c" "lib/info.c" "lib/lpc.c" "lib/lsp.c"
	"lib/mapping0.c" "lib/mdct.c" "lib/psy.c" "lib/registry.c" "lib/res0.c" "lib/sharedbook.c" "lib/smallft.c" "lib/synthesis.c" "lib/vorbisfile.c" "lib/window.c"
)

file(GLOB SRCS_H "lib/*.h" "*.h")

add_library(vorbis ${SRCS_H} ${SRCS_C})
set_property(TARGET vorbis PROPERTY FOLDER "3rdparty")
