#include "At3SongImporter.h"

#include "../../business/serialization/instrument/InstrumentNodes.h"
#include "../../business/serialization/instrument/InstrumentSerializer.h"
#include "../../business/serialization/song/CellSerializer.h"
#include "../../business/serialization/song/ExpressionSerializer.h"
#include "../../business/serialization/song/PatternSerializer.h"
#include "../../business/serialization/song/PositionSerializer.h"
#include "../../business/serialization/song/SpecialCellSerializer.h"
#include "../../export/at3/SongNodes.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/XmlHelper.h"

namespace arkostracker 
{

At3SongImporter::At3SongImporter() noexcept :
        songBuilder()
{
}


// SongImporter method implementations.
// =======================================

bool At3SongImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Tries to extract it as an XML document.
    const auto documentText = inputStream.readEntireStreamAsString();
    const auto songNode = parseXML(documentText);
    if (songNode == nullptr) {
        return false;
    }

    // Gets the "format version" in the "song".
    if (songNode->getTagName() != SongNodes::song) {
        return false;
    }
    const auto* formatVersionNode = songNode->getChildByName(SongNodes::formatVersion);
    return ((formatVersionNode != nullptr) && (formatVersionNode->getAllSubText() == "3.0"));
}

ImportedFormat At3SongImporter::getFormat() noexcept
{
    return ImportedFormat::native;
}

std::unique_ptr<SongImporter::Result> At3SongImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*configuration*/) noexcept
{
    songBuilder = std::make_unique<SongBuilder>(true);

    const auto documentText = inputStream.readEntireStreamAsString();
    const auto songNode = parseXML(documentText);
    if (songNode != nullptr) {
        parse(*songNode);
    } else {
        jassertfalse;           // Abnormal, this method is called only when the format matches!
    }

    // Gets the result from the Builder and returns it.
    auto [song, errorReport] = songBuilder->buildSong();

    return std::make_unique<Result>(std::move(song), std::move(errorReport));
}


// =======================================

void At3SongImporter::parse(const juce::XmlElement& songNode) const noexcept
{
    // Extracts the metadata of the Song.
    const auto title = XmlHelper::readString(songNode, SongNodes::title);
    const auto author = XmlHelper::readString(songNode, SongNodes::author);
    const auto composer = XmlHelper::readString(songNode, SongNodes::composer);
    const auto comments = XmlHelper::readString(songNode, SongNodes::comment);
    const auto creationDateMs = static_cast<juce::int64>(XmlHelper::readUnsignedInt(songNode, SongNodes::creationDateMs));
    const auto modificationDateMs = static_cast<juce::int64>(XmlHelper::readUnsignedInt(songNode, SongNodes::modificationDateMs));   // Idem.

    songBuilder->setSongMetaData(title, author, composer, comments, creationDateMs, modificationDateMs);

    // Parses the song proper.
    parseInstruments(songNode);
    parseExpressions(true, songNode);
    parseExpressions(false, songNode);

    parseSubsongs(songNode);
}

void At3SongImporter::parseInstruments(const juce::XmlElement& songNode) const noexcept
{
    const auto* instrumentsNode = songNode.getChildByName(InstrumentNodes::nodeInstrumentsRoot);
    if (instrumentsNode == nullptr) {
        jassertfalse;
        songBuilder->addError("No instruments node found! Abnormal.");
        return;
    }

    // Deserializes.
    auto instruments = InstrumentSerializer::deserializeInstruments(*instrumentsNode);
    if (instruments.empty()) {
        songBuilder->addError("Instruments could not be deserialize!");
        return;
    }

    songBuilder->setInstruments(0, instruments);
}

void At3SongImporter::parseExpressions(const bool isArpeggio, const juce::XmlElement& songNode) const noexcept
{
    const auto* expressionsNode = songNode.getChildByName(isArpeggio ? ExpressionSerializer::nodeArpeggiosRoot : ExpressionSerializer::nodePitchesRoot);
    if (expressionsNode == nullptr) {
        jassertfalse;
        songBuilder->addError("No Expressions node found! Abnormal.");
        return;
    }

    // Deserializes.
    const auto expressions = ExpressionSerializer::deserializeExpressions(*expressionsNode);
    if (expressions.empty()) {
        songBuilder->addError("Expressions could not be deserialize!");
        return;
    }

    songBuilder->setExpressions(isArpeggio, 0, expressions);
}

void At3SongImporter::parseSubsongs(const juce::XmlElement& songNode) const noexcept
{
    const auto* subsongsNode = songNode.getChildByName(SongNodes::subsongs);
    if (subsongsNode == nullptr) {
        jassertfalse;
        songBuilder->addError("No Subsongs node found! Abnormal.");
        return;
    }

    auto subsongIndex = 0;
    for (const auto* subsongNode : XmlHelper::getChildrenList(subsongsNode, SongNodes::subsong)) {
        auto& subsongBuilder = songBuilder->startNewSubsong(subsongIndex);
        parseSubsong(subsongBuilder, *subsongNode);
        songBuilder->finishCurrentSubsong();
        ++subsongIndex;
    }
}

void At3SongImporter::parseSubsong(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept
{
    // Reads the metadata.
    const auto title = XmlHelper::readString(subsongNode, SongNodes::title);
    const auto initialSpeed = XmlHelper::readInt(subsongNode, SongNodes::initialSpeed);
    const auto digiChannel = XmlHelper::readInt(subsongNode, SongNodes::digiChannel);
    const auto highlightSpacing = XmlHelper::readInt(subsongNode, SongNodes::highlightSpacing);
    const auto secondaryHighlight = XmlHelper::readInt(subsongNode, SongNodes::secondaryHighlight);
    const auto loopStartPosition = XmlHelper::readInt(subsongNode, SongNodes::loopStartPosition);
    const auto endPosition = XmlHelper::readInt(subsongNode, SongNodes::endPosition);
    const auto replayFrequencyHz = XmlHelper::readFloat(subsongNode, SongNodes::replayFrequencyHz);

    subsongBuilder.setMetadata(title, initialSpeed, replayFrequencyHz, digiChannel, highlightSpacing, secondaryHighlight);
    subsongBuilder.setLoop(loopStartPosition, endPosition);

    // Reads the PSGs.
    const auto* psgsNode = subsongNode.getChildByName(SongNodes::psgs);
    if (psgsNode == nullptr) {
        songBuilder->addError("No psgs node found! Abnormal.");
        return;
    }
    auto psgIndex = 0;
    for (const auto* psgNode : XmlHelper::getChildrenList(psgsNode, SongNodes::psg)) {
        const auto typeString = XmlHelper::readString(*psgNode, SongNodes::type);
        auto type = PsgTypeUtil::serializationTextToPsgType(typeString);
        if (type.isAbsent()) {
            type = PsgType::ay;
            songBuilder->addWarning("Psg index " + juce::String(psgIndex) + " has no valid PSG type: " + typeString);
        }
        auto psgFrequencyHz = XmlHelper::readInt(*psgNode, SongNodes::frequencyHz, -1);
        if ((psgFrequencyHz < PsgFrequency::minimumPsgFrequency) || (psgFrequencyHz > PsgFrequency::maximumPsgFrequency)) {
            psgFrequencyHz = PsgFrequency::psgFrequencyCPC;
            songBuilder->addWarning("PSG frequency is invalid or absent.");
        }
        auto referenceFrequencyHz = XmlHelper::readFloat(*psgNode, SongNodes::referenceFrequencyHz, -1.0F);
        if ((referenceFrequencyHz < PsgFrequency::minimumReferenceFrequency) || (referenceFrequencyHz > PsgFrequency::maximumReferenceFrequency)) {
            referenceFrequencyHz = PsgFrequency::defaultReferenceFrequencyHz;
            songBuilder->addWarning("Reference frequency is invalid or absent.");
        }
        auto samplePlayerFrequencyHz = XmlHelper::readInt(*psgNode, SongNodes::samplePlayerFrequencyHz);
        if ((samplePlayerFrequencyHz < PsgFrequency::minimumSamplePlayerFrequency) || (samplePlayerFrequencyHz > PsgFrequency::maximumSamplePlayerFrequency)) {
            samplePlayerFrequencyHz = PsgFrequency::defaultSamplePlayerFrequencyHz;
            songBuilder->addWarning("Sample player frequency is invalid or absent.");
        }
        const auto mixingOutputString = XmlHelper::readString(*psgNode, SongNodes::mixingOutput);
        auto mixingOutput = PsgMixingOutputUtil::serializationTextToPsgMixingOutput(mixingOutputString);
        if (mixingOutput.isAbsent()) {
            mixingOutput = PsgMixingOutput::ABC;
            songBuilder->addWarning("Mixing output for PSG index " + juce::String(psgIndex) + " has no valid type: " + mixingOutputString);
        }
        subsongBuilder.addPsg(Psg(type.getValue(), psgFrequencyHz, referenceFrequencyHz,
            samplePlayerFrequencyHz, mixingOutput.getValue()));

        ++psgIndex;
    }

    // Reads the SID player capability. It may not exist.
    if (const auto* sidPlayerCapabilityNode = subsongNode.getChildByName(SongNodes::sidPlayerCapability); sidPlayerCapabilityNode != nullptr) {
        const auto typeString = XmlHelper::readString(*sidPlayerCapabilityNode, SongNodes::profile);
        const auto profile = SidPlayerProfileUtil::serializationTextToSidPlayerProfileTo(typeString);
        auto sidPlayerCapability = SidPlayerCapability::buildDefault();
        if (profile.isAbsent()) {
            songBuilder->addWarning("SID player capability profile could not be read. Fallback on default.");
        } else {
            switch (profile.getValue()) {
                case SidPlayerProfile::cpc:
                    sidPlayerCapability = SidPlayerCapability::buildForCpc();
                    break;
                case SidPlayerProfile::amstradPlus:
                    sidPlayerCapability = SidPlayerCapability::buildForAmstradPlus();
                    break;
                case SidPlayerProfile::atariSt:
                    sidPlayerCapability = SidPlayerCapability::buildForAtariSt();
                    break;
                case SidPlayerProfile::custom:
                    const auto frequencyHz = NumberUtil::correctNumber(
                        XmlHelper::readInt(*sidPlayerCapabilityNode, SongNodes::frequencyHz, PsgValues::defaultSidPlayerFrequencyHz),
                        PsgValues::sidPlayerMinimumFrequencyHz, PsgValues::sidPlayerMaximumFrequencyHz);
                    const auto minimumPeriod = NumberUtil::correctNumber(
                        XmlHelper::readInt(*sidPlayerCapabilityNode, SongNodes::minimumPeriod, PsgValues::sidMinimumPeriod),
                        PsgValues::sidMinimumPeriod, PsgValues::sidMaximumPeriod);
                    const auto maximumPeriod = NumberUtil::correctNumber(
                        XmlHelper::readInt(*sidPlayerCapabilityNode, SongNodes::maximumPeriod, PsgValues::sidMaximumPeriod),
                        minimumPeriod, PsgValues::sidMaximumPeriod);    // Capped to the minimum read before.
                    sidPlayerCapability = SidPlayerCapability::buildForCustom(frequencyHz, minimumPeriod, maximumPeriod);
                    break;
            }
            subsongBuilder.setSidPlayerCapability(sidPlayerCapability);
        }
    }

    // Reads the tracks, special Tracks, patterns and positions.
    parseTracks(subsongBuilder, subsongNode);
    parseSpecialTracks(true, subsongBuilder, subsongNode);
    parseSpecialTracks(false, subsongBuilder, subsongNode);
    parsePositions(subsongBuilder, subsongNode);
    parsePatterns(subsongBuilder, subsongNode);
}

void At3SongImporter::parseTracks(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept
{
    const auto* tracksNode = subsongNode.getChildByName(SongNodes::tracks);
    if (tracksNode == nullptr) {
        songBuilder->addError("No tracks node found! Abnormal.");
        return;
    }
    CellSerializer cellSerializer;

    // Empty Tracks are not encoded.
    for (const auto* trackNode : XmlHelper::getChildrenList(tracksNode, SongNodes::track)) {
        const auto trackIndex = XmlHelper::readInt(*trackNode, SongNodes::index, -1);
        if (trackIndex < 0) {
            songBuilder->addError("Track index is illegal or not found.");
            return;
        }

        for (const auto* cellNode : XmlHelper::getChildrenList(trackNode, CellSerializer::nodeCell)) {
            const auto [cell, cellIndex] = cellSerializer.deserialize(*cellNode);
            if (cellIndex < 0) {
                jassertfalse;
                songBuilder->addError("Illegal cell index, or cell index not given.");
            }

            subsongBuilder.setCellOnTrack(trackIndex, cellIndex, cell);

        }

        // Done after to avoid read-only assertions.
        const auto isReadOnly = XmlHelper::readBool(*trackNode, SongNodes::isReadOnly, false);
        const auto name = XmlHelper::readString(*trackNode, SongNodes::title);
        const auto argbColor = XmlHelper::readUnsignedInt(*trackNode, SongNodes::argbColor, 0);
        subsongBuilder.setTrackMetadata(trackIndex, isReadOnly, name, argbColor);
    }
}

void At3SongImporter::parseSpecialTracks(const bool isSpeedTrack, SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept
{
    const auto* specialTracksNode = subsongNode.getChildByName(isSpeedTrack ? SongNodes::speedTracks : SongNodes::eventTracks);
    const auto specialTrackNodeName = isSpeedTrack ? SongNodes::speedTrack : SongNodes::eventTrack;
    jassert(specialTracksNode != nullptr);

    SpecialCellSerializer specialCellSerializer;

    // Empty SpecialTracks are not encoded.
    for (const auto* specialTrackNode : XmlHelper::getChildrenList(specialTracksNode, specialTrackNodeName)) {
        const auto specialTrackIndex = XmlHelper::readInt(*specialTrackNode, SongNodes::index, -1);
        if (specialTrackIndex < 0) {
            songBuilder->addError("Special Track index is illegal or not found.");
            return;
        }

        for (const auto* specialCellNode : XmlHelper::getChildrenList(specialTrackNode, SpecialCellSerializer::nodeSpecialCell)) {
            const auto [specialCell, cellIndex] = specialCellSerializer.deserialize(*specialCellNode);
            if (cellIndex < 0) {
                jassertfalse;
                songBuilder->addError("Illegal special cell index, or special cell index not given.");
            }

            subsongBuilder.setSpecialCellOnTrack(isSpeedTrack, specialTrackIndex, cellIndex, specialCell);
        }

        // Done after to avoid read-only assertions.
        const auto isReadOnly = XmlHelper::readBool(*specialTrackNode, SongNodes::isReadOnly, false);
        const auto name = XmlHelper::readString(*specialTrackNode, SongNodes::title);
        const auto argbColor = XmlHelper::readUnsignedInt(*specialTrackNode, SongNodes::argbColor, 0);
        subsongBuilder.setSpecialTrackMetadata(isSpeedTrack, specialTrackIndex, isReadOnly, name, argbColor);
    }
}

void At3SongImporter::parsePositions(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept
{
    const auto* positionsNode = subsongNode.getChildByName(SongNodes::positions);
    if (positionsNode == nullptr) {
        jassertfalse;
        songBuilder->addError("No positions encoded, abnormal.");
        return;
    }

    const auto positions = PositionSerializer::deserializePositions(*positionsNode);

    subsongBuilder.addPositions(positions);
}

void At3SongImporter::parsePatterns(SubsongBuilder& subsongBuilder, const juce::XmlElement& subsongNode) const noexcept
{
    const auto* patternsNode = subsongNode.getChildByName(SongNodes::patterns);
    if (patternsNode == nullptr) {
        jassertfalse;
        songBuilder->addError("No patterns encoded, abnormal.");
        return;
    }

    const auto patterns = PatternSerializer::deserializePatterns(*patternsNode);

    subsongBuilder.addPatterns(patterns);
}

}   // namespace arkostracker
