#pragma once

#include <vector>

#include <juce_graphics/juce_graphics.h>

namespace arkostracker 
{

/** Helps finds the dimensions and location of every meter. */
class MetersArranger
{
public:
    /** Prevents instantiation. */
    MetersArranger() = delete;

    /**
     * Finds the new bounds of the given Components.
     * @param channelCount how many items to locate/resize.
     * @param startX the X from where to draw.
     * @param startY the Y from where to draw.
     * @param availableWidth the parent width.
     * @param availableHeight the parent height.
     * @return the new location and size of each Component.
     */
    static std::vector<juce::Rectangle<int>> findBounds(int channelCount, int startX, int startY, int availableWidth, int availableHeight) noexcept;

private:
    static const int horizontalSpacing;
    static const int verticalSpacing;
    static const int itemMinimumWidth;
    static const int itemMinimumHeight;

    /** Holds the data of each kind of ratio. */
    class RatioData
    {
    public:
        RatioData(const int pItemWidth, const int pItemHeight, const int pItemCountPerLine) :
                ratio(static_cast<float>(pItemWidth) / static_cast<float>(pItemHeight)),
                itemWidth(pItemWidth),
                itemHeight(pItemHeight),
                itemCountPerLine(pItemCountPerLine)
        {
        }

        const float ratio;
        const int itemWidth;
        const int itemHeight;
        const int itemCountPerLine;
    };

    /**
     * Builds the location for each item.
     * @param itemCount how many item are present.
     * @param startX the origin X.
     * @param startY the origin Y.
     * @param itemWidth the width of each item. May be too small.
     * @param itemHeight the height of each item. May be too small.
     * @param itemCountPerLine how many items must be per line.
     * @return the locations.
     */
    static std::vector<juce::Rectangle<int>> arrange(int itemCount, int startX, int startY, int itemWidth, int itemHeight, int itemCountPerLine) noexcept;

    /**
     * Calculates a possible malus if the given size becomes too small.
     * @param itemWidth the item width.
     * @param itemHeight the item height.
     * @return 1.0 if no malus, else > 1.
     */
    static float calculateMalus(int itemWidth, int itemHeight) noexcept;
};

}   // namespace arkostracker
