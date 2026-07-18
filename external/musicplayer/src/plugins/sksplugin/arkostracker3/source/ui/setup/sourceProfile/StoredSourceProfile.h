#pragma once

#include "../../../business/sourceProfile/SourceProfile.h"
#include "../../components/StoredProfile.h"

namespace arkostracker 
{

/** A wrapper around a SourceProfile to make it compliant with the ProfileUser class. */
class StoredSourceProfile final : public StoredProfile
{
public:
    /** Default constructor, do not use, only present for insertion in maps. */
    StoredSourceProfile() noexcept;

    /**
     * Constructor.
     * @param sourceProfile the SourceProfile to wrap around.
     * @param profileId the profileId related to this SourceProfile.
     */
    StoredSourceProfile(SourceProfile sourceProfile, int profileId) noexcept;

    /** @return the wrapped SourceProfile. */
    const SourceProfile& getSourceProfile() const noexcept;

    // StoredProfile method implementations.
    // ========================================
    int getId() const override;
    void setId(int id) override;
    juce::String getDisplayedName() const override;
    void setDisplayedName(const juce::String& name) override;
    bool isReadOnly() const override;
    void setReadOnly(bool readOnly) override;

private:
    SourceProfile sourceProfile;
    int profileId;
};

}   // namespace arkostracker
