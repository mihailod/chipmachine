#pragma once

#include <vector>

#include "../../player/PsgRegisters.h"
#include "PsgData.h"

namespace arkostracker
{

/** Converts PSG Registers into a raw array, possibly optimized. */
class SerialConverter
{
public:
    /** Prevents constructor. */
    SerialConverter() = delete;

    /**
     * Encodes a "stop sound".
     * @param psgData the data of each PSG. If empty, no bytes are sent.
     * @return the bytes to send.
     */
    static std::vector<unsigned char> encodeInitialization(const std::vector<PsgData>& psgData) noexcept;

    /**
     * Converts a frame of PSG registers into a stream of optimized bytes.
     * @param psgRegisters the registers of each PSG. If empty, no bytes are sent.
     * @return the bytes to send.
     */
    static std::vector<unsigned char> encodeFrame(const std::vector<PsgRegisters>& psgRegisters) noexcept;

    /**
     * Encodes a "stop sound".
     * @return the bytes to send.
     */
    static std::vector<unsigned char> encodeStopSound() noexcept;

private:
    static const unsigned char initializationTag;          // Tag to declare an Initialization.
    static const unsigned char frameTag;                   // Tag to declare a Frame.
    static const unsigned char stopTag;                    // Tag to declare a Stop Sound.
};

}   // namespace arkostracker
