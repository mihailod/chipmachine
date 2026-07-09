#include "SourceProfileDialog.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../business/serialization/sourceProfile/SourceProfileSerializer.h"
#include "../../../utils/FileExtensions.h"
#include "../../../utils/FileUtil.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/dialogs/SuccessOrErrorDialog.h"
#include "../../lookAndFeel/LookAndFeelConstants.h"

namespace arkostracker 
{

SourceProfileDialog::SourceProfileDialog(std::function<void()> pAcceptCallback, std::function<void()> pCancelCallback) noexcept :
        ModalDialog(juce::translate("Source profiles"), 470, 675,
                    [&] { onOkButtonClicked(); },
                    [&] { onCancelButtonClicked(); },
                    true, true),
        ProfileUser(),
        preferences(PreferencesManager::getInstance()),
        placeholder(SourceGeneratorConfiguration::placeholder),
        currentProfileLabel(juce::String(), juce::translate("Current profile:")),
        explanationsLabel(juce::String(), juce::translate("All declarations must have a placeholder \"" + placeholder + "\" that will be replaced with the value. Multi-line is possible, useful to declare macros.")),
        addressChangeDeclarationEditor(),
        addressChangeDeclarationLabel(juce::String(), juce::translate("Current address declaration")),
        byteDeclarationEditor(),
        byteDeclarationLabel(juce::String(), juce::translate("Byte declaration")),
        wordDeclarationEditor(),
        wordDeclarationLabel(juce::String(), juce::translate("Word declaration")),
        addressDeclarationEditor(),
        addressDeclarationLabel(juce::String(), juce::translate("Address declaration")),
        stringDeclarationEditor(),
        stringDeclarationLabel(juce::String(), juce::translate("String declaration")),
        labelDeclarationEditor(),
        labelDeclarationLabel(juce::String(), juce::translate("Label declaration")),
        constantDeclarationEditor(),
        constantDeclarationLabel(juce::String(), juce::translate("Constant declaration")),
        commentDeclarationEditor(),
        commentDeclarationLabel(juce::String(), juce::translate("Comment declaration")),
        commentToggleButton(juce::translate("Encode comments")),
        littleEndianToggleButton(juce::translate("Little endian")),
        asmExtensionEditor(),
        asmExtensionLabel(juce::String(), juce::translate("Source file extension")),
        binExtensionEditor(),
        binExtensionLabel(juce::String(), juce::translate("Binary file extension")),
        tabulationSlider(*this, juce::translate("Tabulation size"), 2, 1.0, 2.0, SliderIncDec::Filter::integer,
                         216),      // Hardcoded, too bad...
        exportButton(juce::translate("Export"), juce::translate("Export to XML, to be used by command line tools to use custom source profile generation.")),
        profileIdToStoredSourceProfiles(),                  // Filled below.
        nextProfileId(startProfileId),                      // Any number >0 will do.
        acceptCallback(std::move(pAcceptCallback)),
        cancelCallback(std::move(pCancelCallback)),
        errorDialog()
{
    auto usableModalDialogBounds = getUsableModalDialogBounds();
    const auto left = usableModalDialogBounds.getX();
    const auto top = usableModalDialogBounds.getY();
    const auto width = usableModalDialogBounds.getWidth();
    const auto labelsHeight = LookAndFeelConstants::labelsHeight;
    const auto editorsHeight = static_cast<int>(static_cast<double>(LookAndFeelConstants::labelsHeight) * 1.5);
    const auto margins = LookAndFeelConstants::margins;
    const auto itemsSeparator = margins / 2;
    constexpr auto declarationLabelsWidth = 220;
    const auto declarationEditorsWidth = width - declarationLabelsWidth;

    currentProfileLabel.setBounds(left, top, width, labelsHeight);

    // Gets the profile and stores them as StoredProfiles.
    const auto sourceProfiles = preferences.getSourceProfiles();
    for (const auto& sourceProfile : sourceProfiles) {
        auto storedSourceProfile = StoredSourceProfile(sourceProfile, nextProfileId);
        profileIdToStoredSourceProfiles.insert(std::make_pair(nextProfileId, storedSourceProfile));
        ++nextProfileId;
    }

    // Shows the profile views at the top.
    const auto buttonItemsHeight = LookAndFeelConstants::buttonsHeight;
    const auto buttonSeparator = LookAndFeelConstants::buttonSeparator;
    const auto chooserY = currentProfileLabel.getBottom();
    constexpr auto imagesWidth = 40;
    const auto deleteButtonBounds = juce::Rectangle(usableModalDialogBounds.getRight() - imagesWidth, chooserY, imagesWidth, buttonItemsHeight);
    const auto renameButtonBounds = juce::Rectangle(deleteButtonBounds.getX() - imagesWidth - buttonSeparator, chooserY, imagesWidth, buttonItemsHeight);
    const auto newButtonBounds = juce::Rectangle(renameButtonBounds.getX() - imagesWidth - buttonSeparator, chooserY, imagesWidth, buttonItemsHeight);
    const auto chooserBounds = juce::Rectangle(left, chooserY, newButtonBounds.getX() - margins - left, LookAndFeelConstants::comboBoxesHeight);

    setUpProfileViews(chooserBounds, newButtonBounds, renameButtonBounds, deleteButtonBounds);
    startProfile();

    explanationsLabel.setBounds(left, chooserBounds.getBottom() + margins, width, labelsHeight * 2);

    addressChangeDeclarationLabel.setBounds(left, explanationsLabel.getBottom() + margins, declarationLabelsWidth, labelsHeight);
    addressChangeDeclarationEditor.setBounds(addressChangeDeclarationLabel.getRight(), addressChangeDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    addressChangeDeclarationEditor.setLinkedComponent(&addressChangeDeclarationLabel);

    byteDeclarationLabel.setBounds(left, addressChangeDeclarationEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    byteDeclarationEditor.setBounds(byteDeclarationLabel.getRight(), byteDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    byteDeclarationEditor.setLinkedComponent(&byteDeclarationLabel);

    wordDeclarationLabel.setBounds(left, byteDeclarationEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    wordDeclarationEditor.setBounds(wordDeclarationLabel.getRight(), wordDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    wordDeclarationEditor.setLinkedComponent(&wordDeclarationLabel);

    addressDeclarationLabel.setBounds(left, wordDeclarationEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    addressDeclarationEditor.setBounds(addressDeclarationLabel.getRight(), addressDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    addressDeclarationEditor.setLinkedComponent(&addressDeclarationLabel);

    stringDeclarationLabel.setBounds(left, addressDeclarationEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    stringDeclarationEditor.setBounds(stringDeclarationLabel.getRight(), stringDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    stringDeclarationEditor.setLinkedComponent(&stringDeclarationLabel);

    labelDeclarationLabel.setBounds(left, stringDeclarationEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    labelDeclarationEditor.setBounds(labelDeclarationLabel.getRight(), labelDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    labelDeclarationEditor.setLinkedComponent(&labelDeclarationLabel);

    constantDeclarationLabel.setBounds(left, labelDeclarationEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    constantDeclarationEditor.setBounds(constantDeclarationLabel.getRight(), constantDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    constantDeclarationEditor.setLinkedComponent(&constantDeclarationLabel);

    commentDeclarationLabel.setBounds(left, constantDeclarationEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    commentDeclarationEditor.setBounds(commentDeclarationLabel.getRight(), commentDeclarationLabel.getY(), declarationEditorsWidth, editorsHeight);
    commentDeclarationEditor.setLinkedComponent(&commentDeclarationLabel);

    tabulationSlider.setBounds(left, commentDeclarationEditor.getBottom() + margins, width, labelsHeight);

    constexpr auto extensionEditorsWidth = 70;
    asmExtensionLabel.setBounds(left, tabulationSlider.getBottom() + margins, declarationLabelsWidth, labelsHeight);
    asmExtensionEditor.setBounds(asmExtensionLabel.getRight(), asmExtensionLabel.getY(), extensionEditorsWidth, labelsHeight);
    asmExtensionEditor.setLinkedComponent(&asmExtensionLabel);

    binExtensionLabel.setBounds(left, asmExtensionEditor.getBottom() + itemsSeparator, declarationLabelsWidth, labelsHeight);
    binExtensionEditor.setBounds(binExtensionLabel.getRight(), binExtensionLabel.getY(), extensionEditorsWidth, labelsHeight);
    binExtensionEditor.setLinkedComponent(&binExtensionLabel);

    commentToggleButton.setBounds(left, binExtensionLabel.getBottom() + margins, declarationLabelsWidth, labelsHeight);
    littleEndianToggleButton.setBounds(commentToggleButton.getRight(), commentToggleButton.getY(), declarationLabelsWidth, labelsHeight);

    for (auto* editor : { &addressChangeDeclarationEditor, &byteDeclarationEditor, &wordDeclarationEditor, &addressDeclarationEditor, &stringDeclarationEditor,
                          &labelDeclarationEditor, &constantDeclarationEditor, &commentDeclarationEditor}) {
        editor->setMultiLine(true, false);
        editor->setReturnKeyStartsNewLine(true);
    }

    exportButton.setBounds(left, getButtonsY(), 80, getButtonsHeight());
    exportButton.onClick = [&] { onExportButtonClicked(); };

    addComponentToModalDialog(currentProfileLabel);
    addComponentToModalDialog(explanationsLabel);
    addComponentToModalDialog(addressChangeDeclarationLabel);
    addComponentToModalDialog(addressChangeDeclarationEditor);
    addComponentToModalDialog(byteDeclarationLabel);
    addComponentToModalDialog(byteDeclarationEditor);
    addComponentToModalDialog(wordDeclarationLabel);
    addComponentToModalDialog(wordDeclarationEditor);
    addComponentToModalDialog(addressDeclarationLabel);
    addComponentToModalDialog(addressDeclarationEditor);
    addComponentToModalDialog(stringDeclarationLabel);
    addComponentToModalDialog(stringDeclarationEditor);
    addComponentToModalDialog(labelDeclarationLabel);
    addComponentToModalDialog(labelDeclarationEditor);
    addComponentToModalDialog(constantDeclarationLabel);
    addComponentToModalDialog(constantDeclarationEditor);
    addComponentToModalDialog(commentDeclarationLabel);
    addComponentToModalDialog(commentDeclarationEditor);
    addComponentToModalDialog(commentToggleButton);
    addComponentToModalDialog(tabulationSlider);
    addComponentToModalDialog(littleEndianToggleButton);
    addComponentToModalDialog(asmExtensionLabel);
    addComponentToModalDialog(asmExtensionEditor);
    addComponentToModalDialog(binExtensionLabel);
    addComponentToModalDialog(binExtensionEditor);
    addComponentToModalDialog(exportButton);
}

void SourceProfileDialog::onOkButtonClicked() noexcept
{
    // Saves the current profile, else its possible errors won't be saved.
    exitProfile();

    // Validates the profiles.
    const auto errors = checkProfiles();

    if (errors.empty()) {
        persistProfiles();
        acceptCallback();
    } else {
        // Displays the error.
        displayProfileErrors(errors);
    }
}

void SourceProfileDialog::onCancelButtonClicked() const noexcept
{
    cancelCallback();
}


// ProfileUser method implementations.
// ======================================

void SourceProfileDialog::addProfileViewToParent(juce::Component& componentToAdd) noexcept
{
    addComponentToModalDialog(componentToAdd);
}

int SourceProfileDialog::getCurrentlyStoredProfileId() const noexcept
{
    // Gets the index stored in the Preferences, and maps it to the local profiles.
    const auto selectedIndex = preferences.getCurrentSourceProfileIndex();

    auto index = 0;
    for (const auto&[profileId, storedSourceProfile] : profileIdToStoredSourceProfiles) {
        if (index == selectedIndex) {
            return profileId;
        }

        ++index;
    }

    // Not found!
    jassertfalse;
    return startProfileId;
}

void SourceProfileDialog::storeCurrentProfileId(const int profileId) noexcept
{
    // Gets the index of the profile.
    auto foundIndex = -1;
    auto index = 0;
    for (const auto&[browsedProfileId, storedSourceProfile] : profileIdToStoredSourceProfiles) {
        if (profileId == browsedProfileId) {
            foundIndex = index;
            break;
        }

        ++index;
    }

    if (foundIndex < 0) {
        jassertfalse;           // Profile not found? Abnormal.
    } else {
        preferences.setCurrentSourceProfileFromIndex(foundIndex);
    }
}

ProfileDataInChooser SourceProfileDialog::getProfileDataInChooser(const int profileId) const noexcept
{
    const auto& sourceProfile = getSourceProfile(profileId);
    return { sourceProfile.getName(), sourceProfile.isReadOnly() };
}

std::vector<int> SourceProfileDialog::getProfileIds() const noexcept
{
    // The IDS are simple indexes.
    const auto size = profileIdToStoredSourceProfiles.size();
    std::vector<int> ids;
    ids.reserve(size);
    for (const auto&[id, item] : profileIdToStoredSourceProfiles) {
        ids.push_back(id);
    }

    return ids;
}

void SourceProfileDialog::displayProfile(const int profileId) noexcept
{
    const auto& profile = getSourceProfile(profileId);
    const auto& configuration = profile.getSourceGeneratorConfiguration();

    addressChangeDeclarationEditor.setText(configuration.getAddressChangeDeclaration());
    byteDeclarationEditor.setText(configuration.getByteDeclaration());
    wordDeclarationEditor.setText(configuration.getWordDeclaration());
    addressDeclarationEditor.setText(configuration.getAddressDeclaration());
    stringDeclarationEditor.setText(configuration.getStringDeclaration());
    labelDeclarationEditor.setText(configuration.getLabelDeclaration());
    commentDeclarationEditor.setText(configuration.getCommentDeclaration());
    constantDeclarationEditor.setText(configuration.getConstantDeclaration());
    tabulationSlider.setShownValue(configuration.getTabulationLength());
    asmExtensionEditor.setText(configuration.getSourceFileExtension());
    binExtensionEditor.setText(configuration.getBinaryFileExtension());
    commentToggleButton.setToggleState(configuration.doesGenerateComments(), juce::NotificationType::dontSendNotification);
    littleEndianToggleButton.setToggleState(configuration.isLittleEndian(), juce::NotificationType::dontSendNotification);

    // Read-only profiles must not be modifiable.
    const auto enabled = !profile.isReadOnly();
    for (auto* editor : { &addressChangeDeclarationEditor, &byteDeclarationEditor, &wordDeclarationEditor,
                          &addressDeclarationEditor, &stringDeclarationEditor, &labelDeclarationEditor,
                          &constantDeclarationEditor, &commentDeclarationEditor, &asmExtensionEditor, &binExtensionEditor
                          }) {
        editor->setEnabled(enabled);
    }
    commentToggleButton.setEnabled(enabled);
    littleEndianToggleButton.setEnabled(enabled);
    tabulationSlider.setEnabled(enabled);
}

void SourceProfileDialog::persistProfile(StoredProfile& profile) noexcept
{
    // Note: the profile may not be valid, but it is checked when OK is pressed. We only store it locally in this method.

    jassert(profileIdToStoredSourceProfiles.find(profile.getId()) == profileIdToStoredSourceProfiles.cend());       // Profile already exists!

    const auto* storedSourceProfile = dynamic_cast<StoredSourceProfile*>(&profile);
    if (storedSourceProfile == nullptr) {
        jassertfalse;      // Wrong type!! Abnormal!!
        return;
    }

    profileIdToStoredSourceProfiles.insert(std::make_pair(profile.getId(), *storedSourceProfile));
}

int SourceProfileDialog::findNewFreeCustomId() const noexcept
{
    return nextProfileId++;
}

std::unique_ptr<StoredProfile> SourceProfileDialog::getStoredCustomProfile(int profileId) const noexcept
{
    // Wraps a SourceProfile with a StoredProfile.
    const auto& sourceProfile = getSourceProfile(profileId);
    return std::make_unique<StoredSourceProfile>(sourceProfile, profileId);
}

void SourceProfileDialog::storeProfileIfWriteable(int profileId) noexcept
{
    jassert(profileIdToStoredSourceProfiles.find(profileId) != profileIdToStoredSourceProfiles.cend());       // Profile must exist!
    const auto& oldProfile = getSourceProfile(profileId);

    // Builds the configuration from the UI.
    const SourceGeneratorConfiguration sourceGeneratorConfiguration(
            addressChangeDeclarationEditor.getText(),
            commentDeclarationEditor.getText(),
            commentToggleButton.getToggleState(),
            byteDeclarationEditor.getText(),
            wordDeclarationEditor.getText(),
            addressDeclarationEditor.getText(),
            stringDeclarationEditor.getText(),
            constantDeclarationEditor.getText(),
            labelDeclarationEditor.getText(),
            littleEndianToggleButton.getToggleState(),
            asmExtensionEditor.getText(),
            binExtensionEditor.getText(),
            static_cast<int>(tabulationSlider.getValue())
    );

    // Overwrites the one that is existing.
    const SourceProfile sourceProfile(sourceGeneratorConfiguration, oldProfile.getName(), oldProfile.isReadOnly());
    const StoredSourceProfile storedSourceProfile(sourceProfile, profileId);
    profileIdToStoredSourceProfiles[profileId] = storedSourceProfile;
}

bool SourceProfileDialog::isProfileInReadOnly(const int profileId) const noexcept
{
    const auto& sourceProfile = getSourceProfile(profileId);
    return sourceProfile.isReadOnly();
}

void SourceProfileDialog::renameStoredProfile(const int profileId, const juce::String& newName) noexcept
{
    const auto& oldProfile = getSourceProfile(profileId);
    const auto newProfile = SourceProfile(oldProfile, newName, oldProfile.isReadOnly());

    // Overwrites the one that is existing.
    const StoredSourceProfile storedSourceProfile(newProfile, profileId);
    profileIdToStoredSourceProfiles[profileId] = storedSourceProfile;
}

void SourceProfileDialog::deleteStoredProfile(const int profileId) noexcept
{
    profileIdToStoredSourceProfiles.erase(profileId);
}


// SliderIncDec::Listener method implementations.
// =================================================

void SourceProfileDialog::onWantToChangeSliderValue(SliderIncDec& slider, const double valueDouble)
{
    jassert(&slider == &tabulationSlider); (void)slider;
    const auto value = SourceProfileValidator::correctTabulationSize(static_cast<int>(valueDouble));
    tabulationSlider.setShownValue(static_cast<double>(value));
}


// ======================================

const SourceProfile& SourceProfileDialog::getSourceProfile(const int profileId) const noexcept
{
    const auto& storedSourceProfile = profileIdToStoredSourceProfiles.at(profileId);
    return storedSourceProfile.getSourceProfile();
}

std::vector<std::pair<int, std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>>>> SourceProfileDialog::checkProfiles() const noexcept
{
    std::vector<std::pair<int, std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>>>> profileIdsAndErrors;

    // Browses each local profile.
    for (const auto&[profileId, storedSourceProfile] : profileIdToStoredSourceProfiles) {
        // Skips read-only ones.
        if (storedSourceProfile.isReadOnly()) {
            continue;
        }

        const auto& sourceProfile = storedSourceProfile.getSourceProfile();
        const auto sourceProfileErrors = SourceProfileValidator::validate(sourceProfile);
        // If errors, adds them to the result.
        if (!sourceProfileErrors.empty()) {
            profileIdsAndErrors.emplace_back(profileId, sourceProfileErrors);
        }
    }

    return profileIdsAndErrors;
}

void SourceProfileDialog::displayProfileErrors(const std::vector<std::pair<int, std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>>>>& errors)
{
    jassert(errorDialog == nullptr);                // Already present?
    jassert(!errors.empty());

    // Takes the first error of the first profile.
    const auto& error = errors.at(0U);
    const auto profileId = error.first;
    const auto profileName = getSourceProfile(profileId).getName();

    const auto& locationsAndErrors = error.second;
    jassert(!locationsAndErrors.empty());
    const auto&[location, errorType] = locationsAndErrors.at(0U);

    // Gets a displayable location and error.
    const auto locationName = SourceProfileValidator::locationToDisplayableLocation(location);
    const auto errorMessage = SourceProfileValidator::errorToDisplayableError(errorType);

    const auto text = juce::translate("The profile \"" + profileName + "\" has the following error in the " + locationName + ": \r\n" + errorMessage);
    errorDialog = SuccessOrErrorDialog::buildForError(text, [&] { onErrorDialogOkClicked(); }, 600, 150);
}

void SourceProfileDialog::displayCurrentProfileErrors(const std::vector<std::pair<SourceProfileValidator::Location, SourceProfileValidator::Error>>& errors)
{
    jassert(errorDialog == nullptr);                // Already present?
    jassert(!errors.empty());

    const auto&[location, errorType] = errors.at(0U);

    // Gets a displayable location and error.
    const auto locationName = SourceProfileValidator::locationToDisplayableLocation(location);
    const auto errorMessage = SourceProfileValidator::errorToDisplayableError(errorType);

    const auto text = juce::translate("The current profile has the following error in the " + locationName + ": \r\n" + errorMessage);
    errorDialog = SuccessOrErrorDialog::buildForError(text, [&] { onErrorDialogOkClicked(); }, 600, 150);
}

void SourceProfileDialog::onErrorDialogOkClicked() noexcept
{
    errorDialog.reset();
}

void SourceProfileDialog::persistProfiles() noexcept
{
    std::vector<SourceProfile> sourceProfiles;
    sourceProfiles.reserve(profileIdToStoredSourceProfiles.size());

    for (const auto&[profileId, storedSourceProfile] : profileIdToStoredSourceProfiles) {
        sourceProfiles.push_back(storedSourceProfile.getSourceProfile());
    }

    preferences.setSourceProfiles(sourceProfiles);
}

void SourceProfileDialog::onExportButtonClicked() noexcept
{
    // Checks the profile validity.
    exitProfile();      // Saves the current profile, else its possible errors won't be saved.

    const auto sourceProfile = getSourceProfile(getCurrentlyStoredProfileId());
    const auto sourceProfileErrors = SourceProfileValidator::validate(sourceProfile);

    if (!sourceProfileErrors.empty()) {
        // Displays the error.
        displayCurrentProfileErrors(sourceProfileErrors);
        return;
    }

    // Exports the profile.
    const auto xmlNode = SourceProfileSerializer::serialize(sourceProfile);

    // Opens the file picker.
    const auto fileToSaveTo = FileChooserCustom::save(FileChooserCustom::Target::any, FileExtensions::xmlExtensionWithoutDot);
    if (fileToSaveTo.getFullPathName().isEmpty()) {
        return;     // Cancel.
    }

    const auto success = FileUtil::writeXmlToFile(*xmlNode, fileToSaveTo, false);
    errorDialog = success
            ? SuccessOrErrorDialog::buildForSuccess(juce::translate("The export was successful."), [&] { onErrorDialogOkClicked(); })
            : SuccessOrErrorDialog::buildForError(juce::translate("Error while saving the file!"), [&] { onErrorDialogOkClicked(); });
}

}   // namespace arkostracker
