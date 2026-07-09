#include "AppMenuBarModel.h"

#include "../../../app/preferences/PreferencesManager.h"
#include "../../../controllers/MainController.h"
#include "../../containerArranger/GlobalArrangementController.h"
#include "../../keyboard/CommandIds.h"
#include "../../mainView/CommandPerformer.h"

namespace arkostracker 
{

AppMenuBarModel::AppMenuBarModel(CommandPerformer& pCommandPerformer, MainController& pMainController) :
        commandPerformer(pCommandPerformer),
        mainController(pMainController)
{
}

juce::StringArray AppMenuBarModel::getMenuBarNames()
{
    juce::StringArray names;
    names.add(juce::translate("File"));
    names.add(juce::translate("Edit"));
    names.add(juce::translate("Tools"));
    names.add(juce::translate("Help"));
    return names;
}

juce::PopupMenu AppMenuBarModel::getMenuForIndex(int topLevelMenuIndex, const juce::String& /*menuName*/)
{
    // Called to display each menu.
    auto& commandManager = mainController.getCommandManager();

    // Creates the menus.
    juce::PopupMenu menu;
    switch (topLevelMenuIndex) {
        case static_cast<int>(MenuIds::file): {
            juce::PopupMenu recentFilesSubMenu;
            const auto areRecentFilesPresent = fillRecentFilesSubMenu(recentFilesSubMenu, commandManager);

            // Creates the Export submenu.
            juce::PopupMenu exportSubMenu;
            exportSubMenu.addCommandItem(&commandManager, quickExport, juce::translate("Quick export for testing"));
            exportSubMenu.addCommandItem(&commandManager, quickExportToBasic, juce::translate("Quick export to Basic (CPC)"));
            exportSubMenu.addSeparator();
            exportSubMenu.addCommandItem(&commandManager, exportToAkg, juce::translate("Export to AKG (generic)"));
            exportSubMenu.addCommandItem(&commandManager, exportToAkm, juce::translate("Export to AKM (minimalist)"));
            exportSubMenu.addCommandItem(&commandManager, exportToAky, juce::translate("Export to AKY (streamed)"));
            exportSubMenu.addCommandItem(&commandManager, exportToFap, juce::translate("Export to FAP (for demos)"));
            exportSubMenu.addSeparator();
            exportSubMenu.addCommandItem(&commandManager, exportToYm, juce::translate("Export to YM"));
            exportSubMenu.addCommandItem(&commandManager, exportYmToFap, juce::translate("Export YM to FAP"));
            exportSubMenu.addCommandItem(&commandManager, exportToVgm, juce::translate("Export to VGM"));
            exportSubMenu.addCommandItem(&commandManager, exportToWav, juce::translate("Export to WAV"));
            exportSubMenu.addCommandItem(&commandManager, exportInstrumentToWav, juce::translate("Export instrument to WAV"));
            exportSubMenu.addCommandItem(&commandManager, exportToMidi, juce::translate("Export to MIDI"));
            exportSubMenu.addCommandItem(&commandManager, exportToMod, juce::translate("Export to MOD"));
            exportSubMenu.addSeparator();
            exportSubMenu.addCommandItem(&commandManager, exportSfxs, juce::translate("Export sound effects (AKX)"));
            exportSubMenu.addCommandItem(&commandManager, exportEvents, juce::translate("Export events"));
            exportSubMenu.addCommandItem(&commandManager, exportSamples, juce::translate("Export samples"));
            exportSubMenu.addSeparator();
            exportSubMenu.addCommandItem(&commandManager, exportToText, juce::translate("Export to TXT"));
            exportSubMenu.addCommandItem(&commandManager, exportToRaw, juce::translate("Export to RAW (AKR)"));
            exportSubMenu.addCommandItem(&commandManager, exportToRawLinear, juce::translate("Export to RAW Linear"));

            // Creates the Setup submenu.
            juce::PopupMenu setupSubMenu;
            setupSubMenu.addCommandItem(&commandManager, setupGeneral, juce::translate("General"));
            setupSubMenu.addCommandItem(&commandManager, setupKeyboardMapping, juce::translate("Keyboard mapping"));
            setupSubMenu.addSeparator();
            setupSubMenu.addCommandItem(&commandManager, setupSourceProfile, juce::translate("Source profile"));
            setupSubMenu.addSeparator();
            setupSubMenu.addCommandItem(&commandManager, setupAudioInterface, juce::translate("Audio/MIDI interfaces"));
            setupSubMenu.addCommandItem(&commandManager, setupOutput, juce::translate("Output/mix"));
            setupSubMenu.addSeparator();
            setupSubMenu.addCommandItem(&commandManager, setupPatternViewer, juce::translate("Pattern viewer"));
            setupSubMenu.addCommandItem(&commandManager, setupThemeEditor, juce::translate("Theme editor"));
            setupSubMenu.addCommandItem(&commandManager, setupSerial, juce::translate("Serial communication"));

            menu.addCommandItem(&commandManager, newSong, juce::translate("New song"));
            menu.addCommandItem(&commandManager, loadSong, juce::translate("Open song"));
            menu.addSubMenu(juce::translate("Open recent song"), recentFilesSubMenu, areRecentFilesPresent);
            menu.addCommandItem(&commandManager, saveSong, juce::translate("Save song"));
            menu.addCommandItem(&commandManager, saveSongAs, juce::translate("Save song as..."));
            menu.addCommandItem(&commandManager, saveCopy, juce::translate("Save a copy"));
            menu.addSeparator();
            menu.addCommandItem(&commandManager, mergeSong, juce::translate("Merge song"));
            menu.addCommandItem(&commandManager, loadInstrument, juce::translate("Load instrument"));
            menu.addSeparator();
            menu.addSubMenu(juce::translate("Export"), exportSubMenu);
            menu.addSeparator();
            menu.addSubMenu(juce::translate("Setup"), setupSubMenu);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::quit, juce::translate("Quit"));
            break;
        }
        case static_cast<int>(MenuIds::edit): {
            // Creates the Restore Layout submenu.
            juce::PopupMenu layoutSubMenu;
            static constexpr auto layoutCommands = {
                    restoreLayout0, restoreLayout1,
                    restoreLayout2, restoreLayout3,
            };
            jassert(layoutCommands.size() == static_cast<size_t>(GlobalArrangementController::layoutCount));

            // Adds one command for each.
            auto commandIndex = 0;
            for (const auto& command : layoutCommands) {
                layoutSubMenu.addCommandItem(&commandManager, command, translate("Change layout " + juce::String(commandIndex + 1)));
                ++commandIndex;
            }

            const auto undoActionName = mainController.getUndoDescription();
            const auto redoActionName = mainController.getRedoDescription();

            // Creates the Subsongs submenus.
            juce::PopupMenu subsongsSubMenu;
            // Creates as many entries as there are Subsongs.
            const auto subsongNamesAndIds = mainController.getSongController().getSong()->getSubsongNamesAndIds();
            const auto currentSubsongId = mainController.getSongController().getCurrentSubsongId();
            auto subsongIndex = 0;
            for (const auto& [subsongName, pSubsongId] : subsongNamesAndIds) {
                const auto& subsongId = pSubsongId;
                const auto text = juce::translate("Edit ") + subsongName + " (" + juce::String(subsongIndex) + juce::translate(") properties");
                subsongsSubMenu.addItem(text, [&, subsongId] {
                    commandPerformer.openEditSubsongPanel(subsongId);
                });

                ++subsongIndex;
            }
            subsongsSubMenu.addSeparator();
            subsongsSubMenu.addItem(juce::translate("Add new subsong"), [&] { commandPerformer.openAddNewSubsongPanel(); });
            subsongsSubMenu.addSeparator();

            subsongIndex = 0;
            for (const auto& [subsongName, pSubsongId] : subsongNamesAndIds) {
                const auto& subsongId = pSubsongId;
                const auto text = juce::translate("Select ") + subsongName +  " (" + juce::String(subsongIndex) + ")";
                // Prevents the current Subsong from being selectable for a Goto. We're already there!
                const auto isDifferentSubsong = (subsongId != currentSubsongId);
                subsongsSubMenu.addItem(text, isDifferentSubsong, false, [&, subsongId] {
                    commandPerformer.switchToSubsong(subsongId);
                });

                ++subsongIndex;
            }

            // Creates the menu itself.
            menu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::undo, juce::translate("Undo ") + undoActionName);
            menu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::redo, juce::translate("Redo ") + redoActionName);
            menu.addSeparator();
            menu.addCommandItem(&commandManager, editSongProperties, "Song properties");
            menu.addSubMenu(juce::translate("Subsongs..."), subsongsSubMenu);
            menu.addSeparator();
            menu.addSubMenu(juce::translate("Change layout"), layoutSubMenu);
            break;
        }
        case static_cast<int>(MenuIds::tools): {
            juce::PopupMenu patternsSubMenu;
            patternsSubMenu.addCommandItem(&commandManager, rearrangePatterns, juce::translate("Rearrange"));
            patternsSubMenu.addCommandItem(&commandManager, rearrangeAndDeleteUnusedPatterns, juce::translate("Rearrange and delete unused"));
            patternsSubMenu.addCommandItem(&commandManager, clearPatterns, juce::translate("Clear all"));

            menu.addCommandItem(&commandManager, openStreamedMusicAnalyzer, juce::translate("Streamed music analyzer"));
            menu.addSeparator();
            menu.addCommandItem(&commandManager, openErrorChecker, juce::translate("Check for errors"));
            menu.addCommandItem(&commandManager, openInstrumentOptimizer, juce::translate("Optimize instruments"));
            menu.addSubMenu(juce::translate("Patterns..."), patternsSubMenu);
            break;
        }
        case static_cast<int>(MenuIds::help):
            menu.addCommandItem(&commandManager, openAboutPage, juce::translate("About"));
            break;
        default:
            jassertfalse;       // Shouldn't happen.
            break;
    }

    return menu;
}

void AppMenuBarModel::menuItemSelected(int /*menuItemID*/, int /*topLevelMenuIndex*/)
{
    // Nothing to do.
}

bool AppMenuBarModel::fillRecentFilesSubMenu(juce::PopupMenu& recentFilesSubMenu, juce::ApplicationCommandManager& commandManager) const noexcept
{
    const auto& preferencesManager = PreferencesManager::getInstance();

    const auto paths = preferencesManager.getRecentFiles();
    jassert(paths.size() < 50U);     // Too many were encoded!

    // No paths? Then don't show anything.
    if (paths.empty()) {
        return false;
    }

    // Creates one item for each path.
    auto itemIndex = 0;
    for (const auto& path : paths) {
        juce::PopupMenu::Item item;
        item.text = path;
        item.itemID = recentFileFirstId + itemIndex;
        const juce::ReferenceCountedObjectPtr recentFileClickCallback(new RecentFileClickCallback(path, mainController));
        item.customCallback = recentFileClickCallback;
        recentFilesSubMenu.addItem(item);

        ++itemIndex;
    }

    recentFilesSubMenu.addSeparator();
    recentFilesSubMenu.addCommandItem(&commandManager, clearRecentFiles);

    return true;
}



// MainMenuBarModel::RecentFileClickCallback methods.
// ============================================================

AppMenuBarModel::RecentFileClickCallback::RecentFileClickCallback(juce::String pFilePath, MainController& pMainController) noexcept :
        mainController(pMainController),
        filePath(std::move(pFilePath))
{
}

bool AppMenuBarModel::RecentFileClickCallback::menuItemTriggered()
{
    mainController.loadSong(filePath);

    // Don't do anything else.
    return false;
}

}   // namespace arkostracker
