#pragma once

#include "../../lists/view/ListViewImpl.h"

namespace arkostracker 
{

class DataProvider;
class ListViewListener;
class ListItemListener;

/** Implementation of the List View for Expressions. */
class ExpressionTableListViewImpl : public ListViewImpl
{
public:
    /**
     * Constructor.
     * @param listener the listener of the event of this View.
     * @param listItemListener the listener of the event of the items.
     * @param dataProvider object that can return some data about the Expressions.
     * @param isArpeggio true if the View is about Arpeggio, false if about Pitch.
     */
    ExpressionTableListViewImpl(ListViewListener& listener, ListItemListener& listItemListener, DataProvider& dataProvider, bool isArpeggio) noexcept;

    juce::String getPromptRenameTitle() const noexcept override;

private:
    bool isArpeggio;
};

}   // namespace arkostracker
