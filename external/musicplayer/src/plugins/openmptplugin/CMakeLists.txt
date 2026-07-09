cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.openmptplugin)

# libopenmpt 0.8.x modular layout: decode core lives in soundlib/, common/,
# sounddsp/ and libopenmpt/. The src/ tree (src/mpt, src/openmpt) is
# header-only for our purposes (its .cpp are sound-device / stream-encoder
# code we don't build), so it only needs to be on the include path.
file(GLOB SOUNDLIB_FILES   CONFIGURE_DEPENDS openmpt/soundlib/*.cpp)
file(GLOB PLUGIN_FILES     CONFIGURE_DEPENDS openmpt/soundlib/plugins/*.cpp)
file(GLOB DMO_FILES        CONFIGURE_DEPENDS openmpt/soundlib/plugins/dmo/*.cpp)
file(GLOB SOUNDDSP_FILES   CONFIGURE_DEPENDS openmpt/sounddsp/*.cpp)
file(GLOB COMMON_FILES     CONFIGURE_DEPENDS openmpt/common/*.cpp)
file(GLOB LIBOPENMPT_FILES CONFIGURE_DEPENDS openmpt/libopenmpt/libopenmpt_*.cpp)

set(SOURCE_FILES
    OpenMPTPlugin.cpp
    ${COMMON_FILES}
    ${LIBOPENMPT_FILES}
    ${SOUNDLIB_FILES}
    ${PLUGIN_FILES}      # SymMODEcho (Symphonie echo DSP), DigiBoosterEcho, ...
    ${DMO_FILES}
    ${SOUNDDSP_FILES}    # Reverb
)

add_library(openmptplugin ${SOURCE_FILES})
target_link_libraries(openmptplugin PRIVATE coreutils z)
target_include_directories(openmptplugin PRIVATE openmpt openmpt/common openmpt/src .)
# NOTE: plugins + reverb are now compiled in (no NO_PLUGINS/NO_REVERB) because
# Symphonie (.symmod) routes its echo through the SymMODEcho plugin.
target_compile_definitions(openmptplugin PRIVATE
    -DLIBOPENMPT_BUILD
    -DOPENMPT_VERSION_REVISION=0
    -DMPT_CHARSET_CUSTOMUTF8
    -DMPT_WITH_ZLIB)
