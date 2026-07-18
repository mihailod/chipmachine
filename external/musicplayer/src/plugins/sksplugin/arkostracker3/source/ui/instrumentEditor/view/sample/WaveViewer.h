#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../../song/instrument/sample/Sample.h"
#include "../../../../utils/OptionalValue.h"
#include "WaveViewerDisplayedData.h"

namespace arkostracker
{

/** Shows a sample. It does not anything else than the wave. */
class WaveViewer final : public juce::Component
{
public:
    /** Constructor. */
    WaveViewer() noexcept;

    /**
     * Sets the viewed sample, refreshes the UI. If nothing is new, nothing happens.
     * @param sample the sample to view.
     * @param displayedData what to display.
     */
    void setSampleAndRefresh(std::shared_ptr<Sample> sample, const WaveViewerDisplayedData& displayedData) noexcept;

    /**
     * @return the X from the given sample offset. If out of bounds, returns empty.
     * @param sampleOffset the sample offset.
     * @param allowReachWidth true to allow reaching the width.
     */
    OptionalInt getXFromSampleOffset(int sampleOffset, bool allowReachWidth = false) const noexcept;

    /** @return the zoom factor according to the currently visible start/end and view width. */
    double getZoomFactor() const noexcept;


    // Component method implementations.
    // ===================================================
    void paint(juce::Graphics& g) override;

private:
    /**
     * @return the zoom factor.
     * @param startViewedOffset the offset of the viewed start.
     * @param pastEndViewedOffset the offset of the viewed end.
     * @param width the width of this Component.
     */
    static double getZoomFactor(int startViewedOffset, int pastEndViewedOffset, int width) noexcept;

    std::shared_ptr<Sample> sample;
    WaveViewerDisplayedData displayedData;
};

}   // namespace arkostracker
