#include "InstrumentTableListViewImpl.h"

#include "item/InstrumentListModel.h"
#include "../controller/InstrumentDataController.h"

namespace arkostracker 
{

InstrumentTableListViewImpl::InstrumentTableListViewImpl(ListViewListener& pListViewListener, ListItemListener& pListItemListener,
                                                         DataProvider& pDataProvider, InstrumentDataController& pInstrumentDataController) noexcept :
        ListViewImpl(pListViewListener, pListItemListener, pDataProvider,
                     std::make_unique<InstrumentListModel>(pListItemListener, pDataProvider, pInstrumentDataController))
{
}

juce::String InstrumentTableListViewImpl::getPromptRenameTitle() const noexcept
{
    return juce::translate("Rename instrument");
}


}   // namespace arkostracker

