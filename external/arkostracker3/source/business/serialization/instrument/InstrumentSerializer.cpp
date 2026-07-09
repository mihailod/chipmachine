#include "InstrumentSerializer.h"

#include "../../../utils/Base64Util.h"
#include "../../../utils/MemoryBlockUtil.h"
#include "../../../utils/PsgValues.h"
#include "../../../utils/XmlHelper.h"
#include "InstrumentNodes.h"

namespace arkostracker 
{

std::unique_ptr<juce::XmlElement> InstrumentSerializer::serialize(const std::vector<const Instrument*>& instruments) noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(InstrumentNodes::nodeInstrumentsRoot);

    // As soon as an error occurs, returns an empty pointer.
    for (const auto* instrument : instruments) {
        auto xmlPtr = serialize(*instrument);
        if (xmlPtr == nullptr) {
            jassertfalse;       // Serialization failed.
            return nullptr;
        }

        rootXml->addChildElement(xmlPtr.release());
    }

    return rootXml;
}

std::unique_ptr<juce::XmlElement> InstrumentSerializer::serialize(const Instrument& instrument) noexcept
{
    auto rootXml = std::make_unique<juce::XmlElement>(InstrumentNodes::nodeInstrumentRoot);

    XmlHelper::addValueNode(*rootXml, InstrumentNodes::nodeName, instrument.getName());
    XmlHelper::addValueNode(*rootXml, InstrumentNodes::nodeColorArgb, instrument.getArgbColor());

    switch (instrument.getType()) {
        case InstrumentType::psgInstrument:
            serializePsgPart(instrument, *rootXml);
            break;
        case InstrumentType::sampleInstrument:
            serializeSamplePart(instrument, *rootXml);
            break;
        default:
            jassertfalse;           // Unhandled instrument type!
            break;
    }

    return rootXml;
}

void InstrumentSerializer::serializePsgPart(const Instrument& instrument, juce::XmlElement& rootXml) noexcept
{
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeType, InstrumentNodes::valueTypePsg);

    const auto& psgPart = instrument.getConstPsgPart();
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeSpeed, psgPart.getSpeed());
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeIsRetrig, psgPart.isInstrumentRetrig());
    const auto& mainLoop = psgPart.getMainLoop();
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeLoopStartIndex, mainLoop.getStartIndex());
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeEndIndex, mainLoop.getEndIndex());
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeIsLooping, mainLoop.isLooping());
    // Sound effect.
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeSfxIsExported, psgPart.isSoundEffectExported());

    // Encodes the auto-spread.
    for (const auto&[psgSection, loop] : psgPart.getAutoSpreadAllLoops()) {            // TODO TU this.
        // Only encodes the ones that are useful (not really useful... Makes the export less messy).
        if (!loop.isLooping() && (loop.getStartIndex() == 0) && (loop.getEndIndex() == 0)) {
            continue;
        }

        // Converts the PSG Section to a String.
        const auto psgSectionString = PsgSectionHelper::toString(psgSection);

        auto& autoSpreadNode = XmlHelper::addNode(rootXml, InstrumentNodes::nodeAutoSpread);
        XmlHelper::addValueNode(autoSpreadNode, InstrumentNodes::nodePsgSection, psgSectionString);
        XmlHelper::addValueNode(autoSpreadNode, InstrumentNodes::nodeLoopStartIndex, loop.getStartIndex());
        XmlHelper::addValueNode(autoSpreadNode, InstrumentNodes::nodeEndIndex, loop.getEndIndex());
        XmlHelper::addValueNode(autoSpreadNode, InstrumentNodes::nodeIsLooping, loop.isLooping());
    }

    // Encodes each Cell.
    auto& cellsNode = XmlHelper::addNode(rootXml, InstrumentNodes::nodeCells);
    const auto length = psgPart.getLength();
    for (auto cellIndex = 0; cellIndex < length; ++cellIndex) {
        auto& cellNode = XmlHelper::addNode(cellsNode, InstrumentNodes::nodeCell);
        
        // Adds the data to this Cell node.
        const auto& cell = psgPart.getCellRefConst(cellIndex);

        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeVolume, cell.getVolume());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeNoise, cell.getNoise());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodePrimaryPeriod, cell.getPrimaryPeriod());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodePrimaryArpeggioNoteInOctave, cell.getPrimaryArpeggioNoteInOctave());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodePrimaryArpeggioOctave, cell.getPrimaryArpeggioOctave());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodePrimaryPitch, cell.getPrimaryPitch());
        const auto link = cell.getLink();
        const auto linkString = PsgInstrumentCellLinkHelper::toString(link);
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeLink, linkString);
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeRatio, cell.getRatio());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeHardwareEnvelope, cell.getHardwareEnvelope());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSecondaryPeriod, cell.getSecondaryPeriod());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSecondaryArpeggioNoteInOctave, cell.getSecondaryArpeggioNoteInOctave());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSecondaryArpeggioOctave, cell.getSecondaryArpeggioOctave());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSecondaryPitch, cell.getSecondaryPitch());
        XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeIsRetrig, cell.isRetrig());

        // SID. Only if non-default values.
        const auto isSidActivated = cell.isSidActivated();
        const auto sidBalancePercent = cell.getSidBalancePercent();
        const auto sidLowShelfVolume = cell.getSidLowShelfVolume();
        const auto sidRatio = cell.getSidRatio();
        const auto sidArpeggio = cell.getSidArpeggio();
        const auto sidPitch = cell.getSidPitch();
        if (isSidActivated) {
            XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSidIsActivated, isSidActivated);
        }
        if (sidBalancePercent != PsgValues::defaultSidBalancePercent) {
            XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSidCycleBalancePercent, sidBalancePercent);
        }
        if (sidLowShelfVolume != PsgValues::defaultSidLowShelfVolume) {
            XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSidLowShelfVolume, sidLowShelfVolume);
        }
        if (sidRatio != PsgValues::defaultSidRatio) {
            XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSidRatio, sidRatio);
        }
        if (sidArpeggio != PsgValues::defaultSidArpeggio) {
            XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSidArpeggio, sidArpeggio);
        }
        if (sidPitch != PsgValues::defaultSidPitch) {
            XmlHelper::addValueNode(cellNode, InstrumentNodes::nodeSidPitch, sidPitch);
        }
    }
}

void InstrumentSerializer::serializeSamplePart(const Instrument& instrument, juce::XmlElement& rootXml) noexcept
{
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeType, InstrumentNodes::valueTypeSample);

    const auto& samplePart = instrument.getConstSamplePart();
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeFrequencyHz, samplePart.getFrequencyHz());
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeAmplificationRatio, samplePart.getAmplificationRatio());
    if (samplePart.getOriginalFileName().isNotEmpty()) {
        XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeOriginalFilename, samplePart.getOriginalFileName());
    }
    // Encodes the digidrum note only if not default.
    if (samplePart.getDigidrumNote() != PsgValues::digidrumNote) {
        XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeDigiNote, samplePart.getDigidrumNote());
    }

    const auto& loop = samplePart.getLoop();
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeLoopStartIndex, loop.getStartIndex());
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeEndIndex, loop.getEndIndex());
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeIsLooping, loop.isLooping());

    // Encodes the sample data to base64.
    const auto sample = samplePart.getSample();
    if (sample == nullptr) {
        jassertfalse;       // Should never happen!
        return;
    }

    const auto sampleData = sample->getData();
    const auto bae64Sample = juce::Base64::toBase64(sampleData.getData(), sampleData.getSize());
    XmlHelper::addValueNode(rootXml, InstrumentNodes::nodeSampleUnsigned8BitsBase64, bae64Sample);
}


// ====================================================

std::vector<std::unique_ptr<Instrument>> InstrumentSerializer::deserializeInstruments(const juce::XmlElement& root) noexcept
{
    std::vector<std::unique_ptr<Instrument>> instruments;

    // Gets every Instrument node.
    const auto* instrumentNode = root.getChildByName(InstrumentNodes::nodeInstrumentRoot);
    while (instrumentNode != nullptr) {
        auto instrument = deserializeInstrument(*instrumentNode);
        if (instrument == nullptr) {
            jassertfalse;           // Error.
            return { };             // Stops as soon as an error is detected.
        }
        instruments.push_back(std::move(instrument));

        // Next sibling.
        instrumentNode = instrumentNode->getNextElementWithTagName(InstrumentNodes::nodeInstrumentRoot);
    }

    return instruments;
}

std::unique_ptr<Instrument> InstrumentSerializer::deserializeInstrument(const juce::XmlElement& instrumentRoot) noexcept
{
    const auto name = XmlHelper::readString(instrumentRoot, InstrumentNodes::nodeName);
    const auto type = XmlHelper::readString(instrumentRoot, InstrumentNodes::nodeType);
    const auto colorArgb = XmlHelper::readUnsignedInt(instrumentRoot, InstrumentNodes::nodeColorArgb, 0xff000000);

    InstrumentType instrumentType;    // NOLINT(*-init-variables)
    PsgPart psgPart;
    SamplePart samplePart;
    auto error = false;
    if (type == InstrumentNodes::valueTypePsg) {
        instrumentType = InstrumentType::psgInstrument;
        psgPart = createPsgPart(instrumentRoot, error);
    } else if (type == InstrumentNodes::valueTypeSample) {
        instrumentType = InstrumentType::sampleInstrument;
        samplePart = createSamplePart(instrumentRoot, error);
    } else {
        jassertfalse;       // Unknown type!
        return { nullptr };
    }

    auto instrument = std::make_unique<Instrument>(name, instrumentType, psgPart, samplePart);
    instrument->setColor(colorArgb);

    return error ? nullptr : std::move(instrument);
}

PsgPart InstrumentSerializer::createPsgPart(const juce::XmlElement& instrumentRoot, bool& errorOut) noexcept
{
    PsgPart psgPart;

    // The speed and retrig actually belong the root.
    const auto speed = XmlHelper::readInt(instrumentRoot, InstrumentNodes::nodeSpeed, 0);
    psgPart.setSpeed(speed);
    const auto globalRetrig = XmlHelper::readBool(instrumentRoot, InstrumentNodes::nodeIsRetrig);
    psgPart.setInstrumentRetrig(globalRetrig);

    {
        // The main loop.
        const auto isLooping = XmlHelper::readBool(instrumentRoot, InstrumentNodes::nodeIsLooping);
        const auto loopStartIndex = XmlHelper::readInt(instrumentRoot, InstrumentNodes::nodeLoopStartIndex, 0);
        const auto endIndex = XmlHelper::readInt(instrumentRoot, InstrumentNodes::nodeEndIndex, 0);
        psgPart.setMainLoop(Loop(loopStartIndex, endIndex, isLooping));
    }

    {
        // The sound effect.
        const auto isSoundEffectExported = XmlHelper::readBool(instrumentRoot, InstrumentNodes::nodeSfxIsExported, true);
        psgPart.setSoundEffectExported(isSoundEffectExported);
    }

    // Encodes the auto-spread.
    for (const auto* autoSpreadNode : XmlHelper::getChildrenList(instrumentRoot, InstrumentNodes::nodeAutoSpread)) {         // TODO TU this.
        const auto psgSectionString = XmlHelper::readString(*autoSpreadNode, InstrumentNodes::nodePsgSection);
        const auto psgSection = PsgSectionHelper::fromString(psgSectionString);
        if (psgSection.isAbsent()) {
            jassertfalse;           // PSG section couldn't be deserialized!
            continue;
        }
        const auto isLooping = XmlHelper::readBool(*autoSpreadNode, InstrumentNodes::nodeIsLooping);
        const auto loopStartIndex = XmlHelper::readInt(*autoSpreadNode, InstrumentNodes::nodeLoopStartIndex, 0);
        const auto endIndex = XmlHelper::readInt(*autoSpreadNode, InstrumentNodes::nodeEndIndex, 0);

        psgPart.setAutoSpreadLoop(psgSection.getValue(), Loop(loopStartIndex, endIndex, isLooping));
    }

    const auto* cellsNode = instrumentRoot.getChildByName(InstrumentNodes::nodeCells);
    if (cellsNode == nullptr) {
        errorOut = true;            // There should be at least one cell... At least the node should be here.
        return psgPart;
    }

    const auto cellNodes = XmlHelper::getChildrenList(*cellsNode, InstrumentNodes::nodeCell);
    for (const auto* cellNode : cellNodes) {
        const auto linkString = XmlHelper::readString(*cellNode, InstrumentNodes::nodeLink);
        const auto link = PsgInstrumentCellLinkHelper::fromString(linkString);
        if (link.isAbsent()) {
            errorOut = true;
            break;
        }
        const auto volume = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeVolume, 0);
        const auto noise = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeNoise, 0);
        const auto primaryPeriod = XmlHelper::readInt(*cellNode, InstrumentNodes::nodePrimaryPeriod, 0);
        const auto primaryArpeggioNoteInOctave = XmlHelper::readInt(*cellNode, InstrumentNodes::nodePrimaryArpeggioNoteInOctave, 0);
        const auto primaryArpeggioOctave = XmlHelper::readInt(*cellNode, InstrumentNodes::nodePrimaryArpeggioOctave, 0);
        const auto primaryPitch = XmlHelper::readInt(*cellNode, InstrumentNodes::nodePrimaryPitch, 0);
        const auto ratio = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeRatio, PsgValues::defaultRatio);
        const auto hardwareEnvelope = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeHardwareEnvelope, PsgValues::defaultHardwareEnvelope);
        const auto secondaryPeriod = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSecondaryPeriod, 0);
        const auto secondaryArpeggioNoteInOctave = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSecondaryArpeggioNoteInOctave, 0);
        const auto secondaryArpeggioOctave = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSecondaryArpeggioOctave, 0);
        const auto secondaryPitch = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSecondaryPitch, 0);
        const auto retrig = XmlHelper::readBool(*cellNode, InstrumentNodes::nodeIsRetrig);

        const auto isSidActivated = XmlHelper::readBool(*cellNode, InstrumentNodes::nodeSidIsActivated);
        const auto sidCycleBalancePercent = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSidCycleBalancePercent, PsgValues::defaultSidBalancePercent);
        const auto sidLowShelfVolume = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSidLowShelfVolume, PsgValues::defaultSidLowShelfVolume);
        const auto sidRatio = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSidRatio, PsgValues::defaultSidRatio);
        const auto sidArpeggio = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSidArpeggio, PsgValues::defaultSidArpeggio);
        const auto sidPitch = XmlHelper::readInt(*cellNode, InstrumentNodes::nodeSidPitch, PsgValues::defaultSidPitch);

        const PsgInstrumentCell cell(link.getValue(), volume, noise, primaryPeriod, primaryArpeggioNoteInOctave, primaryArpeggioOctave, primaryPitch,
                                     ratio, secondaryPeriod, secondaryArpeggioNoteInOctave, secondaryArpeggioOctave, secondaryPitch, hardwareEnvelope, retrig,
                                     isSidActivated, sidCycleBalancePercent, sidLowShelfVolume, sidRatio, sidArpeggio, sidPitch
        );
        psgPart.addCell(cell);
    }

    return psgPart;
}

SamplePart InstrumentSerializer::createSamplePart(const juce::XmlElement& instrumentRoot, bool& errorOut) noexcept
{
    errorOut = true;

    const auto amplificationRatio = XmlHelper::readFloat(instrumentRoot, InstrumentNodes::nodeAmplificationRatio, 1.0F);
    const auto originalFilename = XmlHelper::readString(instrumentRoot, InstrumentNodes::nodeOriginalFilename);
    const auto frequencyHz = XmlHelper::readInt(instrumentRoot, InstrumentNodes::nodeFrequencyHz, PsgFrequency::defaultSampleFrequencyHz);
    const auto startIndex = XmlHelper::readInt(instrumentRoot, InstrumentNodes::nodeLoopStartIndex, -1);
    const auto endIndex = XmlHelper::readInt(instrumentRoot, InstrumentNodes::nodeEndIndex, -1);
    const auto isLooping = XmlHelper::readBool(instrumentRoot, InstrumentNodes::nodeIsLooping);
    const auto dataBase64 = XmlHelper::readString(instrumentRoot, InstrumentNodes::nodeSampleUnsigned8BitsBase64);
    const auto digiNote = XmlHelper::readInt(instrumentRoot, InstrumentNodes::nodeDigiNote, PsgValues::digidrumNote);

    if ((startIndex < 0) || (endIndex < 0) || (startIndex > endIndex)) {
        jassertfalse;           // Invalid start/end index.
        return { };
    }

    // Mishmash of decoding the base64.
    const auto memoryBlockBase64Data = MemoryBlockUtil::fromString(dataBase64);
    juce::MemoryInputStream memoryInputStream(memoryBlockBase64Data, false);

    juce::MemoryOutputStream memoryOutputStream;
    const auto success = Base64Util::decodeFromBase64(memoryInputStream, memoryOutputStream);
    if (!success) {
        jassertfalse;           // Unable to decode the base-64.
        return { };
    }
    auto sample = std::make_shared<Sample>(memoryOutputStream.getMemoryBlock());

    errorOut = false;
    return { sample, Loop(startIndex, endIndex, isLooping), amplificationRatio, frequencyHz, digiNote, originalFilename };
}

}   // namespace arkostracker
