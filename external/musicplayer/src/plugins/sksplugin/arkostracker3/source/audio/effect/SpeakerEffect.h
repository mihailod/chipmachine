#pragma once

#include "BaseEffect.h"

namespace arkostracker
{

/**
    Processor using RBJ Biquad HighPass, to emulate the Speaker.

    Based on the work of Vincent Falco (https://github.com/vinniefalco/DSPFilters).
    The original code has been stripped down of everything I didn't need (or thought I needed!).

    --------------------------------------------------------------------------------

    License: MIT License (http://www.opensource.org/licenses/mit-license.php)
    Copyright (c) 2009 by Vincent Falco

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.

    *******************************************************************************/
class SpeakerEffect : public BaseEffect
{
public:
    /** Constructor. */
    SpeakerEffect() noexcept;

    // BaseEffect method implementations.
    // ==========================================================
    void prepareToPlay(double sampleRate) noexcept override;
    void releaseResources() override;

protected:
    // BaseEffect method implementations.
    // ==========================================================
    void processBuffer(juce::AudioSampleBuffer* buffer, int positionInBuffer, int afterLastPositionInBuffer) noexcept override;

private:
    const double vsa;

    double m_a1;
    double m_a2;
    double m_b1;
    double m_b2;
    double m_b0;

    double m_x1_left;
    double m_x2_left;
    double m_y1_left;
    double m_y2_left;

    double m_x1_right;
    double m_x2_right;
    double m_y1_right;
    double m_y2_right;
};

}   // namespace arkostracker
