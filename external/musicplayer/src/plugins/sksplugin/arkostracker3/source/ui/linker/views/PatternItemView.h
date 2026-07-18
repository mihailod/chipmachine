#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../utils/MouseChanger.h"

namespace arkostracker 
{

/** Shows the pattern number, its color, its possible selection state, a transposition flag. */
class PatternItemView final : public juce::Component,
                              public juce::DragAndDropTarget,
                              public MouseChanger         // The drag'n'drop Container will make this Component set the mouse back to normal once the d'n'd is finished.
{
public:
    /** Listener to the events. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the use clicked on the item.
         * @param position the position the item is linked to.
         * @param down true if the button is down, false if up.
         * @param shift true if Shift if pressed.
         * @param control true if Control if pressed.
         */
        virtual void onPatternItemClicked(int position, bool down, bool shift, bool control) = 0;

        /**
         * Called when the user double-clicked on the item.
         * @param position the position the item is linked to.
         */
        virtual void onPatternItemDoubleClicked(int position) = 0;

        /**
         * Called when the user wants to edit a position.
         * @param position the position the item is linked to.
         */
        virtual void onUserWantsToEditPosition(int position) = 0;

        /**
         * Called when the user right-clicked on the item.
         * @param position the position the item is linked to.
         */
        virtual void onPatternItemRightClicked(int position) = 0;

        /**
         * Called when the user made a drag'n'drop.
         * @param originalPosition the position of the original item.
         * @param destinationPosition the position of the pointed-to item. Note that, if this is superior to the original,
         * it should be considered "after" this destination position.
         * @param duplicate true to duplicate, false to move.
         */
        virtual void onWantToMoveOrDuplicatePatternItemView(int originalPosition, int destinationPosition, bool duplicate) = 0;

        /**
         * Called when the mouse wheel has been used on the item, maybe with modifier.
         * @param originalPosition the position of the original item.
         * @param offset may be positive or negative.
         */
        virtual void onMouseWheelOnPatternItem(int originalPosition, int offset) = 0;

        /**
         * Called when the Add New icon is clicked.
         * @param position the index of the selected position.
         */
        virtual void onAddNewPositionClicked(int position) noexcept = 0;

        /**
         * Called when the Clone icon is clicked.
         * @param position the index of the selected position.
         */
        virtual void onClonePositionClicked(int position) noexcept = 0;

        /**
         * Called when the Repeat icon is clicked.
         * @param position the index of the selected position.
         */
        virtual void onRepeatPositionClicked(int position) noexcept = 0;
    };

    /** Holds the data and flags to display. Immutable! */
    class DisplayedData
    {
    public:
        /**
         * Constructor.
         * @param position the position.
         * @param patternIndex the index to display.
         * @param patternColor the color of the Pattern.
         * @param withinPlayRange true if the Pattern can be read (that is, before the end loop).
         * @param withTransposition true if a transposition if used in any Track of this Position.
         * @param selected true if selected.
         * @param current true if this is the currently viewed item. May not be selected. For example, it can be the pattern being viewed in the PV.
         */
        DisplayedData(int position, int patternIndex, juce::Colour patternColor, bool withinPlayRange, bool withTransposition, bool selected,
                      bool current) noexcept;

        int getPosition() const noexcept;
        int getPatternIndex() const noexcept;
        juce::Colour getPatternColor() const noexcept;
        bool isWithinPlayRange() const noexcept;
        bool isWithTransposition() const noexcept;
        bool isSelected() const noexcept;
        bool isCurrent() const noexcept;

        bool operator==(const DisplayedData& rhs) const; // NOLINT(fuchsia-overloaded-operator)
        bool operator!=(const DisplayedData& rhs) const; // NOLINT(fuchsia-overloaded-operator)

    private:
        int position;
        int patternIndex;
        juce::Colour patternColor;
        bool withinPlayRange;
        bool withTransposition;
        bool selected;
        bool current;
    };

    /** Data of a Pattern Item View being dragged. */
    class DraggedPatternItemView final : public juce::ReferenceCountedObject
    {
    public:
        /**
         * Constructor.
         * @param pPosition the position (>=0).
         * @param pDuplicate true to duplicate, false to move only.
         */
        explicit DraggedPatternItemView(const int pPosition, const bool pDuplicate) noexcept :
                position(pPosition),
                duplicate(pDuplicate)
        {
        }

        int getPosition() const noexcept
        {
            return position;
        }

        bool isDuplicated() const noexcept
        {
            return duplicate;
        }

    private:
            int position;
            bool duplicate;
    };


    // ==========================================================================

    /**
     * Constructor.
     * @param dragAndDropContainer the container of drag'n'drop. Should be the parent.
     * @param listener the listener, to be aware of the events.
     * @param displayedData the data to display.
     */
    PatternItemView(juce::DragAndDropContainer& dragAndDropContainer, Listener& listener, DisplayedData displayedData) noexcept;

    /**
     * Refreshes the UI, if needed.
     * @param displayedData the data to display.
     */
    void refreshUi(const DisplayedData& displayedData) noexcept;

    // Component method implementations.
    // ===================================
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    // DragAndDropTarget method implementations.
    // ===========================================
    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

private:
    static const int draggedOverRectangleSize;

    /**
     * Sets the "dragged over" flags, and repaint.
     * @param draggedOverLeft true if dragged over, left part.
     * @param draggedOverRight true if dragged over, right part.
     */
    void setDraggedOverAndRepaint(bool draggedOverLeft, bool draggedOverRight) noexcept;

    /** @return the bounds of the swatch. */
    juce::Rectangle<int> getSwatchBounds() const noexcept;

    juce::DragAndDropContainer& dragAndDropContainer;
    Listener& listener;
    DisplayedData displayedData;
    bool draggedOverLeft;
    bool draggedOverRight;
};

}   // namespace arkostracker
