#pragma once

#include "../../../../utils/Id.h"

namespace arkostracker
{

class Song;

class FeatureDetector       // TODO TU this.
{
public:
    class Result
    {
    public:
        Result(const bool pAreDigidrumsUsed, const bool pAreSidUsed, const bool pArePsgInstrumentsUsed,
            const bool pAreSampleInstrumentsUsed, const int pMaximumChannelCount) :
                areDigidrumsUsed(pAreDigidrumsUsed),
                areSidUsed(pAreSidUsed),
                arePsgInstrumentsUsed(pArePsgInstrumentsUsed),
                areSampleInstrumentsUsed(pAreSampleInstrumentsUsed),
                maximumChannelCount(pMaximumChannelCount)
        {
        }

        bool areDigidrumsUsed;
        bool areSidUsed;
        bool arePsgInstrumentsUsed;
        bool areSampleInstrumentsUsed;
        int maximumChannelCount;
    };

    /** Prevents instantiation. */
    FeatureDetector() = delete;

    /**
     * @return the features of the given Song.
     * @param song the Song.
     */
    static Result perform(const Song& song) noexcept;

private:
    /**
     * @return true if digidrums are used.
     * @param song the Song.
     * @param subsongId the Subsong to explore.
     */
    static bool determineAreDigidrumsUsed(const Song& song, const Id& subsongId) noexcept;

    /**
     * @return true if SIDs are used.
     * @param song the Song.
     */
    static bool determineAreSidUsed(const Song& song) noexcept;

    /**
     * @return true if PSGs instruments are used (instrument 0 is excluded), and if sample instruments are used.
     * Only Tracks are tested, not special Tracks.
     * @param song the Song.
     */
    static std::pair<bool, bool> determineArePsgAndSamplesUsed(const Song& song) noexcept;
};

}   // namespace arkostracker
