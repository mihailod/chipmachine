#include "Instrument.h"

#include "psg/PsgPartConstants.h"

namespace arkostracker 
{

const juce::uint32 Instrument::defaultColor = 0xffc0c0c0;
const juce::String Instrument::emptyInstrumentName = juce::translate("Empty");      // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

Instrument::Instrument(juce::String pName, const InstrumentType pInstrumentType, PsgPart pPsgPart, SamplePart pSamplePart, const juce::uint32 pColor) noexcept :
        id(),
        name(std::move(pName)),
        instrumentType(pInstrumentType),
        color(pColor),
        psgPart(std::move(pPsgPart)),
        samplePart(std::move(pSamplePart))
{
}

std::unique_ptr<Instrument> Instrument::buildPsgInstrument(const juce::String& name, const PsgPart& psgPart, juce::uint32 color) noexcept
{
    return std::make_unique<Instrument>(name, InstrumentType::psgInstrument, psgPart, SamplePart(), color);
}

std::unique_ptr<Instrument> Instrument::buildSampleInstrument(const juce::String& name, const SamplePart& samplePart, juce::uint32 color) noexcept
{
    return std::make_unique<Instrument>(name, InstrumentType::sampleInstrument, PsgPart(), samplePart, color);
}

std::unique_ptr<Instrument> Instrument::buildEmptyPsgInstrument() noexcept
{
    PsgPart psgPart;
    psgPart.addCell(PsgInstrumentCell::buildSoftwareCell(0, 0, 0, 0, 0, false));
    psgPart.setMainLoop(Loop(0, 0, true));
    psgPart.setSpeed(PsgPartConstants::slowestSpeed);      // As slow as possible. This is really only useful for the exporters.

    auto instrument = std::make_unique<Instrument>(emptyInstrumentName, InstrumentType::psgInstrument, psgPart, SamplePart());
    instrument->psgPart = psgPart;

    return instrument;
}

Id Instrument::getId() const noexcept
{
    return id;
}

juce::String Instrument::getName() const noexcept
{
    return name;
}

void Instrument::setName(const juce::String& newName) noexcept
{
    name = newName;
}

InstrumentType Instrument::getType() const noexcept
{
    return instrumentType;
}

juce::uint32 Instrument::getArgbColor() const noexcept
{
    return color;
}

void Instrument::setColor(const juce::uint32 newColor) noexcept
{
    color = newColor;
}

const PsgPart& Instrument::getConstPsgPart() const noexcept
{
    jassert(instrumentType == InstrumentType::psgInstrument);
    return psgPart;
}

PsgPart& Instrument::getPsgPart() noexcept
{
    jassert(instrumentType == InstrumentType::psgInstrument);
    return psgPart;
}

const SamplePart& Instrument::getConstSamplePart() const noexcept
{
    jassert(instrumentType == InstrumentType::sampleInstrument);
    return samplePart;
}

SamplePart& Instrument::getSamplePart() noexcept
{
    jassert(instrumentType == InstrumentType::sampleInstrument);
    return samplePart;
}

bool Instrument::operator==(const Instrument& other) const noexcept
{
    // The ID is not compared. Compares the metadata first.
    return (name == other.name) && (color == other.color) && isMusicallyEqualTo(other);
}

bool Instrument::operator!=(const Instrument& other) const noexcept
{
    return !(other == *this);
}

bool Instrument::isMusicallyEqualTo(const Instrument& other) const noexcept
{
    if ((instrumentType != other.instrumentType)) {
        return false;
    }

    // Only compares the relevant part.
    if (instrumentType == InstrumentType::psgInstrument) {
        return (psgPart == other.psgPart);
    }
    if (instrumentType == InstrumentType::sampleInstrument) {
        return (samplePart == other.samplePart);
    }

    jassertfalse;       // Unmanaged Instrument Type?
    return false;
}

}   // namespace arkostracker
