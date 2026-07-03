cmake_minimum_required(VERSION 3.15)

# Auto-detect macOS deployment target from host OS (can be overridden with -D)
if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    execute_process(COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE _MACOS_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "^[0-9]+\\.[0-9]+" _MACOS_MAJOR_MINOR "${_MACOS_VERSION}")
    if(_MACOS_MAJOR_MINOR)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "${_MACOS_MAJOR_MINOR}" CACHE STRING "macOS Deployment Target")
    endif()
endif()

project(chipmachine)

# ---------------------------------------------------------------------------
# macOS & Architecture Initialization (Must run BEFORE target definitions)
# ---------------------------------------------------------------------------
if(APPLE)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    list(APPEND CMAKE_FRAMEWORK_PATH "/opt/homebrew/Frameworks")

    include_directories(/opt/homebrew/include)
    link_directories(/opt/homebrew/lib)

    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64")
    endif()
endif()

# Global overrides
set(ZLIB_LIBRARIES z CACHE INTERNAL "" FORCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# FORCE GLOBAL SILENCING OF INTERFACE OVERRIDE WARNINGS (FIXES RSNPLUGIN LEAK)
if(APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-Wno-inconsistent-missing-override)
endif()

# Compiler Cache
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

# Subproject Paths
set(AP1_ROOT ../apone CACHE PATH "Where apone is checked out")
set(MUSICPLAYER ../musicplayer CACHE PATH "Where musicplayer is checked out")
get_filename_component(VICE_ROOT "${CMAKE_SOURCE_DIR}/../vice310" ABSOLUTE)
set(VICE_ROOT ${VICE_ROOT} CACHE PATH "Where vice is checked out")

include(${AP1_ROOT}/cmake/Utils.cmake)

# Platform specific flag adjustments
if(RASPBERRYPI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6 -mfpu=vfp -mfloat-abi=hard")
endif()

# ---------------------------------------------------------------------------
# Unified Warning Profiles & AppleClang Tweaks
# ---------------------------------------------------------------------------
set(GCC_CLANG_WARNINGS -Wall -Wextra -Wnon-virtual-dtor -pedantic)

if(APPLE OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND GCC_CLANG_WARNINGS -Wno-deprecated-declarations -Wno-string-compare -Wno-pointer-sign -Wno-inconsistent-missing-override)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    string(APPEND CMAKE_CXX_FLAGS " -fno-common")
    string(APPEND CMAKE_C_FLAGS " -fno-common")
    add_link_options("-Wl,-no_warn_duplicate_libraries")
endif()

set(GCC_WARNINGS
    -Wduplicated-branches
    -Wduplicated-cond
    -Wlogical-op
    -Wnull-dereference
    -Wcast-align
    -Wno-missing-field-initializers)

set(MSVC_WARNINGS /W4)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(WARNINGS ${WARNINGS} ${GCC_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(WARNINGS ${WARNINGS} ${CLANG_WARNINGS} ${GCC_CLANG_WARNINGS})
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(WARNINGS ${WARNINGS} ${MSVC_WARNINGS})
endif()

add_library(Warnings INTERFACE)
target_compile_options(Warnings INTERFACE ${WARNINGS})

if(NOT WINDOWS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -funsigned-char")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -funsigned-char")

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O2")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -DCM_DEBUG")
endif()

if(SAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=${SAN}")
endif()

# ---------------------------------------------------------------------------
# Subdirectories & Dependency Layout (Processed early)
# ---------------------------------------------------------------------------
add_subdirectory(external/lua SYSTEM)
add_subdirectory(external/sol2 SYSTEM)

set(CORE_MODULES coreutils audioplayer archive webutils sqlite3 xml bbsutils crypto)
set(GUI_MODULES grappix fft)
set(APONE_MODULES ${GUI_MODULES} ${CORE_MODULES})

add_subdirectory(${AP1_ROOT}/mods ap1mods)

# Define core plugin array
set(MUSICPLAYER_PLUGINS
    openmptplugin htplugin heplugin ndsplugin gmeplugin sc68plugin
    usfplugin stsoundplugin adplugin mp3plugin hivelyplugin rsnplugin
    ayflyplugin mdxplugin s98plugin aoplugin gsfplugin tedplugin v2plugin
    ffmpegplugin uadeplugin fmpplugin pxtoneplugin ptkplugin orgplugin sunvoxplugin eupplugin
    kssplugin quartetplugin wsrplugin zxtuneplugin pokeynoiseplugin bbsongplugin
    soundsmithplugin musxplugin cocoplugin mgtplugin medplugin sbstudioplugin maxtraxplugin sksplugin nedplugin sccmusixxplugin
    playerproplugin jxsplugin monotoneplugin mikmodplugin ixsplugin copplugin
    famitrackerplugin goattrackerplugin dmfplugin
)
add_subdirectory(${MUSICPLAYER}/src/plugins plugins)
add_subdirectory(${MUSICPLAYER}/src/psf psf)

include_directories(${AP1_ROOT}/mods/grappix ${MUSICPLAYER}/plugins)

# liblhasa (vendored under zxtune) decompresses the .lha archives that the
# UnExoticA collection ships its tunes in. The `lhasa` static-lib target is
# created by the zxtuneplugin subproject above.
get_filename_component(ZXTUNE_ROOT "${CMAKE_SOURCE_DIR}/../zxtune" ABSOLUTE)
include_directories(${ZXTUNE_ROOT}/3rdparty/lhasa/lib/public)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")
endif()

if(SAN)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=${SAN}")
endif()

add_library(musicplayer INTERFACE)
add_library(plugin_register STATIC src/plugin_register.cpp)
target_link_libraries(musicplayer INTERFACE psf plugin_register)
target_include_directories(musicplayer INTERFACE ${MUSICPLAYER}/..)

if(APPLE)
    set(APPLE_FRAMEWORKS 
        "-framework CoreFoundation" 
        "-framework IOKit" 
        "-framework Cocoa" 
        "-framework AudioToolbox" 
        "-framework AudioUnit"
        "-framework Foundation"
        "-framework AppKit"
    )
endif()

# Source file layout declarations
set(MAIN_FILES
    src/MusicDatabase.cpp
    src/GZPlugin.cpp
    src/LhaArchive.cpp
    src/MusicPlayer.cpp
    src/MusicPlayerList.cpp
    src/RemoteLoader.cpp
    src/SearchIndex.cpp
    src/SongFileIdentifier.cpp
    src/state_machine.cpp
    src/youtube.cpp
    src/textmode.cpp
    src/CheckForUpdate.mm
    src/NativeDialogs.mm
)

set(GUI_FILES
    src/ChipMachine.cpp
    src/MusicBars.cpp
    src/ChipMachine_config.cpp
    src/ChipMachine_keys.cpp
    src/ChipMachine_commands.cpp
    src/textmode.cpp
)

set(SOURCE_FILES ${SOURCE_FILES} src/version.h src/TextField.h src/TextListView.h src/CueSheet.h
    src/Dialog.h src/LineEdit.h src/Icons.h src/SongInfo.h src/SongInfoField.h src/ChipInterface.h)

file(GLOB DATA_FILES data/*.txt)
file(GLOB LUA_FILES lua/*.lua)
file(GLOB DOC_FILES *.md)
file(GLOB SCRIPT_FILES scripts/*)

add_custom_target(chipmachine.data_files SOURCES ${DATA_FILES})
add_custom_target(chipmachine.lua_files SOURCES ${LUA_FILES})
add_custom_target(chipmachine.doc_files SOURCES ${DOC_FILES})
add_custom_target(chipmachine.script_files SOURCES ${SCRIPT_FILES})

# ---------------------------------------------------------------------------
# Targets Execution definitions (Direct file-level dependency tracking)
# ---------------------------------------------------------------------------

if(APPLE)
    add_executable(chipmachine MACOSX_BUNDLE ${MAIN_FILES} ${GUI_FILES} src/main.cpp)

    # Existing: copy the compiled binary next to the build dir for direct invocation
    add_custom_command(TARGET chipmachine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:chipmachine>
            ${CMAKE_CURRENT_BINARY_DIR})

else()
    add_executable(chipmachine ${MAIN_FILES} ${GUI_FILES} src/main.cpp)
endif()

target_link_libraries(chipmachine PRIVATE
    musicplayer
    plugin_register
    ${CORE_MODULES}
    ${GUI_MODULES}
    ${MUSICPLAYER_PLUGINS}
    lhasa
    vicepluginbridge
    sol
    ${APPLE_FRAMEWORKS}
    avcodec
    avformat
    avutil
    swresample
)
target_compile_definitions(chipmachine PRIVATE -DAPP_NAME=chipmachine)

if(NOT WIN32)
    add_executable(cm ${MAIN_FILES} src/textmode.cpp src/main.cpp)
    target_compile_definitions(cm PUBLIC TEXTMODE_ONLY)
    target_link_libraries(cm PRIVATE
        musicplayer
        ${CORE_MODULES}
        ${MUSICPLAYER_PLUGINS}
        lhasa
        vicepluginbridge
        sol
        ${APPLE_FRAMEWORKS}
        avcodec
        avformat
        avutil
        swresample
        )
endif()

add_executable(mksonglist scripts/mksonglist/mksonglist.cpp src/SongFileIdentifier.cpp)
target_link_libraries(mksonglist PRIVATE coreutils archive sc68plugin mp3plugin)

add_executable(cmtest testmain.cpp test.cpp test_opna_rhythm.cpp ${MAIN_FILES})
target_link_libraries(cmtest PRIVATE
    plugin_register
    ${CORE_MODULES}
    ${MUSICPLAYER_PLUGINS}
    lhasa
    vicepluginbridge
    musicplayer
    sol
    ${APPLE_FRAMEWORKS}
    avcodec
    avformat
    avutil
    swresample
)
target_include_directories(cmtest PRIVATE ${APONE}/mods
    ${MUSICPLAYER}/src/plugins/s98plugin/m_s98/device/fmgen)
target_compile_definitions(cmtest PRIVATE -DTESTING)

# The SunVox engine is a prebuilt shared library that the plugin dlopen()s at
# runtime from next to the executable. Copy it beside each binary that can play
# music so SunVoxPlugin can find it (see SUNVOX_RUNTIME_LIB in sunvoxplugin).
if(SUNVOX_RUNTIME_LIB)
    foreach(_sv_target chipmachine cm cmtest)
        if(TARGET ${_sv_target})
            add_custom_command(TARGET ${_sv_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${SUNVOX_RUNTIME_LIB}
                    $<TARGET_FILE_DIR:${_sv_target}>)
        endif()
    endforeach()
endif()

# --- EXTERNAL STUFF COMPILER WARNING SUPPRESSION ---
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(SILENT_TARGETS
        vicepluginbridge ndsplugin gsfplugin usfplugin heplugin adplugin
        aoplugin v2plugin unrar coreutils lua crypto tedplugin gmeplugin
        openmptplugin sc68plugin ayflyplugin mdxplugin s98plugin ffmpegplugin uadeplugin rsnplugin fmpplugin
    )
    foreach(target_item ${SILENT_TARGETS})
        if(TARGET ${target_item})
            target_compile_options(${target_item} PRIVATE -w)
        endif()
    endforeach()
endif()

target_compile_options(adplugin PRIVATE -fsigned-char)

# ---------------------------------------------------------------------------
# Bundle install() setup — used by cmake --install and as a reference layout.
# Note: package_app.sh is the authoritative release packager; this block
# keeps `cmake --install` consistent and documents the expected asset layout.
# ---------------------------------------------------------------------------
if(APPLE)
    set(APPS ${CMAKE_CURRENT_BINARY_DIR}/Chipmachine.app)
    set(DIRS "")
    install(CODE "
       file(COPY \"\${CMAKE_SOURCE_DIR}/lua\"  DESTINATION Chipmachine.app/Contents/Resources/)
       file(COPY \"\${CMAKE_SOURCE_DIR}/data\" DESTINATION Chipmachine.app/Contents/Resources/)
       file(MAKE_DIRECTORY Chipmachine.app/Contents/Resources/bin)
       file(COPY \"\${CMAKE_SOURCE_DIR}/Info.plist\" DESTINATION Chipmachine.app/Contents/)

       # NEW: Copy .nsfe music tracks so the bundle produced by cmake --install
       # matches what package_app.sh builds. resolve_resource_root() in the C++
       # layer expects these at Contents/Resources/music/Console/.
       if(EXISTS \"\${CMAKE_SOURCE_DIR}/music/Console\")
           file(COPY \"\${CMAKE_SOURCE_DIR}/music/Console\"
                DESTINATION Chipmachine.app/Contents/Resources/music/)
           message(STATUS \"-> Bundled music/Console into Resources\")
       else()
           message(WARNING \"music/Console not found — bundle will have no built-in .nsfe tracks\")
       endif()

       # HVTC (Commodore 16/116/+4 TED .prg) was pivoted from the online
       # plus4world mirror to a shipped local store, same as music/Console.
       if(EXISTS \"\${CMAKE_SOURCE_DIR}/music/hvtc\")
           file(COPY \"\${CMAKE_SOURCE_DIR}/music/hvtc\"
                DESTINATION Chipmachine.app/Contents/Resources/music/)
           message(STATUS \"-> Bundled music/hvtc into Resources\")
       else()
           message(WARNING \"music/hvtc not found — bundle will have no built-in HVTC .prg tracks\")
       endif()

       include(BundleUtilities)
       fixup_bundle(\"\${APPS}\" \"\" \"\$\\\{DIRS\}\")
       file(COPY \"\${CMAKE_SOURCE_DIR}/bin/ffmpeg\" DESTINATION Chipmachine.app/Contents/Resources/bin/)
       " COMPONENT Runtime)
endif()
