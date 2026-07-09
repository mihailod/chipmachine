#pragma once

namespace arkostracker
{

class Id;

/** Observer for objects wanting to be notified when the Subsong metadata has changed. */
class SubsongMetadataObserver
{
public:
    /** Flags about what has changed. */
    enum What : unsigned int
    {
        subsongMetadata = 1U << 0U,         // Do NOT use to check the PSG count.
        psgsData = 1U << 1U,                // Use this to check the PSG count.
        loop = 1U << 2U,

        all = static_cast<unsigned int>(-1)
    };

    /** Destructor. */
    virtual ~SubsongMetadataObserver() = default;

    /**
     * Called when the Subsong metadata has changed.
     * @param subsongId the id of the changed Subsong.
     * @param what flags about what has changed.
     */
    virtual void onSubsongMetadataChanged(const Id& subsongId, unsigned int what) = 0;
};

}   // namespace arkostracker
