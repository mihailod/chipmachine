#pragma once

#include "../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Observer on the index of the selected Expression. */
class SelectedExpressionIndexObserver
{
public:
    /** Destructor. */
    virtual ~SelectedExpressionIndexObserver() = default;

    /**
     * Called when the selected Expressions changed. The client might want to check whether the change is relevant or not.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param selectedExpressionId the selected ID. May be empty, if nothing is selected.
     * @param forceSingleSelection true to indicate the observers the selection must be unique. Useful after expressions are deleted, for example.
     */
    virtual void onSelectedExpressionChanged(bool isArpeggio, const OptionalId& selectedExpressionId, bool forceSingleSelection) = 0;
};

}   // namespace arkostracker
