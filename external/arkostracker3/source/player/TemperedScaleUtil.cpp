#include "TemperedScaleUtil.h"

#include <juce_core/juce_core.h>

namespace arkostracker 
{

float TemperedScaleUtil::getFrequency(const int noteInOctave, const int octave, const float referenceFrequency) noexcept
{
    jassert(noteInOctave <= 11);
    jassert(octave <= 20);

    return getFrequency(noteInOctave + octave * 12, referenceFrequency);
}

float TemperedScaleUtil::getFrequency(const int note, const float referenceFrequency, const int noteReference) noexcept
{
    jassert((note >= 0) && (note <= 256));
    jassert(referenceFrequency > 0);

    return static_cast<float>((referenceFrequency / 32.0) * (pow(2.0, ((static_cast<double>(note) - static_cast<double>(noteReference)) / 12.0))));
}

float TemperedScaleUtil::getSampleFrequency(const int note, const float referenceFrequency) noexcept
{
    return getFrequency(note, referenceFrequency, 12);
}

}   // namespace arkostracker
