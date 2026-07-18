#include "SubsongReader.h"

#include "../../business/song/tool/builder/SubsongBuilder.h"
#include "../../utils/XmlHelper.h"

namespace arkostracker 
{

SubsongReader::SubsongReader(SubsongBuilder& pSubsongBuilder) noexcept :
        subsongBuilder(pSubsongBuilder)
{
}

void SubsongReader::parse(const juce::XmlElement& subsongNode) noexcept
{
    // Parses the metadata.
    const auto title = XmlHelper::readString(subsongNode, "aks:title");
    const auto initialSpeed = XmlHelper::readInt(subsongNode, "aks:initialSpeed");
    const auto endIndex = XmlHelper::readInt(subsongNode, "aks:endIndex");
    const auto loopStartIndex = XmlHelper::readInt(subsongNode, "aks:loopStartIndex");
    const auto replayFrequency = XmlHelper::readFloat(subsongNode, "aks:replayFrequency");
    const auto digiChannel = XmlHelper::readInt(subsongNode, "aks:digiChannel");
    const auto highlightSpacing = XmlHelper::readInt(subsongNode, "aks:highlightSpacing");
    const auto secondaryHighlight = XmlHelper::readInt(subsongNode, "aks:secondaryHighlight");

    subsongBuilder.setMetadata(title, initialSpeed, replayFrequency, digiChannel, highlightSpacing, secondaryHighlight);
    subsongBuilder.setLoop(loopStartIndex, endIndex);

    // Reads the PSGs.
    for (const auto* psgMetadataNode : XmlHelper::getChildrenList(subsongNode, "aks:psgMetadata")) {
        const auto psgTypeString = XmlHelper::readString(*psgMetadataNode, "aks:type");
        auto psgType = PsgTypeUtil::serializationTextToPsgType(psgTypeString);
        if (psgType.isAbsent()) {
            psgType = PsgType::ay;
            subsongBuilder.addWarning("Psg type is invalid: " + psgTypeString);
        }
        const auto psgFrequency = XmlHelper::readInt(*psgMetadataNode, "aks:psgFrequency", PsgFrequency::psgFrequencyCPC);
        const auto referenceFrequency = XmlHelper::readFloat(*psgMetadataNode, "aks:referenceFrequency", PsgFrequency::defaultReferenceFrequencyHz);
        const auto samplePlayerFrequency = XmlHelper::readInt(*psgMetadataNode, "aks:samplePlayerFrequency", PsgFrequency::defaultSamplePlayerFrequencyHz);
        const auto mixingOutputString = XmlHelper::readString(*psgMetadataNode, "aks:mixingOutput");
        auto mixingOutput = PsgMixingOutputUtil::serializationTextToPsgMixingOutput(mixingOutputString);
        if (mixingOutput.isAbsent()) {
            mixingOutput = PsgMixingOutput::ABC;
        }

        const Psg psg(psgType.getValue(), psgFrequency, referenceFrequency, samplePlayerFrequency, mixingOutput.getValue());
        subsongBuilder.addPsg(psg);
    }

    parseTracks(subsongNode);
    parseSpecialTracks(true, subsongNode, "aks:speedTracks", "aks:speedTrack", "aks:speedCell");
    parseSpecialTracks(false, subsongNode, "aks:eventTracks", "aks:eventTrack", "aks:eventCell");
    parseLinker(subsongNode);
}

void SubsongReader::parseTracks(const juce::XmlElement& subsongNode) noexcept
{
    const auto* tracksNode = subsongNode.getChildByName("aks:tracks");
    for (const auto* trackNode : XmlHelper::getChildrenList(tracksNode, "aks:track")) {
        const auto trackIndex = XmlHelper::readInt(*trackNode, "aks:number", -1);
        if (trackIndex < 0) {
            subsongBuilder.addError("Track has an invalid or not present index.");
            return;
        }

        subsongBuilder.startNewTrack(trackIndex);

        // Reads the cells.
        for (const auto* cellNode : XmlHelper::getChildrenList(trackNode, "aks:cell")) {
            const auto cellIndex = XmlHelper::readInt(*cellNode, "aks:index");
            const auto note = XmlHelper::readInt(*cellNode, "aks:note", -1);                   // May be absent.
            const auto instrument = XmlHelper::readInt(*cellNode, "aks:instrument", -1);       // May be absent.
            const auto isNote = (note >= 0);
            const auto isInstrument = (instrument >= 0);

            subsongBuilder.startNewCell(cellIndex);
            if (isNote) {
                subsongBuilder.setCurrentCellNote(note);
            }
            if (isInstrument) {
                subsongBuilder.setCurrentCellInstrument(instrument);
            }
            // Any effect?
            for (const auto* effectAndValueNode : XmlHelper::getChildrenList(cellNode, "aks:effectAndValue")) {
                const auto effectIndex = XmlHelper::readInt(*effectAndValueNode, "aks:index");
                const auto effectString = XmlHelper::readString(*effectAndValueNode, "aks:effect");
                const auto effectValueHexString = XmlHelper::readString(*effectAndValueNode, "aks:hexValue");
                const auto effectOptional = convertEffectStringToEffect(effectString);
                const auto rawValueOptional = extractIntFromHexString(effectValueHexString);
                if (effectOptional.isAbsent() || (rawValueOptional.isAbsent())) {
                    subsongBuilder.addWarning("Unable to parse an effect.");
                } else {
                    // Special case: if glide+note+instrument, removes the instrument to prevent an error in the PV.
                    if (isNote && isInstrument && effectOptional.isPresent() && (effectOptional.getValue() == Effect::pitchGlide)) {
                        subsongBuilder.removeCurrentCellInstrument();
                    }

                    subsongBuilder.setCurrentCellEffectRawValue(effectIndex, effectOptional.getValue(), rawValueOptional.getValue());
                }
            }

            subsongBuilder.finishCurrentCell();
        }
        subsongBuilder.finishCurrentTrack();
    }
}

OptionalValue<Effect> SubsongReader::convertEffectStringToEffect(const juce::String& effectString) noexcept
{
    static const std::map<juce::String, Effect> effectStringToEffect = {
            { "arpeggio3Notes",       Effect::arpeggio3Notes },
            { "arpeggio4Notes",       Effect::arpeggio4Notes },
            { "arpeggioTable",        Effect::arpeggioTable },
            { "forceArpeggioSpeed",   Effect::forceArpeggioSpeed },
            { "forceInstrumentSpeed", Effect::forceInstrumentSpeed },
            { "forcePitchTableSpeed", Effect::forcePitchTableSpeed },
            { "pitchDown",            Effect::pitchDown },
            { "pitchUp",              Effect::pitchUp },
            { "fastPitchDown",        Effect::fastPitchDown },
            { "fastPitchUp",          Effect::fastPitchUp },
            { "pitchGlide",           Effect::pitchGlide },
            { "pitchTable",           Effect::pitchTable },
            { "reset",                Effect::reset },
            { "volume",               Effect::volume },
            { "volumeIn",             Effect::volumeIn },
            { "volumeOut",            Effect::volumeOut },
    };

    // Try to get the effect matching the given text.
    auto iterator = effectStringToEffect.find(effectString);
    if (iterator == effectStringToEffect.cend()) {
        return { };
    }
    return iterator->second;
}

OptionalValue<int> SubsongReader::extractIntFromHexString(const juce::String& effectValueHexString) noexcept {
    // The strings must start with a "#", which must be skipped.
    // The test is performed here, because the Builder manages an int, not a String.
    if (!effectValueHexString.startsWithChar('#')) {
        return { };
    }

    return effectValueHexString.substring(1).getHexValue32();        // Skips the #.
}

void SubsongReader::parseSpecialTracks(bool isSpeedTrack, const juce::XmlElement& subsongNode, const juce::String& collectionTag, const juce::String& itemTag,
                                       const juce::String& cellTag) noexcept
{
    const auto* specialTracksNode = subsongNode.getChildByName(collectionTag);
    for (const auto* specialTrackNode : XmlHelper::getChildrenList(specialTracksNode, itemTag)) {
        const auto specialTrackIndex = XmlHelper::readInt(*specialTrackNode, "aks:number", -1);
        if (specialTrackIndex < 0) {
            subsongBuilder.addError("Special Track has an invalid or not present index.");
            return;
        }

        subsongBuilder.startNewSpecialTrack(isSpeedTrack, specialTrackIndex);
        // Parses the cells.
        for (const auto* cellNode : XmlHelper::getChildrenList(specialTrackNode, cellTag)) {
            const auto cellIndex = XmlHelper::readInt(*cellNode, "aks:index");
            const auto cellValue = XmlHelper::readInt(*cellNode, "aks:value");
            subsongBuilder.setCurrentSpecialTrackValue(isSpeedTrack, cellIndex, cellValue);
        }
        subsongBuilder.finishCurrentSpecialTrack(isSpeedTrack);
    }
}

void SubsongReader::parseLinker(const juce::XmlElement& subsongNode) noexcept
{
    // Parses the Patterns. They are "converted" to Positions on the fly.
    const auto* patternsNode = subsongNode.getChildByName("aks:patterns");
    for (const auto* patternNode : XmlHelper::getChildrenList(patternsNode, "aks:pattern")) {
        const auto height = XmlHelper::readInt(*patternNode, "aks:height");
        const auto speedTrackIndex = XmlHelper::readInt(*patternNode, "aks:speedTrackNumber");
        const auto eventTrackIndex = XmlHelper::readInt(*patternNode, "aks:eventTrackNumber");

        // Parses the tracks of each channel of the Pattern.
        std::vector<int> trackIndexes;
        std::unordered_map<int, int> channelToTrackIndexes;
        std::unordered_map<int, int> channelToTransposition;
        auto channelIndex = 0;
        for (const auto* patternCellNode : XmlHelper::getChildrenList(patternNode, "aks:patternCell")) {
            const auto trackIndex = XmlHelper::readInt(*patternCellNode, "aks:trackNumber");
            const auto transposition = XmlHelper::readInt(*patternCellNode, "aks:transposition");
            trackIndexes.emplace_back(trackIndex);
            channelToTrackIndexes.insert(std::make_pair(channelIndex, trackIndex));

            channelToTransposition.insert(std::make_pair(channelIndex, transposition));

            ++channelIndex;
        }
        // Is this Pattern new or old? If new, encodes it.
        const Pattern fakePattern(trackIndexes, speedTrackIndex, eventTrackIndex);
        const auto result = subsongBuilder.checkRawOriginalPatternAndStoreIfDifferent(fakePattern);
        const auto patternIndex = result.second;
        if (result.first) {
            // Encodes the Pattern.
            subsongBuilder.startNewPattern(patternIndex);
            subsongBuilder.setCurrentPatternSpeedTrackIndex(speedTrackIndex);
            subsongBuilder.setCurrentPatternEventTrackIndex(eventTrackIndex);
            for (const auto& entry : channelToTrackIndexes) {
                subsongBuilder.setCurrentPatternTrackIndex(entry.first, entry.second);
            }
            subsongBuilder.finishCurrentPattern();
        }

        // Encodes the Position.
        subsongBuilder.startNewPosition();
        subsongBuilder.setCurrentPositionHeight(height);
        subsongBuilder.setCurrentPositionPatternIndex(patternIndex);
        for (const auto& entry : channelToTransposition) {
            subsongBuilder.setCurrentPositionTransposition(entry.first, entry.second);
        }
        subsongBuilder.finishCurrentPosition();
    }
}


}   // namespace arkostracker

