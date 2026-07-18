#include "MainControllerObservers.h"

namespace arkostracker 
{

MainControllerObservers::MainControllerObservers() noexcept :
        channelMuteStateObservers(),
        selectedArpeggioIndexObservers(),
        selectedPitchIndexObservers(),
        selectedInstrumentIndexObservers(),
        patternViewerMetadataObservers(),
        generalDataObservers(),
        selectedBlockObservers(),
        cursorObservers()
{
}

Observers<SelectedExpressionIndexObserver>& MainControllerObservers::getSelectedExpressionIndexObservers(const bool isArpeggio)
{
    return isArpeggio ? selectedArpeggioIndexObservers : selectedPitchIndexObservers;
}

Observers<ChannelMuteObserver>& MainControllerObservers::getChannelMuteStateObservers()
{
    return channelMuteStateObservers;
}

Observers<PatternViewerMetadataObserver>& MainControllerObservers::getPatternViewerMetadataObservers()
{
    return patternViewerMetadataObservers;
}

Observers<SelectedInstrumentIndexObserver>& MainControllerObservers::getSelectedInstrumentIndexObservers()
{
    return selectedInstrumentIndexObservers;
}

Observers<SelectedInstrumentIndexesObserver>& MainControllerObservers::getSelectedInstrumentIndexesObservers()
{
    return selectedInstrumentIndexesObserver;
}

Observers<GeneralDataObserver>& MainControllerObservers::getGeneralDataObservers()
{
    return generalDataObservers;
}

Observers<SelectedBlockObserver>& MainControllerObservers::getBlockSelectionObservers()
{
    return selectedBlockObservers;
}

Observers<CursorObserver>& MainControllerObservers::getCursorObservers()
{
    return cursorObservers;
}

}   // namespace arkostracker
