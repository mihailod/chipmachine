#pragma once

namespace arkostracker 
{

/** Observer on the metadata of the PatternViewer (such as the setup). */
class PatternViewerMetadataObserver
{
public:
    /** Destructor. */
    virtual ~PatternViewerMetadataObserver() = default;

    /** Called when the metadata has changed. Due to the small amount of data, it is up to the client to find out what has changed. */
    virtual void onPatternViewerMetadataChanged() = 0;

    /**
     * Called when the Record state has changed.
     * @param newIsRecordingState the new state. True if recording.
     */
    virtual void onRecordStateChanged(bool newIsRecordingState) = 0;
};

}   // namespace arkostracker

