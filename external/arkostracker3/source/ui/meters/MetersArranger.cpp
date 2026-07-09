#include "MetersArranger.h"

#include <algorithm>

#include "../../utils/PsgValues.h"

namespace arkostracker 
{

const int MetersArranger::horizontalSpacing = 2;
const int MetersArranger::verticalSpacing = 2;
const int MetersArranger::itemMinimumWidth = 30;
const int MetersArranger::itemMinimumHeight = itemMinimumWidth;

std::vector<juce::Rectangle<int>> MetersArranger::findBounds(const int channelCount, const int startX, const int startY, const int availableWidth, const int availableHeight) noexcept
{
    if (channelCount == 0) {
        jassertfalse;       // No channel? Shouldn't happen.
        return { };
    }

    const int psgCount = PsgValues::getPsgCount(channelCount);

    // Any space to use?
    if ((availableWidth <= 0) || (availableHeight <= 0)) {
        return { };
    }

    // What if everything is put on one line only?
    const int itemWidthIfOneLine = (availableWidth - horizontalSpacing * static_cast<int>(channelCount - 1)) / channelCount;
    const int itemHeightIfOneLine = availableHeight;
    const int itemCountPerLineIfOneLine = channelCount;
    RatioData ratioDataIfOneLine(itemWidthIfOneLine, itemHeightIfOneLine, itemCountPerLineIfOneLine);

    // What if everything is put on as many lines there are channels?
    const int itemWidthIfRows = availableWidth;
    const int itemHeightIfRows = (availableHeight - verticalSpacing * static_cast<int>(channelCount - 1)) / channelCount;
    constexpr int itemCountPerLineIfRows = 1;
    RatioData ratioDataIfRows(itemWidthIfRows, itemHeightIfRows, itemCountPerLineIfRows);

    // What if everything is put in one psg per line? Do that even if one PSG, this will give the same result, no need to test anything.
    const int itemWidthIfOnePsgPerLine = (availableWidth - horizontalSpacing * static_cast<int>(PsgValues::channelCountPerPsg - 1)) / PsgValues::channelCountPerPsg;
    const int itemHeightIfOnePsgPerLine = (availableHeight - verticalSpacing * static_cast<int>(psgCount - 1)) / psgCount;
    constexpr int itemCountPerLineIfOnePsgPerLine = PsgValues::channelCountPerPsg;
    RatioData ratioDataIfOnePsgPerLine(itemWidthIfOnePsgPerLine, itemHeightIfOnePsgPerLine, itemCountPerLineIfOnePsgPerLine);

    // Uses the ratio that is closest to 1 (square item).
    auto ratios = std::vector { &ratioDataIfOneLine, &ratioDataIfRows, &ratioDataIfOnePsgPerLine };
    std::sort(ratios.begin(), ratios.end(), [] (const RatioData* ratio1, const RatioData* ratio2) {
        // A malus is applied if a dimension becomes too small.
        const auto malus1 = calculateMalus(ratio1->itemWidth, ratio1->itemHeight);
        const auto malus2 = calculateMalus(ratio2->itemWidth, ratio2->itemHeight);

        const float diff1 = std::abs(ratio1->ratio - 1.0F) * malus1;
        const float diff2 = std::abs(ratio2->ratio - 1.0F) * malus2;
        return (diff1 < diff2);
    });

    const auto& ratioToUse = ratios.front();

    // Builds the location for the Components.
    return arrange(channelCount, startX, startY, ratioToUse->itemWidth, ratioToUse->itemHeight, ratioToUse->itemCountPerLine);
}

std::vector<juce::Rectangle<int>> MetersArranger::arrange(const int itemCount, const int startX, const int startY,
                                                          int itemWidth, int itemHeight, const int itemCountPerLine) noexcept
{
    std::vector<juce::Rectangle<int>> result;
    result.reserve(static_cast<size_t>(itemCount));

    // Corrects the possible sizes, even if this means the items will not fit the parent.
    itemWidth = std::max(itemWidth, itemMinimumWidth);
    itemHeight = std::max(itemHeight, itemMinimumHeight);

    for (auto itemIndex = 0; itemIndex < itemCount; ++itemIndex) {
        const auto lineIndex = itemIndex / itemCountPerLine;
        const auto y = startY + (itemHeight + verticalSpacing) * lineIndex;

        const auto rowIndex = itemIndex % itemCountPerLine;
        const auto x = startX + (itemWidth + horizontalSpacing) * rowIndex;

        result.emplace_back(x, y, itemWidth, itemHeight);
    }

    return result;
}

float MetersArranger::calculateMalus(const int itemWidth, const int itemHeight) noexcept
{
    // Adds a malus for each dimension that is too low.
    auto ratio = 1.0F;
    constexpr float correctionRatio = 1.5F;
    if (itemWidth < itemMinimumWidth) {
        ratio += correctionRatio;
    }
    if (itemHeight < itemMinimumHeight) {
        ratio += correctionRatio;
    }

    return ratio;
}

}   // namespace arkostracker
