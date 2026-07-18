#pragma once

#include "../../components/dialogs/ModalDialog.h"
#include "../ImportPanel.h"
#include "../common/MixChannelsOverview.h"

namespace arkostracker 
{

/**
 * Modal dialog box for the MOD import. Allows the user to define which channels to mix, and which ones to keep at the end.
 * It uses the MixChannelsOverview component to show almost everything on the dialog. This allows this component to be shared.
 */
class ModImportPanel : public ModalDialog,
                       public ImportPanel,
                       public MixChannelsOverview::Listener            // To know if none/some channels are checked, and prevents OK to be clicked.
{
public:

    /**
     * Constructor.
     * @param trackCount how many tracks are available in the MIDI file.
     * @param okCallback called when the OK Button is clicked.
     * @param cancelCallback called when the Cancel Button is clicked.
    */
    ModImportPanel(int trackCount, std::function<void()> okCallback, std::function<void()> cancelCallback) noexcept;

    // ImportPanel method implementations.
    // ===================================================
    std::unique_ptr<ImportConfiguration> getConfiguration() override;

    // MixChannelsOverview::Listener method implementations.
    // ========================================================
    void onCheckedChannelCountChanged(int count) override;

private:
    /**
     * Sets the OK button enabled or not according to the given track count.
     * @param trackCount the track count.
     */
    void enableOkButton(int trackCount) noexcept;

    MixChannelsOverview mixChannelsOverview;                // Shows the mix/tracks to keep. Does almost all the job actually!
    juce::ToggleButton samplesToPsgInstrumentToggleButton;  // To convert instrument to PSG instruments.
};


}   // namespace arkostracker

