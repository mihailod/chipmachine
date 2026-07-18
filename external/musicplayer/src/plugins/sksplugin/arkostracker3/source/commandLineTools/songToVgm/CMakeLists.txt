cmake_minimum_required(VERSION 3.12)

project(SongToVgm VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(${TargetName}
        PRODUCT_NAME "Song to VGM")

target_sources(${TargetName}
        PRIVATE
        main.cpp
        ToolSongToVgm.cpp
        ToolSongToVgm.h

        ../../../source/export/vgm/VgmExporter.cpp
        ../../../source/export/vgm/VgmExporter.h
)

target_link_libraries(${TargetName}
        PRIVATE
        BaseExport
        juce::juce_core
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)