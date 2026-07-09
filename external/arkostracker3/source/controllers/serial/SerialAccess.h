#pragma once

#include <vector>

#include <juce_core/juce_core.h>

#include "../../../thirdParty/serial/include/serial.h"
#include "../../player/PsgRegisters.h"
#include "PsgData.h"
#include "model/SerialData.h"

namespace arkostracker
{

/** Helps manages the access to the serial ports. It does NOT manage the threading. */
class SerialAccess
{
public:
    /** Prevents instantiation. */
    SerialAccess() = delete;

    //friend class SerialManager;         // To allow using the conversion classes.

    /** How the operation went. */
    enum class SerialOperationResult : uint8_t
    {
        success,
        portNameEmpty,
        channelCouldNotBeOpened,
        writeFailed
    };

    /** @return the serial ports that can be accessed. Calling this from UI thread seems OK. */
    static std::vector<juce::String> getPorts() noexcept;

     /**
      * Tests the given port. This is synchronous and must be called from a background thread!
      * @param serialData the serial data.
      * @return true if everything went fine.
      */
     static bool testPortSync(const SerialData& serialData);

    /**
     * Creates the port and opens it, if possible. This is synchronous and must be called from a background thread!
     * @param serialData the data of the port.
     * @return the serial port object, or null if the port could not be opened.
     */
    static std::unique_ptr<serial::Serial> createPortAndOpen(const SerialData& serialData);

    /**
     * Sends PSG registers for each PSG. This is synchronous and must be called from a background thread!
     * @param serialPort the instance of the port to use.
     * @param psgData the data of the PSG (frequency, replay frequency).
     * @return how the operation went.
     */
    static SerialOperationResult sendInitializationSync(serial::Serial& serialPort, const std::vector<PsgData>& psgData) noexcept;

    /**
     * Sends PSG registers for each PSG. This is synchronous and must be called from a background thread!
     * @param serialPort the instance of the port to use.
     * @param psgRegisters the registers.
     * @return how the operation went.
     */
    static SerialOperationResult sendPsgRegistersSync(serial::Serial& serialPort, const std::vector<PsgRegisters>& psgRegisters) noexcept;

    /**
     * Asks for all sounds to be stopped. This is synchronous and must be called from a background thread!
     * @param serialPort the instance of the port to use.
     * @return how the operation went.
     */
    static SerialOperationResult sendStopSoundsSync(serial::Serial& serialPort) noexcept;

private:
    static const int timeoutMs;

    /**
     * Converts business data into low-level data used by the Serial library.
     * WARNING, this should be called on a background thread, as creating the serial object will try to connect to it. It can also throw!
     * @throw serial::IOException if the port isn't available.
     */
    static std::unique_ptr<serial::Serial> convertData(const SerialData& serialData);

    /**
     * Sends bytes to the given SerialPort
     * @param serialPort the serial port.
     * @param bytes the bytes to send.
     * @return how the operation went.
     */
    static SerialAccess::SerialOperationResult sendBytes(serial::Serial& serialPort, const std::vector<unsigned char>& bytes);
};

}   // namespace arkostracker
