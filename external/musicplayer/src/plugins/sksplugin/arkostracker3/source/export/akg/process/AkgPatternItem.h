#pragma once

namespace arkostracker 
{

/** Holder of an index of a Track and its transposition. This class is immutable. */
class AkgPatternItem
{
public:
    /**
        Constructor.
        @param trackIndex the index of the Track (>=0).
        @param transposition the transposition.
    */
    explicit AkgPatternItem(int trackIndex = 0, int transposition = 0) noexcept;

    /** Returns the Track index. */
    int getTrackIndex() const noexcept
    {
        return trackIndex;
    }

    /** Returns the transposition. */
    int getTransposition() const noexcept
    {
        return transposition;
    }

    bool operator==(const AkgPatternItem&) const noexcept;

private:
    int trackIndex;             // The index of the Track (>=0).
    int transposition;          // The transposition.
};

}   // namespace arkostracker

