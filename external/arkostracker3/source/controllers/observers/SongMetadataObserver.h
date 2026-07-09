#pragma once

namespace arkostracker 
{

/** Observer for objects wanting to be notified when the Song metadata has changed. */
class SongMetadataObserver
{
public:
    /** Flags about what has changed. */
    enum What : unsigned int
    {
        name = 1U << 0U,
        modified = 1U << 1U,
        savedPath = 1U << 2U,
        shownLocation = 1U << 3U,

        all = static_cast<unsigned int>(-1)
    };

    /** Destructor. */
    virtual ~SongMetadataObserver() = default;

    /**
     * Called when the Song metadata has changed.
     * @param what flags about what has changed.
     */
    virtual void onSongMetadataChanged(unsigned int what) = 0;
};

}   // namespace arkostracker

