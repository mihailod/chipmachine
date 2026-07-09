#include "AkmSongEncoder.h"

#include "../../../song/Song.h"
#include "../../../utils/NumberUtil.h"
#include "../../ExportConfiguration.h"
#include "../../SongExportResult.h"
#include "../../sourceGenerator/SourceGenerator.h"
#include "AkmInstrumentEncoder.h"

namespace arkostracker 
{

AkmSongEncoder::AkmSongEncoder(const Song& pSong, const ExportConfiguration& pExportConfiguration, SongExportResult& pSongExportResult, juce::String pBaseLabel,
                               std::function<juce::String(int subsongIndex)> pGetSubsongLabel) noexcept:
        song(pSong),
        exportConfiguration(pExportConfiguration),
        songExportResult(pSongExportResult),
        baseLabel(std::move(pBaseLabel)),
        getSubsongLabel(std::move(pGetSubsongLabel))
{
}

void AkmSongEncoder::encodeSong() const noexcept
{
    auto songOutputStream = std::make_unique<juce::MemoryOutputStream>();
    SourceGenerator sourceGenerator(exportConfiguration.getSourceConfiguration(), *songOutputStream);
    sourceGenerator.setPrefixForDisark(baseLabel);
    const PlayerConfiguration playerConfiguration;

    encodeSongHeader(sourceGenerator);

    // Encodes the Instruments and its index table.
    encodeInstrumentsAndIndex(sourceGenerator);

    // Encodes the Expressions, and index tables too.
    encodeExpressionsAndIndexes(sourceGenerator);

    songExportResult.setSongStream(std::move(songOutputStream));
}

void AkmSongEncoder::encodeSongHeader(SourceGenerator& sourceGenerator) const noexcept
{
    sourceGenerator.declareComment(song.getTitle() + ", Song part, encoded in the AKM (minimalist) format V1.").addEmptyLine();

    // Org?
    if (exportConfiguration.getAddress().isPresent()) {
        sourceGenerator.declareAddressChange(exportConfiguration.getAddress().getValue()).addEmptyLine();
    }

    // Declares a start label.
    const auto startLabel = baseLabel + "Start";
    sourceGenerator.declareLabel(startLabel);
    sourceGenerator.declareExternalLabel(startLabel).addEmptyLine();

    sourceGenerator.declarePointerRegionStart();
    sourceGenerator.declareComment("Index table for the Instruments.");
    sourceGenerator.declareWord(getInstrumentIndexLabel());

    sourceGenerator.declareComment("Index table for the Arpeggios.");
    // There may be no Arpeggio/Pitch (the 0 is not encoded, but there is still one in the Song).
    if (song.getConstExpressionHandler(true).getCount() <= 1) {
        sourceGenerator.declareForceNonReferenceAreaDuring(2);  // We don't want 0 to be relocated.
        sourceGenerator.declareWord("0");
    } else {
        sourceGenerator.declareWord(getArpeggioIndexLabel() + " - 2");
    }
    sourceGenerator.addEmptyLine();

    sourceGenerator.declareComment("Index table for the Pitches.");
    if (song.getConstExpressionHandler(false).getCount() <= 1) {
        sourceGenerator.declareForceNonReferenceAreaDuring(2);  // We don't want 0 to be relocated.
        sourceGenerator.declareWord("0");
    } else {
        sourceGenerator.declareWord(getPitchIndexLabel() + " - 2");
    }
    sourceGenerator.addEmptyLine();

    // Encodes the Subsong addresses.
    sourceGenerator.declareComment("The subsongs references.");
    for (auto subsongIndex = 0, subsongCount = static_cast<int>(exportConfiguration.getSubsongIds().size()); subsongIndex < subsongCount; ++subsongIndex) {
        sourceGenerator.declareWord(getSubsongLabel(subsongIndex));
    }

    sourceGenerator.declarePointerRegionEnd();
    sourceGenerator.addEmptyLine();
}

juce::String AkmSongEncoder::getInstrumentLabel(const int instrumentIndex) const noexcept
{
    return baseLabel + "Instrument" + juce::String(instrumentIndex);
}

juce::String AkmSongEncoder::getInstrumentIndexLabel() const noexcept
{
    return baseLabel + "InstrumentIndexes";
}

juce::String AkmSongEncoder::getArpeggioIndexLabel() const noexcept
{
    return baseLabel + "ArpeggioIndexes";
}

juce::String AkmSongEncoder::getPitchIndexLabel() const noexcept
{
    return baseLabel + "PitchIndexes";
}

juce::String AkmSongEncoder::getArpeggioLabel(const int index) const noexcept
{
    return baseLabel + "Arpeggio" + juce::String(index);
}

juce::String AkmSongEncoder::getPitchLabel(const int index) const noexcept
{
    return baseLabel + "Pitch" + juce::String(index);
}

void AkmSongEncoder::encodeInstrumentsAndIndex(SourceGenerator& sourceGenerator) const noexcept
{
    // Generates the Instrument indexes.
    sourceGenerator.declareComment("The Instrument indexes.");
    sourceGenerator.declareLabel(getInstrumentIndexLabel());

    sourceGenerator.declarePointerRegionStart();
    const auto instrumentCount = song.getInstrumentCount();
    for (auto instrumentIndex = 0; instrumentIndex < instrumentCount; ++instrumentIndex) {
        sourceGenerator.declareWord(getInstrumentLabel(instrumentIndex));
    }
    sourceGenerator.declarePointerRegionEnd();
    sourceGenerator.addEmptyLine();

    // Generates each Instrument.
    sourceGenerator.declareComment("The Instrument.");
    sourceGenerator.declareByteRegionStart();

    AkmInstrumentEncoder instrumentEncoder(song, sourceGenerator, songExportResult,
                                           false, // Encodes non-inverted ratio.
                                           [this](const int instrumentIndex) -> juce::String {
                                               return getInstrumentLabel(instrumentIndex);
                                           });
    for (auto instrumentIndex = 0; instrumentIndex < instrumentCount; ++instrumentIndex) {
        instrumentEncoder.encodeInstrument(instrumentIndex);
    }
    sourceGenerator.declareByteRegionEnd();
}

void AkmSongEncoder::encodeExpressionsAndIndexes(SourceGenerator& sourceGenerator) const noexcept
{
    // Encodes the Arpeggios.
    encodeExpressionsAndIndexes(sourceGenerator, true,
                                [this] { return getArpeggioIndexLabel(); },
                                [this](const int expressionIndex) { return getArpeggioLabel(expressionIndex); }
                                );

    // Encodes the Pitches.
    encodeExpressionsAndIndexes(sourceGenerator, false,
                                [this] { return getPitchIndexLabel(); },
                                [this](const int expressionIndex) { return getPitchLabel(expressionIndex); }
    );
}

void AkmSongEncoder::encodeExpressionsAndIndexes(SourceGenerator& sourceGenerator, const bool isArpeggio, const std::function<juce::String()>& getExpressionTableLabel,
                                                 const std::function<juce::String(int)>& getExpressionLabel) const noexcept
{
    const auto invertValue = !isArpeggio;

    // Nothing to encode? Stops.
    const auto& expressionHandler = song.getConstExpressionHandler(isArpeggio);
    const auto expressionCount = expressionHandler.getCount();
    if (expressionCount <= 0) {
        return;
    }

    sourceGenerator.declareLabel(getExpressionTableLabel());

    // Encodes each entry, skipping the 0.
    sourceGenerator.declarePointerRegionStart();
    for (auto expressionIndex = 1; expressionIndex < expressionCount; ++expressionIndex) {
        sourceGenerator.declareWord(getExpressionLabel(expressionIndex));
    }
    sourceGenerator.declarePointerRegionEnd();
    sourceGenerator.addEmptyLine();

    // Encodes each Expression.
    constexpr auto maximumHeight = 64;
    constexpr auto maximumValue = 63;
    constexpr auto minimumValue = -64;

    sourceGenerator.declareByteRegionStart();
    for (auto expressionIndex = 1; expressionIndex < expressionCount; ++expressionIndex) {
        const auto expression = expressionHandler.getExpressionCopy(expressionIndex);

        const auto expressionLabel = getExpressionLabel(expressionIndex);
        sourceGenerator.declareLabel(expressionLabel);
        // Header: speed.
        sourceGenerator.declareByte(expression.getSpeed(), "Speed").addEmptyLine();

        auto height = expression.getLength(true);
        if (height > maximumHeight) {
            height = maximumHeight;
            songExportResult.addWarning("Expressions (pitch and arpeggio) too large! have been truncated to " + juce::String(maximumHeight) + ".");
        }

        // Encodes each Cell.
        const auto loopIndex = expression.getLoop(true).getStartIndex();
        for (auto cellIndex = 0; cellIndex < height; ++cellIndex) {
            // Reads the value to encode. It may need a correction, as the AKM has a smaller range than what AT2 allows.
            auto cellValue = expression.getValue(cellIndex, true);
            if (invertValue) {
                cellValue = -cellValue;
            }

            const auto oldCellValue = cellValue;
            cellValue = NumberUtil::correctNumber(cellValue, minimumValue, maximumValue);
            if (oldCellValue != cellValue) {
                songExportResult.addWarning("Expressions (pitch and arpeggio) value too large! have been truncated to " + juce::String(cellValue) + ".");
            }

            // Encodes the value on bits 7-1.
            sourceGenerator.declareByte(static_cast<int>(static_cast<unsigned int>(cellValue) << 1U), "Value: " + juce::String(cellValue));
        }

        // Encodes the loop.
        sourceGenerator.declareByte(juce::String(loopIndex) + " * 2 + 1",
                                    "Loops to index " + juce::String(loopIndex) + ".");
    }
    sourceGenerator.declareByteRegionEnd();
    sourceGenerator.addEmptyLine();
}


}   // namespace arkostracker

