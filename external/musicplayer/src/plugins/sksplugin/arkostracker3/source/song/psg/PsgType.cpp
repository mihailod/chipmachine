#include "PsgType.h"

namespace arkostracker 
{

const juce::String PsgTypeUtil::serializationTextAy = "ay";                   // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgTypeUtil::serializationTextYm = "ym";                   // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgTypeUtil::displayTextAy = "AY";                         // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgTypeUtil::displayTextYm = "YM";                         // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

bool PsgTypeUtil::isSerializationTextPsgType(const juce::String& text)
{
    return (serializationTextAy == text) || (serializationTextYm == text);
}

OptionalValue<PsgType> PsgTypeUtil::serializationTextToPsgType(const juce::String& text)
{
    PsgType psgType;
    if (serializationTextAy == text) {
        psgType = PsgType::ay;
    } else if (serializationTextYm == text) {
        psgType = PsgType::ym;
    } else {
        psgType = PsgType::ay;       // Shouldn't happen, but AT2 format didn't encode it by default.
    }

    return psgType;
}

juce::String PsgTypeUtil::psgTypeToSerializationText(const PsgType psgType)
{
    switch (psgType) {
        default:
            jassertfalse;       // Shouldn't happen. Fallthrough to AY.
        case PsgType::ay:
            return serializationTextAy;
        case PsgType::ym:
            return serializationTextYm;
    }
}

juce::String PsgTypeUtil::psgTypeToDisplayedText(const PsgType& psgType)
{
    switch (psgType) {
        case PsgType::ay:
        default:
            return displayTextAy;
        case PsgType::ym:
            return displayTextYm;
    }
}

}   // namespace arkostracker

