#pragma once

#include <algorithm>

namespace arkostracker
{
    /**
     * Amplifies the given value.
     * @param inputSample an unsigned 8-bit value, from 0 to 255.
     * @param amplificationRatio the amplification ratio. May be 1.0.
     * @return the amplified value, with limits.
     */
    inline unsigned char amplifySampleValue(const unsigned char inputSample, const float amplificationRatio) noexcept
    {
        // Shifts of half for the "top" and "bottom" to be extended in their own way.
        const auto sampleFloat = ((static_cast<float>(inputSample) - 128.0F) * amplificationRatio) + 128.0F;
        return static_cast<unsigned char>(std::max(0, std::min(255, static_cast<int>(sampleFloat))));
    }

}   // namespace arkostracker
