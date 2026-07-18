#pragma once

#include <unordered_map>

#include "StoredProfile.h"
#include "../../utils/WithParent.h"
#include "ButtonWithImage.h"
#include "dialogs/TextFieldDialog.h"

namespace arkostracker 
{

class ModalDialog;

/** Holds the data of a profile, as shown in the Chooser. */
class ProfileDataInChooser
{
public:
    ProfileDataInChooser(juce::String pDisplayedName, const bool pReadOnly) :
            displayedName(std::move(pDisplayedName)),
            readOnly(pReadOnly)
    {
    }

    const juce::String& getDisplayedName() const noexcept
    {
        return displayedName;
    }

    bool isReadOnly() const noexcept
    {
        return readOnly;
    }

private:
    juce::String displayedName;
    bool readOnly;
};

/**
 * Helps share the UI and a bit of logic for all that manage a profile (Source Profile, Theme Chooser, etc.).
 * Use this with inheritance.
 *
 * Note: no data is stored here.
 */
class ProfileUser : public juce::ComboBox::Listener
{
public:
    ProfileUser() noexcept;

    /**
     * Sets up the views. This adds them to the parent.
     * @param parent the parent view (as a modal dialog).
     * @param chooserBounds the bounds of the chooser.
     * @param newButtonBounds the bounds of the New Button.
     * @param renameButtonBounds the bounds of the Rename Button.
     * @param deleteButtonBounds the bounds of the Delete Button.
     */
    void setUpProfileViews(const juce::Rectangle<int>& chooserBounds, const juce::Rectangle<int>& newButtonBounds,
                           const juce::Rectangle<int>& renameButtonBounds, const juce::Rectangle<int>& deleteButtonBounds) noexcept;

    /** Call this when the views have been set-up and you are ready to provide the data. */
    void startProfile() noexcept;

    /** Must be called by the client when exiting (leaving the setup page, etc.), so that saves can be done. If the profile is read-only, nothing is done. */
    void exitProfile() noexcept;

    /** @return the ID of the currently shown profile. */
    int getShownProfileId() const noexcept;

    /** @return the displayed name of the currently seen profile. */
    juce::String getShownProfileName() const noexcept;

    // ===============================================================

    /**
     * Adds the given Component to the parent.
     * @param componentToAdd the component to add.
     */
    virtual void addProfileViewToParent(juce::Component& componentToAdd) noexcept = 0;

    /** @return the currently used Profile id (>0). Called on startup to know what to show. */
    virtual int getCurrentlyStoredProfileId() const noexcept = 0;
    /**
     * Stores the ID of the Profile that currently used. Called at worst when leaving.
     * @param profileId the ID (>0).
     */
    virtual void storeCurrentProfileId(int profileId) noexcept = 0;

    /**
     * @return the data to show in a line of the Chooser.
     * @param profileId the ID of the Profile.
     */
    virtual ProfileDataInChooser getProfileDataInChooser(int profileId) const noexcept = 0;

    /** @return the IDs of the profiles to display, in the order they are displayed. Will show both factory and custom ones. */
    virtual std::vector<int> getProfileIds() const noexcept = 0;

    /**
     * Displays the profile. This is typically called when the juce::ComboBox selection changes.
     * @param profileId the ID of the Profile to show.
     */
    virtual void displayProfile(int profileId) noexcept = 0;

    /**
     * Stores the given Profile.
     * @param profile the Profile.
     */
    virtual void persistProfile(StoredProfile& profile) noexcept = 0;

    /**
     * @return an ID for a custom Profile. Note that it must be after the last one, so that even after deleting a non-last Profile,
     * calling this will no return the same ID.
     */
    virtual int findNewFreeCustomId() const noexcept = 0;

    /**
     * @return a custom Profile, from storage. It should exist.
     * @param profileId the ID of the Profile.
     */
    virtual std::unique_ptr<StoredProfile> getStoredCustomProfile(int profileId) const noexcept = 0;

    /**
     * Stores the current profile, but only if not-read only. This is called on exit, or when the drop-down menu changes.
     * This it NOT called if the Profile is read-only, so implementations don't have to check it.
     * @param profileId the ID of the Profile to save, as a convenience.
     */
    virtual void storeProfileIfWriteable(int profileId) noexcept = 0;

    /**
     * @return true if the Profile is read-only.
     * @param profileId the ID of the Profile.
     */
    virtual bool isProfileInReadOnly(int profileId) const noexcept = 0;

    /**
     * Renames a Profile. This changes the data in the storage.
     * @param profileId the ID of the Profile.
     * @param newName the new name.
     */
    virtual void renameStoredProfile(int profileId, const juce::String& newName) noexcept = 0;

    /**
     * Deletes a Profile, in the storage.
     * @param profileId the ID of the Profile.
     */
    virtual void deleteStoredProfile(int profileId) noexcept = 0;

    /**
     * @return the name of the new profile. The default implementation adds a postfix "(copy)".
     * @param copiedProfileName as a convenience, the name of the copied profile, from which the new name can be derived ("copy of...").
     */
    virtual juce::String getNewProfileName(const juce::String& copiedProfileName) const noexcept;


    // juce::ComboBox::Listener method implementations.
    // ============================================
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;

protected:
    /**
     * Adds the new Profile, selects it. Its ID is ignored, a new one is provided.
     * @param newProfile the new profile. It must not be a read-only.
     */
    void addNewProfileAndSelect(std::unique_ptr<StoredProfile> newProfile) noexcept;

    /** @return a copy of the currently shown profile. */
    std::unique_ptr<StoredProfile> getCurrentlyShownProfile() const noexcept;

private:
    /** Copies the current theme into a new custom one. Selects it. */
    void copyCurrentThemeAndSelectNewOne() noexcept;

    /**
     * Clears and fills the Combobox with real data.
     * @param selectedProfileId the possible ID of the profile to select, or <=0 to select the latest stored one.
     */
    void fillComboBox(int selectedProfileId = 0) noexcept;

    /** Starts renaming. Shows the Rename pop-up. */
    void onRenameButtonClicked() noexcept;
    /** Starts deleting. Shows the confirmation pop-up. */
    void onDeleteButtonClicked() noexcept;

    /** Called when the OK Button was clicked on the Delete Dialog. */
    void onDeleteDialogOkSelected() noexcept;
    /** Called when the Cancel Button was clicked on the Delete Dialog. */
    void onDeleteDialogCancelSelected() noexcept;

    /**
     * Called when the Rename dialog returned with a new name.
     * @param newName the new name.
     */
    void onRenameDialogTextValidated(const juce::String& newName) noexcept;
    /** Called when the Rename dialog was cancelled. */
    void onRenameDialogTextCancelled() noexcept;

    /** Called when the New Button is clicked. */
    void onNewButtonClicked() noexcept;

    juce::ComboBox profileChooser;
    ButtonWithImage newButton;
    ButtonWithImage renameButton;
    ButtonWithImage deleteButton;

    int currentProfileId;               // The one shown in the juce::ComboBox.

    std::unique_ptr<ModalDialog> dialog;
};

}   // namespace arkostracker

