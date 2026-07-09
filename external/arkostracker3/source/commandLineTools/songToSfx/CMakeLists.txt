cmake_minimum_required(VERSION 3.12)

project(SongToSoundEffects VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(${TargetName}
        PRODUCT_NAME "Song to sound effects")

target_sources(${TargetName}
        PRIVATE
        main.cpp
        ToolSongToSfx.cpp
        ToolSongToSfx.h

        ../../../source/export/sfx/SfxExporter.cpp
        ../../../source/export/sfx/SfxExporter.h
        ../../../source/export/SongExportResult.cpp
        ../../../source/export/SongExportResult.h
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