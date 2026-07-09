#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../song/psg/Psg.h"
#include "../components/GroupWithViewport.h"
#include "../components/ButtonWithImage.h"

namespace arkostracker 
{

/** Shows the info of a PSG, to be used by the Subsong Properties panel. */
class PsgView final : public juce::Component
{
public:
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * Called when the Edit PSG Button is clicked.
         * @param psgIndex the PSG index.
         */
        virtual void onEditPsgButtonClicked(int psgIndex) noexcept = 0;

        /**
         * Called when the Delete PSG Button is clicked.
         * @param psgIndex the PSG index.
         */
        virtual void onDeletePsgButtonClicked(int psgIndex) noexcept = 0;

        /**
         * Called when the Add PSG Button is clicked.
         * @param psgIndex the PSG index.
         */
        virtual void onAddPsgButtonClicked(int psgIndex) noexcept = 0;
    };

    /**
     * Constructor.
     * @param listener the listener to the events of this Component.
     */
    explicit PsgView(Listener& listener) noexcept;

    /**
     * Sets the data to display. Updates the UI.
     * @param psgIndex the index of the PSG.
     * @param psgCount how many PSG there are.
     * @param psg the PSG data.
     * @param canDelete true to be able to delete.
     */
    void setDisplayedData(int psgIndex, int psgCount, const Psg& psg, bool canDelete) noexcept;

    // juce::Component method implementations.
    // ==========================================
    void resized() override;

private:
    /** Called when the Edit button is clicked. */
    void onEditClicked() const noexcept;

    /** Called when the Delete button is clicked. */
    void onDeleteClicked() const noexcept;

    /** Called when the Add button is clicked. */
    void onAddButtonClicked() const noexcept;

    Listener& listener;

    GroupWithViewport group;
    juce::Label typeLabel;
    juce::Label psgFrequencyLabel;
    juce::Label referenceFrequencyLabel;
    juce::Label sampleReplayFrequencyLabel;
    ButtonWithImage editButton;
    ButtonWithImage deleteButton;
    ButtonWithImage addButton;

    int psgIndex;
};

}   // namespace arkostracker
