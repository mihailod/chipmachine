#include "SongExporter.h"

#include "../../business/serialization/instrument/InstrumentSerializer.h"
#include "../../business/serialization/song/CellSerializer.h"
#include "../../business/serialization/song/ExpressionSerializer.h"
#include "../../business/serialization/song/PatternSerializer.h"
#include "../../business/serialization/song/PositionSerializer.h"
#include "../../business/serialization/song/SpecialCellSerializer.h"
#include "../../song/Song.h"
#include "../../utils/XmlHelper.h"
#include "SongNodes.h"

namespace arkostracker 
{

std::unique_ptr<juce::XmlElement> SongExporter::exportSong(const Song& song) noexcept
{
    auto songRoot = std::make_unique<juce::XmlElement>(SongNodes::song);
    // Adds the attributes to the root.
    songRoot->setAttribute(juce::Identifier(SongNodes::declareNamespaceAttribute), SongNodes::declareNamespaceValueForSong);
    songRoot->setAttribute(juce::Identifier(SongNodes::declareXsiAttribute), SongNodes::declareXsiValue);
    songRoot->setAttribute(juce::Identifier(SongNodes::declareSchemaLocationAttribute), SongNodes::declareSchemaLocationValueForSong);

    // Generic song data.
    XmlHelper::addValueNode(*songRoot, SongNodes::formatVersion, "3.0");
    XmlHelper::addValueNode(*songRoot, SongNodes::title, song.getTitle());
    XmlHelper::addValueNode(*songRoot, SongNodes::author, song.getAuthor());
    XmlHelper::addValueNode(*songRoot, SongNodes::composer, song.getComposer());
    XmlHelper::addValueNode(*songRoot, SongNodes::comment, song.getComments());
    XmlHelper::addValueNode(*songRoot, SongNodes::creationDateMs, song.getCreationDateMs());
    XmlHelper::addValueNode(*songRoot, SongNodes::modificationDateMs, song.getModificationDateMs());

    // Encodes the instruments, arpeggios, pitches.
    encodeInstruments(song, *songRoot);
    encodeExpressions(song, *songRoot, true);
    encodeExpressions(song, *songRoot, false);

    // Encodes each Subsong.
    auto& subsongsNode = XmlHelper::addNode(*songRoot, SongNodes::subsongs);
    for (const auto& subsongId : song.getSubsongIds()) {
        song.performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
            encodeSubsong(subsong, subsongsNode);
        });
    }

    return songRoot;
}

void SongExporter::encodeInstruments(const Song& song, juce::XmlElement& songRoot) noexcept
{
    // Copies all the Instruments.
    const auto instrumentIds = song.getInstrumentIds();
    const auto instrumentCount = instrumentIds.size();
    std::vector<Instrument> instruments;
    instruments.reserve(instrumentCount);

    for (const auto& instrumentId : instrumentIds) {
        song.performOnConstInstrument(instrumentId, [&](const Instrument& instrument) {
            instruments.push_back(instrument);
        });
    }

    // Creates a list of pointers.
    std::vector<const Instrument*> instrumentPtrs;
    instrumentPtrs.reserve(instruments.size());
    for (auto& instrument : instruments) {
        instrumentPtrs.push_back(&instrument);
    }

    // Serializes all the Instruments.
    auto instrumentsNode = InstrumentSerializer::serialize(instrumentPtrs);
    jassert(instrumentsNode != nullptr);

    songRoot.addChildElement(instrumentsNode.release());    // Not owned anymore!
}

void SongExporter::encodeExpressions(const Song& song, juce::XmlElement& songRoot, const bool isArpeggio) noexcept
{
    const auto& expressionHandler = song.getConstExpressionHandler(isArpeggio);
    const auto expressionIds = expressionHandler.getIds();
    const auto expressionCount = expressionIds.size();

    std::vector<Expression> expressions;
    expressions.reserve(expressionCount);

    // Gets all the Expressions.
    for (const auto& expressionId : expressionIds) {
        expressionHandler.performOnConstExpression(expressionId, [&](const Expression& expression) {
            expressions.push_back(expression);
        });
    }

    auto expressionsNode = ExpressionSerializer::serialize(expressions, true, isArpeggio);
    jassert(expressionsNode != nullptr);

    songRoot.addChildElement(expressionsNode.release());    // Not owned anymore!
}

void SongExporter::encodeSubsong(const Subsong& subsong, juce::XmlElement& subsongsNode) noexcept
{
    auto& subsongNode = XmlHelper::addNode(subsongsNode, SongNodes::subsong);

    const auto metadata = subsong.getMetadata();
    XmlHelper::addValueNode(subsongNode, SongNodes::title, metadata.getName());
    XmlHelper::addValueNode(subsongNode, SongNodes::initialSpeed, metadata.getInitialSpeed());
    XmlHelper::addValueNode(subsongNode, SongNodes::digiChannel, metadata.getDigiChannel());
    XmlHelper::addValueNode(subsongNode, SongNodes::highlightSpacing, metadata.getHighlightSpacing());
    XmlHelper::addValueNode(subsongNode, SongNodes::secondaryHighlight, metadata.getSecondaryHighlight());
    XmlHelper::addValueNode(subsongNode, SongNodes::loopStartPosition, metadata.getLoopStartPosition());
    XmlHelper::addValueNode(subsongNode, SongNodes::endPosition, metadata.getEndPosition());
    XmlHelper::addValueNode(subsongNode, SongNodes::replayFrequencyHz, metadata.getReplayFrequencyHz());

    // Encodes the PSGs.
    auto& psgsNode = XmlHelper::addNode(subsongNode, SongNodes::psgs);

    const auto& psgs = subsong.getPsgRefs();
    jassert(!psgs.empty());
    for (const auto& psg : psgs) {
        auto& psgNode = XmlHelper::addNode(psgsNode, SongNodes::psg);
        XmlHelper::addValueNode(psgNode, SongNodes::type, PsgTypeUtil::psgTypeToSerializationText(psg.getType()));
        XmlHelper::addValueNode(psgNode, SongNodes::frequencyHz, psg.getPsgFrequency());
        XmlHelper::addValueNode(psgNode, SongNodes::referenceFrequencyHz, psg.getReferenceFrequency());
        XmlHelper::addValueNode(psgNode, SongNodes::samplePlayerFrequencyHz, psg.getSamplePlayerFrequency());
        XmlHelper::addValueNode(psgNode, SongNodes::mixingOutput, PsgMixingOutputUtil::psgMixingOutputToSerializationText(psg.getPsgMixingOutput()));
    }

    // Encodes the SID, if not default.
    if (const auto sidPlayerCapability = metadata.getSidPlayerCapability(); sidPlayerCapability.getSidPlayerProfile() != SidPlayerCapability::buildDefault().getSidPlayerProfile()) {
        auto& sidNode = XmlHelper::addNode(subsongNode, SongNodes::sidPlayerCapability);
        XmlHelper::addValueNode(sidNode, SongNodes::profile, SidPlayerProfileUtil::sidPlayerProfileToSerializationText(sidPlayerCapability.getSidPlayerProfile()));
        // For custom, adds the custom fields.
        if (sidPlayerCapability.getSidPlayerProfile() == SidPlayerProfile::custom) {
            XmlHelper::addValueNode(sidNode, SongNodes::frequencyHz, sidPlayerCapability.getFrequencyHz());
            XmlHelper::addValueNode(sidNode, SongNodes::minimumPeriod, sidPlayerCapability.getMinimumPeriod());
            XmlHelper::addValueNode(sidNode, SongNodes::maximumPeriod, sidPlayerCapability.getMaximumPeriod());
        }
    }

    // Encodes the Tracks.
    encodeTracks(subsong, subsongNode);
    encodeSpecialTracks(subsong, subsongNode, true);
    encodeSpecialTracks(subsong, subsongNode, false);

    // Encodes the Positions and the Patterns.
    encodePositions(subsong, subsongNode);
    encodePatterns(subsong, subsongNode);
}

void SongExporter::encodeTracks(const Subsong& subsong, juce::XmlElement& subsongNode) noexcept
{
    auto& tracksNode = XmlHelper::addNode(subsongNode, SongNodes::tracks);

    CellSerializer cellSerializer;

    const auto trackCount = subsong.getTrackCount();
    for (auto trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        const auto& track = subsong.getTrackRefFromIndex(trackIndex);
        const auto cellIndexesAndCells = track.getNonEmptyItems();
        const auto& trackName = track.getName();
        const auto& trackColor = track.getColor();
        // Only non-empty Tracks are encoded, if read-only, or has a name/color.
        if (const auto readOnly = track.isReadOnly(); !cellIndexesAndCells.empty() || readOnly || trackName.isNotEmpty() || trackColor.isPresent()) {
            auto& trackNode = XmlHelper::addNode(tracksNode, SongNodes::track);
            XmlHelper::addValueNode(trackNode, SongNodes::index, trackIndex);
            if (readOnly) {
                XmlHelper::addValueNode(trackNode, SongNodes::isReadOnly, readOnly);
            }
            if (trackName.isNotEmpty()) {
                XmlHelper::addValueNode(trackNode, SongNodes::title, trackName);
            }
            if (trackColor.isPresent()) {
                XmlHelper::addValueNode(trackNode, SongNodes::argbColor, trackColor.getValue());
            }

            // Encodes the non-empty cells.
            for (const auto& [cellIndex, cell] : cellIndexesAndCells) {
                auto cellNode = cellSerializer.serialize(cellIndex, cell);
                jassert(cellNode != nullptr);

                trackNode.addChildElement(cellNode.release());                  // Transfers ownership!
            }
        }
    }
}

void SongExporter::encodeSpecialTracks(const Subsong& subsong, juce::XmlElement& subsongNode, const bool isSpeedTrack) noexcept
{
    auto& tracksNode = XmlHelper::addNode(subsongNode, isSpeedTrack ? SongNodes::speedTracks : SongNodes::eventTracks);

    SpecialCellSerializer specialCellSerializer;

    const auto specialTrackCount = subsong.getSpecialTrackCount(isSpeedTrack);
    for (auto specialTrackIndex = 0; specialTrackIndex < specialTrackCount; ++specialTrackIndex) {
        const auto& specialTrack = subsong.getSpecialTrackRefFromIndex(specialTrackIndex, isSpeedTrack);
        const auto indexesAndSpecialCells = specialTrack.getNonEmptyItems();
        const auto& specialTrackName = specialTrack.getName();
        const auto& specialTrackColor = specialTrack.getColor();
        // Only non-empty Tracks, if read-only, or has a name/color, are encoded.
        if (const auto readOnly = specialTrack.isReadOnly(); !indexesAndSpecialCells.empty() || readOnly || specialTrackName.isNotEmpty()
            || specialTrackColor.isPresent()) {
            auto& trackNode = XmlHelper::addNode(tracksNode, isSpeedTrack ? SongNodes::speedTrack : SongNodes::eventTrack);
            XmlHelper::addValueNode(trackNode, SongNodes::index, specialTrackIndex);
            if (readOnly) {
                XmlHelper::addValueNode(trackNode, SongNodes::isReadOnly, readOnly);
            }
            if (specialTrackName.isNotEmpty()) {
               XmlHelper::addValueNode(trackNode, SongNodes::title, specialTrackName);
            }
            if (specialTrackColor.isPresent()) {
                XmlHelper::addValueNode(trackNode, SongNodes::argbColor, specialTrackColor.getValue());
            }

            // Encodes the non-empty cells.
            for (const auto& [cellIndex, specialCell] : indexesAndSpecialCells) {
                auto cellNode = specialCellSerializer.serialize(cellIndex, specialCell);
                jassert(cellNode != nullptr);

                trackNode.addChildElement(cellNode.release());                  // Transfers ownership!
            }
        }
    }
}

void SongExporter::encodePositions(const Subsong& subsong, juce::XmlElement& subsongNode) noexcept
{
    const auto length = subsong.getLength();

    // Puts all the positions in a Vector.
    std::vector<Position> positions;
    positions.reserve(static_cast<size_t>(length));
    for (auto positionIndex = 0; positionIndex < length; ++positionIndex) {
        positions.push_back(subsong.getPosition(positionIndex));
    }

    // Serializes all the positions.
    auto positionsNode = PositionSerializer::serialize(positions);
    jassert(positionsNode != nullptr);

    subsongNode.addChildElement(positionsNode.release());                  // Transfers ownership!
}

void SongExporter::encodePatterns(const Subsong& subsong, juce::XmlElement& subsongNode) noexcept
{
    const auto patternCount = subsong.getPatternCount();

    // Puts all the positions in a Vector.
    std::vector<Pattern> patterns;
    patterns.reserve(static_cast<size_t>(patternCount));
    for (auto patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
        patterns.push_back(subsong.getPatternFromIndex(patternIndex));
    }

    // Serializes all the positions.
    auto patternsNode = PatternSerializer::serialize(patterns);
    jassert(patternsNode != nullptr);

    subsongNode.addChildElement(patternsNode.release());                  // Transfers ownership!
}

}   // namespace arkostracker
