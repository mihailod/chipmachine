#pragma once

#include <memory>
#include <vector>

#include "../../../../../song/tracks/Track.h"
#include "../../../controller/Selection.h"
#include "../../cells/cell/CellView.h"
#include "../TrackViewMetadata.h"
#include "TrackHeader.h"
#include "TrackView.h"

namespace arkostracker 
{

class TrackColors;

/** Implementation of a TrackView (music track). */
class TrackViewImpl final : public TrackView,
                            public CellView::Listener,
                            TrackHeader::Listener
{
public:
    /**
     * Constructor.
     * @param listener the listener to the event of this Component.
     * @param channelIndex the channel index. Handy for the callbacks.
     * @param normalFont the Font to use.
     * @param doubleHeightFont the Font to use for the middle line.
     * @param trackViewMetadata data that are not from the look'n'feel (instrument colors, highlight step, etc.).
     * @param transmittedDisplayedData the data to display.
     */
    TrackViewImpl(TrackView::Listener& listener, int channelIndex, juce::Font& normalFont, juce::Font& doubleHeightFont,
                  std::shared_ptr<TrackViewMetadata> trackViewMetadata, const TrackView::TransmittedDisplayedData& transmittedDisplayedData) noexcept;

    // TrackView method implementations.
    // ======================================
    void setDisplayedDataAndRefresh(const TransmittedDisplayedData& transmittedDisplayedData) noexcept override;
    int calculateTrackWidth(bool minimized) const noexcept override;
    TrackType getTrackType() const noexcept override;
    int getChannelIndex() const noexcept override;
    void updateMeterAndRefresh(const PeriodAndNoiseMeterInput& periodAndNoiseMeterInput) noexcept override;
    void setEffectContextText(const juce::String& text) noexcept override;

    // CellView::Listener method implementations.
    // ============================================
    void onUserClickedOnRank(int viewRank, CellCursorRank cursorRank, bool leftButton, bool shift) override;
    void onUserClickIsUp(bool leftButton) override;
    void onUserDraggedCursor(const juce::MouseEvent& event) override;

protected:
    void onResizedAccepted() noexcept override;
    juce::Font& getNormalFont() noexcept override;
    juce::Font& getDoubleHeightFont() noexcept override;
    bool isMinimized() const noexcept override;
    void refreshCell(int cellIndex, int cellRank, bool somethingToShow, bool isMiddleLine, OptionalBool primaryHighlight, bool isPlayedLine) noexcept override;
    int getTrackHeight() const noexcept override;
    std::vector<AbstractCellView*> getCellViews() const noexcept override;
    void resizeCellViewCount(size_t newCount) noexcept override;
    int getCellCount() const noexcept override;
    AbstractCellView& createAndStorePrototypeCell() noexcept override;
    OptionalValue<CursorLocation> applyCursorLocationOnTrack(const OptionalValue<CursorLocation>& cursorLocationToTest) const noexcept override;
    OptionalInt getPlayedLocationLine() const noexcept override;

    // AbstractTrackView method implementations.
    // ======================================
    void postPaintOverChildren(juce::Graphics& g, int trackY, int trackHeight) noexcept override;

private:
    static const float effectContextFontHeight;
    static const int effectContextBackgroundHeight;

    /**
     * @return true if there is an Error for the given effect. This checks the expression value.
     * @param cellEffect the cell effect.
     */
    bool isErrorInEffectValue(const CellEffect& cellEffect) const noexcept;

    // TrackHeader::Listener method implementations.
    // ===============================================
    void onUserLeftClickedOnChannelNumber() override;
    void onUserRightClickedOnChannelNumber() override;
    void onUserWantsToChangeTransposition() override;
    void onUserWantsToNameTrack(const juce::String& currentName, OptionalValue<juce::Colour> currentColor) override;
    void onUserWantsToManageLink() override;

    /**
     * @return the color of the instrument. If not found, an error color is returned.
     * @param instrumentIndex the index.
     */
    juce::Colour getInstrumentColor(int instrumentIndex) const noexcept;

    TrackView::Listener& listener;
    const int channelIndex;                                                 // >=0. Handy for the callbacks.
    juce::Font& normalFont;
    juce::Font& doubleHeightFont;

    CellView::DisplayedData outOfBoundsDisplayedData;                       // Cached DisplayedData when out of bounds.

    TrackHeader trackHeader;

    LinkState linkState;
    bool muted;
    bool minimized;
    Track displayedTrack;                                                   // The visible Track.
    int trackHeight;
    OptionalInt playedLocationLine;                                         // If absent, must not be shown.
    Selection selection;                                                    // The selection, if any.

    std::vector<std::unique_ptr<CellView>> cells;                           // The visible Cells.

    juce::String effectContextText;
    juce::Font effectContextFont;
};

}   // namespace arkostracker
