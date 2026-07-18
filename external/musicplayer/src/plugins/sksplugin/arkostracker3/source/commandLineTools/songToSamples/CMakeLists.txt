cmake_minimum_required(VERSION 3.12)

project(SongToSamples VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(${TargetName}
        PRODUCT_NAME "Song to samples")

target_sources(${TargetName}
        PRIVATE
        main.cpp
        ToolSongToSamples.cpp
        ToolSongToSamples.h

        ../../../source/export/samples/SampleExporter.cpp
        ../../../source/export/samples/SampleExporter.h
        ../../../source/export/samples/SampleCodeGenerator.cpp
        ../../../source/export/samples/SampleCodeGenerator.h
        ../../../source/export/samples/SampleEncoderFlags.cpp
        ../../../source/export/samples/SampleEncoderFlags.h
        ../../../source/export/samples/SampleEncoder.cpp
        ../../../source/export/samples/SampleEncoder.h
        ../../../source/export/SongExportResult.cpp
        ../../../source/export/SongExportResult.h

        ../../../source/business/instrument/SampleResampler.cpp
        ../../../source/business/instrument/SampleResampler.h
        ../../../source/business/song/tool/InstrumentCounter.cpp
        ../../../source/business/song/tool/InstrumentCounter.h
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