#pragma once

#include "../../../business/sourceProfile/SourceProfile.h"
#include "../../../business/sourceProfile/SourceProfileValidator.h"
#include "../../components/ProfileUser.h"
#include "../../components/SliderIncDec.h"
#include "../../components/dialogs/ModalDialog.h"
#include "StoredSourceProfile.h"

namespace arkostracker 
{

class PreferencesManager;

/** Dialog to set up the source profile. */
class SourceProfileDialog final : public ModalDialog,
                                  public ProfileUser,
                                  public SliderIncDec::Listener
{
public:
    /**
     * Constructor.
     * @param acceptCallback called when the Accepting the dialog.
     * @param cancelCallback called when the Canceling the dialog.
     */
    explicit SourceProfileDialog(std::function<void()> acceptCallback, std::function<void()> cancelCallback) noexcept;

    // ProfileUser method implementations.
    // ======================================
    void addProfileViewToParent(Component& componentToAdd) noexcept override;
    int getCurrentlyStoredProfileId() const noexcept override;
    void storeCurrentProfileId(int profileId) noexcept override;
    ProfileDataInChooser getProfileDataInChooser(int profileId) const noexcept override;
    std::vector<int> getProfileIds() const noexcept override;
    void displayProfile(int profileId) noexcept override;
    void persistProfile(StoredProfile& profile) noexcept override;
    int findNewFreeCustomId() const noexcept override;
    std::unique_ptr<StoredProfile> getStoredCustomProfile(int profileId) const noexcept override;
    void storeProfileIfWriteable(int profileId) noexcept override;
    bool isProfileInReadOnly(int profileId) const noexcept override;
    void renameStoredProfile(int profileId, const juce::String& newName) noexcept override;
    void deleteStoredProfile(int profileId) noexcept override;

    // SliderIncDec::Listener method implementations.
    // =================================================
    void onWantToChangeSliderValue(SliderIncDec& slider, double value) override;

private:
    static constexpr auto startProfileId = 1000;

    /**
     * @return the source profile, according to the given ProfileId.
     * @param profileId the Profile id.
     */
    const SourceProfile& getSourceProfile(int profileId) const noexcept;

    /** Called when OK is clicked. */
    void onOkButtonClicked() noexcept;
    /** Called when Cancel is clicked. */
    void onCancelButtonClicked() const noexcept;

    /**
     * Checks the locally stored profiles for errors. Read-only ones are skipped.
     * @return the profileIds and their error/location.
     */
    std::vector<std::pair<int, std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>>>> checkProfiles() const noexcept;

    /**
     * Shows a pop-up indicating what errors are in what profile.
     * @param errors the profile IDs linked to their errors.
     */
    void displayProfileErrors(const std::vector<std::pair<int, std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>>>>& errors);

    /**
     * Shows a pop-up showing a one error among the given ones. The text shows "current profile".
     * @param errors the errors.
     */
    void displayCurrentProfileErrors(const std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>>& errors);

    /** Called when OK of the Error Dialog is clicked. */
    void onErrorDialogOkClicked() noexcept;

    /** Persists all the local profiles. */
    void persistProfiles() noexcept;

    /** Checks the profile validity, then exports to XML. */
    void onExportButtonClicked() noexcept;

    PreferencesManager& preferences;
    juce::String placeholder;
    juce::Label currentProfileLabel;
    juce::Label explanationsLabel;

    EditText addressChangeDeclarationEditor;
    juce::Label addressChangeDeclarationLabel;
    EditText byteDeclarationEditor;
    juce::Label byteDeclarationLabel;
    EditText wordDeclarationEditor;
    juce::Label wordDeclarationLabel;
    EditText addressDeclarationEditor;
    juce::Label addressDeclarationLabel;
    EditText stringDeclarationEditor;
    juce::Label stringDeclarationLabel;
    EditText labelDeclarationEditor;
    juce::Label labelDeclarationLabel;
    EditText constantDeclarationEditor;
    juce::Label constantDeclarationLabel;
    EditText commentDeclarationEditor;
    juce::Label commentDeclarationLabel;

    juce::ToggleButton commentToggleButton;
    juce::ToggleButton littleEndianToggleButton;

    EditText asmExtensionEditor;
    juce::Label asmExtensionLabel;
    EditText binExtensionEditor;
    juce::Label binExtensionLabel;

    SliderIncDec tabulationSlider;

    juce::TextButton exportButton;

    std::map<int, StoredSourceProfile> profileIdToStoredSourceProfiles;             // Must be ordered.
    mutable int nextProfileId;

    std::function<void()> acceptCallback;
    std::function<void()> cancelCallback;

    std::unique_ptr<ModalDialog> errorDialog;
};

}   // namespace arkostracker
