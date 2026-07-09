cmake_minimum_required(VERSION 3.12)

project(SongToAkm VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(${TargetName}
        PRODUCT_NAME "Song to AKM")

target_sources(${TargetName}
        PRIVATE
        main.cpp
        ToolSongToAkm.cpp
        ToolSongToAkm.h

        ../../../source/export/akm/AkmExporter.cpp
        ../../../source/export/akm/AkmExporter.h
        ../../../source/export/akm/process/AkmInstrumentEncoder.cpp
        ../../../source/export/akm/process/AkmInstrumentEncoder.h
        ../../../source/export/akm/process/AkmSequenceEncoder.cpp
        ../../../source/export/akm/process/AkmSequenceEncoder.h
        ../../../source/export/akm/process/AkmSongEncoder.cpp
        ../../../source/export/akm/process/AkmSongEncoder.h
        ../../../source/export/akm/process/AkmSubsongEncoder.cpp
        ../../../source/export/akm/process/AkmSubsongEncoder.h
        ../../../source/export/akm/process/LinkerEncoder.cpp
        ../../../source/export/akm/process/LinkerEncoder.h
        ../../../source/export/SongExportResult.cpp
        ../../../source/export/SongExportResult.h
)

target_compile_features(${TargetName} PRIVATE cxx_std_11)

target_link_libraries(${TargetName}
        PRIVATE
        BaseExport
        juce::juce_core
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)