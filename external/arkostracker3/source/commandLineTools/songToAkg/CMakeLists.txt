cmake_minimum_required(VERSION 3.12)

project(SongToAkg VERSION 1.0.0)

set(TargetName ${PROJECT_NAME})

juce_add_console_app(${TargetName}
        PRODUCT_NAME "Song to AKG")

target_sources(${TargetName}
        PRIVATE
        main.cpp
        ToolSongToAkg.cpp
        ToolSongToAkg.h

        ../../../source/export/akg/AkgExporter.cpp
        ../../../source/export/akg/AkgExporter.h
        ../../../source/export/akg/process/AkgPattern.cpp
        ../../../source/export/akg/process/AkgPattern.h
        ../../../source/export/akg/process/AkgPatternItem.cpp
        ../../../source/export/akg/process/AkgPatternItem.h
        ../../../source/export/akg/process/AkgSubsongExporter.cpp
        ../../../source/export/akg/process/AkgSubsongExporter.h
        ../../../source/export/akg/process/EncodedEffect.h
        ../../../source/export/akg/process/EncodedEffectBlock.cpp
        ../../../source/export/akg/process/EncodedEffectBlock.h
        ../../../source/export/akg/process/LinkerBlock.cpp
        ../../../source/export/akg/process/LinkerBlock.h
        ../../../source/export/akg/process/LinkerBlockContainer.cpp
        ../../../source/export/akg/process/LinkerBlockContainer.h
        ../../../source/export/akg/process/PsgInstrumentCellEncoder.cpp
        ../../../source/export/akg/process/PsgInstrumentCellEncoder.h
        ../../../source/export/samples/SampleCodeGenerator.cpp
        ../../../source/export/samples/SampleCodeGenerator.h
        ../../../source/export/samples/SampleEncoder.cpp
        ../../../source/export/samples/SampleEncoder.h
        ../../../source/export/samples/SampleEncoderFlags.cpp
        ../../../source/export/samples/SampleEncoderFlags.h
        ../../../source/export/SongExportResult.cpp
        ../../../source/export/SongExportResult.h

        ../../../source/business/instrument/SampleResampler.cpp
        ../../../source/business/instrument/SampleResampler.h
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