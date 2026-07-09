#include "PasteData.h"

#include <utility>

namespace arkostracker 
{

PasteData::PasteData() :
        height(0),
        tracksToCells(),
        captureFlags(),
        speedCells(),
        eventCells()
{
}

PasteData::PasteData(const int pHeight, std::vector<std::vector<Cell>> pTracksToCells, std::vector<CaptureFlags> pCaptureFlags,
                     std::vector<SpecialCell> pSpeedCells, std::vector<SpecialCell> pEventCells) :
        height(pHeight),
        tracksToCells(std::move(pTracksToCells)),
        captureFlags(std::move(pCaptureFlags)),
        speedCells(std::move(pSpeedCells)),
        eventCells(std::move(pEventCells))
{
}

bool PasteData::isValid() const noexcept
{
    return (height > 0);
}

}   // namespace arkostracker
