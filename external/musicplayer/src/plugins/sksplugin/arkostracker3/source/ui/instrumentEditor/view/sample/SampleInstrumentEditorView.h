#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../../song/instrument/sample/SamplePart.h"
#include "WaveViewerDisplayedData.h"

namespace arkostracker 
{

/** An abstract Sample Instrument Editor. */
class SampleInstrumentEditorView : public juce::Component
{
public:
    /**
     * Refreshes the UI with the given SamplePart.
     * @param samplePart the sample part to display.
     * @param waveViewerDisplayedData how to display the data.
     */
    virtual void refreshAllUi(SamplePart samplePart, WaveViewerDisplayedData waveViewerDisplayedData) noexcept = 0;

    /**
     * Refreshes the UI without the sample changing..
     * @param waveViewerDisplayedData how to display the data.
     */
    virtual void refreshUi(WaveViewerDisplayedData waveViewerDisplayedData) noexcept = 0;
};

}   // namespace arkostracker

