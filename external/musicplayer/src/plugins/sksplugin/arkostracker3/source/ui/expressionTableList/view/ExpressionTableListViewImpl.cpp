#include "ExpressionTableListViewImpl.h"

#include "item/ExpressionListModel.h"

namespace arkostracker 
{

ExpressionTableListViewImpl::ExpressionTableListViewImpl(ListViewListener& pListViewListener,
                                                         ListItemListener& pListItemListener,
                                                         DataProvider& pDataProvider, bool pIsArpeggio) noexcept :
        ListViewImpl(pListViewListener, pListItemListener, pDataProvider,
                     std::make_unique<ExpressionListModel>(pListItemListener, pDataProvider,
                                                           pIsArpeggio ? DraggedItem::ItemType::arpeggio : DraggedItem::ItemType::pitch)),
        isArpeggio(pIsArpeggio)
{
}

juce::String ExpressionTableListViewImpl::getPromptRenameTitle() const noexcept
{
    return isArpeggio ? juce::translate("Rename arpeggio") : juce::translate("Rename pitch");
}

}   // namespace arkostracker
