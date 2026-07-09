#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../../business/actions/patternViewer/SelectedData.h"

namespace arkostracker
{

class SongController;

/** Controller to the Toolbox. */
class ToolboxController
{
public:
    /** Abstract top controller to get high-level information. */
    class Controller
    {
    public:
        /** Destructor. */
        virtual ~Controller() = default;

        /** @return the selected data. */
        virtual SelectedData getSelectedData() = 0;

        /** @return the selected data, as the whole current Pattern. */
        virtual SelectedData getPatternSelection() = 0;

        /** @return the selected data, as the whole current Subsong. */
        virtual SelectedData getCurrentSubsongSelection() = 0;

        /** @return the selected data, on per whole current Subsong. */
        virtual std::vector<SelectedData> getAllSubsongsSelection() = 0;
    };

    enum class ToolType : uint8_t
    {
        transpose,
        deleteNote,
        remapInstrument,
        swapInstrument,
        swapCells,
    };

    enum class Target : uint8_t
    {
        selection,
        wholePattern,
        wholeSubsong,
        wholeSong,

        first = selection,
    };

    class DisplayedData
    {
    public:
        DisplayedData(const ToolType pToolType, juce::String pSelectedInstrumentsText, juce::String pButtonText,
            const bool pButtonEnabled, const Target pSelectionTarget, const int pTranspositionRate,
            const int pCurrentSubsongChannelCount, const int pFirstChannelIndex, const int pSecondChannelIndex) noexcept :
                toolType(pToolType),
                selectedInstrumentsText(std::move(pSelectedInstrumentsText)),
                buttonText(std::move(pButtonText)),
                buttonEnabled(pButtonEnabled),
                selectionTarget(pSelectionTarget),
                transpositionRate(pTranspositionRate),
                currentSubsongChannelCount(pCurrentSubsongChannelCount),
                firstChannelIndex(pFirstChannelIndex),
                secondChannelIndex(pSecondChannelIndex)
        {
        }

        ToolType getToolType() const noexcept
        {
            return toolType;
        }

        juce::String getSelectedInstrumentsText() const noexcept
        {
            return selectedInstrumentsText;
        }

        juce::String getButtonText() const noexcept
        {
            return buttonText;
        }

        bool isButtonEnabled() const noexcept
        {
            return buttonEnabled;
        }

        Target getSelectionTarget() const noexcept
        {
            return selectionTarget;
        }

        int getTranspositionRate() const noexcept
        {
            return transpositionRate;
        }

        int getCurrentSubsongChannelCount() const noexcept
        {
            return currentSubsongChannelCount;
        }

        int getFirstChannelIndex() const noexcept
        {
            return firstChannelIndex;
        }

        int getSecondChannelIndex() const noexcept
        {
            return secondChannelIndex;
        }

        bool operator==(const DisplayedData& other) const noexcept
        {
            return toolType == other.toolType
                   && selectedInstrumentsText == other.selectedInstrumentsText
                   && buttonText == other.buttonText
                   && buttonEnabled == other.buttonEnabled
                   && selectionTarget == other.selectionTarget
                   && transpositionRate == other.transpositionRate
                   && currentSubsongChannelCount == other.currentSubsongChannelCount
                   && firstChannelIndex == other.firstChannelIndex
                   && secondChannelIndex == other.secondChannelIndex
            ;
        }

        bool operator!=(const DisplayedData& rhs) const noexcept
        {
            return !(*this == rhs);
        }

    private:
        ToolType toolType;
        juce::String selectedInstrumentsText;
        juce::String buttonText;
        bool buttonEnabled;
        Target selectionTarget;
        int transpositionRate;
        int currentSubsongChannelCount;
        int firstChannelIndex;
        int secondChannelIndex;
    };

    /** Destructor. */
    virtual ~ToolboxController() = default;

    /**
     * Shows the toolbox.
     * @param parent the parent to add the view.
     * @param toolBoxButtonX as an indication of where to display the toolbox, the X of the toolbox button.
     * @param toolBoxButtonY as an indication of where to display the toolbox, the Y of the toolbox button.
     */
    virtual void show(juce::Component& parent, int toolBoxButtonX, int toolBoxButtonY) noexcept = 0;
    /** Hides the UI. */
    virtual void hide() noexcept = 0;

    /**
     * Changes the position of the toolbox.
     * @param toolBoxButtonX as an indication of where to display the toolbox, the X of the toolbox button.
     * @param toolBoxButtonY as an indication of where to display the toolbox, the Y of the toolbox button.
     */
    virtual void locate(int toolBoxButtonX, int toolBoxButtonY) noexcept = 0;

    /** Sets the selected target. Refreshes the UI. */
    virtual void onUserSelectedTarget(Target target) noexcept = 0;

    /** Sets the first channel to swap. Refreshes the UI. */
    virtual void onUserWantsToChangeFirstChannelToSwap(int channelIndex) noexcept = 0;
    /** Sets the second channel to swap. Refreshes the UI. */
    virtual void onUserWantsToChangeSecondChannelToSwap(int channelIndex) noexcept = 0;

    /** A user selected a new tool type. Refreshes the UI. */
    virtual void onUserChangedToolType(ToolType toolType) noexcept = 0;

    /** The user wants to change the transposition rate. Refreshes the UI. */
    virtual void onUserWantsToChangeTranspositionRate(int newTranspositionRate) noexcept = 0;

    /** Performs the action, if possible. */
    virtual void perform() noexcept = 0;

    /**
     * Called when the song metadata has changed. The UI may need to be refreshed.
     * @param subsongId the ID of the displayed Subsong.
     */
    virtual void onSongMetadataChanged(const Id& subsongId) noexcept = 0;

    /**
     * Called when the subsong metadata has changed. The UI may need to be refreshed.
     * @param subsongId the ID of the displayed Subsong.
     */
    virtual void onSubsongMetadataChanged(const Id& subsongId) noexcept = 0;
};

} // namespace arkostracker
