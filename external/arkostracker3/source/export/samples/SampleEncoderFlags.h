#pragma once

namespace arkostracker 
{

/** Simple holder of the Sample Export flags, as stored in the Preferences. */
class SampleEncoderFlags
{
public:
    /**
     * Constructor.
     * @param exportSamples true to export the samples.
     * @param amplitude the amplitude of the signal (how many values can be taken). For example, 16, 256. Min is 2.
     * @param offset an offset to add to the exported samples.
     * @param paddingLength a possible padding at the end.
     * @param fadeOutSampleCountForNonLoopingSamples length of a fade-out, if not looping.
     * @param forcePaddingAndFadeOutToLowestValue true to add a padding and fade out.
     * @param onlyExportUsedLength if true, only the used length is exported.
     * @param ignoreUnusedSamples if true, only the used samples are encoded.
     * @param generateIndexTable if true, an index table is encoded.
     */
    explicit SampleEncoderFlags(bool exportSamples = true, int amplitude = 256, int offset = 0, int paddingLength = 0,
                                int fadeOutSampleCountForNonLoopingSamples = 0, bool forcePaddingAndFadeOutToLowestValue = false,
                                bool onlyExportUsedLength = false,
                                bool ignoreUnusedSamples = true, bool generateIndexTable = true) noexcept;

    /** Constructor not to export any samples. */
    static SampleEncoderFlags buildNoExport() noexcept;

    /** Builder with a new "are sample exported" flag. */
    SampleEncoderFlags withAreSampleExported(bool exported) const noexcept;
    /** Builder with a new "generate index table" flag. */
    SampleEncoderFlags withGenerateIndexTable(bool generateIndexTable) const noexcept;

    /** @return whether the samples are exported. */
    bool areSamplesExported() const noexcept;

    /** @return the amplitude of the signal (how many values can be taken). For example, 16, 256. */
    int getAmplitude() const noexcept;

    /** @return the offset to add to the samples. */
    int getOffset() const noexcept;

    /** @return the padding at the end (>=0). */
    int getPaddingLength() const noexcept;

    /** @return how long, in sample, is the fade out (>=0). Only for non-looping samples. */
    int getFadeOutSampleCountForNonLoopingSamples() const;

    /** @return true to force the padding and the fade out to lowest value. */
    bool isForcePaddingAndFadeOutToLowestValue() const;

    /** @return if true, only the used length. */
    bool isOnlyExportUsedLength() const;

    /** @return if true, only the used samples are encoded. */
    bool isIgnoreUnusedSamples() const;
    /** @return if true, an index table is encoded. */
    bool isGenerateIndexTable() const;

private:
    bool exportSamples;								// True to export the samples.
    int amplitude;                                  // The amplitude of the signal (how many values can be taken). For example, 16, 256.
    int offset;                                     // An offset to add to the exported samples.
    int paddingLength;                              // A possible padding at the end.
    int fadeOutSampleCountForNonLoopingSamples;     // How long, in sample, is the fade out (>=0). Only for non-looping samples.
    bool forcePaddingAndFadeOutToLowestValue;       // True to force the padding and the fade out to lowest value.
    bool onlyExportUsedLength;                      // If true, only the used length is exported.

    bool ignoreUnusedSamples;                       // If true, only the used samples are encoded.
    bool generateIndexTable;                        // If true, an index table is encoded.
};

}   // namespace arkostracker
