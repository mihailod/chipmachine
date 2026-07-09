#pragma once

#include <set>

#include "../../components/dialogs/ModalDialog.h"
#include "../../components/GroupWithToggles.h"

namespace arkostracker 
{

/** A modal Dialog to edit the monophonic/polyphonic channels. */
class EditChannelsDialog final : public ModalDialog
{
public:
    /**
     * Constructor.
     * @param channelCount how many channels there are.
     * @param monophonicUsesCursorPosition true if monophonic uses the cursor position, and not a specific channel.
     * @param monophonicChannel the monophonic channel to show.
     * @param polyphonicChannels the polyphonic channels to show.
     * @param okCallback the callback when OK is clicked, with "use cursor position for monophony", the selected monophonic channel and the polyphonic channels.
     * @param cancelCallback the callback when Cancel is clicked.
     */
    EditChannelsDialog(int channelCount, bool monophonicUsesCursorPosition, int monophonicChannel, const std::set<int>& polyphonicChannels,
                       std::function<void(bool, int, const std::set<int>&)> okCallback, std::function<void()> cancelCallback) noexcept;

private:
    static const int togglesWidth;

    /** Called when the OK Button is clicked. */
    void onOkButtonClicked() const noexcept;
    /** Called when the Cancel Button is clicked. */
    void onCancelButtonClicked() const noexcept;

    /** Called when the All Button is clicked. */
    void onAllButtonClicked() noexcept;
    /** Called when the One Button is clicked. */
    void onOneButtonClicked() noexcept;

    /** Called when the state of the "monophonic use cursor position" changes. */
    void onMonophonicUseCursorPositionChange() noexcept;

    std::function<void(bool, int, const std::set<int>&)> okCallback;
    std::function<void()> cancelCallback;

    juce::ToggleButton monophonicUsesCursorPositionToggle;
    GroupWithToggles monophonicToggles;
    GroupWithToggles polyphonicToggles;
    juce::TextButton allButton;
    juce::TextButton oneButton;
};

}   // namespace arkostracker
