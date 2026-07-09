#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** How channels of one PSG can be mixed. */
enum class PsgMixingOutput
{
    ABC = 0,
    ACB,
    BAC,
    BCA,
    CAB,
    CBA,
    threeChannelsToLeft,
    threeChannelsToMiddle,
    threeChannelsToRight,
};

/** Utility class for the PSG Mixing Output. */
class PsgMixingOutputUtil
{
public:
    static const juce::String serializationTextABC;
    static const juce::String serializationTextACB;
    static const juce::String serializationTextBAC;
    static const juce::String serializationTextBCA;
    static const juce::String serializationTextCAB;
    static const juce::String serializationTextCBA;
    static const juce::String serializationTextLeft;
    static const juce::String serializationTextMiddle;
    static const juce::String serializationTextRight;

    static const juce::String displayTextABC;
    static const juce::String displayTextACB;
    static const juce::String displayTextBAC;
    static const juce::String displayTextBCA;
    static const juce::String displayTextCAB;
    static const juce::String displayTextCBA;
    static const juce::String displayTextLeft;
    static const juce::String displayTextMiddle;
    static const juce::String displayTextRight;

    /** Indicates whether the given serialization text is a valid mixing output. */
    static bool isSerializationTextPsgMixing(const juce::String &text);

    /**
     * Converts the given text to a PsgMixingOutput. The "isTextPsgMixing" method should be called first.
     * @param text the text ("ABC", "left", etc.). Used by the XML serializer.
     * @return the PsgMixingOutput, or empty if it couldn't be found.
     */
    static OptionalValue<PsgMixingOutput> serializationTextToPsgMixingOutput(const juce::String &text);

    /**
     * Converts the PsgMixingOutput to a text, as used by the XML serializer.
     * @param psgMixingOutput how the PSG are mixed.
     * @return the text, as used by the XML serializer.
    */
    static juce::String psgMixingOutputToSerializationText(const PsgMixingOutput &psgMixingOutput);

    /**
     * Converts the PsgMixingOutput to a text, for display.
     * @param psgMixingOutput how the PSG are mixed.
     * @return the text.
    */
    static juce::String psgMixingOutputToDisplayedText(const PsgMixingOutput &psgMixingOutput);
};

}   // namespace arkostracker

