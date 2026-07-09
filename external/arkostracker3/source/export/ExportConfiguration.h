#pragma once

#include <juce_core/juce_core.h>

#include "../utils/Id.h"
#include "../utils/OptionalValue.h"
#include "samples/SampleEncoderFlags.h"
#include "sourceGenerator/SourceGeneratorConfiguration.h"

namespace arkostracker 
{

/** A simple generic holder of various configuration data used by exports. All exporters may not use all the data. */
class ExportConfiguration
{
public:
    /**
     * Constructor.
     * @param sourceConfiguration the source configuration.
     * @param subsongIds the IDs of the Subsongs. Must not be empty.
     * @param baseLabel a base label (possibly with separator), or empty.
     * @param address the possible address to encode (ORG).
     * @param sampleEncoderFlags the sample encoder flags.
     * @param encodeAllAddressesAsRelativeToSongStart so far, only used by the AKY export. If true, all the encoded addresses (DW myLabel for example)
     * will be relative to the beginning of the Song (dw myLabel - songStart for example). Useful for Atari ST players notably.
     */
    ExportConfiguration(SourceGeneratorConfiguration sourceConfiguration, std::vector<Id> subsongIds, juce::String baseLabel, OptionalInt address = {},
                        SampleEncoderFlags sampleEncoderFlags = SampleEncoderFlags::buildNoExport(),
                        bool encodeAllAddressesAsRelativeToSongStart = false) noexcept;

    /** @return the first Subsong ID. */
    Id getFirstSubsongId() const noexcept;

    /** After a Song optimization, the SubsongIds change. This is to set them right again. */
    void setSubsongIds(const std::vector<Id>& subsongIds) noexcept;
    /** @return the subsong IDs. After optimization, they may have changed, so it advised to not store them. */
    std::vector<Id> getSubsongIds() const noexcept;

    const SourceGeneratorConfiguration& getSourceConfiguration() const noexcept;
    const juce::String& getBaseLabel() const noexcept;
    const OptionalInt& getAddress() const noexcept;
    const SampleEncoderFlags& getSampleEncoderFlags() const noexcept;
    bool mustEncodeAllAddressesAsRelativeToSongStart() const noexcept;

    bool areSamplesExported() const noexcept;

private:
    const SourceGeneratorConfiguration sourceConfiguration;
    const juce::String baseLabel;
    const OptionalInt address;
    const SampleEncoderFlags sampleEncoderFlags;
    std::vector<Id> subsongIds;
    const bool encodeAllAddressesAsRelativeToSongStart;
};

}   // namespace arkostracker
