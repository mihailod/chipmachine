#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Provides labels (to be used in a assembly code) for RegisterBlocks. */
class RegisterBlockLabelProvider
{
public:
    /** Destructor. */
    virtual ~RegisterBlockLabelProvider() = default;

    /** @return a label for the given RegisterBlock id and the given line index, or without an index if the lineIndex < 0. */
    virtual juce::String getLabelForRegisterBlockIndex(int registerBlockId, int lineIndex) noexcept = 0;
};

}   // namespace arkostracker

