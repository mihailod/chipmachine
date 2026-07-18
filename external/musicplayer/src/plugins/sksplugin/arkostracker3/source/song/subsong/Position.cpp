#include "Position.h"

#include "../../ui/components/colors/ColorConstants.h"
#include "../tracks/TrackConstants.h"

namespace arkostracker 
{

const int Position::minimumPositionHeight = 1;
const int Position::maximumPositionHeight = 128;

const int Position::minimumTransposition = -48;
const int Position::maximumTransposition = 48;
const juce::String Position::firstMarkerName = "Start";                     // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

Position::Position(const int pPatternIndex, const int pHeight, juce::String pMarkerName, const juce::uint32 pMarkerColor, std::unordered_map<int, int> pNewChannelToTransposition) noexcept :
        patternIndex(pPatternIndex),
        height(pHeight),
        markerName(std::move(pMarkerName)),
        markerColor(pMarkerColor),
        channelToTransposition(std::move(pNewChannelToTransposition))
{
    jassert((height >= minimumPositionHeight) && (height <= maximumPositionHeight));
}

Position::Position(const int pPatternIndex, const int pHeight) noexcept :
        patternIndex(pPatternIndex),
        height(pHeight),
        markerName(),
        markerColor(ColorConstants::defaultColorAsArgb),
        channelToTransposition()
{
}

Position::Position() noexcept :
        patternIndex(0),
        height(TrackConstants::defaultPositionHeight),
        markerName(juce::String()),
        markerColor(ColorConstants::defaultColorAsArgb),
        channelToTransposition()
{
}

Position Position::buildEmptyInstance() noexcept
{
    return { };
}

Position Position::withPatternIndex(int newPatternIndex) const noexcept
{
    return { newPatternIndex, height, markerName, markerColor, channelToTransposition };
}

Position Position::withHeight(int newHeight) const noexcept
{
    return { patternIndex, newHeight, markerName, markerColor, channelToTransposition };
}

Position Position::withTranspositions(std::unordered_map<int, int> newChannelToTransposition) const noexcept
{
#if DEBUG
    for (const auto[_, transposition] : newChannelToTransposition) {
        jassert(transposition != 0);        // No 0 should be encoded!
    }
#endif

    return { patternIndex, height, markerName, markerColor, std::move(newChannelToTransposition) };
}

Position Position::withTransposition(const int channelIndex, const int transposition) const noexcept
{
    auto newChannelToTransposition = channelToTransposition;
    if (transposition == 0) {
        // No transposition to 0 should be encoded.
        newChannelToTransposition.erase(channelIndex);
    } else {
        newChannelToTransposition[channelIndex] = transposition;
    }
    return { patternIndex, height, markerName, markerColor, newChannelToTransposition };
}

Position Position::withMarkerName(const juce::String& newMarkerName) const noexcept
{
    return { patternIndex, height, newMarkerName, markerColor, channelToTransposition };
}

int Position::getPatternIndex() const noexcept
{
    return patternIndex;
}

int Position::getHeight() const
{
    return height;
}

const juce::String& Position::getMarkerName() const noexcept
{
    return markerName;
}

const juce::uint32& Position::getMarkerColor() const noexcept
{
    return markerColor;
}

void Position::setMarkerNameAndColour(const juce::String& name, const juce::uint32 colour) noexcept
{
    markerName = name;
    markerColor = colour;
}

bool Position::isTranspositionUsed() const noexcept
{
    return !channelToTransposition.empty();
}

std::unordered_map<int, int> Position::getChannelToTransposition() const noexcept
{
    return channelToTransposition;
}

int Position::getTransposition(const int channelIndex) const noexcept
{
    auto iterator = channelToTransposition.find(channelIndex);
    if (iterator == channelToTransposition.cend()) {
        return 0;       // Should happen often, only non-transposition are encoded.
    }
    return iterator->second;
}

void Position::removeMarker() noexcept
{
    markerName = "";
}

void Position::setPatternIndex(const int newPatternIndex) noexcept
{
    patternIndex = newPatternIndex;
}

void Position::setHeight(const int newHeight) noexcept
{
    height = newHeight;
}

bool Position::operator==(const Position& rhs) const
{
    return (patternIndex == rhs.patternIndex) &&
           (height == rhs.height) &&
           (markerName == rhs.markerName) &&
           (markerColor == rhs.markerColor) &&
           (channelToTransposition == rhs.channelToTransposition);
}

bool Position::operator!=(const Position& rhs) const
{
    return !(rhs == *this);
}

}   // namespace arkostracker
