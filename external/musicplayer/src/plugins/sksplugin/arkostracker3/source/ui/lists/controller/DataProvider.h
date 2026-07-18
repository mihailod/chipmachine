#pragma once

#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Provides the data to fill the list. */
class DataProvider
{
public:
    /** Destructor. */
    virtual ~DataProvider() = default;

    /** @return how many items there are. */
    virtual int getItemCount() const noexcept = 0;
    /**
     * @return the text to display, for the given row.
     * @param rowNumber the row number (>=0).
     */
    virtual juce::String getItemTitle(int rowNumber) const noexcept = 0;

    /** @return what to display before each number (for example "Arp" or "Pitch"). */
    virtual juce::String getNumberPrefix() const noexcept = 0;

    /** @return the selected index, or absent if there is none. */
    virtual OptionalInt getSelectedIndex() const noexcept = 0;
};

}   // namespace arkostracker
