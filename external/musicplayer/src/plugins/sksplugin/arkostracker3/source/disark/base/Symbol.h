#pragma once

#include <juce_core/juce_core.h>

namespace arkostracker 
{

/** Holds a label name, an address, and a generated flag. */
class Symbol
{
public:
    /**
     * Constructor.
     * @param label the label.
     * @param address the address.
     * @param generated true if generated.
     */
    Symbol(juce::String label, int address, bool generated = false) noexcept;

    /** @return the label. */
    const juce::String& getLabel() const;
    /** @return the address. */
    int getAddress() const;
    /** @return true if generated. */
    bool isGenerated() const;

private:
    juce::String label;     // The label.
    int address;            // The address.
    bool generated;         // True if generated.
};

}   // namespace arkostracker

