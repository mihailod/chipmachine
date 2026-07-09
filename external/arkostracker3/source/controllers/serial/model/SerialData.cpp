#include "SerialData.h"

#include <utility>

namespace arkostracker
{

SerialData::SerialData(juce::String pPort, int pBaudRate, int pByteSize, int pParity, StopBit pStopBits,
                       FlowControl pFlowControl) noexcept :
    port(std::move(pPort)),
    baudRate(pBaudRate),
    byteSize(pByteSize),
    parity(pParity),
    stopBits(pStopBits),
    flowControl(pFlowControl)
{
}

const juce::String& SerialData::getPort() const noexcept
{
    return port;
}

int SerialData::getBaudRate() const noexcept
{
    return baudRate;
}

int SerialData::getByteSize() const noexcept
{
    return byteSize;
}

int SerialData::getParity() const noexcept
{
    return parity;
}

StopBit SerialData::getStopBits() const noexcept
{
    return stopBits;
}

FlowControl SerialData::getFlowControl() const noexcept
{
    return flowControl;
}

}   // namespace arkostracker
