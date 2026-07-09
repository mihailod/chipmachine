#include "SerialAccess.h"

#include "SerialConverter.h"

namespace arkostracker
{

const int SerialAccess::timeoutMs = 5000U;


// SerialData
// =================================
/*SerialAccess::SerialData::SerialData(std::string port, int baudRate, serial::bytesize_t byteSize, serial::parity_t parity, serial::stopbits_t stopBits,
                                     serial::flowcontrol_t flowControl) noexcept:
        port(std::move(port)), baudRate(baudRate), byteSize(byteSize), parity(parity),
        stopBits(stopBits), flowControl(flowControl)
{}*/


// =================================

std::vector<juce::String> SerialAccess::getPorts() noexcept
{
    std::vector<juce::String> result;

    const auto devices = serial::list_ports();
    for (const auto& device : devices) {
        result.emplace_back(device.port);
    }

    return result;
}

bool SerialAccess::testPortSync(const SerialData& serialData)
{
    const auto serialPort = createPortAndOpen(serialData);
    return (serialPort != nullptr);
}

std::unique_ptr<serial::Serial> SerialAccess::createPortAndOpen(const SerialData& serialData)
{
    // Checks the port.
    try {
        auto serialPort = convertData(serialData);  // Can throw!
        return serialPort->isOpen() ? std::move(serialPort) : nullptr;
    } catch (serial::IOException&) {
        return nullptr;
    }
}

std::unique_ptr<serial::Serial> SerialAccess::convertData(const SerialData& serialData)
{
    // Converts the input data.
    serial::bytesize_t byteSizeEnum;    // NOLINT(*-init-variables)
    switch (serialData.getByteSize()) {
        case 5: byteSizeEnum = serial::bytesize_t::fivebits; break;
        case 6: byteSizeEnum = serial::bytesize_t::sixbits; break;
        case 7: byteSizeEnum = serial::bytesize_t::sevenbits; break;
        case 8: byteSizeEnum = serial::bytesize_t::eightbits; break;
        default: byteSizeEnum = serial::bytesize_t::fivebits; jassertfalse; break;      // Should not happen!
    }

    serial::parity_t parityEnum;    // NOLINT(*-init-variables)
    switch (serialData.getParity()) {
        case 0: parityEnum = serial::parity_t::parity_none; break;
        case 1: parityEnum = serial::parity_t::parity_odd; break;
        case 2: parityEnum = serial::parity_t::parity_even; break;
        case 3: parityEnum = serial::parity_t::parity_mark; break;
        case 4: parityEnum = serial::parity_t::parity_space; break;
        default: parityEnum = serial::parity_t::parity_none; jassertfalse; break;      // Should not happen!
    }

    serial::stopbits_t stopBitsEnum;    // NOLINT(*-init-variables)
    switch (serialData.getStopBits()) {
        case StopBit::stopBit1: stopBitsEnum = serial::stopbits_t::stopbits_one; break;
        case StopBit::stopBit1p5: stopBitsEnum = serial::stopbits_t::stopbits_one_point_five; break;
        case StopBit::stopBit2: stopBitsEnum = serial::stopbits_t::stopbits_two; break;
        default: stopBitsEnum = serial::stopbits_t::stopbits_one; jassertfalse; break;      // Should not happen!
    }

    serial::flowcontrol_t flowControlEnum;    // NOLINT(*-init-variables)
    switch (serialData.getFlowControl()) {
        case FlowControl::none: flowControlEnum = serial::flowcontrol_t::flowcontrol_none; break;
        case FlowControl::software: flowControlEnum = serial::flowcontrol_t::flowcontrol_software; break;
        case FlowControl::hardware: flowControlEnum = serial::flowcontrol_t::flowcontrol_hardware; break;
        default: flowControlEnum = serial::flowcontrol_t::flowcontrol_none; jassertfalse; break;      // Should not happen!
    }

    const auto portName = serialData.getPort().toStdString();

    return std::make_unique<serial::Serial>(portName, static_cast<unsigned int>(serialData.getBaudRate()), serial::Timeout::simpleTimeout(timeoutMs),
                          byteSizeEnum, parityEnum, stopBitsEnum, flowControlEnum);
}

SerialAccess::SerialOperationResult SerialAccess::sendInitializationSync(serial::Serial& serialPort, const std::vector<PsgData>& psgData) noexcept
{
    const auto bytes = SerialConverter::encodeInitialization(psgData);
    return sendBytes(serialPort, bytes);
}

SerialAccess::SerialOperationResult SerialAccess::sendPsgRegistersSync(serial::Serial& serialPort, const std::vector<PsgRegisters>& psgRegisters) noexcept
{
    const auto bytes = SerialConverter::encodeFrame(psgRegisters);
    return sendBytes(serialPort, bytes);
}

SerialAccess::SerialOperationResult SerialAccess::sendStopSoundsSync(serial::Serial& serialPort) noexcept
{
    const auto bytes = SerialConverter::encodeStopSound();
    return sendBytes(serialPort, bytes);
}

SerialAccess::SerialOperationResult SerialAccess::sendBytes(serial::Serial& serialPort, const std::vector<unsigned char>& bytes)
{
    try {
        // Is the port open?
        if (!serialPort.isOpen()) {
            return SerialOperationResult::channelCouldNotBeOpened;
        }

        // Try to send all the bytes.
        const auto sizeSent = serialPort.write(bytes);
        return (sizeSent == bytes.size()) ? SerialOperationResult::success : SerialOperationResult::writeFailed;
    } catch (const serial::IOException&) {
        return SerialOperationResult::writeFailed;
    }
}

}   // namespace arkostracker
