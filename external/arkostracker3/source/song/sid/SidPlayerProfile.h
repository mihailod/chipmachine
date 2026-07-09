#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker
{

enum class SidPlayerProfile : uint8_t
{
    cpc,
    amstradPlus,
    atariSt,
    custom,
};

class SidPlayerProfileUtil
{
public:
    /**
     * @return convert a profile into a string for serialization.
     * @param profile the profile.
     */
    static juce::String sidPlayerProfileToSerializationText(const SidPlayerProfile& profile) noexcept;

    /**
     * @return convert a profile as text into a profile, or absent if not known.
     * @param inputText the text to parse.
     */
    static OptionalValue<SidPlayerProfile> serializationTextToSidPlayerProfileTo(const juce::String& inputText) noexcept;

private:
    static const juce::String serializationTextSidProfileCpc;
    static const juce::String serializationTextSidProfileAmstradPlus;
    static const juce::String serializationTextSidProfileAtariSt;
    static const juce::String serializationTextSidProfileCustom;
};

}   // namespace arkostracker
