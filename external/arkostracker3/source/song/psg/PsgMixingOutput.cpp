#include "PsgMixingOutput.h"

namespace arkostracker 
{

const juce::String PsgMixingOutputUtil::serializationTextABC = "ABC";                 // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextACB = "ACB";                 // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextBAC = "BAC";                 // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextBCA = "BCA";                 // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextCAB = "CAB";                 // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextCBA = "CBA";                 // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextLeft = "left";               // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextMiddle = "center";           // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::serializationTextRight = "right";             // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

const juce::String PsgMixingOutputUtil::displayTextABC = "ABC";                       // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextACB = "ACB";                       // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextBAC = "BAC";                       // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextBCA = "BCA";                       // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextCAB = "CAB";                       // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextCBA = "CBA";                       // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextLeft = juce::translate("Left");          // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextMiddle = juce::translate("Center");      // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String PsgMixingOutputUtil::displayTextRight = juce::translate("Right");        // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

bool PsgMixingOutputUtil::isSerializationTextPsgMixing(const juce::String& text)
{
    return (serializationTextABC == text)
           || (serializationTextACB == text)
           || (serializationTextBAC == text)
           || (serializationTextBCA == text)
           || (serializationTextCAB == text)
           || (serializationTextCBA == text)
           || (serializationTextLeft == text)
           || (serializationTextMiddle == text)
           || (serializationTextRight == text);
}

OptionalValue<PsgMixingOutput> PsgMixingOutputUtil::serializationTextToPsgMixingOutput(const juce::String& text)
{
    if (serializationTextABC == text) {
        return PsgMixingOutput::ABC;
    }
    if (serializationTextACB == text) {
        return PsgMixingOutput::ACB;
    }
    if (serializationTextBAC == text) {
        return PsgMixingOutput::BAC;
    }
    if (serializationTextBCA== text) {
        return PsgMixingOutput::BCA;
    }
    if (serializationTextCAB == text) {
        return PsgMixingOutput::CAB;
    }
    if (serializationTextCBA == text) {
        return PsgMixingOutput::CBA;
    }
    if (serializationTextLeft == text) {
        return PsgMixingOutput::threeChannelsToLeft;
    }
    if (serializationTextMiddle == text) {
        return  PsgMixingOutput::threeChannelsToMiddle;
    }
    if (serializationTextRight == text) {
        return  PsgMixingOutput::threeChannelsToRight;
    }

    // Not found!
    return { };
}

juce::String PsgMixingOutputUtil::psgMixingOutputToSerializationText(const PsgMixingOutput& psgMixingOutput)
{
    switch (psgMixingOutput) {
        case PsgMixingOutput::ABC:
        default:
            return serializationTextABC;
        case PsgMixingOutput::ACB:
            return serializationTextACB;
        case PsgMixingOutput::BAC:
            return serializationTextBAC;
        case PsgMixingOutput::BCA:
            return serializationTextBCA;
        case PsgMixingOutput::CAB:
            return serializationTextCAB;
        case PsgMixingOutput::CBA:
            return serializationTextCBA;
        case PsgMixingOutput::threeChannelsToLeft:
            return serializationTextLeft;
        case PsgMixingOutput::threeChannelsToMiddle:
            return serializationTextMiddle;
        case PsgMixingOutput::threeChannelsToRight:
            return serializationTextRight;
    }
}

juce::String PsgMixingOutputUtil::psgMixingOutputToDisplayedText(const PsgMixingOutput& psgMixingOutput)
{
    switch (psgMixingOutput) {
        case PsgMixingOutput::ABC:
        default:
            return displayTextABC;
        case PsgMixingOutput::ACB:
            return displayTextACB;
        case PsgMixingOutput::BAC:
            return displayTextBAC;
        case PsgMixingOutput::BCA:
            return displayTextBCA;
        case PsgMixingOutput::CAB:
            return displayTextCAB;
        case PsgMixingOutput::CBA:
            return displayTextCBA;
        case PsgMixingOutput::threeChannelsToLeft:
            return displayTextLeft;
        case PsgMixingOutput::threeChannelsToMiddle:
            return displayTextMiddle;
        case PsgMixingOutput::threeChannelsToRight:
            return displayTextRight;
    }
}


}   // namespace arkostracker

