#pragma once

#include <vector>

#include "../../../utils/Id.h"

namespace arkostracker
{

class Song;

/** Class to find which instruments are used in the whole song, including samples from events. Returns a count. */
class InstrumentCounter
{
public:
    class InstrumentResult
    {
    public:
        InstrumentResult(Id pId, juce::String pName, const int pIndex, const int pCount, const bool pIsSample) noexcept :
                id(std::move(pId)),
                name(std::move(pName)),
                index(pIndex),
                count(pCount),
                sample(pIsSample)
        {
        }

        Id getId() const noexcept
        {
            return id;
        }

        juce::String getName() const noexcept
        {
            return name;
        }

        int getIndex() const noexcept
        {
            return index;
        }

        int getCount() const noexcept
        {
            return count;
        }

        bool isSample() const noexcept
        {
            return sample;
        }

        bool operator==(const InstrumentResult& other) const
        {
            return id == other.id
                   && name == other.name
                   && index == other.index
                   && count == other.count
                   && sample == other.sample;
        }

        bool operator!=(const InstrumentResult& other) const
        {
            return !(*this == other);
        }

    private:
        Id id;
        juce::String name;
        int index;
        int count;
        bool sample;
    };

    /** Prevents instantiation. */
    InstrumentCounter() = delete;

    /**
     * @return the instruments used in the songs. They are all returned. More information is returned as a convenience.
     * Only the 0th is skipped.
     * @param song the Song.
     */
    static std::vector<InstrumentResult> countInstruments(const Song& song) noexcept;
};

}   // namespace arkostracker
