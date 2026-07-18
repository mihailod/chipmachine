#pragma once

#include "../../../utils/OccurrenceMap.h"

namespace arkostracker 
{

class Id;
class Song;

/** Class that counts how many instruments there are, in a Subsong. Rather use InstrumentCounter, which is more complete. */
class InstrumentSimpleCounter
{
public:
    /** Prevents instantiation. */
    InstrumentSimpleCounter() = delete;

    /**
     * @return an occurrence map linking an instrument index to how many times it was used.
     * @param song the Song.
     * @param subsongId the ID of the Subsong.
     */
    static OccurrenceMap<int> countInstrument(const Song& song, const Id& subsongId) noexcept;
};

}   // namespace arkostracker
