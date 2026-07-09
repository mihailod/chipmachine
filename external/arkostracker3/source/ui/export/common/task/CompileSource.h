#pragma once

#include <utility>

#include <juce_core/juce_core.h>

#include "../../../../utils/task/Task.h"

namespace arkostracker 
{

/** Compiles the given source using RASM (so the source must be Z80!). The goal is either to save a binary, or to find the length of a compiled binary. */
class CompileSource
{
public:
    /** Prevents instantiation. */
    CompileSource() = delete;

    /**
     * Compiles the given source into Z80 binary. Should be RASM compatible!!
     * @return the binary, or nullptr if an error occurred.
     */
    static std::unique_ptr<juce::MemoryBlock> compile(const juce::MemoryBlock& sourceMemoryBlock) noexcept;
};

}   // namespace arkostracker
