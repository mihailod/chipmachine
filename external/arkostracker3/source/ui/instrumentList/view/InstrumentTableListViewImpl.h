#pragma once



#include "../../lists/view/ListViewImpl.h"

namespace arkostracker 
{

class DataProvider;
class ListViewListener;
class ListItemListener;
class InstrumentDataController;

/** Implementation of the List View for Instruments. */
class InstrumentTableListViewImpl final : public ListViewImpl
{
public:
    /**
     * Constructor.
     * @param listener the listener of the event of this View.
     * @param listItemListener the listener of the event of the items.
     * @param dataProvider object that can return some data about the Expressions.
     * @param instrumentDataController provides specialized data about the instrument, and can be sent events about it.
     */
    InstrumentTableListViewImpl(ListViewListener& listener, ListItemListener& listItemListener, DataProvider& dataProvider,
                                InstrumentDataController& instrumentDataController) noexcept;

    juce::String getPromptRenameTitle() const noexcept override;
};

}   // namespace arkostracker

