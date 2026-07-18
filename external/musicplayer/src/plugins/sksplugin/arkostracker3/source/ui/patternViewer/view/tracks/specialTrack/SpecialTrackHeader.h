#pragma once

#include "../BaseHeaderView.h"
#include "../../../../../song/subsong/LinkState.h"

namespace arkostracker 
{

/** An abstract header for the special tracks, showing an image in the header. */
class SpecialTrackHeader : public BaseHeaderView,
                           public juce::SettableTooltipClient
{
public:
    /** Listener to the events of this Component. */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /**
         * The users wants the name the track.
         * @param currentName the current name for display. This is a simplification to bypass the need to ask for it to the controller.
         */
        virtual void onUserWantsToNameSpecialTrack(const juce::String& currentName) = 0;
        /** The user wants to link/unlink. */
        virtual void onUserWantsToManageSpecialLink() = 0;
    };

    /** Holder of the data to display. */
    class DisplayedData
    {
    public:
        /**
         * Constructor.
         * @param specialTrackName the name of the track.
         * @param linkState the state of the link of the Track.
         */
        DisplayedData(juce::String specialTrackName, LinkState linkState) noexcept;

        const juce::String& getSpecialTrackName() const noexcept;
        LinkState getLinkState() const noexcept;

        bool operator==(const DisplayedData& rhs) const;
        bool operator!=(const DisplayedData& rhs) const;

    private:
        juce::String specialTrackName;
        LinkState linkState;
    };

    /**
     * Constructor.
     * @param listener the listener to the event of this Component.
     */
    explicit SpecialTrackHeader(Listener& listener) noexcept;

    /** Sets the data to display. This causes a repaint if different. */
    void setDisplayedData(DisplayedData displayedData) noexcept;

    // Component method implementations.
    // ======================================
    void mouseDown(const juce::MouseEvent& event) override;

protected:
    void paintAfter(juce::Graphics& g, juce::Colour backgroundColor) noexcept override;

    /** @return the image to display in the header. */
    virtual const juce::Image& getImage() const noexcept = 0;

private:
    /** What to display/what to do when clicked. */
    enum class IconState
    {
        none,
        name,
        linked,
        linkedTo,
    };

    /** @return the state, according to the currently displayed data. */
    IconState determineIconState() const noexcept;

    Listener& listener;

    DisplayedData displayedData;
    juce::Image namePresentImage;
    juce::Image linkedImage;
    juce::Image linkedToImage;
};

}   // namespace arkostracker
