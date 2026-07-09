#pragma once

#include "../../../business/model/Speed.h"
#include "../../../song/instrument/psg/PsgInstrumentCellLink.h"
#include "../../editorWithBars/controller/HeaderController.h"
#include "../../instrumentEditor/controller/ItemEditorController.h"
#include "../view/component/BarAreaSize.h"
#include "BarAreaController.h"
#include "BarEditorCursorLocation.h"

namespace arkostracker 
{

/** Abstract controller for any EditorWithBars. */
class EditorWithBarsController : public ItemEditorController,
                                 public HeaderController,
                                 public BarAreaController
{
public:
    /** Destructor. */
    ~EditorWithBarsController() override = default;

    /**
     * Called when a Cell has changed. What UI part to update?
     * @param newItemId the index of the item (expression, instrument, etc).
     * @param cellIndex the cell index.
     * @param mustAlsoRefreshPastIndex true if the past indexes must also be refreshed (in case of insert, spread, etc.).
     */
    virtual void onItemCellChanged(const Id& newItemId, int cellIndex, bool mustAlsoRefreshPastIndex) noexcept = 0;

    /**
     * Called when the item metadata has changed. Should the UI be updated?
     * @param newItemId the index of the item (expression, instrument, etc.).
     * @param whatChanged the flag about what has changed. The implementation should know what to do with that according to the observer it registers to.
     */
    virtual void onItemMetadataChanged(const Id& newItemId, unsigned int whatChanged) noexcept = 0;

    /**
     * Called when the selected item has changed, and the UI must be updated (because this has been already checked).
     * @param newSelectedItemId the newly selected item id. It *may* be the same, but the update must be performed anyway (after a delete).
     */
    virtual void onSelectedItemChangedMustRefreshUi(const Id& newSelectedItemId) noexcept = 0;

    /** @return the id of the possibly shown item. */
    virtual OptionalId getCurrentlyShownItemId() const noexcept = 0;

    /**
     * @return the size of the given area. It should exist, else asserts and a default value is returned.
     * @param areaType the area.
     */
    virtual BarAreaSize getAreaSize(AreaType areaType) const noexcept = 0;

    /** @return the possible index of the hovered bar (by the mouse). */
    virtual OptionalInt getHoveredBarIndex() const noexcept = 0;

    /** @return where the cursor is. */
    virtual BarEditorCursorLocation getBarEditorCursorLocation() const noexcept = 0;

    /** The user wants to insert a bar. */
    virtual void onUserWantsToInsert() noexcept = 0;
    /** The user wants to delete a bar. */
    virtual void onUserWantsToDelete() noexcept = 0;
    /** The user wants to toggle a "bar" retrig. */
    virtual void onUserWantsToToggleRetrig() noexcept = 0;
    /** The user wants to generate an increasing volume curve. */
    virtual void onUserWantsToGenerateIncreasingVolume() noexcept = 0;
    /** The user wants to generate a decreasing volume curve. */
    virtual void onUserWantsToGenerateDecreasingVolume() noexcept = 0;

    /**
     * The user wants to move the cursor in X.
     * @param left true for left, false for right.
     * @param speed how fast to go.
     */
    virtual void onUserWantsToMoveCursorInX(bool left, Speed speed) noexcept = 0;
    /**
     * The user wants to move the cursor in Y.
     * @param up true for up, false for down.
     * @param speed how fast to go.
     */
    virtual void onUserWantsToMoveCursorInY(bool up, Speed speed) noexcept = 0;

    /** The user wants to duplicate the column where the cursor is. */
    virtual void onUserWantsToDuplicateColumnWhereCursorIs() noexcept = 0;
    /** The user wants to duplicate the value where the cursor is. */
    virtual void onUserWantsToDuplicateValueWhereCursorIs() noexcept = 0;

    /**
     * The user wants to increase/decrease the value where the cursor is.
     * @param increase true for increase, false for decrease.
     * @param speed how fast to inc/decrease the value.
     */
    virtual void onUserWantsToChangeValue(bool increase, Speed speed) noexcept = 0;

    /** The user wants to reset the value where the cursor is. */
    virtual void onUserWantsToResetValue() noexcept = 0;

    /**
     * @return the parent component where the Views are put, and which inherit from juce::ApplicationCommandTarget,
     * which will be useful for showing a Context menu without flickering. Can return nullptr.
     */
    virtual juce::Component* getComponentWithTargetCommands() noexcept = 0;

    /**
     * Moves the cursor to the given location. Refreshes the UI if needed.
     * @param areaType the area type. Must be valid.
     * @param barIndex the bar index. May be invalid, will be corrected.
     */
    virtual void moveCursorLocationTo(AreaType areaType, int barIndex) noexcept = 0;

    /** The user wants to edit the value where the cursor is. */
    virtual void onUserWantsToEditValue() noexcept = 0;

    /**
     * Highlights indexes. Typically used by instrument being played.
     * @param indexes the indexes. May be empty.
     */
    virtual void highlightBarIndexes(const std::vector<int>& indexes) noexcept = 0;

    /** @return the default value for volume (thus for a software sound) (it is considered "useless" when hiding). */
    static int getDefaultVolumeValue() noexcept;
    /** @return the default value for hardware envelope (thus for a hardware sound) (it is considered "useless" when hiding). */
    static int getDefaultHardwareEnvelopeValue() noexcept;
    /** @return the default value for hardware ratio (thus for a hardware sound) (it is considered "useless" when hiding). */
    static int getDefaultHardwareRatioValue() noexcept;
    /** @return the default value for noise (it is considered "useless" when hiding). */
    static int getDefaultNoiseValue() noexcept;
    /** @return the default value for period (it is considered "useless" when hiding). */
    static int getDefaultPeriodValue() noexcept;
    /** @return the default value for pitch (it is considered "useless" when hiding). */
    static int getDefaultPitchValue() noexcept;
    /** @return the default value for arpeggio octave (it is considered "useless" when hiding). */
    static int getDefaultArpeggioOctaveValue() noexcept;
    /** @return the default value for arpeggio note in octave (it is considered "useless" when hiding). */
    static int getDefaultArpeggioNoteInOctaveValue() noexcept;
    /** @return the default value for link (it is considered "useless" when hiding). */
    static PsgInstrumentCellLink getDefaultSoundTypeValue() noexcept;

    static bool getDefaultSidIsActivated() noexcept;
    static int getDefaultSidBalancePercent() noexcept;
    static int getDefaultSidLowShelfVolume() noexcept;
    static int getDefaultSidArpeggio() noexcept;
    static int getDefaultSidPitch() noexcept;
};

}   // namespace arkostracker
