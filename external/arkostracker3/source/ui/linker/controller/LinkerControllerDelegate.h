#pragma once

#include <memory>

#include "../../../song/subsong/Position.h"
#include "../../components/dialogs/ModalDialog.h"
#include "../dialogs/EditPositionDialog.h"
#include "../dialogs/EditPositionsDialog.h"

namespace arkostracker 
{

class LinkerControllerImpl;

/** A friend class adding possibility to the Linker Controller, so that the class does not grow too big. */
class LinkerControllerDelegate final : public EditPositionDialog::Listener,
                                       public EditPositionsDialog::Listener
{
public:
    /**
     * Constructor.
     * @param linkerControllerImpl the class to extend the functionality of.
     */
    explicit LinkerControllerDelegate(LinkerControllerImpl& linkerControllerImpl) noexcept;

    /** Copies the selection to clipboard. If no selection, nothing happens. */
    void copySelectionToClipboard() const noexcept;
    /** Cuts the selection to clipboard. If no selection, nothing happens. */
    void cutSelectionToClipboard() const noexcept;
    /** Pastes the clipboard to where the cursor is. If the clipboard cannot be deserialized, silently fails. */
    void pasteClipboard() noexcept;

    /**
     * Shows the dialog to edit one or more Positions (different UI if one).
     * @param positionIndexes the Positions.
     */
    void showEditPositionDialog(const std::set<int>& positionIndexes) noexcept;

    /**
     * Called when the user wants to increase/decrease the Pattern value. It may be accepted or not (out of bounds for example).
     * @param position the Position where to change the Pattern.
     * @param offset may be positive or negative.
     */
    void onWantToIncDecPatternItemValue(int position, int offset) const noexcept;

    /**
     * Called when the use right-clicked on a Pattern item.
     * @param position the position the item is linked to.
     */
    void onPatternItemRightClicked(int position) const noexcept;

    // EditPositionDialog::Listener method implementations.
    // =========================================================
    void onWantToCancelEditPosition() override;
    void onWantToEditPosition(int positionIndex, Position position, Pattern pattern) override;

    // EditPositionsDialog::Listener method implementations.
    // =========================================================
    void onWantToEditPositions(std::set<int> positionIndexes, OptionalInt newHeight, OptionalValue<juce::uint32> newPatternColorArgb) override;

private:
    /** Copies the selection to clipboard. If no selection, nothing happens. */
    void copySelectionToClipboardInternal() const noexcept;

    LinkerControllerImpl& linkerControllerImpl;
    std::unique_ptr<ModalDialog> modalDialog;
};

}   // namespace arkostracker
