#include "ModImportPanel.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

ModImportPanel::ModImportPanel(int pTrackCount, std::function<void()> pOkCallback, std::function<void()> pCancelCallback) noexcept :
        ModalDialog(juce::translate("Import MOD file"), 400, 440,
                    std::move(pOkCallback), std::move(pCancelCallback), true, true, false),
        mixChannelsOverview(*this, pTrackCount, { 0, 1, 2 }, true),
        samplesToPsgInstrumentToggleButton(juce::translate("Replace samples with raw PSG instruments"))
{
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto usableWidth = getUsableModalDialogBounds().getWidth();

    const auto left = margins;

    mixChannelsOverview.setBounds(left, margins, usableWidth, MixChannelsOverview::preferredHeight);
    samplesToPsgInstrumentToggleButton.setBounds(left, mixChannelsOverview.getBottom() + margins, usableWidth, labelsHeight);

    addComponentToModalDialog(mixChannelsOverview);
    addComponentToModalDialog(samplesToPsgInstrumentToggleButton);
}

std::unique_ptr<ImportConfiguration> ModImportPanel::getConfiguration()
{
    // Builds the configuration from the stored elements. Let the MixChannelOverview object do the job.
    const auto channelMixerKeeper = mixChannelsOverview.getChannelMixerConfiguration();
    const auto modConfiguration = ModConfiguration(samplesToPsgInstrumentToggleButton.getToggleState(),
                                                   channelMixerKeeper.getTrackMixes(), channelMixerKeeper.getTrackIndexesToKeep());

    // Creates an Import Configuration from it.
    const auto importConfiguration = ImportConfiguration(&modConfiguration);
    return std::make_unique<ImportConfiguration>(importConfiguration);
}

void ModImportPanel::onCheckedChannelCountChanged(int count)
{
    enableOkButton(count);
}

void ModImportPanel::enableOkButton(int trackCount) noexcept
{
    // No channels are clicked? Then OK shouldn't be enabled.
    setOkButtonEnable(trackCount > 0);
}


}   // namespace arkostracker

