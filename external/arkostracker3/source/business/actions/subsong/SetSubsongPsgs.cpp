#include "SetSubsongPsgs.h"

#include "../../../controllers/SongController.h"
#include "../../../controllers/observers/SubsongMetadataObserver.h"
#include "../../../utils/PsgValues.h"

namespace arkostracker 
{

SetSubsongPsgs::SetSubsongPsgs(SongController& pSongController, Id pSubsongId, std::vector<Psg> pPsgs, Properties pMetadata) noexcept :
        songController(pSongController),
        subsongId(std::move(pSubsongId)),
        newPsgs(std::move(pPsgs)),
        oldPsgs(),
        newMetadata(std::move(pMetadata)),
        oldMetadata(),
        oldSubsong()
{
}

bool SetSubsongPsgs::perform()
{
    songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
        // Gets the previous PSGs.
        oldPsgs = subsong.getPsgs();

        if (oldPsgs.size() == newPsgs.size()) {
            // Simpler case: same amount of PSG. Simply change the metadata and the PSG metadata. No need to add/remove Tracks.

            // Gets the previous metadata for Undo, sets the new one.
            oldMetadata = std::make_unique<Properties>(subsong.getMetadata());
            newMetadata.setMetadataToSubsong(subsong);

            // Sets the PSGs.
            subsong.setPsgs(newPsgs);
        } else {
            // More complex case: the PSG count is not the same. Stores the Subsong for Undo before modifying the Positions to add/remove Tracks.
            oldSubsong = std::make_unique<Subsong>(subsong);

            // Sets the metadata/psg directly, no need to save the previous states, as we have stored the whole Subsong!
            newMetadata.setMetadataToSubsong(subsong);
            subsong.setPsgs(newPsgs);

            // Now we need to remove or create channels, and Tracks.
            const auto newChannelCount = PsgValues::getChannelCount(static_cast<int>(newPsgs.size()));
            const auto positionCount = subsong.getLength();
            for (auto positionIndex = 0; positionIndex < positionCount; ++positionIndex) {
                // To be clean, removes the transpositions that are out of bounds.
                const auto& originalPosition = subsong.getPositionRef(positionIndex);
                const auto& originalChannelToTransposition = originalPosition.getChannelToTransposition();
                std::unordered_map<int, int> newChannelToTransposition;
                std::copy_if(originalChannelToTransposition.cbegin(), originalChannelToTransposition.cend(),
                             std::inserter(newChannelToTransposition, std::begin(newChannelToTransposition)),   // With the help of https://stackoverflow.com/a/39080735.
                             [&](const auto& entry) {
                    return (entry.first < newChannelCount);
                });
                const auto newPosition = originalPosition.withTranspositions(newChannelToTransposition);

                subsong.setPosition(positionIndex, newPosition);
            }

            const auto patternCount = subsong.getPatternCount();
            for (auto patternIndex = 0; patternIndex < patternCount; ++patternIndex) {
                const auto originalPattern = subsong.getPatternFromIndex(patternIndex);
                // Must create or remove Tracks?
                const auto patternChannelCount = originalPattern.getChannelCount();
                if (newChannelCount == patternChannelCount) {
                    jassertfalse;           // Shouldn't happen, the PSG count is supposed to be different!
                    continue;
                }
                std::vector<Pattern::TrackIndexAndLinkedTrackIndex> newTrackIndexes;

                if (newChannelCount < patternChannelCount) {
                    // Easier case: Tracks must be removed. Only keeps the ones from within the new channel count.
                    for (auto channelIndex = 0; channelIndex < newChannelCount; ++channelIndex) {
                        newTrackIndexes.emplace_back(originalPattern.getTrackIndexAndLinkedTrackIndex(channelIndex));
                    }
                } else /*if (newChannelCount > patternChannelCount)*/ {
                    // More complex case: Tracks must be added.
                    // First adds the existing ones.
                    for (auto channelIndex = 0; channelIndex < patternChannelCount; ++channelIndex) {
                        newTrackIndexes.emplace_back(originalPattern.getTrackIndexAndLinkedTrackIndex(channelIndex));
                    }
                    // Then creates new Tracks, and adds them in the Pattern.
                    for (auto channelIndex = patternChannelCount; channelIndex < newChannelCount; ++channelIndex) {
                        const auto newTrackIndex = subsong.addTrack({ });
                        newTrackIndexes.emplace_back(newTrackIndex);
                    }
                }
                // We can now create the new Pattern, mostly from the data of the original Pattern.
                const auto newPattern = Pattern(newTrackIndexes, originalPattern.getSpecialTrackIndexAndLinkedTrackIndex(true),
                                                originalPattern.getSpecialTrackIndexAndLinkedTrackIndex(false), originalPattern.getArgbColor());
                subsong.setPattern(patternIndex, newPattern);
            }
        }
    });

    notify();

    return true;
}

bool SetSubsongPsgs::undo()
{
    // Either simple (only metadata), or more brutal (whole Subsong).
    if (oldSubsong == nullptr) {
        songController.performOnSubsong(subsongId, [&](Subsong& subsong) noexcept {
            // Metadata only, and PSG count is the same.
            jassert(oldMetadata != nullptr);
            oldMetadata->setMetadataToSubsong(subsong);
            subsong.setPsgs(oldPsgs);
        });
    } else {
        // Whole Subsong.
        const auto song = songController.getSong();
        song->replaceSubsong(subsongId, std::move(oldSubsong));
        oldSubsong.reset();
    }

    notify();

    return true;
}

void SetSubsongPsgs::notify() const noexcept
{
    songController.getSubsongMetadataObservers().applyOnObservers([&](SubsongMetadataObserver* observer) noexcept {
        observer->onSubsongMetadataChanged(subsongId, SubsongMetadataObserver::What::psgsData);
    });
}

}   // namespace arkostracker
