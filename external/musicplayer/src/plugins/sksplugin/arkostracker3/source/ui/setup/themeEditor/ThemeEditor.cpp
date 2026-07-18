#include "ThemeEditor.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../utils/FileExtensions.h"
#include "../../components/FileChooserCustom.h"
#include "../../components/LookAndFeelChanger.h"
#include "../../lookAndFeel/CustomLookAndFeel.h"
#include "../../lookAndFeel/LookAndFeelFactory.h"
#include "../../lookAndFeel/serialization/XmlSerializer.h"
#include "../../lookAndFeel/theme/ThemeDefaultBuilder.h"
#include "ColorItem.h"
#include "HeaderItem.h"

namespace arkostracker 
{

ThemeEditor::ThemeEditor(LookAndFeelChanger& pLookAndFeelChanger, std::function<void()> pOkCallback) noexcept :
        ModalDialog(juce::translate("Theme editor"), 500, 500,
                    [&] { onOkButtonClicked(); }, [&] { onOkButtonClicked(); }),        // Cancel does the same thing.
        ProfileUser(),
        okCallback(std::move(pOkCallback)),
        colorIdToThemeColor(ThemeColors::getInstance().getColorIdToThemeColor()),
        lookAndFeelChanger(pLookAndFeelChanger),
        treeView(),
        rootItem(std::make_unique<HeaderItem>(juce::String())),       // Nothing to display in the root item.
        selectedColourId(0),
        editedColor(),
        preferencesManager(PreferencesManager::getInstance()),
        exportButton(juce::translate("Export"), juce::translate("Export the current theme as XML")),
        importButton(juce::translate("Import"), juce::translate("Import an XML theme")),
        colorSelectorModalDialog(),
        colorSelector()
{
    auto usableModalDialogBounds = getUsableModalDialogBounds();
    const auto left = usableModalDialogBounds.getX();
    const auto top = usableModalDialogBounds.getY();

    // Shows the profile views at the top.
    const auto buttonItemsHeight = LookAndFeelConstants::buttonsHeight;
    const auto margins = LookAndFeelConstants::margins;
    const auto buttonSeparator = LookAndFeelConstants::buttonSeparator;
    const auto chooserY = top;
    constexpr auto imagesWidth = 40;
    const auto deleteButtonBounds = juce::Rectangle(usableModalDialogBounds.getRight() - imagesWidth, chooserY, imagesWidth, buttonItemsHeight);
    const auto renameButtonBounds = juce::Rectangle(deleteButtonBounds.getX() - imagesWidth - buttonSeparator, chooserY, imagesWidth, buttonItemsHeight);
    const auto newButtonBounds = juce::Rectangle(renameButtonBounds.getX() - imagesWidth - buttonSeparator, chooserY, imagesWidth, buttonItemsHeight);
    const auto chooserBounds = juce::Rectangle(left, chooserY, newButtonBounds.getX() - margins - left, LookAndFeelConstants::comboBoxesHeight);

    setUpProfileViews(chooserBounds, newButtonBounds, renameButtonBounds, deleteButtonBounds);

    // The import/export Buttons at the bottom.
    const auto okButtonY = getButtonsY();
    const auto buttonsHeight = getButtonsHeight();
    exportButton.setBounds(left, okButtonY, 70, buttonsHeight);
    importButton.setBounds(exportButton.getRight() + margins / 2, okButtonY, 70, buttonsHeight);
    addComponentToModalDialog(exportButton);
    addComponentToModalDialog(importButton);
    exportButton.onClick = [&] { onExportButtonClicked(); };
    importButton.onClick = [&] { onImportButtonClicked(); };

    const auto headerHeight = chooserBounds.getBottom() + margins;

    // All the categories to display, in order.
    const static std::vector<DisplayableCategory> displayableCategories = {
            { juce::translate("General"), ColorCategory::general },
            { juce::translate("Panels"), ColorCategory::panels },
            { juce::translate("Toolbars"), ColorCategory::toolbars },
            { juce::translate("Buttons"), ColorCategory::buttons },
            { juce::translate("Lists"), ColorCategory::lists },
            { juce::translate("Meters"), ColorCategory::meters },
            { juce::translate("Linker"), ColorCategory::linker },
            { juce::translate("Bar editors"), ColorCategory::editorWithBars },
            { juce::translate("Pattern viewer"), ColorCategory::patternViewer },
    };
    jassert(displayableCategories.size() == static_cast<size_t>(ColorCategory::count));         // Missing any category?

    addComponentToModalDialog(treeView);
    usableModalDialogBounds.removeFromTop(headerHeight);
    treeView.setBounds(usableModalDialogBounds);

    // Creates the tree view. They are only linked to ColorIds, they will be updated automatically when changing the theme.
    // Creates a first node encompassing all the others, but hides it.
    treeView.setDefaultOpenness(true);
    treeView.setRootItemVisible(false);
    treeView.setRootItem(rootItem.get());

    for (const auto& displayableCategory : displayableCategories) {
        // Gets all the colors for this category.
        // This is not optimized at all!!! Who cares, there are not many colors.
        const auto& category = displayableCategory.colorCategory;
        const auto& categoryName = displayableCategory.displayedCategory;

        std::vector<ThemeColor> themeColors;
        for (const auto& colorEntry : colorIdToThemeColor) {
            if (colorEntry.second.category == category) {
                themeColors.emplace_back(colorEntry.second);
            }
        }
        jassert(!themeColors.empty());       // No colors in this Category??

        auto subItem = std::make_unique<HeaderItem>(categoryName);

        // Adds colorItems to each Category Item.
        for (const auto& themeColor : themeColors) {
            const auto& colorName = themeColor.displayedName;
            const auto colorId = themeColor.colorId;

            // The ColorID is given only: thus the view will get the color directly from the theme,
            // and will update itself when it changes.
            auto colorItem = std::make_unique<ColorItem>(colorId, colorName, *this);
            subItem->addSubItem(colorItem.release());
        }

        rootItem->addSubItem(subItem.release());
    }

    startProfile();
}

ThemeEditor::~ThemeEditor()
{
    treeView.setRootItem(nullptr);
}

void ThemeEditor::onOkButtonClicked() noexcept
{
    exitProfile();

    okCallback();
}

void ThemeEditor::setNewColorAndNotify(const int colorId, const juce::Colour newColor) noexcept {
    // Changes the color in the current look and feel, applies the change for the whole window to change.
    // This will automatically change the swatches too!
    auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();

    localLookAndFeel.setColour(colorId, newColor);
    CustomLookAndFeel::setAllBackgroundAndOutlineColors(localLookAndFeel);
    CustomLookAndFeel::setAllTextColors(localLookAndFeel);

    lookAndFeelChanger.notifyLookAndFeelChange();
    // The 2 modals must be notified.
    sendLookAndFeelChange();
    if (colorSelectorModalDialog != nullptr) {                  // May have already been killed.
        colorSelectorModalDialog->sendLookAndFeelChange();
    }
}


// ColorItem::Listener method implementations.
// =================================================

void ThemeEditor::onColorSwatchClicked(const int colorId)
{
    // Creates the modal.
    jassert((colorSelectorModalDialog == nullptr) && (colorSelector == nullptr));       // Color selector Dialog already present? Abnormal.
    colorSelectorModalDialog = std::make_unique<ModalDialog>(juce::translate("Select a color"), 400, 400,
                                                             [&] { onColorSelectorDialogOk(); },
                                                             [&] { onColorSelectorDialogCancelled(); },
                                                             true, true);
    // Adds the Color Selector to it.
    auto selectorFlags = juce::ColourSelector::ColourSelectorOptions::showColourspace + juce::ColourSelector::ColourSelectorOptions::showSliders;

    // Alpha possible? Finds more data about the color.
    const auto iterator = colorIdToThemeColor.find(colorId);
    if (iterator == colorIdToThemeColor.cend()) {
        jassertfalse;           // Should never happen!
        return;
    }
    if (iterator->second.allowTransparency) {
        selectorFlags += juce::ColourSelector::ColourSelectorOptions::showAlphaChannel;
    }

    const auto& localLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    const auto color = localLookAndFeel.findColour(colorId);
    // Saves the color/colorId to allow cancellation.
    editedColor = color;
    selectedColourId = colorId;

    colorSelector = std::make_unique<juce::ColourSelector>(selectorFlags);
    colorSelector->setBounds(colorSelectorModalDialog->getUsableModalDialogBounds());
    colorSelector->setCurrentColour(color, juce::NotificationType::dontSendNotification);
    colorSelector->addChangeListener(this);
    colorSelectorModalDialog->addComponentToModalDialog(*colorSelector);
}


// ChangeListener method implementations.
// =================================================

void ThemeEditor::changeListenerCallback(juce::ChangeBroadcaster* /*source*/)
{
    const juce::Colour newColor(colorSelector->getCurrentColour());
    setNewColorAndNotify(selectedColourId, newColor);
}


// =================================================

void ThemeEditor::onColorSelectorDialogOk() noexcept
{
    closeColorSelectorModalDialog();
}

void ThemeEditor::onColorSelectorDialogCancelled() noexcept
{
    closeColorSelectorModalDialog();

    // Changes to the previously stored color.
    setNewColorAndNotify(selectedColourId, editedColor);
}

void ThemeEditor::closeColorSelectorModalDialog() noexcept
{
    colorSelector.reset();
    colorSelectorModalDialog.reset();
}


// ProfileUser method implementations.
// =================================================

void ThemeEditor::addProfileViewToParent(juce::Component& componentToAdd) noexcept
{
    addComponentToModalDialog(componentToAdd);
}

std::vector<int> ThemeEditor::getProfileIds() const noexcept
{
    // Adds the default ones.
    std::vector<int> ids = LookAndFeelFactory::getNativeThemeIds();

    // Adds the user ones at the end.
    const auto customThemeIds = preferencesManager.getAllCustomThemeIds();
    ids.insert(ids.cend(), customThemeIds.cbegin(), customThemeIds.cend());

    return ids;
}

ProfileDataInChooser ThemeEditor::getProfileDataInChooser(const int profileId) const noexcept
{
    const auto theme = LookAndFeelFactory::retrieveLookAndFeel(profileId);
    return { theme->displayedName, theme->isReadOnly() };
}

void ThemeEditor::displayProfile(const int profileId) noexcept
{
    // Gets the profile, but only to know it if is read-only or not.
    const auto customLookAndFeel = LookAndFeelFactory::retrieveLookAndFeel(profileId);
    const auto readOnly = customLookAndFeel->isReadOnly();
    // Applies the theme.
    lookAndFeelChanger.setLookAndFeel(profileId);

    treeView.setEnabled(!readOnly);
}

int ThemeEditor::getCurrentlyStoredProfileId() const noexcept
{
    return preferencesManager.getCurrentThemeId();
}

void ThemeEditor::storeCurrentProfileId(const int profileId) noexcept
{
    preferencesManager.setCurrentThemeId(profileId);
}

void ThemeEditor::persistProfile(StoredProfile& storedProfile) noexcept
{
    const auto* storedLookAndFeel = dynamic_cast<StoredLookAndFeel*>(&storedProfile);
    if (storedLookAndFeel == nullptr) {
        jassertfalse;      // Wrong type!! Abnormal!!
        return;
    }
    if (storedLookAndFeel->isReadOnly()) {
        jassertfalse;                   // Trying to save a native profile??
        return;
    }

    preferencesManager.storeCustomLookAndFeel(*storedLookAndFeel);
}

std::unique_ptr<StoredProfile> ThemeEditor::getStoredCustomProfile(const int profileId) const noexcept
{
    return LookAndFeelFactory::retrieveLookAndFeel(profileId);
}

int ThemeEditor::findNewFreeCustomId() const noexcept
{
    return preferencesManager.findNewFreeCustomLookAndFeelId();
}

void ThemeEditor::storeProfileIfWriteable(const int profileId) noexcept
{
    const auto customLookAndFeel = LookAndFeelFactory::retrieveLookAndFeel(profileId);
    jassert(!customLookAndFeel->isReadOnly());

    // Builds the profile from the current Theme data.
    const auto& currentLookAndFeel = juce::LookAndFeel::getDefaultLookAndFeel();
    const auto storedLookAndFeel = LookAndFeelFactory::buildStoredLookAndFeelFromGivenLookAndFeel(currentLookAndFeel, profileId,
                                                                                            customLookAndFeel->getDisplayedName(),
                                                                                            customLookAndFeel->isReadOnly());
    // Stores it.
    persistProfile(*storedLookAndFeel);
}

bool ThemeEditor::isProfileInReadOnly(const int profileId) const noexcept
{
    return (profileId < static_cast<int>(LookAndFeelConstants::ThemeId::startIdCustomTheme));
}

void ThemeEditor::renameStoredProfile(const int profileId, const juce::String& newName) noexcept
{
    jassert(!isProfileInReadOnly(profileId));       // Modifying a read-only theme?

    // Loads the Profile. It must exist.
    const auto storedProfile = getStoredCustomProfile(profileId);
    jassert(storedProfile != nullptr);

    storedProfile->setDisplayedName(newName);

    // Saves it.
    persistProfile(*storedProfile);
}

void ThemeEditor::deleteStoredProfile(const int profileId) noexcept
{
    jassert(!isProfileInReadOnly(profileId));       // Modifying a read-only theme?

    preferencesManager.deleteCustomLookAndFeel(profileId);
}

void ThemeEditor::onExportButtonClicked() noexcept
{
    // Exports the current (possibly modified) theme, not the stored one.
    auto storedProfile = getCurrentlyShownProfile();
    const auto* storedLookAndFeel = dynamic_cast<StoredLookAndFeel*>(storedProfile.get());

    XmlSerializer xmlSerializer;
    const auto xmlNode = xmlSerializer.serialize(*storedLookAndFeel);

    // Where to save?
    const auto outputFile = FileChooserCustom::save(FileChooserCustom::Target::any, FileExtensions::xmlExtensionWithoutDot);
    if (outputFile.getFullPathName().isEmpty()) {
        return;
    }

    xmlNode->writeTo(outputFile);
}

void ThemeEditor::onImportButtonClicked() noexcept
{
    // What to load?
    FileChooserCustom fileChooser(juce::translate("Import theme"), FolderContext::other, FileExtensions::xmlExtensionFilter);
    if (!fileChooser.browseForFileToOpen()) {
        return;
    }
    const auto fileToLoad = fileChooser.getResult();
    const auto xmlRootElement = juce::XmlDocument::parse(fileToLoad);
    if (xmlRootElement == nullptr) {
        jassertfalse;       // Wrong format?
        return;
    }

    // Loads the file.
    const auto defaultStoredLookAndFeel = ThemeDefaultBuilder::build();
    XmlSerializer xmlSerializer;
    auto newTheme = xmlSerializer.deserialize(*xmlRootElement, *defaultStoredLookAndFeel);

    // Creates a new local entry.
    addNewProfileAndSelect(std::move(newTheme));
}

}   // namespace arkostracker
