#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../utils/Id.h"
#include "PatternItemView.h"
#include "../../components/LoopBar.h"
#include "PlayLineView.h"

namespace arkostracker 
{

class SongController;
class LinkerController;

/**
 * Abstract class holding all the Views of the Linker. It is as dumb as possible, and will transmit all the
 * UI interaction to the Controller.
 */
class LinkerView : public juce::Component
{
public:
    /**
     * Constructor.
     * @param linkerController the Controller, to which give all the events.
     */
    explicit LinkerView(LinkerController& linkerController) noexcept;

    /**
     * Ask for all the items to be refreshed. This is the same calling refreshItems with an empty set.
     * In all cases, the count is checked again, and a scrolling may be performed if no item is visible.
     * @param itemsData all the items to display.
     * @param loopData the loop data.
     * @param playLineData the "play line" data.
     * @param positionToMarker map linking a position to its marker.
     */
    virtual void refreshAllItems(const std::vector<PatternItemView::DisplayedData>& itemsData, const LoopBar::DisplayData& loopData,
                                 const PlayLineView::DisplayedData& playLineData, const std::unordered_map<int, juce::String>& positionToMarker) noexcept = 0;

    /**
     * Makes sure that the given position is visible.
     * @param position where to go. May be invalid.
     */
    virtual void scrollToItem(int position) noexcept = 0;

    /**
     * The user wants to edit the items.
     * @param positionIndexes the position indexes.
     */
    virtual void editItems(const std::set<int>& positionIndexes) noexcept = 0;

protected:
    LinkerController& linkerController;
};

}   // namespace arkostracker
