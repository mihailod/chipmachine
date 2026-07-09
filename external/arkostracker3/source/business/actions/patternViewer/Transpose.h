#pragma once

#include "CellsModifier.h"
#include "TransposeRate.h"

namespace arkostracker 
{

/** Action to transpose Cells. This includes not only notes, but also instruments, effect, special tracks. However, to be logical, this is exclusive. */
class Transpose final : public CellsModifier
{
public:
    /**
     * Constructor.
     * @param songController the Song Controller to reach the data.
     * @param transposeRate how much to transpose.
     * @param selectedData info about the data to process.
     */
    Transpose(SongController& songController, TransposeRate transposeRate, SelectedData selectedData) noexcept;

protected:
    std::unique_ptr<Cell> performOnCell(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept override;
    std::unique_ptr<SpecialCell> performOnSpecialCell(bool isSpeedTrack, const SpecialCell& inputSpecialCell) const noexcept override;

private:
    /** What is going to be transposed.*/
    enum class Mode : juce::uint8
    {
        note,
        instrument,
        effect,
        speedTrack,
        eventTrack,
    };

    std::unique_ptr<Cell> performOnNotes(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept;
    std::unique_ptr<Cell> performOnInstruments(const Cell& inputCell) const noexcept;
    std::unique_ptr<Cell> performOnEffects(const Cell& inputCell, const CaptureFlags& captureFlags) const noexcept;

    /**
     * @return a transpose amount.
     * @param transposeRate the rate.
     * @param fastValue how much when "fast".
     */
    static int getTransposeAmount(TransposeRate transposeRate, int fastValue = 2) noexcept;

    /**
     * Manages one effect.
     * @param isEffectCaptured true if the effect is captured by the selection.
     * @param cellEffects the Cell Effects. Will be modified.
     * @param effectIndex the effect index.
     * @param hasChangedOut set to true if the effect has changed.
     * @param transposeRateValue the transpose value.
     */
    static void managePerformOnEffect(bool isEffectCaptured, CellEffects& cellEffects, int effectIndex, bool& hasChangedOut, int transposeRateValue) noexcept;

    const TransposeRate transposeRate;
    Mode mode;
};

}   // namespace arkostracker
