#include "At2SongImporter.h"

#include "../../utils/XmlHelper.h"
#include "SubsongReader.h"

namespace arkostracker 
{

At2SongImporter::At2SongImporter() noexcept :
        songBuilder()
{
}

// SongImporter method implementations.
// =======================================

bool At2SongImporter::doesFormatMatch(juce::InputStream& inputStream, const juce::String& /*extension*/) const noexcept
{
    // Tries to extract it as an XML document.
    const auto documentText = inputStream.readEntireStreamAsString();
    const auto songNode = juce::parseXML(documentText);
    if (songNode == nullptr) {
        return false;
    }

    // Gets the "format version" in the "song".
    if (songNode->getTagName() != "aks:song") {
        return false;
    }
    const auto* formatVersionNode = songNode->getChildByName("aks:formatVersion");
    return ((formatVersionNode != nullptr) && (formatVersionNode->getAllSubText() == "1.0"));
}

std::unique_ptr<SongImporter::Result> At2SongImporter::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*configuration*/) noexcept
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

void At2SongImporter::parse(const juce::XmlElement& songNode) noexcept
{
    // Extracts the metadata of the Song.
    const auto title = XmlHelper::readString(songNode, "aks:title");
    const auto author = XmlHelper::readString(songNode, "aks:author");
    const auto composer = XmlHelper::readString(songNode, "aks:composer");
    const auto comments = XmlHelper::readString(songNode, "aks:comment");
    const auto creationDate = XmlHelper::readString(songNode, "aks:creationDate");           // In YYYY-MM-DD format.
    const auto modificationDate = XmlHelper::readString(songNode, "aks:modificationDate");   // Idem.

    const auto creationDateMs = convertDateToEpoch(creationDate);
    const auto modificationDateMs = convertDateToEpoch(modificationDate);

    songBuilder->setSongMetaData(title, author, composer, comments, creationDateMs, modificationDateMs);

    // Parses the song proper.
    parsePsgInstruments(songNode);
    parseSampleInstruments(songNode);
    parseArpeggios(songNode);
    parsePitches(songNode);

    parseSubsongs(songNode);
}

juce::int64 At2SongImporter::convertDateToEpoch(const juce::String& originalDate) noexcept
{
    const auto date = originalDate.trim();
    // The size should be at least enough for YYYY-MM-DD!
    if (date.length() < 10) {
        return juce::Time::currentTimeMillis();
    }
    const auto year = date.substring(0, 4).getIntValue();
    const auto month = date.substring(5, 7).getIntValue() - 1;      // -1 because Juce wants from 0 to 11 for the month.
    const auto day = date.substring(8, 10).getIntValue();

    const juce::Time time(year, month, day, 0, 0);
    return time.toMilliseconds();
}

void At2SongImporter::parsePsgInstruments(const juce::XmlElement& songNode) noexcept
{
    // There might be no instruments (the 0 is not encoded).
    const auto* instrumentsNode = songNode.getChildByName("aks:fmInstruments");
    for (const auto* instrumentNode : XmlHelper::getChildrenList(instrumentsNode, "aks:fmInstrument")) {

        const auto instrumentIndex = XmlHelper::readInt(*instrumentNode, "aks:number", -1);
        if (instrumentIndex < 1) {
            songBuilder->addError("PSG Instrument index illegal or not found.");
            return;
        }

        const auto name = XmlHelper::readString(*instrumentNode, "aks:title");
        const auto speed = XmlHelper::readInt(*instrumentNode, "aks:speed");
        const auto isLooping = XmlHelper::readBool(*instrumentNode, "aks:isLooping");
        const auto loopStartIndex = XmlHelper::readInt(*instrumentNode, "aks:loopStartIndex");
        const auto endIndex = XmlHelper::readInt(*instrumentNode, "aks:endIndex");
        const auto isInstrumentRetrig = XmlHelper::readBool(*instrumentNode, "aks:isRetrig");
        const auto colorArgb = XmlHelper::readUnsignedInt(*instrumentNode, "aks:colorArgb", Instrument::defaultColor);

        songBuilder->startNewPsgInstrument(instrumentIndex);
        songBuilder->setCurrentInstrumentName(name);
        songBuilder->setCurrentPsgInstrumentMainLoop(loopStartIndex, endIndex, isLooping);
        songBuilder->setCurrentPsgInstrumentRetrig(isInstrumentRetrig);
        songBuilder->setCurrentPsgInstrumentSpeed(speed);
        songBuilder->setCurrentInstrumentColor(colorArgb);

        // Reads the cells.
        for (const auto* cellNode : XmlHelper::getChildrenList(*instrumentNode, "aks:fmInstrumentCell")) {
            parsePsgInstrumentCell(*cellNode);
        }

        songBuilder->finishCurrentInstrument();
    }
}

void At2SongImporter::parsePsgInstrumentCell(const juce::XmlElement& psgInstrumentCellNode) noexcept
{
    const auto linkString = XmlHelper::readString(psgInstrumentCellNode, "aks:link");
    const auto volume = XmlHelper::readInt(psgInstrumentCellNode, "aks:volume");
    const auto noise = XmlHelper::readInt(psgInstrumentCellNode, "aks:noise");
    const auto softwarePeriod = XmlHelper::readInt(psgInstrumentCellNode, "aks:softwarePeriod");
    const auto softwareArpeggio = XmlHelper::readInt(psgInstrumentCellNode, "aks:softwareArpeggio");
    const auto softwarePitch = XmlHelper::readInt(psgInstrumentCellNode, "aks:softwarePitch");
    const auto ratio = XmlHelper::readInt(psgInstrumentCellNode, "aks:ratio");
    const auto hardwareEnvelope = XmlHelper::readInt(psgInstrumentCellNode, "aks:hardwareCurve");
    const auto hardwarePeriod = XmlHelper::readInt(psgInstrumentCellNode, "aks:hardwarePeriod");
    const auto hardwareArpeggio = XmlHelper::readInt(psgInstrumentCellNode, "aks:hardwareArpeggio");
    const auto hardwarePitch = XmlHelper::readInt(psgInstrumentCellNode, "aks:hardwarePitch");
    const auto isRetrig = XmlHelper::readBool(psgInstrumentCellNode, "aks:isRetrig");

    // Parses the Link.
    const auto linkOptional = parseLinkFromString(linkString);
    PsgInstrumentCellLink link;    // NOLINT(*-init-variables)
    if (linkOptional.isAbsent()) {
        songBuilder->addWarning("Unable to parse the Link for PSG instrument cell: " + linkString);
        link = PsgInstrumentCellLink::softOnly;
    } else {
        link = linkOptional.getValue();
    }

    songBuilder->startNewPsgInstrumentCell();

    songBuilder->setCurrentPsgInstrumentCellLink(link);
    songBuilder->setCurrentPsgInstrumentCellVolume(volume);
    songBuilder->setCurrentPsgInstrumentCellNoise(noise);
    songBuilder->setCurrentPsgInstrumentCellRatio(ratio);
    songBuilder->setCurrentPsgInstrumentCellRetrig(isRetrig);
    songBuilder->setCurrentPsgInstrumentCellHardwareEnvelope(hardwareEnvelope);
    switch (link) {
        case PsgInstrumentCellLink::noSoftNoHard: [[fallthrough]];
        case PsgInstrumentCellLink::softOnly: [[fallthrough]];
        case PsgInstrumentCellLink::softToHard: [[fallthrough]];
        case PsgInstrumentCellLink::softAndHard:
            songBuilder->setCurrentPsgInstrumentCellPrimaryPeriod(softwarePeriod);
            songBuilder->setCurrentPsgInstrumentCellPrimaryArpeggio(softwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellPrimaryPitch(softwarePitch);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPeriod(hardwarePeriod);
            songBuilder->setCurrentPsgInstrumentCellSecondaryArpeggio(hardwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPitch(hardwarePitch);
            break;
        case PsgInstrumentCellLink::hardOnly: [[fallthrough]];
        case PsgInstrumentCellLink::hardToSoft:
            songBuilder->setCurrentPsgInstrumentCellPrimaryPeriod(hardwarePeriod);
            songBuilder->setCurrentPsgInstrumentCellPrimaryArpeggio(hardwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellPrimaryPitch(hardwarePitch);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPeriod(softwarePeriod);
            songBuilder->setCurrentPsgInstrumentCellSecondaryArpeggio(softwareArpeggio);
            songBuilder->setCurrentPsgInstrumentCellSecondaryPitch(softwarePitch);
            break;
    }

    songBuilder->finishCurrentPsgInstrumentCell();
}

OptionalValue<PsgInstrumentCellLink> At2SongImporter::parseLinkFromString(const juce::String& linkString) noexcept
{
    if (linkString == "softOnly") {
        return PsgInstrumentCellLink::softOnly;
    }
    if (linkString == "hardOnly") {
        return PsgInstrumentCellLink::hardOnly;
    }
    if (linkString == "hardToSoft") {
        return PsgInstrumentCellLink::hardToSoft;
    }
    if (linkString == "softToHard") {
        return PsgInstrumentCellLink::softToHard;
    }
    if (linkString == "softAndHard") {
        return PsgInstrumentCellLink::softAndHard;
    }
    if (linkString == "noSoftNoHard") {
        return PsgInstrumentCellLink::noSoftNoHard;
    }

    return { };
}

void At2SongImporter::parseSampleInstruments(const juce::XmlElement& songNode) noexcept
{
    // There might be no sample instruments.
    const auto* instrumentsNode = songNode.getChildByName("aks:sampleInstruments");
    for (const auto* instrumentNode : XmlHelper::getChildrenList(instrumentsNode, "aks:sampleInstrument")) {
        const auto instrumentIndex = XmlHelper::readInt(*instrumentNode, "aks:number", -1);
        if (instrumentIndex < 1) {
            songBuilder->addError("Sample Instrument index illegal or not found.");
            return;
        }

        const auto title = XmlHelper::readString(*instrumentNode, "aks:title");
        const auto frequencyHz = XmlHelper::readInt(*instrumentNode, "aks:frequencyHz");
        const auto isLooping = XmlHelper::readBool(*instrumentNode, "aks:isLooping");
        const auto loopStartIndex = XmlHelper::readInt(*instrumentNode, "aks:loopStartIndex");
        const auto endIndex = XmlHelper::readInt(*instrumentNode, "aks:endIndex");
        const auto volumeRatio = XmlHelper::readFloat(*instrumentNode, "aks:volumeRatio");
        const auto sampleData8BitsUnsignedBase64 = XmlHelper::readString(*instrumentNode, "aks:sampleData8bitsUnsignedBase64");

        songBuilder->startNewSampleInstrument(instrumentIndex);
        songBuilder->setCurrentInstrumentName(title);
        songBuilder->setCurrentSampleInstrumentFrequencyHz(frequencyHz);
        songBuilder->setCurrentSampleInstrumentLoop(Loop(loopStartIndex, endIndex, isLooping));
        songBuilder->setCurrentSampleInstrumentVolumeRatio(volumeRatio);
        songBuilder->setCurrentSampleInstrumentSample(sampleData8BitsUnsignedBase64);
        songBuilder->finishCurrentInstrument();
    }
}

void At2SongImporter::parseArpeggios(const juce::XmlElement& songNode) noexcept
{
    // There might be no arpeggios.
    const auto* arpeggiosNode = songNode.getChildByName("aks:arpeggios");
    for (const auto* arpeggioNode : XmlHelper::getChildrenList(arpeggiosNode, "aks:arpeggio")) {
        const auto arpeggioIndex = XmlHelper::readInt(*arpeggioNode, "aks:index", -1);
        if (arpeggioIndex < 1) {
            songBuilder->addError("Arpeggio index illegal or not found.");
            return;
        }

        const auto title = XmlHelper::readString(*arpeggioNode, "aks:name");
        const auto speed = XmlHelper::readInt(*arpeggioNode, "aks:speed");
        const auto loopStartIndex = XmlHelper::readInt(*arpeggioNode, "aks:loopStartIndex");
        const auto endIndex = XmlHelper::readInt(*arpeggioNode, "aks:endIndex");

        songBuilder->startNewExpression(true, arpeggioIndex, title);
        songBuilder->setCurrentExpressionSpeed(speed);
        songBuilder->setCurrentExpressionLoop(loopStartIndex, endIndex);

        // Parses each cell.
        static const juce::String tagArpeggioCell = "aks:arpeggioCell";
        const auto* cellNode = arpeggioNode->getChildByName(tagArpeggioCell);
        while ((cellNode != nullptr) && (cellNode->getTagName() == tagArpeggioCell)) {       // Tag comparison is a security for the next element.
            const auto noteInOctave = XmlHelper::readInt(*cellNode, "aks:note");
            const auto octave = XmlHelper::readInt(*cellNode, "aks:octave");
            const auto note = (octave * 12) + noteInOctave;
            songBuilder->addCurrentExpressionValue(note);

            cellNode = cellNode->getNextElement();
        }

        songBuilder->finishCurrentExpression();
    }
}

void At2SongImporter::parsePitches(const juce::XmlElement& songNode) noexcept
{
    // There might be no pitches.
    const auto* pitchesNode = songNode.getChildByName("aks:pitchs");
    for (const auto* pitchNode : XmlHelper::getChildrenList(pitchesNode, "aks:pitch")) {
        const auto pitchIndex = XmlHelper::readInt(*pitchNode, "aks:index", -1);
        if (pitchIndex < 1) {
            songBuilder->addError("Pitch index illegal or not found.");
            return;
        }

        const auto title = XmlHelper::readString(*pitchNode, "aks:name");
        const auto speed = XmlHelper::readInt(*pitchNode, "aks:speed");
        const auto loopStartIndex = XmlHelper::readInt(*pitchNode, "aks:loopStartIndex");
        const auto endIndex = XmlHelper::readInt(*pitchNode, "aks:endIndex");

        songBuilder->startNewExpression(false, pitchIndex, title);
        songBuilder->setCurrentExpressionSpeed(speed);
        songBuilder->setCurrentExpressionLoop(loopStartIndex, endIndex);

        // Parses each values.
        static const juce::String tagPitchValue = "aks:value";
        const auto* valueNode = pitchNode->getChildByName(tagPitchValue);
        while ((valueNode != nullptr) && (valueNode->getTagName() == tagPitchValue)) {       // Tag comparison is a security for the next element.
            const auto value = valueNode->getAllSubText().getIntValue();
            songBuilder->addCurrentExpressionValue(value);

            valueNode = valueNode->getNextElement();
        }

        songBuilder->finishCurrentExpression();
    }
}

void At2SongImporter::parseSubsongs(const juce::XmlElement& songNode) noexcept
{
    // There must be at least one Subsong, but this is tested by the builder itself.
    const auto* subsongsNode = songNode.getChildByName("aks:subsongs");
    auto subsongIndex = 0;
    for (const auto* subsongNode : XmlHelper::getChildrenList(subsongsNode, "aks:subsong")) {
        // Creates the Subsong, parses it and closes it.
        auto& subsongBuilder = songBuilder->startNewSubsong(subsongIndex);

        SubsongReader subsongReader(subsongBuilder);
        subsongReader.parse(*subsongNode);
        songBuilder->finishCurrentSubsong();

        // Next subsong.
        ++subsongIndex;
    }
}

ImportedFormat At2SongImporter::getFormat() noexcept
{
    return ImportedFormat::at2;
}

}   // namespace arkostracker
