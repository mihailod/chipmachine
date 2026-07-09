#pragma once

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Observer on the index of the selected Instrument. */
class SelectedInstrumentIndexObserver
{
public:
    /** Destructor. */
    virtual ~SelectedInstrumentIndexObserver() = default;

    /**
     * Called when the selected Instrument changed. The client might want to check whether the change is relevant or not.
     * @param selectedInstrumentId the ID of selected Instrument, but may be absent if no instrument is selected.
     * @param forceSingleSelection true to indicate the observers the selection must be unique. Useful after instruments are deleted, for example.
     */
    virtual void onSelectedInstrumentChanged(const OptionalId& selectedInstrumentId, bool forceSingleSelection) = 0;        // FIXME With ID, the force can probably be removed.
};

}   // namespace arkostracker

