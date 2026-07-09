#include "MixChannelsOverview.h"

#include <utility>

#include "../../../import/common/ChannelMixerKeeper.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

const juce::String MixChannelsOverview::indexTag = "tag";               // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

MixChannelsOverview::MixChannelsOverview(Listener& pListener, int pChannelCount, std::set<int> pChannelIndexesToSet, bool pAutoTickThreeFirst) noexcept :
    listener(pListener),
    channelCount(pChannelCount),
    inputIndexesToSet(std::move(pChannelIndexesToSet)),
    autoTickThreeFirst(pAutoTickThreeFirst),
    detectedChannelLabel(juce::String(), juce::String(channelCount) + juce::translate(" channels were detected.")),
    mixGroup(juce::String(), juce::translate("Channels to mix")),
    mixedChannelsViewport(),
    mixedChannelsComponentsHolder(),
    mixedChannelsLabels(),
    mixedChannelsDeleteButtons(),
    mixedChannelsEditButtons(),
    noMixedChannelsLabel(juce::String(), juce::translate("<none>")),
    addMixChannelButton("+"),

    keepGroup(juce::String(), juce::translate("Channels to keep")),
    keptChannelsViewport(),
    keptChannelsComponentsHolder(),
    psgCountLabel(),

    channelsToKeepButtonListener(*this),
    deleteMixEntryListener(*this),
    editMixEntryListener(*this),

    channelsToKeepToggleButtons(),

    sourceToDestinationChannelsToMix(),

    mixChannelsPanel(),
    //mixChannelsPanelCallback(*this),
    indexMixChannelToEditOrAdd(0)
{
    // By default, mixes Track 4 to 3. --> Maybe not, after all.
    //sourceToDestinationChannelsToMix.push_back(std::pair<int, int>(3, 2));

    addMixChannelButton.addListener(this);

    addAndMakeVisible(detectedChannelLabel);
    addAndMakeVisible(mixGroup);
    addAndMakeVisible(mixedChannelsViewport);
    addAndMakeVisible(keepGroup);
    addAndMakeVisible(keptChannelsViewport);
    addAndMakeVisible(psgCountLabel);
}


// Component method implementations.
// ===========================================

void MixChannelsOverview::resized()
{
    const auto width = getWidth();
    const auto margins = LookAndFeelConstants::margins;

    // Sets up the text at the top.
    detectedChannelLabel.setBounds(0, 0, width, labelsHeight);

    // Sets up the "channels to mix" group.
    const int mixGroupHeight = 140;
    const int marginTopInsideGroup = 15;
    const int keepGroupHeight = 105;
    noMixedChannelsLabel.setSize(70, labelsHeight);         // Defines the size, the position will be later.
    addMixChannelButton.setSize(40, labelsHeight);

    mixGroup.setBounds(0, detectedChannelLabel.getBottom() + margins, width, mixGroupHeight);
    mixedChannelsViewport.setBounds(mixGroup.getX() + margins, mixGroup.getY() + marginTopInsideGroup, mixGroup.getWidth() - 2 * margins,
                                    mixGroup.getHeight() - 2 * marginTopInsideGroup);
    mixedChannelsViewport.setScrollBarsShown(true, false, false, false);
    mixedChannelsComponentsHolder.setBounds(0, 0, mixedChannelsViewport.getWidth(), noMixedChannelsLabel.getBottom());  // Only holds the "none" for now.
    mixedChannelsViewport.setViewedComponent(&mixedChannelsComponentsHolder, false);
    mixedChannelsComponentsHolder.addAndMakeVisible(noMixedChannelsLabel);
    mixedChannelsComponentsHolder.addAndMakeVisible(addMixChannelButton);
    updateMixedChannelsArea();

    // Sets up the "channels to keep" group.
    const auto keepGroupY = mixGroup.getBottom() + margins;
    keepGroup.setBounds(0, keepGroupY, mixGroup.getWidth(), keepGroupHeight);
    keptChannelsViewport.setBounds(keepGroup.getX() + margins, keepGroup.getY() + marginTopInsideGroup, keepGroup.getWidth() - 2 * margins,
                                   keepGroup.getHeight() - 2 * marginTopInsideGroup);
    keptChannelsViewport.setScrollBarsShown(true, false, false, false);
    keptChannelsViewport.setViewedComponent(&keptChannelsComponentsHolder, false);

    fillChannelsToKeep();

    // Sets up the "psg count" label at the bottom.
    psgCountLabel.setBounds(0, keptChannelsViewport.getBottom() + 2 * margins, width, labelsHeight);

    updatePsgCountLabel();
}


// Button::Listener method implementations.
// ===================================================

void MixChannelsOverview::buttonClicked(juce::Button* button)
{
    if (button == &addMixChannelButton) {
        onAddMixChannelButtonClicked();
    } else {
        jassertfalse;           // Shouldn't happen.
    }
}


// ===========================================

void MixChannelsOverview::fillChannelsToKeep() noexcept
{
    channelsToKeepToggleButtons.clear();

    // Creates and shows as many Checkboxes as there are channels.
    const auto buttonWidth = 42;
    const auto buttonHeight = 25;
    const auto buttonsPerLine = (keptChannelsViewport.getWidth() - 20) / buttonWidth;    // Offset to remove the scroll bar.
    auto x = 0;
    auto y = 0;
    auto latestY = y;
    for (auto channelIndex = 0; channelIndex  < channelCount; ++channelIndex) {
        const auto channelNumber = channelIndex + 1;

        std::unique_ptr toggleButton(std::make_unique<juce::ToggleButton>(juce::String(channelNumber)));

        toggleButton->setBounds(x, y, buttonWidth, buttonHeight);
        keptChannelsComponentsHolder.addAndMakeVisible(*toggleButton);
        latestY = y;

        // Advances horizontally, and maybe vertically.
        if ((channelNumber % buttonsPerLine) == 0) {
            x = 0;
            y += buttonHeight;
        } else {
            x += buttonWidth;
        }

        // Ticks the channel? Yes if auto tick the first three, or if the channel is in the ones we want to tick.
        if ((autoTickThreeFirst && (channelIndex < 3)) || (inputIndexesToSet.find(channelIndex) != inputIndexesToSet.cend())) {
            toggleButton->setToggleState(true, juce::dontSendNotification);
        }

        toggleButton->addListener(&channelsToKeepButtonListener);

        // Stores the view for deletion.
        channelsToKeepToggleButtons.push_back(std::move(toggleButton));
    }

    // Finally, we can set the height of Component inside the Viewport.
    keptChannelsComponentsHolder.setBounds(0, 0, keptChannelsViewport.getWidth(), latestY + buttonHeight);
}

void MixChannelsOverview::onChannelToKeepButtonsStateChanged() noexcept
{
    // Counts how many checked Toggles there are.
    int checkedCount = 0;
    for (auto& button : channelsToKeepToggleButtons) {
        if (button->getToggleState()) {
            ++checkedCount;
        }
    }

    // Notifies the client.
    listener.onCheckedChannelCountChanged(checkedCount);

    updatePsgCountLabel();
}

void MixChannelsOverview::updatePsgCountLabel() noexcept
{
    // How many checked ToggleButton?
    int channelToKeepCount = 0;
    for (auto& button : channelsToKeepToggleButtons) {
        if (button->getToggleState()) {
            ++channelToKeepCount;
        }
    }

    int psgCount = (channelToKeepCount / 3) + ((channelToKeepCount % 3) > 0 ? 1 : 0);

    psgCountLabel.setText(juce::translate("PSG count: ") + juce::String(psgCount) + ".", juce::NotificationType::dontSendNotification);
}

void MixChannelsOverview::updateMixedChannelsArea() noexcept
{
    const auto margins = LookAndFeelConstants::margins;

    // Erases all the already there Views. Raw but much simpler!
    mixedChannelsLabels.clear();
    mixedChannelsDeleteButtons.clear();
    mixedChannelsEditButtons.clear();

    int y = 0;

    // No mixed channels yet? If yes, displays the "none".
    noMixedChannelsLabel.setVisible(sourceToDestinationChannelsToMix.empty());

    const int labelWidth = 70;
    const int deleteButtonWidth = 30;
    const int editButtonWidth = 60;
    int index = 0;
    for (auto& sourceAndDestination : sourceToDestinationChannelsToMix) {
        // Creates the label.
        juce::String text = juce::String(sourceAndDestination.first + 1) + juce::translate(" to ") + juce::String(sourceAndDestination.second + 1);     // +1 because the data is an index.
        std::unique_ptr label(std::make_unique<juce::Label>(juce::String(), text));
        label->setBounds(0, y, 100, labelsHeight);

        // Creates the Buttons with their index and callback.
        auto deleteButton = std::make_unique<juce::TextButton>(juce::translate("X"));
        deleteButton->setBounds(labelWidth, y, deleteButtonWidth, labelsHeight);
        deleteButton->addListener(&deleteMixEntryListener);
        deleteButton->getProperties().set(indexTag, index);

        auto editButton = std::make_unique<juce::TextButton>(juce::translate("Edit"));
        editButton->setBounds(labelWidth + deleteButtonWidth + margins, y, editButtonWidth, labelsHeight);
        editButton->addListener(&editMixEntryListener);
        editButton->getProperties().set(indexTag, index);

        // Makes the views visible.
        mixedChannelsComponentsHolder.addAndMakeVisible(*label);
        mixedChannelsComponentsHolder.addAndMakeVisible(*deleteButton);
        mixedChannelsComponentsHolder.addAndMakeVisible(*editButton);

        // Stores them.
        mixedChannelsLabels.push_back(std::move(label));
        mixedChannelsDeleteButtons.push_back(std::move(deleteButton));
        mixedChannelsEditButtons.push_back(std::move(editButton));

        y += labelsHeight + 5;      // Nicer with a little margin.
        ++index;
    }

    // Locates the "add" button.
    addMixChannelButton.setTopLeftPosition(labelWidth, y);
    // Resizes the Viewport.
    mixedChannelsComponentsHolder.setSize(mixedChannelsComponentsHolder.getWidth(), y + labelsHeight);
}

void MixChannelsOverview::onAddMixChannelButtonClicked() noexcept
{
    jassert(mixChannelsPanel == nullptr);       // Shouldn't be present!

    indexMixChannelToEditOrAdd = (int)sourceToDestinationChannelsToMix.size();

    // Opens the Dialog box to mix the channels.
    mixChannelsPanel = std::make_unique<MixChannelsPanel>(channelCount, 3, 2,        // Default indexes are given.
                                                          [&] { onMixChannelsPanelOkClicked(); },
                                                          [&] { onMixChannelsPanelCanceled(); });
}

void MixChannelsOverview::onDeleteMixEntryButtonClicked(int index) noexcept
{
    jassert(index < (int)sourceToDestinationChannelsToMix.size());
    jassert(index < (int)mixedChannelsDeleteButtons.size());

    sourceToDestinationChannelsToMix.erase(sourceToDestinationChannelsToMix.begin() + index, sourceToDestinationChannelsToMix.begin() + index + 1);

    updateMixedChannelsArea();
}

void MixChannelsOverview::onEditMixEntryButtonClicked(int index) noexcept
{
    indexMixChannelToEditOrAdd = index;

    // Opens the Dialog box to mix the channels.
    auto& channels = sourceToDestinationChannelsToMix.at((size_t)index);
    mixChannelsPanel = std::make_unique<MixChannelsPanel>(channelCount, channels.first, channels.second,
                                                          [&] { onMixChannelsPanelOkClicked(); },
                                                          [&] { onMixChannelsPanelCanceled(); });
}

void MixChannelsOverview::updateConfigurationFromUi(ChannelMixerKeeper& channelMixerKeeper) const noexcept
{
    // Adds the tracks to keep.
    int index = 0;
    for (auto& button : channelsToKeepToggleButtons) {
        if (button->getToggleState()) {
            channelMixerKeeper.addTrackToKeep(index);
        }
        ++index;
    }

    // Adds the track mixes.
    for (auto& sourceToDestination : sourceToDestinationChannelsToMix) {
        channelMixerKeeper.addTrackMix(sourceToDestination.first, sourceToDestination.second);
    }
}

ChannelMixerKeeper MixChannelsOverview::getChannelMixerConfiguration() const noexcept
{
    ChannelMixerKeeper channelMixerKeeper;
    updateConfigurationFromUi(channelMixerKeeper);
    return channelMixerKeeper;
}

void MixChannelsOverview::onMixChannelsPanelOkClicked() noexcept
{
    // Retrieves the data of the Panel before closing it.
    const auto sourceChannel = mixChannelsPanel->getSourceChannel();
    const auto destinationChannel = mixChannelsPanel->getDestinationChannel();
    // Replaces the existing value, or adds a new one if the index is superior to the size.
    jassert(indexMixChannelToEditOrAdd <= (int)sourceToDestinationChannelsToMix.size());

    auto newPair(std::pair(sourceChannel, destinationChannel));
    if (indexMixChannelToEditOrAdd == (int)sourceToDestinationChannelsToMix.size()) {
        // Addition.
        sourceToDestinationChannelsToMix.push_back(newPair);
    } else {
        // Edition. Replaces the value.
        sourceToDestinationChannelsToMix.erase(sourceToDestinationChannelsToMix.begin() + indexMixChannelToEditOrAdd,
                                                      sourceToDestinationChannelsToMix.begin() + indexMixChannelToEditOrAdd + 1);
        sourceToDestinationChannelsToMix.insert(sourceToDestinationChannelsToMix.begin() + indexMixChannelToEditOrAdd,
                                                       newPair);
    }

    // Closes the Dialog.
    mixChannelsPanel.reset();

    updateMixedChannelsArea();
}

void MixChannelsOverview::onMixChannelsPanelCanceled() noexcept
{
    // Simply closes the Dialog, that's all.
    mixChannelsPanel.reset();
}


}   // namespace arkostracker

