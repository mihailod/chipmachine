#pragma once

#include "../../../../../song/subsong/LinkState.h"
#include "../../../../../utils/OptionalValue.h"
#include "../../../../components/ButtonWithImage.h"
#include "../../../../components/ClickableLabel.h"
#include "../../../../components/PeriodAndNoiseMeter.h"

namespace arkostracker 
{

/** The header at the top of a (music) Track. */
class TrackHeader final : public juce::Component
{
public:
    /** Listener to the events of this Component. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** The user left-clicked on the channel number. */
        virtual void onUserLeftClickedOnChannelNumber() = 0;
        /** The user right-clicked on the channel number. */
        virtual void onUserRightClickedOnChannelNumber() = 0;
        /** The user wants to change the transposition. */
        virtual void onUserWantsToChangeTransposition() = 0;
        /**
         * The user wants the name the track.
         * @param currentName the current name for display. This is a simplification to bypass the need to ask for it to the controller.
         * @param currentColor the possible current color for display. This is a simplification to bypass the need to ask for it to the controller.
         */
        virtual void onUserWantsToNameTrack(const juce::String& currentName, OptionalValue<juce::Colour> currentColor) = 0;
        /** The user wants to link/unlink. */
        virtual void onUserWantsToManageLink() = 0;
    };

    /** Holder of the data to display. */
    class DisplayedData
    {
    public:
        /**
         * Constructor.
         * @param channelOn true if the channel is on.
         * @param channelNumber the channel number (>0).
         * @param transposition the transposition.
         * @param trackName the name of the track.
         * @param trackColor the possible color of the track.
         * @param linkState the state of the link of the Track.
         */
        DisplayedData(bool channelOn, int channelNumber, int transposition, juce::String trackName, OptionalValue<juce::Colour> trackColor, LinkState linkState) noexcept;

        bool isChannelOn() const noexcept;
        int getChannelNumber() const noexcept;
        int getTransposition() const noexcept;
        const juce::String& getTrackName() const noexcept;
        OptionalValue<juce::Colour> getTrackColor() const noexcept;
        LinkState getLinkState() const noexcept;

        bool operator==(const DisplayedData& rhs) const;
        bool operator!=(const DisplayedData& rhs) const;

    private:
        bool channelOn;
        int channelNumber;
        int transposition;
        juce::String trackName;
        OptionalValue<juce::Colour> trackColor;
        LinkState linkState;
    };


    // =====================================

    /**
     * Constructor.
     * @param listener the listener to the event of this Component.
     */
    explicit TrackHeader(Listener& listener) noexcept;

    // Component method implementations.
    // =====================================
    void resized() override;
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

    // =====================================

    /** Sets the data to display. This causes a repaint. This does NOT include the VuMeter. */
    void setDisplayedData(DisplayedData displayedData) noexcept;

    /**
     * Updates the Meter.
     * @param periodAndNoiseMeterInput the periods and volume to show.
     */
    void updateMeterAndRefresh(const PeriodAndNoiseMeterInput& periodAndNoiseMeterInput) noexcept;

private:
    static const float cornerSize;
    static const float channelPathWidth;
    static const float channelFontHeight;
    static const float pathSeparation;                          // Space between the channel and the rest.
    static const float displayedDataX;                          // Starts of the displayed data (after the channel).
    static const float alphaButtonsUseless;
    static const int trackColorHeight;

    /** Called when the transposition button is clicked. */
    void onTranspositionButtonClicked() const noexcept;
    /** Called when the Link button is clicked. */
    void onLinkButtonClicked() const noexcept;

    Listener& listener;
    juce::Path channelBackgroundPath;                   // The cached path of the channel.
    juce::Path corneredBackgroundPath;                  // The cached path of the background.

    ClickableLabel trackNameLabel;
    juce::TextButton transpositionButton;
    ButtonWithImage linkButton;

    juce::Font channelFont;

    TrackHeader::DisplayedData displayedData;

    PeriodAndNoiseMeter meter;
};

}   // namespace arkostracker
