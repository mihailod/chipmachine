#pragma once

#include <unordered_map>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Holds data of a position, including the pattern index, the transposition, the possible marker. */
class Position
{
public:
    static const int minimumPositionHeight;             // The minimum height a Position can have.
    static const int maximumPositionHeight;             // The maximum height a Position can have.

    static const int minimumTransposition;              // The minimum transposition possible.
    static const int maximumTransposition;              // The maximum transposition possible.
    static const juce::String firstMarkerName;          // Used for generated Subsong.

    /**
     * Constructor.
     * @param patternIndex the index of the Pattern.
     * @param height the height, from 1 to 128. Never 0, never >128!
     * @param markerName the name of the Marker, or empty if there is none.
     * @param markerArgbColor the color of the Marker. Only relevant if there is a Marker.
     * @param channelToTransposition map linking a channel to its transposition value. Only non 0 values should be present.
     */
    Position(int patternIndex, int height, juce::String markerName, juce::uint32 markerArgbColor, std::unordered_map<int, int> channelToTransposition) noexcept;

    /**
     * Simpler constructor.
     * @param patternIndex the index of the Pattern.
     * @param height the height, from 1 to 128. Never 0, never >128!
     */
    explicit Position(int patternIndex, int height = 64) noexcept;

    /** @return an instance with nothing inside. Especially useful outside of lambda. */
    static Position buildEmptyInstance() noexcept;

    /**
     * @return a new instance with a different pattern index.
     * @param newPatternIndex the new pattern index.
     */
    Position withPatternIndex(int newPatternIndex) const noexcept;

    /**
     * @return a new instance with a different height.
     * @param newHeight the new height.
     */
    Position withHeight(int newHeight) const noexcept;

    /**
     * @return a new instance with different transpositions. The previous ones are deleted.
     * @param channelToTransposition the map linking a channel to a transposition. Only non 0 values should be present.
     */
    Position withTranspositions(std::unordered_map<int, int> channelToTransposition) const noexcept;

    /**
     * @return a new instance with a different transposition. If already present, it is overwritten.
     * @param channelIndex the channel index.
     * @param transposition the transposition.
     */
    Position withTransposition(int channelIndex, int transposition) const noexcept;

    /**
     * @return a new instance with a different marker name.
     * @param newMarkerName the new marker name. May be empty, meaning none.
     */
    Position withMarkerName(const juce::String& newMarkerName) const noexcept;

    /** @return the index of the Pattern. */
    int getPatternIndex() const noexcept;
    /** @return the height, from 1 to 128. */
    int getHeight() const;
    /** @return the marker name. Empty if there is none. */
    const juce::String& getMarkerName() const noexcept;
    /** @return irrelevant if there is no marker name. */
    const juce::uint32& getMarkerColor() const noexcept;
    /** @return the map linking a channel to a transposition. Only non-0 transposition are encoded. */
    std::unordered_map<int, int> getChannelToTransposition() const noexcept;

    /**
     * @return the transposition from the channel index. If out of bounds, simply returns 0.
     * @param channelIndex the channel index (>=0, may be >2).
     */
    int getTransposition(int channelIndex) const noexcept;

    /**
     * Sets the Pattern index.
     * @param patternIndex the new Pattern index.
     */
    void setPatternIndex(int patternIndex) noexcept;

    /**
     * Sets the name and colour of a Marker.
     * @param name the name.
     * @param color the color.
     */
    void setMarkerNameAndColour(const juce::String& name, juce::uint32 color) noexcept;

    /**
     * Sets the height.
     * @param newHeight the new height.
     */
    void setHeight(int newHeight) noexcept;

    /** Removes the Marker. */
    void removeMarker() noexcept;

    /** @return true if at least one channel uses a transposition. */
    bool isTranspositionUsed() const noexcept;

    bool operator==(const Position& rhs) const;
    bool operator!=(const Position& rhs) const;

private:
    /** A default constructor. Only useful when wanting an instance, regardless of the data inside. */
    Position() noexcept;

    int patternIndex;
    int height;                                                 // From 1 to 128.
    juce::String markerName;                                    // Empty if there is none.
    juce::uint32 markerColor;
    std::unordered_map<int, int> channelToTransposition;        // Only the channel indexes with a transposition must be written here.
};

}   // namespace arkostracker
