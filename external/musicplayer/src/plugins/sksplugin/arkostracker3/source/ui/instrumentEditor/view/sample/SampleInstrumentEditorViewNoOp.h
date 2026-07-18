#pragma once

#include "SampleInstrumentEditorView.h"

namespace arkostracker 
{

/** A Sample Instrument Editor that does nothing. */
class SampleInstrumentEditorViewNoOp final : public SampleInstrumentEditorView
{
public:
    void refreshAllUi(SamplePart samplePart, WaveViewerDisplayedData waveViewerDisplayedData) noexcept override;
    void refreshUi(WaveViewerDisplayedData waveViewerDisplayedData) noexcept override;
};

}   // namespace arkostracker

