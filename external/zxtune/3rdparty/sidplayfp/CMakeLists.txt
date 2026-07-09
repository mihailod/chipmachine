project(libsidplayfp)

add_definitions(-DHAVE_CONFIG_H -DSID_EXTERN=)
include_directories(".")
include_directories("src")

file(GLOB SRCS_CXX "src/*.cpp" "src/c64/*.cpp" "src/c64/CIA/*.cpp" "src/c64/CPU/*.cpp" "src/c64/VIC_II/*.cpp" "src/sidplayfp/*.cpp" "src/sidtune/*.cpp"
	"src/utils/MD5/*.cpp" "src/utils/md5Factory.cpp"
	"src/builders/resid-builder/*.cpp"
	"src/builders/resid-builder/resid/dac.cc"
	"src/builders/resid-builder/resid/envelope.cc"
	"src/builders/resid-builder/resid/extfilt.cc"
	"src/builders/resid-builder/resid/filter.cc"
	"src/builders/resid-builder/resid/pot.cc"
	"src/builders/resid-builder/resid/sid.cc"
	"src/builders/resid-builder/resid/version.cc"
	"src/builders/resid-builder/resid/voice.cc"
	"src/builders/resid-builder/resid/wave.cc"
)

file(GLOB SRCS_H "src/*.h" "src/c64/*.h" "src/c64/CIA/*.h" "src/c64/CPU/*.h" "src/c64/VIC_II/*.h" "src/sidplayfp/*.h" "src/sidtune/*.h"
	"src/utils/MD5/*.h" "src/utils/*.h"
	"src/builders/resid-builder/*.h"
	"src/builders/resid-builder/resid/*.h")

add_library(sidplayfp ${SRCS_CXX} ${SRCS_H})
set_property(TARGET sidplayfp PROPERTY FOLDER "3rdparty")
