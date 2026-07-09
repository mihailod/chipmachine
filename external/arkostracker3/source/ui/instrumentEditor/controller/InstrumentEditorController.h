#pragma once

#include "../../utils/ParentViewLifeCycleAware.h"

namespace arkostracker 
{

/** The virtual Controller for the Instrument Editor. It holds any of the "real" editor (PSG, sample etc. editor). */
class InstrumentEditorController : public ParentViewLifeCycleAware
{
public:
    /** Destructor. */
    ~InstrumentEditorController() override = default;       // FIXME Should probably be removed.

    /** Gets the keyboard focus to the view. */
    virtual void getKeyboardFocus() noexcept = 0;
};

}   // namespace arkostracker

