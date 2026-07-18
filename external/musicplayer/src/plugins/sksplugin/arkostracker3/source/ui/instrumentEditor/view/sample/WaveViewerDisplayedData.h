#pragma once

namespace arkostracker
{

/** It does NOT contain the sample, which has its own method, for quickness purpose. */
class WaveViewerDisplayedData
{
public:
    /** Constructor to show nothing. */
    WaveViewerDisplayedData() noexcept;

    WaveViewerDisplayedData(double multiplier, int startViewedOffset, int pastEndViewedOffset, int sampleLength, int sampleFrequencyHz, bool looping,
        int digidrumNote) noexcept;

    double getMultiplier() const noexcept;
    int getStartViewedOffset() const noexcept;
    int getPastEndViewedOffset() const noexcept;
    int getSampleLength() const noexcept;
    int getSampleFrequencyHz() const noexcept;
    bool isLooping() const noexcept;
    int getDigidrumNote() const noexcept;

    /** @return true if the data is valid (offset valid, multiplier not 0, sample length not 0, sample frequency not 0). */
    bool isDisplayValid() const noexcept;

    bool operator==(const WaveViewerDisplayedData& rhs) const;
    bool operator!=(const WaveViewerDisplayedData& rhs) const;

private:
    double multiplier;                  // The current multiplier value. May be 0 if nothing is visible.
    int startViewedOffset;
    int pastEndViewedOffset;
    int sampleLength;
    int sampleFrequencyHz;
    bool looping;
    int digidrumNote;
};

}   // namespace arkostracker
