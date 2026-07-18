#include "EditChannelsDialog.h"

#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const int EditChannelsDialog::togglesWidth = 50;

EditChannelsDialog::EditChannelsDialog(const int pChannelCount, const bool pMonophonicUsesCursorPosition, const int pMonophonicChannel, const std::set<int>& pPolyphonicChannels,
                                       std::function<void(bool, int, const std::set<int>&)> pOkCallback, std::function<void()> pCancelCallback) noexcept:
        ModalDialog(juce::translate("Edit test channels"), (togglesWidth * 6) + 60, 320,
                    [&] { onOkButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                    true, true),
        okCallback(std::move(pOkCallback)),
        cancelCallback(std::move(pCancelCallback)),
        monophonicUsesCursorPositionToggle(juce::translate("Monophonic uses cursor position")),
        monophonicToggles(juce::translate("Monophonic channel"), pChannelCount, false, true, togglesWidth,
                          [&](const int channelIndex) { return juce::String(channelIndex + 1); },
                          [&](const int channelIndex) { return (channelIndex == pMonophonicChannel); }),
        polyphonicToggles(juce::translate("Polyphonic channels"), pChannelCount, false, false, togglesWidth,
                          [&](const int channelIndex) { return juce::String(channelIndex + 1); },
                          [&](const int channelIndex) { return (pPolyphonicChannels.find(channelIndex) != pPolyphonicChannels.cend()); }),
        allButton(juce::translate("All"), juce::translate("Select all the channels")),
        oneButton(juce::translate("One"), juce::translate("Select one channel only"))
{
    addComponentToModalDialog(monophonicUsesCursorPositionToggle);
    addComponentToModalDialog(monophonicToggles);
    addComponentToModalDialog(polyphonicToggles);
    addComponentToModalDialog(allButton);
    addComponentToModalDialog(oneButton);

    const auto bounds = getUsableModalDialogBounds();
    const auto usableWidth = bounds.getWidth();
    const auto x = bounds.getX();
    const auto top = bounds.getY();
    constexpr auto groupsHeight = 80;
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto smallMargins = LookAndFeelConstants::margins / 2;
    constexpr auto buttonsWidth = 60;
    const auto buttonsHeight = getButtonsHeight();

    monophonicUsesCursorPositionToggle.setBounds(x, top, usableWidth, labelsHeight);
    monophonicToggles.setBounds(x, monophonicUsesCursorPositionToggle.getBottom() + smallMargins, usableWidth, groupsHeight);
    polyphonicToggles.setBounds(x, monophonicToggles.getBottom() + margins, monophonicToggles.getWidth(), groupsHeight);
    allButton.setBounds(polyphonicToggles.getRight() - buttonsWidth, polyphonicToggles.getBottom() + smallMargins, buttonsWidth, buttonsHeight);
    oneButton.setBounds(allButton.getX() - buttonsWidth - smallMargins, allButton.getY(), buttonsWidth, buttonsHeight);
    monophonicUsesCursorPositionToggle.onClick = [&] { onMonophonicUseCursorPositionChange(); };
    allButton.onClick = [&] { onAllButtonClicked(); };
    oneButton.onClick = [&] { onOneButtonClicked(); };

    monophonicUsesCursorPositionToggle.setToggleState(pMonophonicUsesCursorPosition, juce::NotificationType::sendNotification);
}

void EditChannelsDialog::onOkButtonClicked() const noexcept
{
    // There must be only one monophonic channel.
    const auto monophonicChannels = monophonicToggles.getToggledIndexes();
    if (monophonicChannels.size() != 1) {
        jassertfalse;           // Must not happen, must have been tested by the component.
        return;
    }
    const auto monophonicChannel = *monophonicChannels.cbegin();

    // There must be at least one polyphonic channels.
    const auto polyphonicChannels = polyphonicToggles.getToggledIndexes();
    if (polyphonicChannels.empty()) {
        jassertfalse;           // Must not happen, must have been tested by the component.
        return;
    }

    const auto monophonyUsesCursorPosition = monophonicUsesCursorPositionToggle.getToggleState();
    okCallback(monophonyUsesCursorPosition, monophonicChannel, polyphonicChannels);
}

void EditChannelsDialog::onCancelButtonClicked() const noexcept
{
    cancelCallback();
}

void EditChannelsDialog::onAllButtonClicked() noexcept
{
    polyphonicToggles.selectAll();
}

void EditChannelsDialog::onOneButtonClicked() noexcept
{
    polyphonicToggles.setSelected({ 1 });       // Sets the "middle" channel.
}

void EditChannelsDialog::onMonophonicUseCursorPositionChange() noexcept
{
    const auto ticked = monophonicUsesCursorPositionToggle.getToggleState();
    monophonicToggles.setEnabled(!ticked);
}

}   // namespace arkostracker
