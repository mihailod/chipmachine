#include "InstrumentDeserializerAt2.h"

#include "../../../import/at2/At2SongImporter.h"
#include "../../../utils/NoteUtil.h"
#include "../../../utils/XmlHelper.h"

namespace arkostracker
{

const juce::String InstrumentDeserializerAt2::nodeRoot = juce::String("aks:instrument");                    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeFmInstrument = juce::String("aks:fmInstrument");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeTitle = juce::String("aks:title");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeSpeed = juce::String("aks:speed");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeIsLooping = juce::String("aks:isLooping");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeLoopStartIndex = juce::String("aks:loopStartIndex");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeEndIndex = juce::String("aks:endIndex");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeIsRetrig = juce::String("aks:isRetrig");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeFmInstrumentCell = juce::String("aks:fmInstrumentCell");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

const juce::String InstrumentDeserializerAt2::nodeLink = juce::String("aks:link");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeVolume = juce::String("aks:volume");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeNoise = juce::String("aks:noise");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeSoftwarePeriod = juce::String("aks:softwarePeriod");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeSoftwareArpeggio = juce::String("aks:softwareArpeggio");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeSoftwarePitch = juce::String("aks:softwarePitch");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeRatio = juce::String("aks:ratio");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeHardwarePeriod = juce::String("aks:hardwarePeriod");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeHardwareArpeggio = juce::String("aks:hardwareArpeggio");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeHardwarePitch = juce::String("aks:hardwarePitch");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
const juce::String InstrumentDeserializerAt2::nodeHardwareEnvelope = juce::String("aks:hardwareCurve");          // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

std::unique_ptr<Instrument> InstrumentDeserializerAt2::deserializeInstrument(const juce::XmlElement& instrumentRoot) noexcept
{
    if (instrumentRoot.getTagName() != nodeRoot) {
        return nullptr;
    }

    const auto* fmInstrument = instrumentRoot.getChildByName(nodeFmInstrument);
    if (fmInstrument == nullptr) {
        return nullptr;
    }

    const auto name = XmlHelper::readString(*fmInstrument, nodeTitle);
    const auto speed = XmlHelper::readInt(*fmInstrument, nodeSpeed);
    const auto isLooping = XmlHelper::readBool(*fmInstrument, nodeIsLooping);
    const auto loopStartIndex = XmlHelper::readInt(*fmInstrument, nodeLoopStartIndex);
    const auto loopEndIndex = XmlHelper::readInt(*fmInstrument, nodeEndIndex);
    const auto isRetrig = XmlHelper::readBool(*fmInstrument, nodeIsRetrig);

    PsgPart psgPart;
    // Reads all the Cells.
    for (const auto& cellNode : XmlHelper::getChildrenList(*fmInstrument, nodeFmInstrumentCell)) {
        addCell(psgPart, *cellNode);
    }

    psgPart.setSpeed(speed);
    psgPart.setMainLoop(Loop(loopStartIndex, loopEndIndex, isLooping));
    psgPart.setInstrumentRetrig(isRetrig);

    return std::make_unique<Instrument>(name, InstrumentType::psgInstrument, psgPart, SamplePart());
}

void InstrumentDeserializerAt2::addCell(PsgPart& psgPart, const juce::XmlElement& cellNode) noexcept
{
    const auto readLink = XmlHelper::readString(cellNode, nodeLink);
    auto link = At2SongImporter::parseLinkFromString(readLink);
    if (link.isAbsent()) {
        jassertfalse;
        link = PsgInstrumentCellLink::softOnly;
    }
    const auto volume = XmlHelper::readInt(cellNode, nodeVolume);
    const auto noise = XmlHelper::readInt(cellNode, nodeNoise);
    const auto softwarePeriod = XmlHelper::readInt(cellNode, nodeSoftwarePeriod);
    const auto softwareArpeggio = XmlHelper::readInt(cellNode, nodeSoftwareArpeggio);
    const auto softwarePitch = XmlHelper::readInt(cellNode, nodeSoftwarePitch);
    const auto hardwarePeriod = XmlHelper::readInt(cellNode, nodeHardwarePeriod);
    const auto hardwareArpeggio = XmlHelper::readInt(cellNode, nodeHardwareArpeggio);
    const auto hardwarePitch = XmlHelper::readInt(cellNode, nodeHardwarePitch);
    const auto hardwareEnvelope = XmlHelper::readInt(cellNode, nodeHardwareEnvelope);
    const auto ratio = XmlHelper::readInt(cellNode, nodeRatio);
    const auto isRetrig = XmlHelper::readBool(cellNode, nodeIsRetrig);

    auto primaryPeriod = 0;
    auto primaryArpeggio = 0;
    auto primaryPitch = 0;
    auto secondaryPeriod = 0;
    auto secondaryArpeggio = 0;
    auto secondaryPitch = 0;
    switch (link.getValue()) {
        case PsgInstrumentCellLink::noSoftNoHard: [[fallthrough]];
        case PsgInstrumentCellLink::softOnly: [[fallthrough]];
        case PsgInstrumentCellLink::softToHard: [[fallthrough]];
        case PsgInstrumentCellLink::softAndHard:
            primaryPeriod = softwarePeriod;
            primaryArpeggio = softwareArpeggio;
            primaryPitch = softwarePitch;
            secondaryPeriod = hardwarePeriod;
            secondaryArpeggio = hardwareArpeggio;
            secondaryPitch = hardwarePitch;
            break;
        case PsgInstrumentCellLink::hardOnly: [[fallthrough]];
        case PsgInstrumentCellLink::hardToSoft:
            primaryPeriod = hardwarePeriod;
            primaryArpeggio = hardwareArpeggio;
            primaryPitch = hardwarePitch;
            secondaryPeriod = softwarePeriod;
            secondaryArpeggio = softwareArpeggio;
            secondaryPitch = softwarePitch;
            break;
    }

    const auto primaryArpeggioOctave = NoteUtil::getOctaveFromNote(primaryArpeggio);
    const auto primaryArpeggioNoteInOctave = NoteUtil::getNoteInOctave(primaryArpeggio);

    const auto secondaryArpeggioOctave = NoteUtil::getOctaveFromNote(secondaryArpeggio);
    const auto secondaryArpeggioNoteInOctave = NoteUtil::getNoteInOctave(secondaryArpeggio);

    const PsgInstrumentCell cell(link.getValue(), volume, noise, primaryPeriod, primaryArpeggioNoteInOctave, primaryArpeggioOctave, primaryPitch,
                      ratio, secondaryPeriod, secondaryArpeggioNoteInOctave, secondaryArpeggioOctave, secondaryPitch,
                      hardwareEnvelope, isRetrig,
                      PsgValues::defaultSidActivated, PsgValues::defaultSidBalancePercent, PsgValues::defaultSidLowShelfVolume, PsgValues::defaultSidRatio,
                      PsgValues::defaultSidArpeggio, PsgValues::defaultSidPitch
    );

    psgPart.addCell(cell);
}

}   // namespace arkostracker
