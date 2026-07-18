#pragma once

#include <set>
#include <unordered_set>

namespace arkostracker 
{

class Id;

/** Observer to a change in the Linker (positions, markers, transpositions, etc.). */
class LinkerObserver
{
public:
    /** Flags about what has changed. */
    enum What : unsigned char
    {
        marker = 1U << 0U,
        transposition = 1U << 1U,
        patternIndex = 1U << 2U,
        /** Color, name. */
        patternMetadata = 1U << 3U,
        positionHeight = 1U << 4U,
        /** When a Track is linked/unlinked in a Pattern. */
        link = 1U << 5U,

        all = marker | transposition | patternIndex | positionHeight | link
    };

    /** Destructor. */
    virtual ~LinkerObserver() = default;

    /**
     * Called when a Linker item (position) has changed, but not its index (nothing added or deleted).
     * @param subsongId the id of the Subsong where the change happened.
     * @param index the index of the Linker.
     * @param whatChanged what has changed. See the What enum flags.
     */
    virtual void onLinkerPositionChanged(const Id& subsongId, int index, unsigned int whatChanged) = 0;

    /**
     * Called when Linker items (position) has changed, but not their index (nothing added or deleted).
     * @param subsongId the id of the Subsong where the change happened.
     * @param indexes the indexes of the Linker.
     * @param whatChanged what has changed. See the What enum flags.
     */
    virtual void onLinkerPositionsChanged(const Id& subsongId, const std::unordered_set<int>& indexes, unsigned int whatChanged) = 0;

    /**
     * Called when Linker items has changed, including the loop. Use this when one is added, deleted, moved, etc.
     * @param subsongId the id of the Subsong where the change happened.
     * @param highlightedItems the indexes that are either new (on do), or put back (on undo). This helps select the items right after the action.
     */
    virtual void onLinkerPositionsInvalidated(const Id& subsongId, const std::set<int>& highlightedItems) = 0;

    /**
     * Called when Patterns metadata has changed (color/name change, etc.).
     * @param subsongId the id of the Subsong where the change happened.
     * @param patternIndex the index of the pattern. It must exist.
     * @param whatChanged what has changed. See the What enum flags.
     */
    virtual void onLinkerPatternInvalidated(const Id& subsongId, int patternIndex, unsigned int whatChanged) = 0;
};

}   // namespace arkostracker
