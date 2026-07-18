#include "LinkDialog.h"

#include <utility>

#include "../../../../controllers/SongController.h"
#include "../../../../utils/NumberUtil.h"
#include "item/GotoItem.h"
#include "item/LinkHeaderItem.h"

namespace arkostracker
{

const juce::String LinkDialog::unnamedTrackName = juce::translate("Unnamed");    // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

LinkDialog::LinkDialog(SongController& pSongController, Id pSubsongId, const int pPositionIndex, const int pChannelIndex,
    std::function<void(int targetPositionIndex, int targetChannelIndex)> pLinkCallback,
    std::function<void(int targetPositionIndex, int targetChannelIndex)> pGotoCallback,
    std::function<void(int targetPositionIndex, int targetChannelIndex)> pUnlinkCallback,
    std::function<void()> pCancelCallback) noexcept :
        ModalDialog(juce::translate("Edit link for track"), 500, 300,
                [&] { onOkClicked(); },
                [&] { onCancelled(); }
        ),
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        positionIndex(pPositionIndex),
        channelIndex(pChannelIndex),
        namedTracksResults(),
        linkCallback(std::move(pLinkCallback)),
        gotoCallback(std::move(pGotoCallback)),
        unlinkCallback(std::move(pUnlinkCallback)),
        cancelCallback(std::move(pCancelCallback)),
        topMessageLabel(),
        group(),
        treeView(),
        rootItem(std::make_unique<LinkHeaderItem>(juce::String(), false, 0, false,        // Nothing to display in the root item.
            [] (int) {})),
        unlinkButton(juce::translate("Unlink"))
{
    topMessageLabel.setJustificationType(juce::Justification::topLeft);
    addComponentToModalDialog(topMessageLabel);
    setOkButtonText(juce::translate("Close"));

    buildUi();
}

LinkDialog::~LinkDialog()
{
    treeView.setRootItem(nullptr);
}

void LinkDialog::onOkClicked() const noexcept
{
    cancelCallback();
}

void LinkDialog::onCancelled() const noexcept
{
    cancelCallback();
}

void LinkDialog::buildUi() noexcept
{
    auto linkState = LinkState::none;
    songController.performOnConstSubsong(subsongId, [&](const Subsong& subsong) {
        linkState = subsong.getTrackLinkState(positionIndex, channelIndex);
    });

    // Linked or not?
    switch (linkState) {
        default:
            jassertfalse;       // Not managed?
            [[fallthrough]];
        case LinkState::none:
            buildNonLinkedUi();
            break;
        case LinkState::link:
            buildLinkedUi();
            break;
        case LinkState::linkedTo:
            buildLinkedToUi();
            break;
    }
}

void LinkDialog::buildNonLinkedUi() noexcept
{
    // Finds all the names tracks, and where. There may be none.
    namedTracksResults = LinkedTrackHelper::findNamedTracks(*songController.getSong(), subsongId, positionIndex, channelIndex);

    const auto areNamedTracksPresent = !namedTracksResults.empty();

    // Special message if there are no named tracks.
    const auto topMessage = areNamedTracksPresent ? juce::translate("You can link this track to the following named tracks:") :
        juce::translate("No named tracks were found. Only named tracks can be linked to.");

    displayUi(topMessage, true, false);
}

void LinkDialog::buildLinkedToUi() noexcept
{
    // Finds all the names tracks, and where. There may be none.
    namedTracksResults = LinkedTrackHelper::findLinkedToTracks(*songController.getSong(), subsongId, positionIndex, channelIndex);

    displayUi(juce::translate("This track is linked to by these other tracks:"),
        false, true);     // Cannot link to anything else, it is linked to, it would be too complex. But don't want to see our track in the result.
}

void LinkDialog::buildLinkedUi() noexcept
{
    // Finds all the tracks that are using the current one. The current one is filtered by the method.
    namedTracksResults = LinkedTrackHelper::findLinkedTracks(*songController.getSong(), subsongId, positionIndex, channelIndex);
    if (namedTracksResults.empty()) {
        jassert(!namedTracksResults.empty());       // How can this be linked to, then?
        return;
    }
    auto trackName = namedTracksResults.at(0).getTrackName();
    if (trackName.isEmpty()) {
        trackName = unnamedTrackName;     // May happen if a named Track is renamed afterwards.
    }

    const auto topMessage = juce::translate("This track is linked to [" + trackName + "]. It is also used here:");
    displayUi(topMessage, false, false);

    // Adds the Unlink button.
    const auto margins = LookAndFeelConstants::margins;
    unlinkButton.setBounds(margins, getButtonsY(), 80, getButtonsHeight());
    addComponentToModalDialog(unlinkButton);
    unlinkButton.onClick = [&] { onUnlinkButtonClicked(); };
}

void LinkDialog::displayUi(const juce::String& topMessage, const bool allowLinking, const bool openItems) noexcept
{
    const auto bounds = getUsableModalDialogBounds();
    const auto left = bounds.getX();
    const auto top = bounds.getY();
    const auto width = bounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto smallMargins = LookAndFeelConstants::smallMargins;

    topMessageLabel.setText(topMessage, juce::NotificationType::dontSendNotification);
    topMessageLabel.setBounds(left, top, width, labelsHeight);

    if (namedTracksResults.empty()) {
        return;     // Stops here, nothing to display.
    }

    constexpr auto groupHeight = 200;
    const auto groupBounds = juce::Rectangle(left, topMessageLabel.getBottom(), width, groupHeight);
    group.setBounds(groupBounds);
    treeView.setBounds(groupBounds.reduced(smallMargins * 2, smallMargins * 4));
    addComponentToModalDialog(group);
    addComponentToModalDialog(treeView);

    // Creates the tree view. Creates a first node encompassing all the others, but hides it.
    treeView.setDefaultOpenness(false);
    treeView.setRootItemVisible(false);
    treeView.setRootItem(rootItem.get());

    auto headerId = -1;      // A simple index.
    for (const auto& namedTrack : namedTracksResults) {
        ++headerId;

        const auto& originalLocations =  namedTrack.getLocations();
        // Filter the current Track.
        std::vector<TrackLocation> locations;
        std::copy_if(originalLocations.cbegin(), originalLocations.cend(), std::back_inserter(locations), [&](const TrackLocation& item) {
            return (item.getPositionIndex() != positionIndex) || (item.getChannelIndex() != channelIndex);
        });

        const auto useCount = locations.size();
        // Don't show this item if it is empty.
        if (useCount == 0) {
            continue;
        }

        const auto countMessage = (useCount == 1U) ? juce::translate(("used once.")) :
            (juce::translate("used ") + juce::String(useCount) + juce::translate(" times."));

        // If linking allowed, make sure you cannot select your own track. Have to pre-parse the locations.
        auto showLinkButton = allowLinking;
        if (showLinkButton) {
            for (const auto& location : locations) {
                if ((location.getPositionIndex() == positionIndex) && (location.getChannelIndex() == channelIndex)) {
                    showLinkButton = false;
                    break;
                }
            }
        }

        auto trackName = namedTrack.getTrackName();
        if (trackName.isEmpty()) {
            trackName = unnamedTrackName;
        }
        auto subItem = std::make_unique<LinkHeaderItem>(trackName + " - " + countMessage, true, headerId, showLinkButton,
            [&] (const int clickedIndex) {
            onLinkButtonClicked(clickedIndex);
        });
        subItem->mightContainSubItems();
        subItem->setOpen(openItems);

        // Adds the Goto item to each Item.
        for (const auto& location : locations) {
            const auto itemPositionIndex = location.getPositionIndex();
            const auto itemChannelIndex = location.getChannelIndex();
            const auto& text = juce::translate("Position ") + juce::String::toHexString(itemPositionIndex) +
                               juce::translate(", channel ") + juce::String(location.getChannelIndex() + 1);

            auto colorItem = std::make_unique<GotoItem>(text, *this, itemPositionIndex, itemChannelIndex);
            subItem->addSubItem(colorItem.release());
        }

        rootItem->addSubItem(subItem.release());
    }
}

void LinkDialog::onLinkButtonClicked(const int headerId) const noexcept
{
    jassert(headerId < static_cast<int>(namedTracksResults.size()));

    const auto& result = namedTracksResults.at(static_cast<size_t>(headerId)).getLocations().at(0U);
    linkCallback(result.getPositionIndex(), result.getChannelIndex());
}

void LinkDialog::onUnlinkButtonClicked() const noexcept
{
    unlinkCallback(positionIndex, channelIndex);
}


// GotoItem::Listener method implementations.
// ==========================================

void LinkDialog::onGotoClicked(const int clickedPositionIndex, const int clickedChannelIndex)
{
    gotoCallback(clickedPositionIndex, clickedChannelIndex);
}

}   // namespace arkostracker
