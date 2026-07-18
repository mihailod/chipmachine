# Empty project with only the bare necessary classes to make an CLI tool.

cmake_minimum_required(VERSION 3.12)

project(BaseCli)

set(TargetName ${PROJECT_NAME})

add_library(${TargetName})

target_compile_definitions(${TargetName} PRIVATE)

target_sources(
        ${TargetName}
        PRIVATE
        ../utils/CommandLineArgumentDescriptor.cpp
        ../utils/CommandLineArgumentDescriptor.h
        ../utils/CommandLineConstants.cpp
        ../utils/CommandLineConstants.h
        ../utils/CommandLineParser.cpp
        ../utils/CommandLineParser.h
        ../utils/Option.cpp
        ../utils/Option.h
        ../utils/Parameter.cpp
        ../utils/Parameter.h
        ../../utils/FileUtil.cpp
        ../../utils/FileUtil.h
        ../../utils/MemoryBlockUtil.cpp
        ../../utils/MemoryBlockUtil.h
        ../../utils/ZipHelper.cpp
        ../../utils/ZipHelper.h
        ../../utils/NumberUtil.cpp
        ../../utils/NumberUtil.h
)

target_compile_definitions(${TargetName}
        PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_LADSPA=0
        JUCE_USE_XCURSOR=0
        JUCE_MODULE_AVAILABLE_juce_opengl=0
        DONT_SET_USING_JUCE_NAMESPACE=1
        JUCE_SILENCE_XCODE_15_LINKER_WARNING=1
)

target_link_libraries(${TargetName}
        PRIVATE
        ThirdPartyLzh                   # ZipHelper is included to FileUtil.
        PUBLIC
        juce::juce_events               # Needed for the juce::ScopedJuceInitialiser_GUI assertion removal.
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    target_link_options(${TargetName}
            PUBLIC
            -Wl,-ld_classic     # To avoid Linker warnings. See https://forum.juce.com/t/vst-au-builds-fail-after-upgrading-to-xcode-15/57936/64?u=jnv
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(${TargetName}
            PUBLIC
            -no-pie             # To have mime type application/x-executable instead of application/x-sharedlib, and thus allowing direct launch.
    )
endif()
