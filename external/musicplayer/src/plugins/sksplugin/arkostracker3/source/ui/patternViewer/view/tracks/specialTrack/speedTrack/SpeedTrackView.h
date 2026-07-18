#pragma once

#include "../SpecialTrackView.h"
#include "SpeedTrackHeader.h"

namespace arkostracker 
{

/** The Speed Track View. Holds a header. */
class SpeedTrackView final : public SpecialTrackView,
                             public SpecialTrackHeader::Listener
{
public:
    /**
     * Constructor.
     * @param listener the listener to the events of this class.
     * @param line the "middle" line.
     * @param trackHeight the track height.
     * @param trackViewMetadata data that are not from the look'n'feel (instrument colors, highlight step, etc.).
     * @param normalFont the Font to use.
     * @param doubleHeightFont the Font to use for the middle line.
     */
    SpeedTrackView(SpecialTrackView::Listener& listener, int line, int trackHeight, std::shared_ptr<TrackViewMetadata> trackViewMetadata, juce::Font& normalFont,
                   juce::Font& doubleHeightFont) noexcept;

    // SpecialTrackView method implementations.
    // ===================================================
    TrackType getTrackType() const noexcept override;

    // SpecialTrackHeader::Listener method implementations.
    // ===================================================
    void onUserWantsToNameSpecialTrack(const juce::String& currentName) override;
    void onUserWantsToManageSpecialLink() override;

protected:
    void onResizedAccepted() noexcept override;
    void buildTrackColorsFromLookAndFeel(const juce::LookAndFeel& lookAndFeel) noexcept override;
    juce::Colour getCellTextColor() const noexcept override;
    bool isSpeedTrack() const noexcept override;
    void setHeaderDisplayedData(const SpecialTrackHeader::DisplayedData& displayedData) noexcept override;

private:
    /**
     * Finds and sets the text color, from the given Look And Feel.
     * @param lookAndFeel the look and feel.
     */
    void findAndSetTextColor(const juce::LookAndFeel& lookAndFeel) noexcept;

    SpeedTrackHeader header;                    // The header at the top.
    juce::Colour textColor;
};

}   // namespace arkostracker

