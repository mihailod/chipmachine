#pragma once

#include "PsgRegisters.h"
#include "SampleData.h"

namespace arkostracker 
{

/** Virtual class for an object that can return PSG registers for *one* PSG at the time. */
class PsgRegistersProvider
{
public:
    /** Destructor.*/
    virtual ~PsgRegistersProvider() = default;

    /**
     * @return the next batch of registers.
     * @param psgIndex the index of the PSG that asks the registers (>=0).
     * @return the 14 registers, plus a possible event.
     */
    virtual std::pair<std::unique_ptr<PsgRegisters>, std::unique_ptr<SampleData>> getNextRegisters(int psgIndex) noexcept = 0;
};

}   // namespace arkostracker

