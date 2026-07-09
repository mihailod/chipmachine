#pragma once

#include <unordered_map>

#include "../../../business/model/Loop.h"
#include "AreaType.h"
#include "component/AllBarsAreaDataProvider.h"
#include "component/BarAreaSize.h"
#include "component/BarCaptionDisplayedData.h"
#include "component/BarData.h"
#include "component/LeftHeaderDisplayData.h"

namespace arkostracker 
{

/** An abstract View for an editor with bars (Instrument editor, Pitch/Arpeggio editor, etc.). */
class EditorWithBarsView : public juce::Component,
                           public AllBarsAreaDataProvider       // Implemented by the subclasses.
{
public:
    /** Holders the "non data". That is, the loop, speed, etc., but not the Cells. */
    class DisplayTopHeaderData
    {
    public:
        /**
         * Constructor.
         * @param pLoopStart the loop start.
         * @param pEnd the end.
         * @param pLoop true if looping.
         * @param pRetrig true if global retrig on.
         * @param pSpeed the speed.
         * @param pShift the shift (for expressions).
         */
        DisplayTopHeaderData(const int pLoopStart, const int pEnd, const bool pLoop, const bool pRetrig, const int pSpeed, const int pShift) noexcept :
                loopStart(pLoopStart),
                end(pEnd),
                loop(pLoop),
                retrig(pRetrig),
                speed(pSpeed),
                shift(pShift)
        {
            jassert(loopStart >= 0);
            jassert(end >= 0);
            jassert(end >= loopStart);
            jassert(speed >= 0);
            jassert(shift >= 0);
        }

        int getLoopStart() const noexcept
        {
            return loopStart;
        }

        int getEnd() const noexcept
        {
            return end;
        }

        bool isLoop() const noexcept
        {
            return loop;
        }

        bool isRetrig() const noexcept
        {
            return retrig;
        }

        int getSpeed() const noexcept
        {
            return speed;
        }

        int getShift() const noexcept
        {
            return shift;
        }

        bool operator==(const DisplayTopHeaderData& rhs) const noexcept
        {
            return loopStart == rhs.loopStart &&
                   end == rhs.end &&
                   loop == rhs.loop &&
                   retrig == rhs.retrig &&
                   speed == rhs.speed &&
                   shift == rhs.shift;
        }

        bool operator!=(const DisplayTopHeaderData& rhs) const noexcept
        {
            return !(rhs == *this);
        }

    private:
        int loopStart;
        int end;
        bool loop;
        bool retrig;
        int speed;
        int shift;
    };

    /** Destructor. */
    ~EditorWithBarsView() override = default;

    /**
     * Sets the bar count. Bars may be removed, or created. Created bars will have default empty values.
     * @param barCount how many bars.
     */
    virtual void setDisplayedBarCount(int barCount) noexcept = 0;

    /**
     * Sets the data to a Bar. This refreshes the UI. Nothing happens if the data is the same.
     * @param areaType the type of the bar.
     * @param barIndex the index of the bar.
     * @param barData the data of the bar to display.
     * @param captionData the data of the Caption.
     */
    virtual void showBarData(AreaType areaType, int barIndex, const BarData& barData, const BarCaptionDisplayedData& captionData) noexcept = 0;

    /**
     * Sets the metadata data to display. This will refresh the UI if needed.
     * @param areaTypeToSize the map linking each area type to their size. It MUST be complete.
     * @param areaTypeToLeftHeaderDisplayData the map linking each area type to the data in the left header. It MUST be complete.
     * @param areaTypeToAutoSpreadData links area type to the auto spread data. It MUST be complete.
     */
    virtual void setMetadataDisplayData(std::unordered_map<AreaType, BarAreaSize> areaTypeToSize,
                                        std::unordered_map<AreaType, LeftHeaderDisplayData> areaTypeToLeftHeaderDisplayData,
                                        std::unordered_map<AreaType, Loop> areaTypeToAutoSpreadData) noexcept = 0;

    /**
     * Sets the displayed non-data. This might refresh the UI if there are changes.
     * @param displayTopHeaderData the top header data to display.
     */
    virtual void setDisplayTopHeaderData(const DisplayTopHeaderData& displayTopHeaderData) noexcept = 0;

    /**
     * Sets the X zoom rate. This will refresh the UI if needed.
     * @param xZoomRate the X zoom rate.
     */
    virtual void setXZoomRate(int xZoomRate) noexcept = 0;

    /**
     * @return the value held by a bar. If not found, 0 may be returned.
     * @param areaType the area type. Must be valid.
     * @param barIndex the index of the bar. May be invalid.
     */
    virtual int getValue(AreaType areaType, int barIndex) noexcept = 0;

    /**
     * @return a next/previous area type.
     * @param areaType the current area type. If not found, the same area type is returned (asserts).
     * @param offset positive to go "down". May be out of bounds, will be corrected.
     */
    virtual AreaType getShiftedAreaType(AreaType areaType, int offset) const noexcept = 0;

    /**
     * Scrolls in order to see the given position.
     * @param areaType the area type. Must be valid.
     * @param barIndex the index of the bar. May be invalid.
     */
    virtual void ensureVisible(AreaType areaType, int barIndex) noexcept = 0;

    /**
     * Highlights indexes. Typically used by instrument being played.
     * @param indexes the indexes. May be empty.
     */
    virtual void highlightBarIndexes(const std::vector<int>& indexes) noexcept = 0;
};

}   // namespace arkostracker
