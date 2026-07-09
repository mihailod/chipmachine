cmake_minimum_required(VERSION 3.12)

project(SongToAky VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(${TargetName}
        PRODUCT_NAME "Song to AKY")

target_sources(${TargetName}
        PRIVATE
        main.cpp
        ToolSongToAky.cpp
        ToolSongToAky.h

        ../../../source/export/aky/AkyExporter.cpp
        ../../../source/export/aky/AkyExporter.h
        ../../../source/export/aky/process/RegisterBlockLabelProvider.h
        ../../../source/export/aky/process/AkyConstants.h
        ../../../source/export/aky/process/AkyPattern.cpp
        ../../../source/export/aky/process/AkyPattern.h
        ../../../source/export/aky/process/AkyPatterns.cpp
        ../../../source/export/aky/process/AkyPatterns.h
        ../../../source/export/aky/process/AkyTrack.cpp
        ../../../source/export/aky/process/AkyTrack.h
        ../../../source/export/aky/process/ChannelData.cpp
        ../../../source/export/aky/process/ChannelData.h
        ../../../source/export/aky/process/EncodedLine.cpp
        ../../../source/export/aky/process/EncodedLine.h
        ../../../source/export/aky/process/EncodedLine.cpp
        ../../../source/export/aky/process/EncodedLine.h
        ../../../source/export/aky/process/EncodedSequence.cpp
        ../../../source/export/aky/process/EncodedSequence.h
        ../../../source/export/aky/process/RegisterBlock.cpp
        ../../../source/export/aky/process/RegisterBlock.h
        ../../../source/export/aky/process/RegisterBlockEncoder.cpp
        ../../../source/export/aky/process/RegisterBlockEncoder.h
        ../../../source/export/aky/process/RegisterBlockLabelProvider.h
        ../../../source/export/aky/process/RegisterBlockOptimizer.cpp
        ../../../source/export/aky/process/RegisterBlockOptimizer.h
        ../../../source/export/aky/process/RegisterBlockPool.cpp
        ../../../source/export/aky/process/RegisterBlockPool.h
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