#include "WyzSongImporter.h"

#include <algorithm>

#include "../../song/cells/CellConstants.h"
#include "../../song/subsong/SubsongConstants.h"
#include "../../utils/Base64Util.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../../utils/StreamUtil.h"
#include "../../utils/StringUtil.h"
#include "../../utils/XmlHelper.h"

namespace arkostracker 
{

const int WyzSongImporter::wyzRstNoteValue = 'P';
const int WyzSongImporter::fxCellNote = (4 * 12) + 0;             // Arbitrary.

WyzSongImporter::WyzSongImporter() :
        songBuilder(),
        highestInstrumentIndex(),
        fxCells(),
        currentInstrumentPerChannel()
{
}


// SongImporter method implementations.
// =======================================

bool WyzSongImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Simply looks for the tag "<MutedChannels>" at the beginning. No need to go further!
    return StreamUtil::findStringInStream(inputStream, 256, "<MutedChannels>");
}

ImportedFormat WyzSongImporter::getFormat() noexcept
{
    return ImportedFormat::wyz;
}

std::unique_ptr<SongImporter::Result> WyzSongImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*importConfiguration*/) noexcept
{
    songBuilder = std::make_unique<SongBuilder>();

    const auto documentText = inputStream.readEntireStreamAsString();
    const auto songNode = juce::parseXML(documentText);
    if (songNode != nullptr) {
        parse(*songNode);
    } else {
        jassertfalse;           // Abnormal, this method is called only when the format matches!
    }

    // Gets the result from the Builder and returns it.
    auto songAndReport = songBuilder->buildSong();

    return std::make_unique<SongImporter::Result>(std::move(songAndReport.first), std::move(songAndReport.second));
}

// =======================================

void WyzSongImporter::parse(const juce::XmlElement& songNode) noexcept
{
    const auto songTitle = XmlHelper::readString(songNode, "Name");
    songBuilder->setSongMetaData(songTitle, juce::String(), juce::String(), "Converted by Arkos Tracker 3");

    auto& subsongBuilder = songBuilder->startNewSubsong(0);

    const auto initialSpeed = XmlHelper::readInt(songNode, "Tempo", SubsongConstants::defaultSpeed);
    const auto psgFrequency = XmlHelper::readInt(songNode, "ChipFrequency", PsgFrequency::psgFrequencySpectrum);
    const auto fxChannel = XmlHelper::readInt(songNode, "FxChannel", 0);
    subsongBuilder.addPsg(Psg(PsgType::ay, psgFrequency));
    subsongBuilder.setMetadata(SubsongConstants::defaultFirstSubsongName, initialSpeed);

    parsePatternsAndPositions(songNode);
    parseInstruments(songNode);
    parseFxs(songNode);

    encodeFxs(fxChannel);

    songBuilder->finishCurrentSubsong();
}

void WyzSongImporter::parsePatternsAndPositions(const juce::XmlElement& songNode) noexcept
{
    auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();

    std::unordered_map<int, int> patternIndexToHeight;

    // Reads the Patterns (which consists of tracks).
    auto patternIndex = 0;
    for (const auto* patternNode : XmlHelper::getChildrenList(songNode.getChildByName("Patterns"), "Pattern")) {
        const auto patternHeight = XmlHelper::readInt(*patternNode, "Length", TrackConstants::defaultPositionHeight);
        patternIndexToHeight.insert({ patternIndex, patternHeight });

        // Generates the Pattern.
        subsongBuilder.startNewPattern(patternIndex);
        const auto baseTrackIndex = patternIndex * 3;
        subsongBuilder.setCurrentPatternTrackIndex(0, baseTrackIndex + 0);
        subsongBuilder.setCurrentPatternTrackIndex(1, baseTrackIndex + 1);
        subsongBuilder.setCurrentPatternTrackIndex(2, baseTrackIndex + 2);
        subsongBuilder.setCurrentPatternSpeedTrackIndex(patternIndex);
        subsongBuilder.setCurrentPatternEventTrackIndex(patternIndex);
        subsongBuilder.finishCurrentPattern();

        currentInstrumentPerChannel.clear();
        currentInstrumentPerChannel.resize(3, 0);

        // Reads all the lines of 3 channels (one line at a time).
        auto cellIndex = 0;
        for (const auto* patternLine : XmlHelper::getChildrenList(patternNode->getChildByName("Lines"), "ChannelLine")) {
            auto channelIndex = 0;
            for (const auto* cellNode : XmlHelper::getChildrenList(patternLine->getChildByName("Notes"), "ChannelNote")) {
                const auto trackIndex = baseTrackIndex + channelIndex;
                parseCell(trackIndex, channelIndex, cellIndex, *cellNode);
                ++channelIndex;
            }

            // Any FX? Stores it for later.
            const auto readFx = XmlHelper::readInt(*patternLine, "Fx", -1);
            if (readFx >= 0) {
                fxCells.emplace_back(readFx, baseTrackIndex, cellIndex);
            }

            ++cellIndex;
        }

        ++patternIndex;
    }

    // Reads the positions.
    auto position = 0;
    for (const auto* positionNode : XmlHelper::getChildrenList(songNode.getChildByName("PlayOrder"), "int")) {
        patternIndex = XmlHelper::readInt(*positionNode, 0);

        subsongBuilder.startNewPosition();
        subsongBuilder.setCurrentPositionPatternIndex(patternIndex);
        // What height?
        if (auto it = patternIndexToHeight.find(patternIndex); it != patternIndexToHeight.cend()) {
            subsongBuilder.setCurrentPositionHeight(it->second);
        }
        subsongBuilder.finishCurrentPosition();

        ++position;
    }

    const auto loopStartIndex = XmlHelper::readInt(songNode, "LoopToPattern", 0);
    subsongBuilder.setLoop(loopStartIndex, position - 1);
}

void WyzSongImporter::parseCell(int trackIndex, int channelIndex, int cellIndex, const juce::XmlElement& cellNode)
{
    if (channelIndex >= 3) {
        jassertfalse;           // More than 3 channels par line?
        return;
    }

    const auto octave = XmlHelper::readInt(cellNode, "Octave");
    const auto noteChar = XmlHelper::readInt(cellNode, "Note");
    const auto semiNoteChar = XmlHelper::readInt(cellNode, "Seminote");
    const auto readInstrument = XmlHelper::readInt(cellNode, "Instrument", -1);

    OptionalValue<Note> note;
    auto instrument = (readInstrument >= 0) ? (readInstrument + 1) : OptionalInt();      // Increases the instrument, as the 0th is reserved in AT2.
    // Stores the instrument, if present).
    if (instrument.isPresent()) {
        currentInstrumentPerChannel[static_cast<size_t>(channelIndex)] = instrument.getValue();
    } else {
        // The instrument is absent: use a previously stored one.
        instrument = currentInstrumentPerChannel.at(static_cast<size_t>(channelIndex));
    }

    if (octave >= 0) {          // -2147483648 means no note.
        // Converts the "note" to a key. It is actually an ascii character!
        // Special case: a "P" (RST).
        if (noteChar == wyzRstNoteValue) {
            // Encodes a RST.
            instrument = CellConstants::rstInstrument;
            note = Note::buildRst();
        } else {
            int keyInOctave;    // NOLINT(*-init-variables)
            switch (noteChar) {
                case 'C':
                default:
                    keyInOctave = 0;
                    break;
                case 'D':
                    keyInOctave = 2;
                    break;
                case 'E':
                    keyInOctave = 4;
                    break;
                case 'F':
                    keyInOctave = 5;
                    break;
                case 'G':
                    keyInOctave = 7;
                    break;
                case 'A':
                    keyInOctave = 9;
                    break;
                case 'B':
                    keyInOctave = 11;
                    break;
            }

            // Adds a semi-note, if any.
            if (semiNoteChar != 0) {
                ++keyInOctave;
            }

            // We can now determine the note.
            note = Note::buildNote((octave * 12) + keyInOctave);
        }
    }

    // Writes the note, if present.
    if (note.isPresent()) {
        auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();

        const Cell cell(note, instrument);
        subsongBuilder.setCellOnTrack(trackIndex, cellIndex, cell);
    }
}

void WyzSongImporter::parseInstruments(const juce::XmlElement& songNode) noexcept
{
    for (const auto* instrumentNode : XmlHelper::getChildrenList(songNode.getChildByName("Instruments"), "Instrument")) {
        // The "ID" can be "R" (reserved), in which case it is ignored.
        const auto idString = XmlHelper::readString(*instrumentNode, "ID");
        bool success;    // NOLINT(*-init-variables)
        const auto instrumentId = StringUtil::stringToInt(idString, success);
        if (!success) {
            continue;
        }
        const auto title = XmlHelper::readString(*instrumentNode, "Name");
        const auto isLooping = XmlHelper::readBool(*instrumentNode, "Looped");
        const auto loopStartIndex = XmlHelper::readInt(*instrumentNode, "LoopStart");
        // All the Instrument is encoded within this "volumes" node, but it contains more than just volumes.
        const auto base64Volumes = XmlHelper::readString(*instrumentNode, "Volumes");
        std::vector<int> arpeggios;
        for (const auto* arpeggioNode : XmlHelper::getChildrenList(instrumentNode->getChildByName("PitchModifiers"), "int")) {
            const auto pitch = XmlHelper::readInt(*arpeggioNode, 0);
            arpeggios.emplace_back(pitch);
        }
        const auto values = Base64Util::decodeBase64StringToVector(base64Volumes, success);
        if (!success) {
            songBuilder->addError("Unable to decode the Base64 volume for instrument ID " + idString);
            continue;
        }
        const auto length = static_cast<int>(values.size());
        const auto endIndex = length - 1;

        const auto instrumentIndex = instrumentId + 1;                  // 0 is reserved in AT.
        highestInstrumentIndex = std::max(highestInstrumentIndex, instrumentIndex);
        songBuilder->startNewPsgInstrument(instrumentIndex);
        songBuilder->setCurrentInstrumentName(title);
        songBuilder->setCurrentPsgInstrumentMainLoop(loopStartIndex, endIndex, isLooping);

        auto currentOctaveShift = 0;

        // Extracts its data.
        auto cellIndex = 0;
        for (const auto rawData : values) {
            songBuilder->startNewPsgInstrumentCell();

            const auto volume = static_cast<int>(rawData & 0b1111U);
            // These bits are normally exclusive.
            const auto isArpeggio = ((rawData & 0b10000U) != 0);           // Bit 4: arpeggio.
            const auto isOctaveMinus1 = ((rawData & 0b100000U) != 0);      // Bit 5: octave-1.
            const auto isOctavePlus1 = ((rawData & 0b1000000U) != 0);      // Bit 6: octave+1.

            // Reads the Arpeggio value (which may not exist).
            const auto pitchOrArpeggio = (cellIndex < static_cast<int>(arpeggios.size())) ? arpeggios.at(static_cast<size_t>(cellIndex)) : 0;
            auto arpeggio = 0;
            // Encodes the pitch, or prepare the arpeggio (encoded below).
            if (isArpeggio) {
                arpeggio = pitchOrArpeggio;
            } else {
                songBuilder->setCurrentPsgInstrumentCellPrimaryPitch(pitchOrArpeggio);

                // Encodes the possible octave change (as there is no arpeggio).
                // Important: also stores this because the octave is "trailing" within a WYZ Instrument.
                if (isOctaveMinus1) {
                    --currentOctaveShift;
                } else if (isOctavePlus1) {
                    ++currentOctaveShift;
                }
            }

            // What is the final arpeggio? Warning, it can overflow!
            const auto octaveArpeggio = currentOctaveShift * 12;
            const auto finalArpeggio = NumberUtil::correctNumber(arpeggio + octaveArpeggio, PsgValues::minimumArpeggio, PsgValues::maximumArpeggio);

            songBuilder->setCurrentPsgInstrumentCellPrimaryArpeggio(finalArpeggio);

            songBuilder->setCurrentPsgInstrumentCellLink(PsgInstrumentCellLink::softOnly);
            songBuilder->setCurrentPsgInstrumentCellVolume(volume);

            songBuilder->finishCurrentPsgInstrumentCell();
            ++cellIndex;
        }

        songBuilder->finishCurrentInstrument();
    }
}

void WyzSongImporter::parseFxs(const juce::XmlElement& songNode) const noexcept
{
    // Puts the FXs at the end.
    const auto firstEffectIndex = highestInstrumentIndex + 1;

    for (const auto* effectNode : XmlHelper::getChildrenList(songNode.getChildByName("Effects"), "Effect")) {
        const auto effectId = XmlHelper::readInt(*effectNode, "ID");
        const auto title = XmlHelper::readString(*effectNode, "Name");
        const auto base64Volumes = XmlHelper::readString(*effectNode, "Volumes");
        const auto base64Noises = XmlHelper::readString(*effectNode, "Noises");
        const auto base64HardwareEnvelopes = XmlHelper::readString(*effectNode, "EnvTypes");

        const auto effectIndex = effectId + firstEffectIndex;

        std::vector<int> softwareFrequencies;
        for (const auto* frequencyNode : XmlHelper::getChildrenList(effectNode->getChildByName("Frequencies"), "int")) {
            const auto frequency = XmlHelper::readInt(*frequencyNode, 0);
            softwareFrequencies.emplace_back(frequency);
        }
        std::vector<int> hardwareFrequencies;
        for (const auto* frequencyNode : XmlHelper::getChildrenList(effectNode->getChildByName("EnvFreqs"), "int")) {
            const auto frequency = XmlHelper::readInt(*frequencyNode, 0);
            hardwareFrequencies.emplace_back(frequency);
        }

        bool success; // NOLINT(*-init-variables)
        const auto rawVolumes = Base64Util::decodeBase64StringToVector(base64Volumes, success);
        if (!success) {
            songBuilder->addError("Unable to extract volumes for Effect " + juce::String(effectId));
            continue;
        }
        const auto rawNoises = Base64Util::decodeBase64StringToVector(base64Noises, success);
        if (!success) {
            songBuilder->addError("Unable to extract noises for Effect " + juce::String(effectId));
            continue;
        }
        const auto rawHardwareEnvelopes = Base64Util::decodeBase64StringToVector(base64HardwareEnvelopes, success);
        if (!success) {
            songBuilder->addError("Unable to extract hardware envelopes for Effect " + juce::String(effectId));
            continue;
        }

        // They should have the same length! It should also be non-zero.
        const auto length = rawVolumes.size();
        if (rawNoises.size() != length) {
            songBuilder->addError("The volumes and noises length for Effect " + juce::String(effectId) + " have a different count!");
            continue;
        }

        songBuilder->startNewPsgInstrument(effectIndex);
        songBuilder->setCurrentInstrumentName("Fx: " + title);
        songBuilder->setCurrentInstrumentColor(0xff8b0000);     // juce::Colours::darkred

        for (size_t lineIndex = 0; lineIndex < length; ++lineIndex) {
            songBuilder->startNewPsgInstrumentCell();

            // Decodes the data. The hardware flag is encoded within the noise.
            const auto volume = rawVolumes.at(lineIndex);
            songBuilder->setCurrentPsgInstrumentCellVolume(volume);

            const auto rawNoise = rawNoises.at(lineIndex);
            const auto noise = static_cast<int>(rawNoise & 0b11111U);
            songBuilder->setCurrentPsgInstrumentCellNoise(noise);

            const auto isHardware = ((rawNoise & 0b10000000U) != 0);   // Bit 7 is 'hardware period?'.

            // Is there a software and/or a hardware frequency?
            auto isSoftware = false;
            if (lineIndex < softwareFrequencies.size()) {
                isSoftware = true;
                const auto softwarePeriod = softwareFrequencies[lineIndex];
                songBuilder->setCurrentPsgInstrumentCellPrimaryPeriod(softwarePeriod);
            }

            if (isHardware && (lineIndex < hardwareFrequencies.size())) {
                const auto hardwarePeriod = hardwareFrequencies[lineIndex];
                songBuilder->setCurrentPsgInstrumentCellSecondaryPeriod(hardwarePeriod);
            }

            PsgInstrumentCellLink link;     // NOLINT(*-init-variables)
            if (isSoftware) {
                link = isHardware ? PsgInstrumentCellLink::softAndHard : PsgInstrumentCellLink::softOnly;
            } else {
                link = isHardware ? PsgInstrumentCellLink::hardOnly : PsgInstrumentCellLink::noSoftNoHard;
            }
            // Sets the hardware curve in any case (if present and if valid).
            if (lineIndex < rawHardwareEnvelopes.size()) {
                auto hardwareEnvelope = static_cast<int>(rawHardwareEnvelopes[lineIndex]);
                // Converts the given envelope to an AT compliant curve.
                hardwareEnvelope = PsgValues::convertEnvelopeCurveToAt(hardwareEnvelope);
                songBuilder->setCurrentPsgInstrumentCellHardwareEnvelope(hardwareEnvelope);
            }
            songBuilder->setCurrentPsgInstrumentCellLink(link);

            songBuilder->finishCurrentPsgInstrumentCell();
        }

        songBuilder->setCurrentPsgInstrumentMainLoop(0, static_cast<int>(length) - 1, false);
        songBuilder->finishCurrentInstrument();
    }
}

void WyzSongImporter::encodeFxs(const int fxChannel) const noexcept
{
    jassert(fxChannel < 3);

    const auto fxBaseIndex = highestInstrumentIndex + 1;
    auto& subsongBuilder = songBuilder->getCurrentSubsongBuilder();

    // Overwrites the cell with the FX cell.
    for (const auto& fxCell : fxCells) {
        // Since each Pattern has its own 3 Tracks, we know where to write the FX cell.
        const auto trackIndex = fxCell.baseTrackIndex + fxChannel;
        const auto cellIndex = fxCell.lineIndex;
        const auto fxIndex = fxBaseIndex + fxCell.fxId;
        const auto cell = Cell::build(fxCellNote, fxIndex);

        subsongBuilder.setCellOnTrack(trackIndex, cellIndex, cell);
    }
}

}   // namespace arkostracker