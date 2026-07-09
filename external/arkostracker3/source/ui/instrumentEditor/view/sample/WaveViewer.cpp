#include "WaveViewer.h"

#include "../../../../utils/NumberUtil.h"
#include "../../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker
{

WaveViewer::WaveViewer() noexcept:
        sample(),
        displayedData()
{
    setOpaque(true);
}

void WaveViewer::setSampleAndRefresh(std::shared_ptr<Sample> pSample, const WaveViewerDisplayedData& pDisplayedData) noexcept
{
    // Any change? If not, don't do anything.
    if ((sample == pSample) && (displayedData == pDisplayedData)) {
        return;
    }

    sample = std::move(pSample);
    displayedData = pDisplayedData;

    repaint();
}

void WaveViewer::paint(juce::Graphics& g)
{
    const auto backgroundColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(static_cast<int>(LookAndFeelConstants::Colors::dialogBackground));

    const auto startViewedOffset = displayedData.getStartViewedOffset();
    const auto pastEndViewedOffset = displayedData.getPastEndViewedOffset();
    const auto multiplier = displayedData.getMultiplier();

    // No sample? Then shows nothing.
    if ((sample == nullptr) || (sample->getLength() <= 0) || juce::exactlyEqual(multiplier, 0.0) || (pastEndViewedOffset <= startViewedOffset)) {
        g.fillAll(backgroundColor);
        return;
    }

    const auto verticalLinesColor = juce::LookAndFeel::getDefaultLookAndFeel().findColour(juce::Label::ColourIds::textColourId);
    const auto levelsColor = verticalLinesColor.contrasting(0.7F).withAlpha(0.7F);
    g.fillAll(backgroundColor);

    const auto width = getWidth();
    const auto height = getHeight();
    const auto middleY = static_cast<float>(height) / 2.0F;
    const auto stepY = middleY / 256.0F;       // 8-bit sample, so 256 possibilities.

    const auto sampleLength = static_cast<double>(sample->getLength());
    const auto lastSampleOffset = static_cast<int>(sampleLength) - 1;
    const auto zoomFactor = getZoomFactor(startViewedOffset, pastEndViewedOffset, width);

    // Draws the vertical lines of the sample.
    g.setColour(verticalLinesColor);
    auto currentOffsetInSample = static_cast<double>(startViewedOffset);
    for (auto x = 0; x < width; ++x) {
        const auto nextOffsetInSample = currentOffsetInSample + zoomFactor;

        // To reduce "flickering" when scrolling, instead of reading one sample, gets the max on the several samples that one pixel represents.
        // However, this is not perfect. Still better than AT2...
        auto readSample = sample->operator[](std::min(static_cast<int>(currentOffsetInSample), lastSampleOffset));    // Security, overflow can happen.
        for (auto i = static_cast<int>(currentOffsetInSample), iAfterMax = static_cast<int>(nextOffsetInSample); i < iAfterMax; ++i) {
            const auto offsetToRead = std::min(i, lastSampleOffset);    // Security, overflow can happen.
            const auto newReadSample = sample->operator[](offsetToRead);
            if (static_cast<int>(newReadSample) > static_cast<int>(readSample)) {
                readSample = newReadSample;
            }
        }

        // Multiplies by the multiplier, with a limit.
        const auto finalSample = NumberUtil::correctNumber(static_cast<double>(readSample) * multiplier, 0.0, 255.0);

        const auto middleHeightLine = static_cast<float>(finalSample * stepY);
        g.drawVerticalLine(x, middleY - middleHeightLine, middleY + middleHeightLine);

        currentOffsetInSample = nextOffsetInSample;
    }

    // Draws the scales.
    g.setColour(levelsColor);
    g.fillRect(0, 0, width, 1);
    g.fillRect(0, height / 2, width, 1);
    g.fillRect(0, height - 1, width, 1);
}

OptionalInt WaveViewer::getXFromSampleOffset(const int sampleOffset, const bool allowReachWidth) const noexcept
{
    const auto startViewedOffset = displayedData.getStartViewedOffset();
    const auto width = getWidth();

    const auto zoomFactor = getZoomFactor();

    const auto x = static_cast<int>(static_cast<double>(sampleOffset - startViewedOffset) / zoomFactor);

    return ((x >= 0) && ((x < width) || (allowReachWidth && (x == width)))) ? x : OptionalInt();
}

double WaveViewer::getZoomFactor() const noexcept
{
    const auto startViewedOffset = displayedData.getStartViewedOffset();
    const auto pastEndViewedOffset = displayedData.getPastEndViewedOffset();
    const auto width = getWidth();

    return getZoomFactor(startViewedOffset, pastEndViewedOffset, width);
}

double WaveViewer::getZoomFactor(const int startViewedOffset, const int pastEndViewedOffset, const int width) noexcept
{
    return static_cast<double>(pastEndViewedOffset - startViewedOffset) / static_cast<double>(width);
}

}   // namespace arkostracker
