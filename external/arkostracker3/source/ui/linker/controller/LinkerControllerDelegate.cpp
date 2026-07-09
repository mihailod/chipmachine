#include "LinkerControllerDelegate.h"

#include "../../../business/serialization/song/PositionSerializer.h"
#include "../../../controllers/MainController.h"
#include "../../../controllers/SongController.h"
#include "../../../utils/NumberUtil.h"
#include "../../keyboard/CommandIds.h"
#include "../../utils/Clipboard.h"
#include "LinkerControllerImpl.h"

namespace arkostracker 
{

LinkerControllerDelegate::LinkerControllerDelegate(LinkerControllerImpl& pLinkerControllerImpl) noexcept :
        linkerControllerImpl(pLinkerControllerImpl),
        modalDialog()
{
}


// EditPositionDialog::Listener method implementations.
// =========================================================

void LinkerControllerDelegate::onWantToCancelEditPosition()
{
    modalDialog.reset();
}

void LinkerControllerDelegate::onWantToEditPosition(const int positionIndex, const Position position, const Pattern pattern)
{
    modalDialog.reset();

    // Modifies the Position and the Pattern.
    // TO IMPROVE: ideally, use the same transaction for these actions. However, each action is ignored if
    // nothing changes, so this mitigates the problem.
    const auto shownSubsongId = linkerControllerImpl.getShownSubsongId();
    linkerControllerImpl.songController.modifyPosition(shownSubsongId, positionIndex, position);
    const auto patternIndex = position.getPatternIndex();
    linkerControllerImpl.songController.modifyPattern(shownSubsongId, patternIndex, pattern);
}


// EditPositionsDialog::Listener method implementations.
// =========================================================

void LinkerControllerDelegate::onWantToEditPositions(const std::set<int> positionIndexes, const OptionalInt newHeight,
    const OptionalValue<juce::uint32> newPatternColorArgb)
{
    modalDialog.reset();

    linkerControllerImpl.songController.modifyPositionsData(linkerControllerImpl.getShownSubsongId(), positionIndexes, newHeight, newPatternColorArgb);
}

// =========================================================

void LinkerControllerDelegate::copySelectionToClipboard() const noexcept
{
    copySelectionToClipboardInternal();
}

void LinkerControllerDelegate::copySelectionToClipboardInternal() const noexcept
{
    // Anything to do?
    if (linkerControllerImpl.selection.isEmpty()) {
        return;
    }
    const auto selectedItems = linkerControllerImpl.selection.getSelectedItems();

    // Gets the Positions from the selected Items.
    std::vector<Position> positions;
    linkerControllerImpl.songController.performOnConstSubsong(linkerControllerImpl.getShownSubsongId(), [&](const Subsong& subsong) {
        for (const auto positionIndex : selectedItems) {
            auto position = subsong.getPosition(positionIndex);
            positions.push_back(position);
        }
    });

    // Serializes the Positions.
    const auto xmlRoot = PositionSerializer::serialize(positions);
    jassert(xmlRoot != nullptr);

    // Stores to the clipboard.
    Clipboard::storeXmlNode(xmlRoot.get());
}

void LinkerControllerDelegate::cutSelectionToClipboard() const noexcept
{
    const auto& selection = linkerControllerImpl.selection;
    if (selection.isEmpty()) {
        return;
    }
    // Just like the copy first.
    copySelectionToClipboardInternal();

    const auto shownSubsong = linkerControllerImpl.getShownSubsongId();

    // Deletes the selected Positions.
    linkerControllerImpl.songController.deletePositions(shownSubsong, selection.getSelectedItems(), false);
}

void LinkerControllerDelegate::pasteClipboard() noexcept
{
    const auto& selection = linkerControllerImpl.selection;
    auto& songController = linkerControllerImpl.songController;
    const auto shownSubsong = linkerControllerImpl.getShownSubsongId();

    if (selection.isEmpty()) {
        return;
    }

    // Tries to deserialize from the clipboard. May fail!
    const auto xmlRoot = Clipboard::retrieveXmlNode();
    if (xmlRoot == nullptr) {
        return;         // Silently fails.
    }

    const auto positions = PositionSerializer::deserializePositions(*xmlRoot);
    if (positions.empty()) {
        return;         // Silently fails.
    }

    // Are the data valid? Heights, transpositions must be within range, patterns must exist.
    auto valid = true;
    for (const auto& position : positions) {
        // Correct height?
        const auto height = position.getHeight();
        valid &= ((height >= Position::minimumPositionHeight) && (height <= Position::maximumPositionHeight));

        // Correct transpositions? We don't check the channel.
        const auto& channelIndexToTransposition = position.getChannelToTransposition();
        for (const auto& entry : channelIndexToTransposition) {
            const auto transposition = entry.second;
            valid &= ((transposition >= Position::minimumTransposition) && (transposition <= Position::maximumTransposition));
        }

        // The Patterns must exist.
        const auto patternIndex = position.getPatternIndex();
        songController.performOnConstSubsong(shownSubsong, [&] (const auto& subsong) {
            valid &= subsong.doesPatternExist(patternIndex);
        });

        if (!valid) {
            break;
        }
    }

    if (!valid) {
        return;
    }

    // Where to copy?
    const auto destinationIndex = selection.getHighest();
    songController.insertPositions(shownSubsong, positions, destinationIndex, true, false);
}

void LinkerControllerDelegate::showEditPositionDialog(const std::set<int>& positionIndexes) noexcept
{
    if (positionIndexes.empty()) {
        jassertfalse; // Should have been tested earlier.
        return;
    }
    jassert(modalDialog == nullptr);            // Already a Dialog?

    // Gathers the data to show.
    auto& songController = linkerControllerImpl.songController;
    const auto& subsongId = linkerControllerImpl.getShownSubsongId();

    // What dialog to show? Different according to the number.
    if (positionIndexes.size() == 1) {
        modalDialog = std::make_unique<EditPositionDialog>(*this, songController, subsongId, *positionIndexes.cbegin());
    } else {
        modalDialog = std::make_unique<EditPositionsDialog>(*this, songController, subsongId, positionIndexes);
    }
}

void LinkerControllerDelegate::onWantToIncDecPatternItemValue(const int position, const int offset) const noexcept
{
    if (offset == 0) {
        return;
    }

    auto& songController = linkerControllerImpl.songController;
    const auto shownSubsong = linkerControllerImpl.getShownSubsongId();

    // What is current PatternItem? Can we change it?
    std::unique_ptr<Position> newPosition;
    auto newPatternIndex = 0;
    auto success = false;
    songController.performOnConstSubsong(shownSubsong, [&](const Subsong& subsong) {
        newPosition = std::make_unique<Position>(subsong.getPosition(position));
        const auto currentPatternIndex = newPosition->getPatternIndex();

        // Makes sure the new pattern item is valid!
        const auto patternCount = subsong.getPatternCount();
        newPatternIndex = NumberUtil::correctNumber(currentPatternIndex + offset, 0, patternCount - 1);

        // No need to change anything if we're stuck on the same Pattern item.
        success = (newPatternIndex != currentPatternIndex);
    });

    if (success) {
        newPosition->setPatternIndex(newPatternIndex);
        songController.modifyPosition(shownSubsong, position, *newPosition);
    }
}

void LinkerControllerDelegate::onPatternItemRightClicked(const int position) const noexcept
{
    // If already selected, don't modify the selection.
    if (!linkerControllerImpl.selection.contains(position)) {
        linkerControllerImpl.selectItem(position);
    }

    // Technically, this should be the LinkerView showing this, but there is really no point, the view wouldn't do anything more.
    auto& commandManager = linkerControllerImpl.songController.getMainController().getCommandManager();

    juce::PopupMenu popupMenu;
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::cut, juce::translate("Cut"));
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::copy, juce::translate("Copy"));
    popupMenu.addCommandItem(&commandManager, juce::StandardApplicationCommandIDs::paste, juce::translate("Paste"));
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, linkerCreateNew, juce::translate("New"));
    popupMenu.addCommandItem(&commandManager, linkerDuplicateSelectedPositions, juce::translate("Duplicate"));
    popupMenu.addCommandItem(&commandManager, linkerCloneSelectedPositions, juce::translate("Clone"));
    popupMenu.addCommandItem(&commandManager, linkerDeleteSelectedPositions, juce::translate("Delete"));
    popupMenu.addCommandItem(&commandManager, linkerEditPositions, juce::translate("Edit"));
    popupMenu.addSeparator();
    popupMenu.addCommandItem(&commandManager, linkerMarkAsLoopStartAndEnd, juce::translate("Mark as loop"));
    popupMenu.addCommandItem(&commandManager, linkerMarkLoopStart, juce::translate("Mark as loop start"));
    popupMenu.addCommandItem(&commandManager, linkerMarkEnd, juce::translate("Mark as end"));

    popupMenu.show();
}

}   // namespace arkostracker
