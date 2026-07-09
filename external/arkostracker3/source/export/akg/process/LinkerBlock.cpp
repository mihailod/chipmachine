#include "LinkerBlock.h"

namespace arkostracker 
{

LinkerBlock::LinkerBlock(AkgPattern pattern) noexcept :
    originalPattern(std::move(pattern))
{
}

bool LinkerBlock::operator==(const LinkerBlock& other) const noexcept
{
    // Do NOT compare the Track Indexes! But the Speed/Event Track indexes are.
    auto equal = ((originalPattern.getHeight() == other.originalPattern.getHeight())
            && (originalPattern.getEventTrackIndex() == other.originalPattern.getEventTrackIndex())
            && (originalPattern.getSpeedTrackIndex() == other.originalPattern.getSpeedTrackIndex())
        );

    if (equal) {
        // Compares each Transposition. The Track count must be the same (just a security, it shouldn't happen).
        const auto channelCount = originalPattern.getChannelCount();
        equal = (channelCount == other.originalPattern.getChannelCount());
        for (auto channelIndex = 0; equal && (channelIndex < channelCount); ++channelIndex) {
            equal = (originalPattern.getTransposition(channelIndex) == other.originalPattern.getTransposition(channelIndex));
        }
    }

    return equal;
}

bool LinkerBlock::operator!=(const LinkerBlock& other) const noexcept
{
    return !operator==(other);
}

int LinkerBlock::getHeight() const noexcept
{
    return originalPattern.getHeight();
}

int LinkerBlock::getTransposition(int channelIndex) const noexcept
{
    return originalPattern.getTransposition(channelIndex);
}

int LinkerBlock::getSpeedTrackIndex() const noexcept
{
    return originalPattern.getSpeedTrackIndex();
}

int LinkerBlock::getEventTrackIndex() const noexcept
{
    return originalPattern.getEventTrackIndex();
}

}   // namespace arkostracker

