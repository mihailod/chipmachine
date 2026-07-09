#pragma once

#include "../utils/Observers.h"
#include "observers/CursorObserver.h"
#include "observers/GeneralDataObserver.h"
#include "observers/PatternViewerMetadataObserver.h"
#include "observers/SelectedBlockObserver.h"
#include "observers/SelectedExpressionIndexObserver.h"
#include "observers/SelectedInstrumentIndexObserver.h"
#include "observers/SelectedInstrumentIndexesObserver.h"

namespace arkostracker 
{

class ChannelMuteObserver;
class SelectedExpressionIndexObserver;

/** Allows clients to make observations on some generic items. */
class MainControllerObservers
{
public:
    friend class MainControllerImpl;

    /** Constructor. */
    MainControllerObservers() noexcept;

    /** @return observers interested when the selected expression changes. */
    Observers<SelectedExpressionIndexObserver>& getSelectedExpressionIndexObservers(bool isArpeggio);
    Observers<ChannelMuteObserver>& getChannelMuteStateObservers();
    Observers<SelectedInstrumentIndexObserver>& getSelectedInstrumentIndexObservers();
    Observers<SelectedInstrumentIndexesObserver>& getSelectedInstrumentIndexesObservers();
    Observers<PatternViewerMetadataObserver>& getPatternViewerMetadataObservers();
    Observers<GeneralDataObserver>& getGeneralDataObservers();
    Observers<SelectedBlockObserver>& getBlockSelectionObservers();
    Observers<CursorObserver>& getCursorObservers();

private:
    Observers<ChannelMuteObserver> channelMuteStateObservers;                       // Observes the Channel Mute state change.
    Observers<SelectedExpressionIndexObserver> selectedArpeggioIndexObservers;      // Observes the currently selected Arpeggio index (not the Arpeggio data).
    Observers<SelectedExpressionIndexObserver> selectedPitchIndexObservers;         // Observes the currently selected Pitch index (not the Pitch data).
    Observers<SelectedInstrumentIndexObserver> selectedInstrumentIndexObservers;    // Observes the currently selected Instrument index.
    Observers<SelectedInstrumentIndexesObserver> selectedInstrumentIndexesObserver; // Observes the selected Instrument indexes.
    Observers<PatternViewerMetadataObserver> patternViewerMetadataObservers;        // Observes the PV metadata (setup).
    Observers<GeneralDataObserver> generalDataObservers;                            // Observes the general data (layout).
    Observers<SelectedBlockObserver> selectedBlockObservers;                        // Observes the selected blocks.
    Observers<CursorObserver> cursorObservers;                                      // Observes the cursor "in X".

};

}   // namespace arkostracker
