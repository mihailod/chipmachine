#include "MidiImportPanel.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

MidiImportPanel::MidiImportPanel(const int pTrackCount, const int pOriginalSongPpq, const std::set<int>& pTrackIndexesWithNotes, std::function<void()> pOkCallback,
                                 std::function<void()> pCancelCallback) noexcept :
        ModalDialog(juce::translate("Import Midi file"), 400, 630,
                    std::move(pOkCallback), std::move(pCancelCallback), true, true, false),
        mixChannelsOverview(*this, pTrackCount, pTrackIndexesWithNotes, false),
        originalPpqLabel(),
        ppqLabel(juce::String(), juce::translate("Select a PPQ to use (the lower, the more accurate, but produces more lines). For best result, should be a divisible of the original ppq:")),
        ppqSlider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight),
        drumChannelLabel(juce::String(), juce::translate("Drum channel (10 is standard):")),
        drumChannelSlider(juce::Slider::SliderStyle::LinearHorizontal, juce::Slider::TextEntryBoxPosition::TextBoxRight),
        importVelocityToggleButton(juce::translate("Import note velocities"))
{
    const auto margins = LookAndFeelConstants::margins;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto usableWidth = getUsableModalDialogBounds().getWidth();

    const auto left = margins;

    mixChannelsOverview.setBounds(left, margins, usableWidth, MixChannelsOverview::preferredHeight);

    originalPpqLabel.setBounds(left, mixChannelsOverview.getBottom(), usableWidth, labelsHeight);
    ppqLabel.setBounds(left, originalPpqLabel.getBottom(), usableWidth, 80);
    ppqSlider.setBounds(left, ppqLabel.getBottom(), usableWidth, labelsHeight);

    drumChannelLabel.setBounds(left, ppqSlider.getBottom() + margins, usableWidth, labelsHeight);
    drumChannelSlider.setBounds(left, drumChannelLabel.getBottom(), usableWidth, labelsHeight);

    importVelocityToggleButton.setBounds(left, drumChannelSlider.getBottom() + margins, usableWidth, labelsHeight);

    originalPpqLabel.setText(juce::translate("Song PPQ (parts per quarter): " + juce::String(pOriginalSongPpq)),
                             juce::NotificationType::dontSendNotification);
    ppqSlider.setRange(12, 960 * 2, 1);
    ppqSlider.setValue((static_cast<double>(pOriginalSongPpq) / 4), juce::NotificationType::dontSendNotification);

    drumChannelSlider.setRange(1, 16, 1);
    drumChannelSlider.setValue(10, juce::NotificationType::dontSendNotification);

    importVelocityToggleButton.setToggleState(true, juce::NotificationType::dontSendNotification);

    addComponentToModalDialog(mixChannelsOverview);
    addComponentToModalDialog(originalPpqLabel);
    addComponentToModalDialog(ppqLabel);
    addComponentToModalDialog(ppqSlider);
    addComponentToModalDialog(drumChannelLabel);
    addComponentToModalDialog(drumChannelSlider);
    addComponentToModalDialog(importVelocityToggleButton);

    enableOkButton(pTrackCount);     // Use a non-virtual method to do that, calling onCheckedChannelCountChanged is dangerous.
}

void MidiImportPanel::enableOkButton(const int count) noexcept
{
    // No channels are clicked? Then OK shouldn't be enabled.
    setOkButtonEnable(count > 0);
}


// ImportPanel method implementations.
// ===================================================

std::unique_ptr<ImportConfiguration> MidiImportPanel::getConfiguration()
{
    // Builds the configuration from the stored elements. Let the MixChannelOverview object do the job.
    const auto channelMixerKeeper = mixChannelsOverview.getChannelMixerConfiguration();
    const auto midiConfiguration = MidiConfiguration(static_cast<int>(ppqSlider.getValue()), static_cast<int>(drumChannelSlider.getValue()),
                                               importVelocityToggleButton.getToggleState(), channelMixerKeeper);

    // Creates an Import Configuration from it.
    const auto importConfiguration = ImportConfiguration(nullptr, &midiConfiguration);
    return std::make_unique<ImportConfiguration>(importConfiguration);
}


// MixChannelsOverview::Listener method implementations.
// ========================================================

void MidiImportPanel::onCheckedChannelCountChanged(const int count)
{
    enableOkButton(count);
}

}   // namespace arkostracker
