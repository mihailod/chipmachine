#pragma once

namespace arkostracker
{

/** Observer on the index of the selected Instruments. */
class SelectedInstrumentIndexesObserver
{
public:
    /** Destructor. */
    virtual ~SelectedInstrumentIndexesObserver() = default;

    /**
     * Called when the selected Instrument changed. The client might want to check whether the change is relevant or not.
     * @param selectedInstrumentIndexes the indexes of the selected Instruments, the last being the selected.
     */
    virtual void onSelectedInstrumentsChanged(const std::vector<int>& selectedInstrumentIndexes) = 0;
};

}   // namespace arkostracker
