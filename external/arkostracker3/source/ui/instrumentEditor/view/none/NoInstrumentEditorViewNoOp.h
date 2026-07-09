#pragma once

#include "NoInstrumentEditorView.h"

namespace arkostracker 
{

/** Implementation of the "none selected" Instrument Editor View, that does nothing. */
class NoInstrumentEditorViewNoOp final : public NoInstrumentEditorView
{
public:
    void getKeyboardFocus() noexcept override;
};



}   // namespace arkostracker

