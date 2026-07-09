#include "SampleEncoderFlags.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

SampleEncoderFlags::SampleEncoderFlags(const bool pExportSamples, const int pAmplitude, const int pOffset, const int pPaddingLength,
                                       const int pFadeOutSampleCountForNonLoopingSamples, const bool pForcePaddingAndFadeOutToLowestValue,
                                       const bool pOnlyExportUsedLength,
                                       const bool pIgnoreUnusedSamples, const bool pGenerateIndexTable) noexcept :
    exportSamples(pExportSamples),
    amplitude(pAmplitude),
    offset(pOffset),
    paddingLength(pPaddingLength),
    fadeOutSampleCountForNonLoopingSamples(pFadeOutSampleCountForNonLoopingSamples),
    forcePaddingAndFadeOutToLowestValue(pForcePaddingAndFadeOutToLowestValue),
    onlyExportUsedLength(pOnlyExportUsedLength),
    ignoreUnusedSamples(pIgnoreUnusedSamples),
    generateIndexTable(pGenerateIndexTable)
{
    jassert((amplitude >= 2) && (amplitude <= 256));
    jassert((offset >= 0) && (offset <= 256));
    jassert((paddingLength >= 0) && (paddingLength <= 0x4000));
}

SampleEncoderFlags SampleEncoderFlags::buildNoExport() noexcept
{
    return SampleEncoderFlags(false);
}

SampleEncoderFlags SampleEncoderFlags::withAreSampleExported(const bool exported) const noexcept
{
    return SampleEncoderFlags(
        exported, amplitude, offset, paddingLength, fadeOutSampleCountForNonLoopingSamples, forcePaddingAndFadeOutToLowestValue, onlyExportUsedLength, ignoreUnusedSamples, generateIndexTable
    );
}

SampleEncoderFlags SampleEncoderFlags::withGenerateIndexTable(const bool newGenerateIndexTable) const noexcept
{
    return SampleEncoderFlags(
        exportSamples, amplitude, offset, paddingLength, fadeOutSampleCountForNonLoopingSamples, forcePaddingAndFadeOutToLowestValue, onlyExportUsedLength, ignoreUnusedSamples,
        newGenerateIndexTable
    );
}

bool SampleEncoderFlags::areSamplesExported() const noexcept
{
    return exportSamples;
}

int SampleEncoderFlags::getAmplitude() const noexcept
{
    return amplitude;
}

int SampleEncoderFlags::getOffset() const noexcept
{
    return offset;
}

int SampleEncoderFlags::getPaddingLength() const noexcept
{
    return paddingLength;
}

int SampleEncoderFlags::getFadeOutSampleCountForNonLoopingSamples() const
{
    return fadeOutSampleCountForNonLoopingSamples;
}

bool SampleEncoderFlags::isForcePaddingAndFadeOutToLowestValue() const
{
    return forcePaddingAndFadeOutToLowestValue;
}

bool SampleEncoderFlags::isOnlyExportUsedLength() const
{
    return onlyExportUsedLength;
}

bool SampleEncoderFlags::isIgnoreUnusedSamples() const
{
    return ignoreUnusedSamples;
}

bool SampleEncoderFlags::isGenerateIndexTable() const
{
    return generateIndexTable;
}

}   // namespace arkostracker
