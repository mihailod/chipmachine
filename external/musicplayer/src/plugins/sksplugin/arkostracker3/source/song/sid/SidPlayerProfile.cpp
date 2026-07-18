#include "SidPlayerProfile.h"

namespace arkostracker
{

const juce::String SidPlayerProfileUtil::serializationTextSidProfileCpc = "cpc";                    // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String SidPlayerProfileUtil::serializationTextSidProfileAmstradPlus = "amstradPlus";    // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String SidPlayerProfileUtil::serializationTextSidProfileAtariSt = "atariSt";            // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)
const juce::String SidPlayerProfileUtil::serializationTextSidProfileCustom = "custom";              // NOLINT(cert-err58-cpp,fuchsia-statically-constructed-objects)

juce::String SidPlayerProfileUtil::sidPlayerProfileToSerializationText(const SidPlayerProfile& profile) noexcept
{
    switch (profile) {
        default:
        case SidPlayerProfile::cpc:
            jassertfalse;       // Shouldn't happen, CPC is default, shouldn't be encoded.
            return serializationTextSidProfileCpc;
        case SidPlayerProfile::amstradPlus:
            return serializationTextSidProfileAmstradPlus;
        case SidPlayerProfile::atariSt:
            return serializationTextSidProfileAtariSt;
        case SidPlayerProfile::custom:
            return serializationTextSidProfileCustom;
    }
}

OptionalValue<SidPlayerProfile> SidPlayerProfileUtil::serializationTextToSidPlayerProfileTo(const juce::String& inputText) noexcept
{
    if (inputText == serializationTextSidProfileCpc) {
        return SidPlayerProfile::cpc;
    }
    if (inputText == serializationTextSidProfileAmstradPlus) {
        return SidPlayerProfile::amstradPlus;
    }
    if (inputText == serializationTextSidProfileAtariSt) {
        return SidPlayerProfile::atariSt;
    }
    if (inputText == serializationTextSidProfileCustom) {
        return SidPlayerProfile::custom;
    }

    jassertfalse;       // Unknown profile!
    return { };
}

}   // namespace arkostracker
