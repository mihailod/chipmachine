#pragma once

#include "juce_core/juce_core.h"

namespace arkostracker
{

enum class StopBit : uint8_t
{
    stopBit1 = 1,      // >0 to ease the mapping with the UI.
    stopBit1p5,
    stopBit2,

    firstValue = stopBit1,
    lastValue = stopBit2,
};

enum class FlowControl : uint8_t
{
    none = 1,          // >0 to ease the mapping with the UI.
    software,
    hardware,

    firstValue = none,
    lastValue = hardware,
};

/** Holds the data used to define a Port. */
class SerialData
{
public:
    SerialData(juce::String port, int baudRate, int byteSize, int parity, StopBit stopBits, FlowControl flowControl) noexcept;

    const juce::String& getPort() const noexcept;
    int getBaudRate() const noexcept;
    int getByteSize() const noexcept;
    int getParity() const noexcept;
    StopBit getStopBits() const noexcept;
    FlowControl getFlowControl() const noexcept;

private:
    juce::String port;
    int baudRate;
    int byteSize;
    int parity;
    StopBit stopBits;
    FlowControl flowControl;
};

}   // namespace arkostracker
