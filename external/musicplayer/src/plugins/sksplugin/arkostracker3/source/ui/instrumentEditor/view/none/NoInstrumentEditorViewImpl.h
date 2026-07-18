#pragma once

#include "NoInstrumentEditorView.h"

namespace arkostracker 
{

/** Implementation of the "none selected" Instrument Editor View. */
class NoInstrumentEditorViewImpl final : public NoInstrumentEditorView
{
public:
    /** Constructor. */
    NoInstrumentEditorViewImpl() noexcept;

    void resized() override;
    void getKeyboardFocus() noexcept override;

private:
    juce::Label label;
};

}   // namespace arkostracker
