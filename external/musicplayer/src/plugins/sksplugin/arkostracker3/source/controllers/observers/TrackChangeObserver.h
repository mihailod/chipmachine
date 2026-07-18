#pragma once

namespace arkostracker 
{

/** Observer of changes on Tracks. For now, this is also used for Special Tracks. Probably not useful to separate the two. */
class TrackChangeObserver
{
public:
    /** Destructor. */
    virtual ~TrackChangeObserver() = default;

    /**
     * Called when a track of a Subsong has changed (cell modified).
     * @param subsongId the id of the changed Subsong.
     */
    virtual void onTrackDataChanged(const Id& subsongId) = 0;

    /**
     * Called when a track metadata of a Subsong has changed (name/color changed).
     * @param subsongId the id of the changed Subsong.
     */
    virtual void onTrackMetaDataChanged(const Id& subsongId) = 0;
};

}   // namespace arkostracker
