#pragma once

#include <map>

#include "../../song/cells/Effect.h"
#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

class SubsongBuilder;

/** Imports the Subsong of an AT2 song. */
class SubsongReader
{
public:
    /**
     * Constructor. The Subsong is considered already opened.
     * @param subsongBuilder to build the Subsong.
     */
    explicit SubsongReader(SubsongBuilder& subsongBuilder) noexcept;

    /**
     * Parses the given Subsong node and fills the Subsong Builder. The Subsong is considered already opened.
     * This method will not close it.
     * @param subsongNode the node of the Subsong.
     */
    void parse(const juce::XmlElement& subsongNode) noexcept;

private:
    /**
     * Parses the Tracks.
     * @param subsongNode the node of the Subsong.
     */
    void parseTracks(const juce::XmlElement& subsongNode) noexcept;

    /**
     * @return the effect from the String, or absent if couldn't be parsed.
     * @param effectString the effect, as a String.
     */
    static OptionalValue<Effect> convertEffectStringToEffect(const juce::String& effectString) noexcept;

    /**
     * @return the int from the given Hex String (#1a25 for example), or absent if the parsing failed.
     * @param effectValueHexString the input value.
     */
    static OptionalValue<int> extractIntFromHexString(const juce::String& effectValueHexString) noexcept;

    /**
     * Parses the Special Tracks.
     * @param isSpeedTrack true if Speed Track, false if Event Track.
     * @param subsongNode the node of the Subsong.
     * @param collectionTag the tag of the collection of the Tracks.
     * @param itemTag the tag of the each Track.
     * @param cellTag the tag of the each cell.
     */
    void parseSpecialTracks(bool isSpeedTrack, const juce::XmlElement& subsongNode, const juce::String& collectionTag, const juce::String& itemTag,
                            const juce::String& cellTag) noexcept;

    /**
     * Parses the Linker to create the positions/patterns.
     * @param subsongNode the node of the Subsong.
     */
    void parseLinker(const juce::XmlElement& subsongNode) noexcept;

    SubsongBuilder& subsongBuilder;
};


}   // namespace arkostracker

