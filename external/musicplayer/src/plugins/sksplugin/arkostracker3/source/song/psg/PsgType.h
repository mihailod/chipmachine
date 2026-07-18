#pragma once

#include <juce_core/juce_core.h>

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** The type of the PSG (AY/YM). */
enum class PsgType
{
    /** CPC, MSX, Spectrum, etc. */
    ay,
    /** Atari ST. */
    ym
};


/** Utility class for the PSG Type. */
class PsgTypeUtil
{
public:
    static const juce::String serializationTextAy;
    static const juce::String serializationTextYm;

    static const juce::String displayTextAy;
    static const juce::String displayTextYm;

    /** Indicates whether the given serialization text is a valid type output. */
    static bool isSerializationTextPsgType(const juce::String& text);

    /**
     * Converts the given text to a PsgType. The "isTextPsgMixing" method should be called first.
     * @param text the text ("ym", "ay", etc.). Used by the XML serializer.
     * @return the PsgType, or absent if it couldn't be found.
     */
    static OptionalValue<PsgType> serializationTextToPsgType(const juce::String& text);

    /**
     * Converts the PsgType to a text, as used by the XML serializer.
     * @param psgType the type.
     * @return the text, as used by the XML serializer.
    */
    static juce::String psgTypeToSerializationText(PsgType psgType);

    /**
     * Converts the PsgMixingOutput to a text, for display.
     * @param psgType the type.
     * @return the text.
    */
    static juce::String psgTypeToDisplayedText(const PsgType& psgType);
};

}   // namespace arkostracker

