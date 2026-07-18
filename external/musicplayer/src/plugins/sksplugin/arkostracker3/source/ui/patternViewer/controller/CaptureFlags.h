#pragma once

namespace arkostracker 
{

/** Flag representing what is captured in a Cell. Immutable. */
class CaptureFlags          // TODO TU this.
{
public:
    /**
     * Constructor.
     * @param isNoteCaptured true if the note was captured in the selection.
     * @param isInstrumentCaptured true if the instrument was captured in the selection.
     * @param isEffect1Captured true if the effect 1 was captured in the selection.
     * @param isEffect2Captured true if the effect 2 was captured in the selection.
     * @param isEffect3Captured true if the effect 3 was captured in the selection.
     * @param isEffect4Captured true if the effect 4 was captured in the selection.
     */
    CaptureFlags(bool isNoteCaptured, bool isInstrumentCaptured, bool isEffect1Captured, bool isEffect2Captured, bool isEffect3Captured, bool isEffect4Captured) noexcept;

    bool isNoteCaptured() const noexcept;
    bool isInstrumentCaptured() const noexcept;
    bool isEffect1Captured() const noexcept;
    bool isEffect2Captured() const noexcept;
    bool isEffect3Captured() const noexcept;
    bool isEffect4Captured() const noexcept;

    /** @return true if the whole cell is captured. */
    bool isAllCaptured() const noexcept;

    /**
     * @return a new instance with the effect flags shifted.
     * @param shift the shift. Positive to go "to the right".
     */
    CaptureFlags withEffectShift(int shift) const noexcept;

    /**
     * @return a new instance with a new note captured flag.
     * @param captured true if the note is captured.
     */
    CaptureFlags withNoteCaptured(bool captured) const noexcept;

    /**
     * @return a new instance with a new note captured flag.
     * @param noteCaptured true if the note is captured.
     * @param instrumentCaptured true if the instrument is captured.
     */
    CaptureFlags withNoteAndInstrumentCaptured(bool noteCaptured, bool instrumentCaptured) const noexcept;

private:
    bool noteCaptured;
    bool instrumentCaptured;
    bool effect1Captured;
    bool effect2Captured;
    bool effect3Captured;
    bool effect4Captured;
};

}   // namespace arkostracker
