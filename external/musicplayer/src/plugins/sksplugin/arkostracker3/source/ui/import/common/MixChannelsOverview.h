#pragma once

#include <memory>
#include <set>
#include <vector>



#include "../common/MixChannelsPanel.h"

namespace arkostracker 
{

class ChannelMixerKeeper;

/** Generic component, used by MOD and Midi import for example, for the user to select which channels must be mixed into others, and which channels to keep. */
class MixChannelsOverview : public juce::Component,
                            public juce::Button::Listener
{
public:
    static const int preferredHeight = 320;                             // The height this Component should be.

    /** Listener to this class events. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the user has changed how many channels are checked. The client class might want to forbid "OK" to be clicked, for example.
         * @param count how many channels are checked.
         */
        virtual void onCheckedChannelCountChanged(int count) = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to this class events.
     * @param channelCount how many channels there are in the input Song.
     * @param channelIndexesToSet the indexes to "tick".
     * @param tickThreeFirst if true, automatically tick the first three, regardless of the indexes that are set with the other parameter.
     */
    MixChannelsOverview(Listener& listener, int channelCount, std::set<int> channelIndexesToSet, bool autoTickThreeFirst) noexcept;

    /**
     * Updates the given configuration file, thanks to the UI data.
     * @param channelMixerKeeper the object to fill.
     */
    void updateConfigurationFromUi(ChannelMixerKeeper& channelMixerKeeper) const noexcept;

    /** @return a mixer configuration, from the current states. */
    ChannelMixerKeeper getChannelMixerConfiguration() const noexcept;


    // Component method implementations.
    // ===========================================
    void resized() override;


    // Button::Listener method implementations.
    // ===================================================
    void buttonClicked(juce::Button*) override;


private:
    static const int labelsHeight = 25;     // Generic height of Labels.
    static const juce::String indexTag;

    /** Fills the channels to keep. */
    void fillChannelsToKeep() noexcept;

    /** One of the "channels to keep" ToggleButton has changed. The "OK" Button must be disabled if no channel is selected. */
    void onChannelToKeepButtonsStateChanged() noexcept;

    /** Updates the area where the mixed channels are displayed, according to the stored data. */
    void updateMixedChannelsArea() noexcept;

    /** Called when the "add mix channel" is clicked. Should open a pop-up. */
    void onAddMixChannelButtonClicked() noexcept;

    /**
        Called when the "delete mix entry" is clicked.
        @param index the index of the Button/entry (>=0).
    */
    void onDeleteMixEntryButtonClicked(int index) noexcept;
    /**
        Called when the "edit mix entry" is clicked.
        @param index the index of the Button/entry (>=0).
    */
    void onEditMixEntryButtonClicked(int index) noexcept;

    /** Updates the text of the PSG count label, according to the selected channels. */
    void updatePsgCountLabel() noexcept;

    /** Called when OK is clicked on the Mix Channels Panel. */
    void onMixChannelsPanelOkClicked() noexcept;
    /** Called when the Mix Channels Panel is clicked. */
    void onMixChannelsPanelCanceled() noexcept;

    /** Listener to the "channels to keep" ToggleButtons. */
    class ChannelsToKeepButtonListener : public juce::Button::Listener
    {
    public:
        explicit ChannelsToKeepButtonListener(MixChannelsOverview& pParent) : parent(pParent) {}
        void buttonClicked(juce::Button*) override {
            parent.onChannelToKeepButtonsStateChanged();
        }
    private:
        MixChannelsOverview& parent;
    };

    /** Listener to the "delete one mix entry" ToggleButtons. */
    class DeleteMixEntryListener : public juce::Button::Listener
    {
    public:
        explicit DeleteMixEntryListener(MixChannelsOverview& pParent) : parent(pParent) {}
        void buttonClicked(juce::Button* button) override {
            // Extracts the index of the Button.
            auto index = button->getProperties().operator[](indexTag);
            parent.onDeleteMixEntryButtonClicked(index);
        }
    private:
        MixChannelsOverview& parent;
    };

    /** Listener to the "edit one mix entry" ToggleButtons. */
    class EditMixEntryListener : public juce::Button::Listener
    {
    public:
        explicit EditMixEntryListener(MixChannelsOverview& pParent) : parent(pParent) {}
        void buttonClicked(juce::Button* button) override {
            // Extracts the index of the Button.
            auto index = button->getProperties().operator[](indexTag);
            parent.onEditMixEntryButtonClicked(index);
        }
    private:
        MixChannelsOverview& parent;
    };

    Listener& listener;                                 // Listener to this class events.
    int channelCount;                                   // How many channels are available in the source MODule.
    const std::set<int> inputIndexesToSet;              // The track indexes that are set at the beginning.
    const bool autoTickThreeFirst;                      // If true, automatically tick the first three, regardless of the indexes that are set with the other parameter.

    juce::Label detectedChannelLabel;                   // Indicates how many channels were detected.

    juce::GroupComponent mixGroup;                      // Group for the "channels to mix".
    juce::Viewport mixedChannelsViewport;               // Viewport where the "channels to mix" are shown.
    Component mixedChannelsComponentsHolder;            // Where the "channels to mix" Checkboxes are added.
    std::vector<std::unique_ptr<Component>> mixedChannelsLabels;                   // Labels for each entry, stored for deletion.
    std::vector<std::unique_ptr<juce::TextButton>> mixedChannelsDeleteButtons;     // Delete Button for each entry, stored for deletion.
    std::vector<std::unique_ptr<juce::TextButton>> mixedChannelsEditButtons;       // Edit Button for each entry, stored for deletion.
    juce::Label noMixedChannelsLabel;                   // Label shown when there are no mixed channels.
    juce::TextButton addMixChannelButton;               // Button to add a mix of channel.

    juce::GroupComponent keepGroup;                     // Group for the "channels to keep".
    juce::Viewport keptChannelsViewport;                // Viewport where the "channels to keep" are shown.
    Component keptChannelsComponentsHolder;             // Where the "channels to keep" Checkboxes are added.

    juce::Label psgCountLabel;                          // Indicates how many PSGs will be needed.

    ChannelsToKeepButtonListener channelsToKeepButtonListener;              // Listener to the "channels to keep" ToggleButtons.
    DeleteMixEntryListener deleteMixEntryListener;                          // Listener to the "delete mix entry" ToggleButtons.
    EditMixEntryListener editMixEntryListener;                              // Listener to the "edit mix entry" ToggleButtons.

    std::vector<std::unique_ptr<juce::ToggleButton>> channelsToKeepToggleButtons; // Stores the ToggleButtons of the channels to keep. Contains on/off buttons!

    std::vector<std::pair<int, int>> sourceToDestinationChannelsToMix;      // List of source/destination channels to mix.

    std::unique_ptr<MixChannelsPanel> mixChannelsPanel;                     // Modal Dialog to select which channel to mix.
    int indexMixChannelToEditOrAdd;                                         // Index of the channel that is being added/mixed.
};

}   // namespace arkostracker

