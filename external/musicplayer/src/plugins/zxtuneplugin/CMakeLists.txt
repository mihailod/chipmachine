cmake_minimum_required(VERSION 3.12)
project(cpp-mods.musicplayer.zxtuneplugin CXX C)

# ZXTune lives as a sibling repo inside chipmachine-as/, like libkss/eupmini.
#   chipmachine-as/
#     chipmachine/   <- CMAKE_SOURCE_DIR (or musicplayer/ for the standalone build)
#     musicplayer/
#     zxtune/        <- here  (checkout of djdron/zxtune @ branch "cmake")
get_filename_component(_zxtune_default
    "${CMAKE_SOURCE_DIR}/external/zxtune" ABSOLUTE)
set(ZXTUNE_ROOT "${_zxtune_default}" CACHE PATH "Where djdron/zxtune is checked out")

if(NOT EXISTS "${ZXTUNE_ROOT}/src/core/service.h")
    message(FATAL_ERROR
        "zxtuneplugin: ZXTune not found at ${ZXTUNE_ROOT}\n"
        "Clone https://github.com/djdron/zxtune (branch 'cmake') next to "
        "chipmachine-as, or pass -DZXTUNE_ROOT=/path/to/zxtune to cmake.")
endif()

# Boost is header-only here (ZXTune uses boost utilities; we link no Boost lib).
# CMake 3.30+ (policy CMP0167) drops the bundled FindBoost module in favour of
# the upstream BoostConfig.cmake that brew/vcpkg ship -- opt into it to avoid the
# deprecation warning. The config package still sets Boost_INCLUDE_DIRS, so the
# include_directories() below is unchanged. Guarded for older CMake.
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost REQUIRED)
find_package(ZLIB REQUIRED)        # binary/compression wraps zlib

# ZXTune is a modern-C++ codebase; build the engine at C++20 regardless of the
# host project's standard (chipmachine builds at C++17). This is directory
# scoped: it does not leak to the other plugins.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${ZXTUNE_ROOT}
    ${ZXTUNE_ROOT}/include
    ${ZXTUNE_ROOT}/src
    ${Boost_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIRS})
add_definitions(-DHAVE_BOOST -DNO_DEBUG_LOGS)

if(NOT MSVC)
    add_compile_options(-w)        # the vendored engine is third-party; keep it quiet
endif()

# Hide every ZXTune symbol so the engine (and its bundled fmt) stays private to
# this archive. chipmachine links its own fmt::fmt; without hidden visibility
# the two fmt copies collide at final link (same trick kssplugin uses for OPLL).
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(_zxt_bin "${CMAKE_CURRENT_BINARY_DIR}/zxt")

# --- vendored 3rdparty (only what the AY/ST decoders actually pull in) ---
add_subdirectory("${ZXTUNE_ROOT}/3rdparty/lhasa" "${_zxt_bin}/lhasa")
add_subdirectory("${ZXTUNE_ROOT}/3rdparty/lzma"  "${_zxt_bin}/lzma")
add_subdirectory("${ZXTUNE_ROOT}/3rdparty/z80ex" "${_zxt_bin}/z80ex")

# --- ZXTune core (trimmed to the AY/YM family + container scanning) ---
add_subdirectory("${ZXTUNE_ROOT}/src/analysis"             "${_zxt_bin}/analysis")
add_subdirectory("${ZXTUNE_ROOT}/src/binary"               "${_zxt_bin}/binary")
add_subdirectory("${ZXTUNE_ROOT}/src/binary/format"        "${_zxt_bin}/binary_format")
add_subdirectory("${ZXTUNE_ROOT}/src/binary/compression"   "${_zxt_bin}/binary_compression")
add_subdirectory("${ZXTUNE_ROOT}/src/formats/archived"     "${_zxt_bin}/formats_archived")
add_subdirectory("${ZXTUNE_ROOT}/src/core"                 "${_zxt_bin}/core")
add_subdirectory("${ZXTUNE_ROOT}/src/core/plugins/archives" "${_zxt_bin}/core_plugins_archives")
add_subdirectory("${ZXTUNE_ROOT}/src/core/plugins/players" "${_zxt_bin}/core_plugins_players")
add_subdirectory("${ZXTUNE_ROOT}/src/debug"                "${_zxt_bin}/dbg")
add_subdirectory("${ZXTUNE_ROOT}/src/devices/aym"          "${_zxt_bin}/devices_aym")
add_subdirectory("${ZXTUNE_ROOT}/src/devices/beeper"       "${_zxt_bin}/devices_beeper")
add_subdirectory("${ZXTUNE_ROOT}/src/devices/dac"          "${_zxt_bin}/devices_dac")
add_subdirectory("${ZXTUNE_ROOT}/src/devices/fm"           "${_zxt_bin}/devices_fm")
add_subdirectory("${ZXTUNE_ROOT}/src/devices/saa"          "${_zxt_bin}/devices_saa")
add_subdirectory("${ZXTUNE_ROOT}/src/devices/z80"          "${_zxt_bin}/devices_z80")
add_subdirectory("${ZXTUNE_ROOT}/src/formats/chiptune"     "${_zxt_bin}/formats_chiptune")
add_subdirectory("${ZXTUNE_ROOT}/src/formats/packed"       "${_zxt_bin}/formats_packed")
add_subdirectory("${ZXTUNE_ROOT}/src/formats/multitrack"   "${_zxt_bin}/formats_multitrack")
add_subdirectory("${ZXTUNE_ROOT}/src/l10n/stub"            "${_zxt_bin}/l10n_stub")
add_subdirectory("${ZXTUNE_ROOT}/src/module/conversion"    "${_zxt_bin}/module_conversion")
add_subdirectory("${ZXTUNE_ROOT}/src/module/players"       "${_zxt_bin}/module_players")
add_subdirectory("${ZXTUNE_ROOT}/src/module/properties"    "${_zxt_bin}/module_properties")
add_subdirectory("${ZXTUNE_ROOT}/src/parameters"           "${_zxt_bin}/parameters")
add_subdirectory("${ZXTUNE_ROOT}/src/platform/version"     "${_zxt_bin}/platform_version")
add_subdirectory("${ZXTUNE_ROOT}/src/sound"                "${_zxt_bin}/sound")
add_subdirectory("${ZXTUNE_ROOT}/src/strings"              "${_zxt_bin}/strings")
add_subdirectory("${ZXTUNE_ROOT}/src/tools"                "${_zxt_bin}/tools")

set(ZXTUNE_LIBS
    lhasa lzma z80ex
    analysis binary binary_format binary_compression
    core core_plugins_archives core_plugins_players
    dbg devices_aym devices_beeper devices_dac devices_fm devices_saa devices_z80
    formats_chiptune formats_packed formats_multitrack formats_archived
    l10n_stub module_conversion module_players module_properties
    parameters platform_version sound strings tools)

# The plugin wrapper + our archive-plugins factory. These must export their
# symbols (zxtuneplugin_register, ZXTune::RegisterArchivePlugins) so reg.cpp and
# the engine's enumerator can resolve them -- override the hidden preset above.
add_library(zxtuneplugin STATIC ZXTunePlugin.cpp zxtune_archives.cpp)
set_target_properties(zxtuneplugin PROPERTIES
    C_VISIBILITY_PRESET default
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF)

target_link_libraries(zxtuneplugin PRIVATE
    ${ZXTUNE_LIBS} ZLIB::ZLIB coreutils)
