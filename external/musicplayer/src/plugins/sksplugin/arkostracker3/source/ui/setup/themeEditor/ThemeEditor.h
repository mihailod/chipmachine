#pragma once

#include <memory>
#include <utility>

#include <juce_gui_extra/juce_gui_extra.h>

#include "../../components/dialogs/ModalDialog.h"
#include "../../components/ProfileUser.h"
#include "../../lookAndFeel/ThemeColors.h"
#include "ColorItem.h"

namespace arkostracker 
{

class LookAndFeelChanger;
class MainController;
class PreferencesManager;

/** Dialog to edit the Look'n'feel. */
class ThemeEditor : public ModalDialog,
                    public ColorItem::Listener,
                    public juce::ChangeListener,
                    public ProfileUser
{
public:
    /**
     * Constructor.
     * @param lookAndFeelChanger knows how to change the look and feel.
     * @param okCallback called when the OK Button is clicked. No cancel callback, it will do the same thing.
     */
    ThemeEditor(LookAndFeelChanger& lookAndFeelChanger, std::function<void()> okCallback) noexcept;

    /** Destructor. */
    ~ThemeEditor() override;

    // ColorItem::Listener method implementations.
    // =================================================
    void onColorSwatchClicked(int colorId) override;

    // ChangeListener method implementations.
    // =================================================
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    // ProfileUser method implementations.
    // =================================================
    void addProfileViewToParent(juce::Component& componentToAdd) noexcept override;
    std::vector<int> getProfileIds() const noexcept override;
    ProfileDataInChooser getProfileDataInChooser(int profileId) const noexcept override;
    void displayProfile(int profileId) noexcept override;
    int getCurrentlyStoredProfileId() const noexcept override;
    void storeCurrentProfileId(int profileId) noexcept override;
    void persistProfile(StoredProfile& profile) noexcept override;
    int findNewFreeCustomId() const noexcept override;
    std::unique_ptr<StoredProfile> getStoredCustomProfile(int profileId) const noexcept override;
    void storeProfileIfWriteable(int profileId) noexcept override;
    bool isProfileInReadOnly(int profileId) const noexcept override;
    void renameStoredProfile(int profileId, const juce::String& newName) noexcept override;
    void deleteStoredProfile(int profileId) noexcept override;

private:
    /** A category and its displayed String. */
    class DisplayableCategory {
    public:
        DisplayableCategory(juce::String pDisplayedCategory, const ColorCategory pColorCategory) :
                displayedCategory(std::move(pDisplayedCategory)),
                colorCategory(pColorCategory)
        {
        }
        const juce::String displayedCategory;
        const ColorCategory colorCategory;
    };

    /**
     * Changes the color, notifies all the Components to apply it.
     * @param colorId the color id to target.
     * @param newColor the color to set.
     */
    void setNewColorAndNotify(int colorId, juce::Colour newColor) noexcept;

    /** Closes the modal dialog where the color selector is. */
    void closeColorSelectorModalDialog() noexcept;

    /** Called when the Ok button is clicked. */
    void onOkButtonClicked() noexcept;

    /** Called when the OK button of the Color Selection Dialog is clicked. */
    void onColorSelectorDialogOk() noexcept;
    /** Called when the Cancel button of the Color Selection Dialog is clicked. */
    void onColorSelectorDialogCancelled() noexcept;

    /** Called when the Export button is clicked. */
    void onExportButtonClicked() noexcept;
    /** Called when the Import button is clicked. */
    void onImportButtonClicked() noexcept;

    std::function<void()> okCallback;

    const std::map<int, ThemeColor>& colorIdToThemeColor;       // Links a colorID to its color data.
    LookAndFeelChanger& lookAndFeelChanger;
    juce::TreeView treeView;
    std::unique_ptr<juce::TreeViewItem> rootItem;               // Where all the Category nodes are added. Invisible.
    int selectedColourId;                                       // The colorId that is selected in the ColourSelector.
    juce::Colour editedColor;                                   // Stored to allow cancellation.
    PreferencesManager& preferencesManager;

    juce::TextButton exportButton;
    juce::TextButton importButton;

    std::unique_ptr<ModalDialog> colorSelectorModalDialog;
    std::unique_ptr<juce::ColourSelector> colorSelector;        // Put in the modal.
};

}   // namespace arkostracker
