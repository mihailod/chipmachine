#pragma once

#include <unordered_map>
#include <unordered_set>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../../controllers/model/Follow.h"
#include "../../../song/cells/Effect.h"
#include "../../../song/subsong/LinkState.h"
#include "../../../song/tracks/SpecialTrack.h"
#include "../../../song/tracks/Track.h"
#include "../../components/PeriodAndNoiseMeterInput.h"
#include "../CursorLocation.h"
#include "../controller/Selection.h"

namespace arkostracker 
{

class PatternViewerController;

/** Abstract View of the Pattern Viewer (holds all the tracks, Viewport, and the components around). */
class PatternViewerView : public juce::Component
{
public:
    /** The metadata to show. This represents all that is not "the song", but does not change too often (that is, NOT the line, NOT the cursor position). */
    class DisplayedMetadata
    {
    public:
        DisplayedMetadata(std::unordered_set<int> pMutedChannelIndexes, std::unordered_set<int> pMinimizedChannelIndexes,
                          std::unordered_map<Effect, juce::juce_wchar> pEffectToChar,
                          std::unordered_map<int, juce::uint32> pInstrumentToColorArgb,
                          const bool pShowLineNumbersInHexadecimal, const int pFontSize, const int pChannelCount,
                          const int pHighlightStep, const int pSecondaryHighlight, const bool pZoomMiddleLine,
                          const int pArpeggioCount, const int pPitchCount) :
                mutedChannelIndexes(std::move(pMutedChannelIndexes)),
                minimizedChannelIndexes(std::move(pMinimizedChannelIndexes)),
                effectToChar(std::move(pEffectToChar)),
                instrumentToColorArgb(std::move(pInstrumentToColorArgb)),
                showLineNumbersInHexadecimal(pShowLineNumbersInHexadecimal),
                fontSize(pFontSize),
                channelCount(pChannelCount),
                highlightStep(pHighlightStep),
                secondaryHighlight(pSecondaryHighlight),
                zoomMiddleLine(pZoomMiddleLine),
                arpeggioCount(pArpeggioCount),
                pitchCount(pPitchCount)
        {
        }

        const std::unordered_set<int>& getMutedChannelIndexes() const noexcept
        {
            return mutedChannelIndexes;
        }

        const std::unordered_set<int>& getMinimizedChannelIndexes() const noexcept
        {
            return minimizedChannelIndexes;
        }

        bool areShowLineNumbersInHexadecimal() const noexcept
        {
            return showLineNumbersInHexadecimal;
        }

        int getFontSize() const noexcept
        {
            return fontSize;
        }

        int getChannelCount() const noexcept
        {
            return channelCount;
        }

        const std::unordered_map<Effect, juce::juce_wchar>& getEffectToChar() const noexcept
        {
            return effectToChar;
        }

        const std::unordered_map<int, juce::uint32>& getInstrumentToColorArgb() const noexcept
        {
            return instrumentToColorArgb;
        }

        int getHighlightStep() const noexcept
        {
            return highlightStep;
        }

        int getSecondaryHighlight() const noexcept
        {
            return secondaryHighlight;
        }

        bool mustZoomMiddleLine() const noexcept
        {
            return zoomMiddleLine;
        }

        int getArpeggioCount() const noexcept
        {
            return arpeggioCount;
        }

        int getPitchCount() const noexcept
        {
            return pitchCount;
        }

        bool operator==(const DisplayedMetadata& rhs) const
        {
            return mutedChannelIndexes == rhs.mutedChannelIndexes &&
                   minimizedChannelIndexes == rhs.minimizedChannelIndexes &&
                   showLineNumbersInHexadecimal == rhs.showLineNumbersInHexadecimal &&
                   effectToChar == rhs.effectToChar &&
                   instrumentToColorArgb == rhs.instrumentToColorArgb &&
                   fontSize == rhs.fontSize &&
                   channelCount == rhs.channelCount &&
                   highlightStep == rhs.highlightStep &&
                   secondaryHighlight == rhs.secondaryHighlight &&
                   zoomMiddleLine == rhs.zoomMiddleLine &&
                   arpeggioCount == rhs.arpeggioCount &&
                   pitchCount == rhs.pitchCount
                   ;
        }

        bool operator!=(const DisplayedMetadata& rhs) const
        {
            return !(rhs == *this);
        }

    private:
        std::unordered_set<int> mutedChannelIndexes;                    // The indexes of the muted channels.
        std::unordered_set<int> minimizedChannelIndexes;                // The indexes of the minimized channels.
        std::unordered_map<Effect, juce::juce_wchar> effectToChar;      // Links an effect to a displayed char.
        std::unordered_map<int, juce::uint32> instrumentToColorArgb;
        bool showLineNumbersInHexadecimal;
        int fontSize;
        int channelCount;
        int highlightStep;
        int secondaryHighlight;
        bool zoomMiddleLine;
        int arpeggioCount;
        int pitchCount;
    };

    /** The data to display, but maybe null if there is no new data. */
    class DisplayedData
    {
    public:
        /**
         * Constructor.
         * @param pLine the "middle" line to show.
         * @param pCursorLocation the cursor location.
         * @param pHeight the height of the position.
         * @param pChannelToChangedTrack link channels to the tracks that have changed. If not present, it means "none".
         * @param pChannelIndexToLinkState link channels to the links that has changed. All should be present.
         * @param pSpeedTrack the speed track, or null if not changed.
         * @param pEventTrack the event track, or null if not changed.
         * @param pSpeedTrackLinkState the link state for the speed track.
         * @param pEventTrackLinkState the link state for the event track.
         * @param pChannelToTransposition map linking a channel to its transposition. If the channel is not present, it means there is no transposition for it.
         * @param pPlayedLocationLine the played line location, if should be displayed.
         * @param pSelection the selection.
         * @param pLineSelection the selection on the lines on the left.
         */
        DisplayedData(const int pLine, const CursorLocation pCursorLocation, const int pHeight, std::unordered_map<int, const Track*> pChannelToChangedTrack,
                      std::unordered_map<int, LinkState> pChannelIndexToLinkState,
                      const SpecialTrack* pSpeedTrack, const SpecialTrack* pEventTrack, const LinkState pSpeedTrackLinkState, const LinkState pEventTrackLinkState,
                      std::unordered_map<int, int> pChannelToTransposition,
                      const OptionalInt pPlayedLocationLine, const Selection& pSelection, const Selection& pLineSelection) noexcept :
                line(pLine),
                cursorLocation(pCursorLocation),
                height(pHeight),
                channelToChangedTrack(std::move(pChannelToChangedTrack)),
                channelIndexToLinkState(std::move(pChannelIndexToLinkState)),
                speedTrack(pSpeedTrack),
                eventTrack(pEventTrack),
                speedTrackLinkState(pSpeedTrackLinkState),
                eventTrackLinkState(pEventTrackLinkState),
                channelToTransposition(std::move(pChannelToTransposition)),
                playedLocationLine(pPlayedLocationLine),
                selection(pSelection),
                lineSelection(pLineSelection)
        {
        }

        int getLine() const noexcept
        {
            return line;
        }

        const CursorLocation& getCursorLocation() const noexcept
        {
            return cursorLocation;
        }

        int getHeight() const noexcept
        {
            return height;
        }

        /** @return null if there is no new Track, else the Track. */
        const Track* getChangedTrackFromChannel(const int channelIndex) const noexcept
        {
            const auto iterator = channelToChangedTrack.find(channelIndex);
            if (iterator == channelToChangedTrack.cend()) {
                return nullptr;
            }
            return iterator->second;
        }

        const std::unordered_map<int, LinkState>& getChannelIndexToLinkState() const noexcept
        {
            return channelIndexToLinkState;
        }

        const SpecialTrack* getSpeedTrack() const noexcept
        {
            return speedTrack;
        }

        const SpecialTrack* getEventTrack() const noexcept
        {
            return eventTrack;
        }

        LinkState getSpeedTrackLinkState() const noexcept
        {
            return speedTrackLinkState;
        }

        LinkState getEventTrackLinkState() const noexcept
        {
            return eventTrackLinkState;
        }

        const std::unordered_map<int, int>& getChannelToTransposition() const noexcept
        {
            return channelToTransposition;
        }

        const OptionalInt& getPlayedLocationLine() const noexcept
        {
            return playedLocationLine;
        }

        const Selection& getSelection() const noexcept
        {
            return selection;
        }

        const Selection& getLineSelection() const noexcept
        {
            return lineSelection;
        }

    private:
        int line;
        CursorLocation cursorLocation;
        int height;
        const std::unordered_map<int, const Track*> channelToChangedTrack;
        const std::unordered_map<int, LinkState> channelIndexToLinkState;
        const SpecialTrack* speedTrack;
        const SpecialTrack* eventTrack;
        const LinkState speedTrackLinkState;
        const LinkState eventTrackLinkState;
        const std::unordered_map<int, int> channelToTransposition;
        OptionalInt playedLocationLine;
        Selection selection;
        Selection lineSelection;
    };


    // =======================================================

    /**
     * Sets the metadata to display. This may update the UI if needed.
     * The Displayed Data must also be passed, because they are indistinguishable by the children, and this class does not store them.
     * However, if the metadata is already the same, nothing happens.
     * @param displayedMetadata the metadata.
     * @param displayedData the data.
     */
    virtual void setDisplayedMetadata(const DisplayedMetadata& displayedMetadata, const DisplayedData& displayedData) noexcept = 0;

    /**
     * Sets the shown line and the possible cursor, if changed. Performs a refresh if needed. This method is a bit anti-MVI but since the
     * displayed data is not an object, it is "tolerated", especially as this will be called many many times.
     * @param line the new line.
     * @param cursorLocation if present, the new position of the cursor.
     */
    virtual void setShownLineAndCursorLocation(int line, OptionalValue<CursorLocation> cursorLocation) noexcept = 0;

    /**
     * Sets the data to display. It may be the same or not, the View is judged to decide whether to refresh or not.
     * @param displayedData the data.
     */
    virtual void setDisplayedData(const DisplayedData& displayedData) noexcept = 0;

    /** @return the X (scrolling) in the Viewport. Useful when recreating the view. */
    virtual int getViewportX() const noexcept = 0;
    /**
     * Sets the X of the Viewport scrolling. Useful when recreating the view.
     * @param x the X.
     */
    virtual void setViewportX(int x) noexcept = 0;

    /**
     * Updates the meters of each track. If the data of a channel is missing, considers no new value is present, simply decreases.
     * Out of bounds data is ignored.
     * @param channelIndexToPeriodAndNoiseMeterInput a map linking a channel index to the data to display.
     */
    virtual void updateMeters(std::unordered_map<int, PeriodAndNoiseMeterInput> channelIndexToPeriodAndNoiseMeterInput) noexcept = 0;

    /**
     * Sets the recording state. This updates the view, but does not notify anything.
     * @param newRecordState true if recording.
     */
    virtual void setRecordingState(bool newRecordState) noexcept = 0;

    /**
     * Sets the follow mode. This updates the view, but does not notify anything.
     * @param newFollowMode the follow mode.
     */
    virtual void setFollowMode(Follow newFollowMode) noexcept = 0;

    /**
     * Sets the state indicating whether the instrument is written when a note is written. This updates the view, but does not notify anything.
     * @param writeInstrumentOnWriteNote true if writing the instrument too.
     */
    virtual void setWriteInstrumentState(bool writeInstrumentOnWriteNote) noexcept = 0;

    /**
     * Sets the state indicating whether the effect is locked. This updates the view, but does not notify anything.
     * @param locked true if locked.
     */
    virtual void setOverwriteDefaultEffect(bool locked) noexcept = 0;

    /**
     * Sets the default effect. This updates the view, but does not notify anything.
     * @param effect the effect.
     */
    virtual void setDefaultEffect(Effect effect) noexcept = 0;

    /**
     * Sets the shown steps. This updates the view, but does not notify anything.
     * @param steps the steps.
     */
    virtual void setSteps(int steps) noexcept = 0;

    /**
     * @return true if out of bounds (left), false if out of bounds (right), absent if not out of bounds.
     * This is useful to know if a horizontal scrolling must be performed.
     * @param mouseEvent the mouse event.
     */
    virtual OptionalBool determineOutOfBoundsDirectionFromMouseEvent(const juce::MouseEvent& mouseEvent) noexcept = 0;

    /**
     * Displays a text in the Information View.
     * @param text the text to display.
     * @param isError true if the text is considered an error.
     */
    virtual void setInformationText(const juce::String& text, bool isError) noexcept = 0;

    /**
     * The user wants to set the name and color to a Music Track.
     * @param channelIndex the channel index. Must be valid.
     * @param currentName the name for display when the pop-up opens.
     * @param currentColor the possible color for display when the pop-up opens.
     */
    virtual void onUserWantsToSetMusicTrackNameAndColor(int channelIndex, const juce::String& currentName, OptionalValue<juce::Colour> currentColor) noexcept = 0;

    /**
     * The user wants to name a Special Track.
     * @param isSpeedTrack true if speed Track, false if event.
     * @param currentName the name for display when the pop-up opens.
     */
    virtual void onUserWantsToSetSpecialTrackName(bool isSpeedTrack, const juce::String& currentName) noexcept = 0;

    /**
     * The user wants to name a Music Track.
     * @param channelIndex the channel index. Must be valid.
     */
    virtual void onUserWantsToSetLink(int channelIndex) noexcept = 0;
    /**
     * The user wants to link a Special Track.
     * @param isSpeedTrack true if speed Track, false if event.
     */
    virtual void onUserWantsToSetSpecialLink(bool isSpeedTrack) noexcept = 0;

    /**
     * Sets the text for the effect context. Only one text should be displayed at one time.
     * @param channelIndex the channel index (>=0).
     * @param text the text. May be empty, meaning nothing must be displayed.
     */
    virtual void setEffectContextText(int channelIndex, const juce::String& text) noexcept = 0;

    /** Triggers a warning (animation) because an action is performed whereas the recording is off. */
    virtual void showWarningBecauseRecordingOff() noexcept = 0;
};

}   // namespace arkostracker
