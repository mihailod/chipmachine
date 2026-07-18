cmake_minimum_required(VERSION 3.12)

project(Z80Profiler VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(
        ${TargetName}
        PRODUCT_NAME "Z80 Profiler")

#Not useful.
#include_directories("${PROJECT_SOURCE_DIR}/../../../thirdParty/z80emulatorLibz80")

target_sources(
        ${TargetName}
        PRIVATE
        main.cpp
        ToolZ80Profiler.cpp
        ToolZ80Profiler.h

        ../../utils/StringUtil.cpp
        ../../utils/StringUtil.h
        ../../utils/StreamUtil.cpp
        ../../utils/StreamUtil.h
        ../../utils/ZipHelper.cpp
        ../../utils/ZipHelper.h
        ../../import/common/AmsdosHeaderUtil.cpp
        ../../import/common/AmsdosHeaderUtil.h
        Z80SimpleEmulator.cpp
        Z80SimpleEmulator.h
)

target_link_libraries(${TargetName}
        PRIVATE
        BaseCli
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
