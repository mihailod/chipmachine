#include "ExportConfiguration.h"

namespace arkostracker 
{

ExportConfiguration::ExportConfiguration(SourceGeneratorConfiguration pSourceConfiguration, std::vector<Id> pSubsongIds, juce::String pBaseLabel, OptionalInt pAddress,
                                         SampleEncoderFlags pSampleEncoderFlags,
                                         bool pEncodeAllAddressesAsRelativeToSongStart) noexcept :
        sourceConfiguration(std::move(pSourceConfiguration)),
        baseLabel(std::move(pBaseLabel)),
        address(pAddress),
        sampleEncoderFlags(pSampleEncoderFlags),
        subsongIds(std::move(pSubsongIds)),
        encodeAllAddressesAsRelativeToSongStart(pEncodeAllAddressesAsRelativeToSongStart)
{
    jassert(!subsongIds.empty());
}

Id ExportConfiguration::getFirstSubsongId() const noexcept
{
    jassert(!subsongIds.empty());
    return *subsongIds.cbegin();
}

void ExportConfiguration::setSubsongIds(const std::vector<Id>& newSubsongIds) noexcept
{
    subsongIds = newSubsongIds;
}

const SourceGeneratorConfiguration& ExportConfiguration::getSourceConfiguration() const noexcept
{
    return sourceConfiguration;
}

const juce::String& ExportConfiguration::getBaseLabel() const noexcept
{
    return baseLabel;
}

const OptionalInt& ExportConfiguration::getAddress() const noexcept
{
    return address;
}

const SampleEncoderFlags& ExportConfiguration::getSampleEncoderFlags() const noexcept
{
    return sampleEncoderFlags;
}

std::vector<Id> ExportConfiguration::getSubsongIds() const noexcept
{
    return subsongIds;
}

bool ExportConfiguration::mustEncodeAllAddressesAsRelativeToSongStart() const noexcept
{
    return encodeAllAddressesAsRelativeToSongStart;
}

bool ExportConfiguration::areSamplesExported() const noexcept
{
    return sampleEncoderFlags.areSamplesExported();
}

}   // namespace arkostracker
