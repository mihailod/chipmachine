#include "ProfileUser.h"

#include <BinaryData.h>

#include "../lookAndFeel/LookAndFeelFactory.h"
#include "dialogs/SimpleTextDialog.h"

namespace arkostracker 
{

ProfileUser::ProfileUser() noexcept :
        profileChooser(),
        newButton(BinaryData::IconNew_png, static_cast<size_t>(BinaryData::IconNew_pngSize),
              juce::translate("New"), [&](int, bool, bool) { onNewButtonClicked(); }),
        renameButton(BinaryData::IconRename_png, static_cast<size_t>(BinaryData::IconRename_pngSize),
                 juce::translate("Rename"), [&](int, bool, bool) { onRenameButtonClicked(); }),
        deleteButton(BinaryData::IconDelete_png, static_cast<size_t>(BinaryData::IconDelete_pngSize),
                 juce::translate("Delete"), [&](int, bool, bool) { onDeleteButtonClicked(); }),
        currentProfileId(-1),       // Unknown at first.
        dialog()
{
}

void ProfileUser::setUpProfileViews(const juce::Rectangle<int>& chooserBounds, const juce::Rectangle<int>& newButtonBounds, const juce::Rectangle<int>& renameButtonBounds,
                                    const juce::Rectangle<int>& deleteButtonBounds) noexcept
{
    profileChooser.setBounds(chooserBounds);
    newButton.setBounds(newButtonBounds);
    renameButton.setBounds(renameButtonBounds);
    deleteButton.setBounds(deleteButtonBounds);

    addProfileViewToParent(profileChooser);
    addProfileViewToParent(newButton);
    addProfileViewToParent(renameButton);
    addProfileViewToParent(deleteButton);

    profileChooser.addListener(this);
}

void ProfileUser::startProfile() noexcept
{
    // Selects the Profile that was previously stored.
    const auto profileId = getCurrentlyStoredProfileId();
    fillComboBox(profileId);
}

void ProfileUser::exitProfile() noexcept
{
    // Stores the ID of the new Profile.
    const auto newProfileId = profileChooser.getSelectedId();
    storeCurrentProfileId(newProfileId);

    // Saves the profile (if not read-only).
    if (!isProfileInReadOnly(currentProfileId)) {
        storeProfileIfWriteable(newProfileId);
    }
}

void ProfileUser::copyCurrentThemeAndSelectNewOne() noexcept
{
    // If modified BEFORE saved, the current modification is NOT copied. So saves it first.
    exitProfile();

    // Gets a copy of the current theme.
    const auto selectedProfileId = profileChooser.getSelectedId();
    auto existingProfile = getStoredCustomProfile(selectedProfileId);
    if (existingProfile == nullptr) {
        jassertfalse;       // Unable to create a new one. Exits.
        return;
    }

    // Modifies the model to a custom one.
    existingProfile->setDisplayedName(getNewProfileName(existingProfile->getDisplayedName()));
    existingProfile->setReadOnly(false);

    addNewProfileAndSelect(std::move(existingProfile));
}

void ProfileUser::addNewProfileAndSelect(std::unique_ptr<StoredProfile> newProfile) noexcept
{
    jassert(!newProfile->isReadOnly());

    // Finds the new possible slot.
    const auto newProfileId = findNewFreeCustomId();
    newProfile->setId(newProfileId);

    // Persists it (important, if someone wants to copy it again directly).
    persistProfile(*newProfile);

    // Refreshes the Combobox and selects it.
    fillComboBox(newProfileId);
}

std::unique_ptr<StoredProfile> ProfileUser::getCurrentlyShownProfile() const noexcept
{
    const auto currentlyShownProfileId = getShownProfileId();
    const auto &currentLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    return LookAndFeelFactory::buildStoredLookAndFeelFromGivenLookAndFeel(currentLookAndFeel, currentlyShownProfileId, getShownProfileName(),
                                                                          isProfileInReadOnly(currentlyShownProfileId));
}

void ProfileUser::fillComboBox(const int selectedProfileId) noexcept
{
    // Fills the juce::ComboBox.
    profileChooser.clear(juce::NotificationType::dontSendNotification);

    const auto profileIds = getProfileIds();
    for (const auto profileId : profileIds) {       // Direct relationship between profileId and JuceID, as our ProfileId are >0. Simpler!
        const auto data = getProfileDataInChooser(profileId);

        auto nameToDisplay = data.getDisplayedName();
        if (data.isReadOnly()) {
            nameToDisplay += juce::translate(" (read only)");
        }

        profileChooser.addItem(nameToDisplay, profileId);
    }

    // Selects the given profile.
    if (selectedProfileId <= 0) {
        // Uses the last one.
        profileChooser.setSelectedItemIndex(profileChooser.getNumItems() - 1, juce::NotificationType::sendNotification);
    } else {
        profileChooser.setSelectedId(selectedProfileId, juce::NotificationType::sendNotification);
    }
}
int ProfileUser::getShownProfileId() const noexcept
{
    return currentProfileId;
}

juce::String ProfileUser::getShownProfileName() const noexcept
{
    return profileChooser.getText();
}

void ProfileUser::onNewButtonClicked() noexcept
{
    copyCurrentThemeAndSelectNewOne();
}

void ProfileUser::onRenameButtonClicked() noexcept
{
    jassert(dialog == nullptr);           // Dialog still present?
    dialog = std::make_unique<TextFieldDialog>(juce::translate("Rename"), juce::translate("Enter the new name:"),
                                                     juce::String(),
                                                     [&] (const juce::String& newText) { onRenameDialogTextValidated(newText); },
                                                     [&] { onRenameDialogTextCancelled(); });
}

void ProfileUser::onDeleteButtonClicked() noexcept
{
    jassert(dialog == nullptr);           // Dialog still present?
    dialog = std::make_unique<SimpleTextDialog>(juce::translate("Delete"),
                                                      juce::translate("Are you sure want to delete this item?"),
                                                      [&] { onDeleteDialogOkSelected(); },
                                                      [&] { onDeleteDialogCancelSelected(); }, 400);
}

juce::String ProfileUser::getNewProfileName(const juce::String& copiedProfileName) const noexcept
{
    return juce::translate(copiedProfileName) + juce::translate(" (copy)");
}


// juce::ComboBox::Listener method implementations.
// ============================================

void ProfileUser::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    // Saves the profile before change (if not read-only).
    if ((currentProfileId > 0) && (!isProfileInReadOnly(currentProfileId))) {     // Unknown on init, don't do anything in that case.
        storeProfileIfWriteable(currentProfileId);
    }

    // Shows the profile from its ID.
    const auto juceId = comboBoxThatHasChanged->getSelectedId();
    currentProfileId = juceId;
    displayProfile(juceId);

    // The Rename/delete Buttons may be disabled if read-only.
    const auto enabled = !isProfileInReadOnly(currentProfileId);
    renameButton.setEnabled(enabled);
    deleteButton.setEnabled(enabled);
}


// ============================================

void ProfileUser::onRenameDialogTextValidated(const juce::String& newName) noexcept
{
    renameStoredProfile(currentProfileId, newName);

    // Done AFTER the name is used.
    dialog.reset();

    // Updates the UI.
    fillComboBox(currentProfileId);
}

void ProfileUser::onRenameDialogTextCancelled() noexcept
{
    dialog.reset();
}


void ProfileUser::onDeleteDialogOkSelected() noexcept
{
    dialog.reset();

    // Deletes the Profile itself.
    const auto profileId = currentProfileId;
    deleteStoredProfile(profileId);
    // Invalidates the current ProfileId.
    currentProfileId = -1;

    // Updates the UI.
    fillComboBox(0);       // Selects the last one.
}

void ProfileUser::onDeleteDialogCancelSelected() noexcept
{
    dialog.reset();
}

}   // namespace arkostracker

