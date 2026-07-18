cmake_minimum_required(VERSION 3.12)

project(SongToWav VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(
        ${TargetName}
        PRODUCT_NAME "Song to WAV")

target_sources(${TargetName}
        PRIVATE
        main.cpp
        ToolSongToWav.cpp
        ToolSongToWav.h

        ../../export/wav/SongWavExporter.cpp
        ../../export/wav/SongWavExporter.h
        ../../export/wav/DcOffsetCalculator.cpp
        ../../export/wav/DcOffsetCalculator.h
)

target_link_libraries(${TargetName}
        PRIVATE
        BaseExport
        juce::juce_audio_formats        # Needed for the WAV generator.
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)