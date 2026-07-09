#pragma once

#include "../../../components/ButtonWithImage.h"
#include "../AreaType.h"
#include "LeftHeaderDisplayData.h"
#include "../../../../utils/WithParent.h"

namespace arkostracker 
{

/**
 * Header at the left of each Area.
 * Holds the name of the area, the +/- buttons, and the "auto spread".
 */
class LeftHeader final : public juce::Component
{
public:
    /** Listener to be aware of the events of this class . */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the Plus button has been clicked.
         * @param areaType the area type of this header.
         */
        virtual void onLeftHeaderPlusClicked(AreaType areaType) noexcept = 0;

        /**
         * Called when the Minus button has been clicked.
         * @param areaType the area type of this header.
         */
        virtual void onLeftHeaderMinusClicked(AreaType areaType) noexcept = 0;

        /**
         * Called when the auto-spread button has been clicked.
         * @param areaType the area type of this header.
         */
        virtual void onLeftHeaderAutoSpreadClicked(AreaType areaType) noexcept = 0;

        /**
         * Called when the hide/show button has been clicked.
         * @param areaType the area type of this header.
         */
        virtual void onLeftHeaderHideClicked(AreaType areaType) noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events.
     * @param canHideRow true if this row can be hidden.
     * @param areaType the Area Type. Useful because sent in the listeners.
     * @param areaName the name to display.
     */
    LeftHeader(Listener& listener, AreaType areaType, bool canHideRow, const juce::String& areaName) noexcept;

    /**
     * Sets the data to display. The UI is refreshed if needed.
     * @param displayData the data to display.
     * @param forceRefresh true to force the refresh.
     */
    void setDisplayedData(const LeftHeaderDisplayData& displayData, bool forceRefresh = false) noexcept;

    // Component method implementations.
    // =====================================
    void resized() override;
    void lookAndFeelChanged() override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    static constexpr auto hideButtonWidth = 16;

    /** Custom mouse listener for the label. */
    class CustomMouseListener final : public MouseListener,
                                      public WithParent<LeftHeader>
    {
    public:
        explicit CustomMouseListener(LeftHeader& parent) : WithParent(parent) { }
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseEnter(const juce::MouseEvent& event) override;
        void mouseExit(const juce::MouseEvent& event) override;
    };

    /** Switches between auto-spread on/off. */
    void onAutoSpreadButtonClicked() const noexcept;

    /** Hides the row, if possible.  */
    void onUserWantsToHide() const noexcept;

    /**
     * Sets the "hide" button icon.
     * @param hidden true if hidden, false if not.
     */
    void setHiddenButton(bool hidden) noexcept;

    /** Sets the size of the area name according to the presence of the hide button. */
    void setAreaNameSize() noexcept;

    Listener& listener;
    AreaType areaType;
    bool canHideRow;

    juce::Label areaNameLabel;
    juce::ImageButton hideButton;
    ButtonWithImage plusButton;
    ButtonWithImage minusButton;

    ButtonWithImage autoSpreadButton;

    LeftHeaderDisplayData displayData;

    CustomMouseListener labelMouseListener;
};

}   // namespace arkostracker
