#pragma once

#include <vector>

#include "../../../song/Location.h"
#include "../../../ui/patternViewer/controller/CaptureFlags.h"
#include "../../../utils/OptionalValue.h"

namespace arkostracker 
{

/** Holds the data useful to represent selected data in the PatternViewer. This is especially useful to the CellsModifier class. */
class SelectedData
{
public:
    class SelectedColumn
    {
    public:
        SelectedColumn(const bool pIsNoteCaptured, const bool pIsInstrumentCaptured, const bool pIsEffectCaptured, const bool pIsSpeedTrackCaptured,
            const bool pIsEventTrackCaptured) noexcept :
            noteCaptured(pIsNoteCaptured),
            instrumentCaptured(pIsInstrumentCaptured),
            effectCaptured(pIsEffectCaptured),
            speedTrackCaptured(pIsSpeedTrackCaptured),
            eventTrackCaptured(pIsEventTrackCaptured)
        {
        }

        bool isNoteCaptured() const
        {
            return noteCaptured;
        }

        bool isInstrumentCaptured() const
        {
            return instrumentCaptured;
        }

        bool isEffectCaptured() const
        {
            return effectCaptured;
        }

        bool isSpeedTrackCaptured() const
        {
            return speedTrackCaptured;
        }

        bool isEventTrackCaptured() const
        {
            return eventTrackCaptured;
        }

        /** @return true if more than one type of column is selected. */
        bool hasSeveralCaptures() const
        {
            auto count = 0;
            count += noteCaptured ? 1 : 0;
            count += instrumentCaptured ? 1 : 0;
            count += effectCaptured ? 1 : 0;
            count += speedTrackCaptured ? 1 : 0;
            count += eventTrackCaptured ? 1 : 0;
            return (count > 1);
        }

    private:
        bool noteCaptured;
        bool instrumentCaptured;
        bool effectCaptured;
        bool speedTrackCaptured;
        bool eventTrackCaptured;
    };

    /**
     * Constructor.
     * @param trackToCaptureFlag the CaptureFlags for each normal Track. May be empty.
     * @param startLocation the start of the selection.
     * @param endLocation the end of the selection.
     * @param startChannelIndex the index of the first channel. Irrelevant if no normal Tracks are used.
     * @param includeSpeedTrack true to include the Speed Track.
     * @param includeEventTrack false to include the Speed Track.
     */
    SelectedData(std::vector<CaptureFlags> trackToCaptureFlag, Location startLocation, const OptionalValue<Location>& endLocation,
                 int startChannelIndex, bool includeSpeedTrack, bool includeEventTrack) noexcept;

    const std::vector<CaptureFlags>& getTrackToCaptureFlag() const noexcept;
    const Location& getStartLocation() const noexcept;
    const OptionalValue<Location>& getEndLocation() const noexcept;
    int getStartChannelIndex() const noexcept;
    bool getIncludeSpeedTrack() const noexcept;
    bool getIncludeEventTrack() const noexcept;
    /** @return the indexes of the selected normal Channels. May be empty. */
    std::set<int> getSelectedChannelIndexes() const noexcept;

    /** @return the selected columns through all the selection. */
    SelectedColumn getSelectedColumns() const noexcept;

private:
    const std::vector<CaptureFlags> trackToCaptureFlag;
    const Location startLocation;
    const OptionalValue<Location> endLocation;
    const int startChannelIndex;
    const bool includeSpeedTrack;
    const bool includeEventTrack;
};

}   // namespace arkostracker
