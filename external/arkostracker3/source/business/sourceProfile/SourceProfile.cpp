#include "SourceProfile.h"

namespace arkostracker 
{

SourceProfile::SourceProfile(SourceGeneratorConfiguration pSourceGeneratorConfiguration, juce::String pName, bool pReadOnly) noexcept :
        sourceGeneratorConfiguration(std::move(pSourceGeneratorConfiguration)),
        name(std::move(pName)),
        readOnly(pReadOnly)
{
}

SourceProfile::SourceProfile(const SourceProfile& pSourceProfile, juce::String pNewName, bool pReadOnly) noexcept :
        sourceGeneratorConfiguration(pSourceProfile.getSourceGeneratorConfiguration()),
        name(std::move(pNewName)),
        readOnly(pReadOnly)
{
}

const SourceGeneratorConfiguration& SourceProfile::getSourceGeneratorConfiguration() const noexcept
{
    return sourceGeneratorConfiguration;
}

juce::String SourceProfile::getName() const noexcept
{
    return name;
}

bool SourceProfile::isReadOnly() const noexcept
{
    return readOnly;
}


}   // namespace arkostracker

