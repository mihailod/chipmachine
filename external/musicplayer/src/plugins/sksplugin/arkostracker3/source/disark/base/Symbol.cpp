#include "Symbol.h"

namespace arkostracker 
{

Symbol::Symbol(juce::String pLabel, int pAddress, bool pGenerated) noexcept :
    label(std::move(pLabel)),
    address(pAddress),
    generated(pGenerated)
{
}

const juce::String& Symbol::getLabel() const
{
    return label;
}

int Symbol::getAddress() const
{
    return address;
}

bool Symbol::isGenerated() const
{
    return generated;
}


}   // namespace arkostracker

