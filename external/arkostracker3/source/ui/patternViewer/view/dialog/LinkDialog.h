#pragma once

#include "../../../../business/link/LinkedTrackHelper.h"
#include "../../../../utils/Id.h"
#include "../../../components/dialogs/ModalDialog.h"
#include "item/GotoItem.h"

namespace arkostracker
{

class SongController;
class Pattern;

/** Dialog to link/unlink a (normal) Track. */
class LinkDialog final : public ModalDialog,
                         public GotoItem::Listener
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller.
     * @param subsongId the ID of the Subsong where the position is.
     * @param positionIndex the position on which we want to link/unlink.
     * @param channelIndex the channel index on which we want to link/unlink.
     * @param linkCallback called when Link is clicked.
     * @param gotoCallback called when Goto is clicked.
     * @param unlinkCallback called when Unlink is clicked.
     * @param cancelCallback called when the dialog is cancelled.
     */
    LinkDialog(SongController& songController, Id subsongId, int positionIndex, int channelIndex,
        std::function<void(int targetPositionIndex, int targetChannelIndex)> linkCallback,
        std::function<void(int targetPositionIndex, int targetChannelIndex)> gotoCallback,
        std::function<void(int targetPositionIndex, int targetChannelIndex)> unlinkCallback,
        std::function<void()> cancelCallback) noexcept;

    /** Destructor. */
    ~LinkDialog() override;

    // GotoItem::Listener method implementations.
    // ==========================================
    void onGotoClicked(int positionIndex, int channelIndex) override;

private:
    static const juce::String unnamedTrackName;

    /** Called when OK is clicked. */
    void onOkClicked() const noexcept;
    /** Called when the dialog is cancelled. */
    void onCancelled() const noexcept;

    /** Builds the UI according to the track being linked or not. */
    void buildUi() noexcept;
    void buildNonLinkedUi() noexcept;
    /** Builds a UI when the Track is linked to (i.e. referred to.). */
    void buildLinkedToUi() noexcept;
    void buildLinkedUi() noexcept;

    /**
     * Called when a Link Button is clicked.
     * @param headerId the ID of the clicked header.
     */
    void onLinkButtonClicked(int headerId) const noexcept;

    /**
     * Displays the UI, according of the parameters and the *already* found linked Tracks.
     * @param topMessage the top message.
     * @param allowLinking true to show the Link button in the Track Group.
     * @param openItems true to open the items.
     */
    void displayUi(const juce::String& topMessage, bool allowLinking, bool openItems) noexcept;

    /** Exits and notifies the parent. */
    void onUnlinkButtonClicked() const noexcept;

    SongController& songController;
    const Id subsongId;
    const int positionIndex;
    const int channelIndex;
    std::vector<LinkedTrackHelper::SearchResult> namedTracksResults;    // To simplify the click management, stored.

    std::function<void(int targetPositionIndex, int targetChannelIndex)> linkCallback;
    std::function<void(int targetPositionIndex, int targetChannelIndex)> gotoCallback;
    std::function<void(int targetPositionIndex, int targetChannelIndex)> unlinkCallback;
    std::function<void()> cancelCallback;

    juce::Label topMessageLabel;

    juce::GroupComponent group;
    juce::TreeView treeView;
    std::unique_ptr<juce::TreeViewItem> rootItem;               // Where all the Category nodes are added. Invisible.
    juce::TextButton unlinkButton;
};

}   // namespace arkostracker
