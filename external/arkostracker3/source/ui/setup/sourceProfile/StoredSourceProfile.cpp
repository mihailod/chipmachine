#include "StoredSourceProfile.h"

namespace arkostracker 
{

StoredSourceProfile::StoredSourceProfile(SourceProfile pSourceProfile, const int pProfileId) noexcept :
        sourceProfile(std::move(pSourceProfile)),
        profileId(pProfileId)
{
}

StoredSourceProfile::StoredSourceProfile() noexcept :
        sourceProfile(SourceGeneratorConfiguration::buildZ80(), juce::String(), true),      // Boiler-plate.
        profileId(-1)
{
}

const SourceProfile& StoredSourceProfile::getSourceProfile() const noexcept
{
    return sourceProfile;
}

// StoredProfile method implementations.
// ========================================

int StoredSourceProfile::getId() const
{
    return profileId;
}

void StoredSourceProfile::setId(const int id)
{
    profileId = id;
}

juce::String StoredSourceProfile::getDisplayedName() const
{
    return sourceProfile.getName();
}

void StoredSourceProfile::setDisplayedName(const juce::String& name)
{
    const auto newProfile = SourceProfile(sourceProfile, name, sourceProfile.isReadOnly());
    sourceProfile = newProfile;
}

bool StoredSourceProfile::isReadOnly() const
{
    return sourceProfile.isReadOnly();
}

void StoredSourceProfile::setReadOnly(const bool readOnly)
{
    const auto newProfile = SourceProfile(sourceProfile, sourceProfile.getName(), readOnly);
    sourceProfile = newProfile;
}

}   // namespace arkostracker
