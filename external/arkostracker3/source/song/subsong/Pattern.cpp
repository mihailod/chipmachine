#include "Pattern.h"

namespace arkostracker
{

Pattern::Pattern(const std::vector<int>& pTrackIndexes, const int pSpeedTrackIndex, const int pEventTrackIndex, const juce::uint32 pColor) noexcept :
        color(pColor),
        trackLinksOrIndexes(TrackIndexAndLinkedTrackIndex::buildFromTrackIds(pTrackIndexes)),
        speedTrackLinkOrIndex(pSpeedTrackIndex),
        eventTrackLinkOrIndex(pEventTrackIndex)
{
}

Pattern::Pattern(std::vector<TrackIndexAndLinkedTrackIndex> pTrackIdAndLinkedTrackId, const Pattern::TrackIndexAndLinkedTrackIndex pSpeedTrackLinkOrIndex,
                 const Pattern::TrackIndexAndLinkedTrackIndex pEventsTrackLinkOrIndex, const juce::uint32 pColor) noexcept :
        color(pColor),
        trackLinksOrIndexes(std::move(pTrackIdAndLinkedTrackId)),
        speedTrackLinkOrIndex(pSpeedTrackLinkOrIndex),
        eventTrackLinkOrIndex(pEventsTrackLinkOrIndex)
{
}

Pattern::Pattern() noexcept :
        color(LookAndFeelConstants::defaultPatternColor),
        trackLinksOrIndexes(),
        speedTrackLinkOrIndex(0),
        eventTrackLinkOrIndex(0)
{
}

Pattern::Pattern(const juce::uint32 pColor) noexcept :
        color(pColor),
        trackLinksOrIndexes(),
        speedTrackLinkOrIndex(0),
        eventTrackLinkOrIndex(0)
{
}

Pattern Pattern::withColor(juce::uint32 newColor) const noexcept
{
    return { trackLinksOrIndexes, speedTrackLinkOrIndex, eventTrackLinkOrIndex, newColor };
}

Pattern Pattern::withSpeedTrackIndex(const int newSpeedTrackIndex) const noexcept
{
    return { trackLinksOrIndexes, Pattern::TrackIndexAndLinkedTrackIndex(newSpeedTrackIndex), eventTrackLinkOrIndex, color };
}

juce::uint32 Pattern::getArgbColor() const noexcept
{
    return color;
}

int Pattern::getCurrentTrackIndex(const int channelIndex) const noexcept
{
    jassert(channelIndex < static_cast<int>(trackLinksOrIndexes.size()));

    return trackLinksOrIndexes.at(static_cast<size_t>(channelIndex)).getUsedTrackIndex();
}

std::vector<int> Pattern::getCurrentTrackIndexes() const noexcept
{
    std::vector<int> trackIds;
    trackIds.reserve(trackLinksOrIndexes.size());

    for (const auto& trackLinkOrIndex : trackLinksOrIndexes) {
        trackIds.push_back(trackLinkOrIndex.getUsedTrackIndex());
    }

    return trackIds;
}

const Pattern::TrackIndexAndLinkedTrackIndex& Pattern::getTrackIndexAndLinkedTrackIndex(const int channelIndex) const noexcept
{
    return trackLinksOrIndexes.at(static_cast<size_t>(channelIndex));
}

const std::vector<Pattern::TrackIndexAndLinkedTrackIndex>& Pattern::getTrackIndexesAndLinkedTrackIndexes() const noexcept
{
    return trackLinksOrIndexes;
}

void Pattern::addTrackIndex(int trackIndex, OptionalInt linkedTrackIndex) noexcept
{
    trackLinksOrIndexes.emplace_back(trackIndex, linkedTrackIndex);
}

void Pattern::addTrackIndex(const TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex) noexcept
{
    trackLinksOrIndexes.emplace_back(trackIndexAndLinkedTrackIndex);
}

void Pattern::setTrackIndex(const int channelIndex, const TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex) noexcept
{
    trackLinksOrIndexes[static_cast<size_t>(channelIndex)] = trackIndexAndLinkedTrackIndex;
}

int Pattern::getCurrentSpecialTrackIndex(const bool speedTrack) const noexcept
{
    return speedTrack ? speedTrackLinkOrIndex.getUsedTrackIndex() : eventTrackLinkOrIndex.getUsedTrackIndex();
}

void Pattern::setSpecialTrackIndex(const bool speedTrack, const int trackIndex, const OptionalInt linkedTrackIndex) noexcept
{
    auto& target = speedTrack ? speedTrackLinkOrIndex : eventTrackLinkOrIndex;
    target = TrackIndexAndLinkedTrackIndex(trackIndex, linkedTrackIndex);
}

void Pattern::setSpecialTrackIndex(const bool speedTrack, const TrackIndexAndLinkedTrackIndex& trackIndexAndLinkedTrackIndex) noexcept
{
    auto& target = speedTrack ? speedTrackLinkOrIndex : eventTrackLinkOrIndex;
    target = trackIndexAndLinkedTrackIndex;
}

Pattern::TrackIndexAndLinkedTrackIndex Pattern::getSpecialTrackIndexAndLinkedTrackIndex(const bool speedTrack) const noexcept
{
    return speedTrack ? speedTrackLinkOrIndex : eventTrackLinkOrIndex;
}

int Pattern::getChannelCount() const noexcept
{
    return static_cast<int>(trackLinksOrIndexes.size());
}

std::vector<int> Pattern::getWhereTrackUsed(const int trackIndexToFind, const LinkType linkType) const noexcept
{
    std::vector<int> channelIndexes;
    auto channelIndex = 0;

    for (const auto& trackIndexes : trackLinksOrIndexes) {
        auto found = false;

        switch (linkType) {
            // The most logical: linked if present, else unlinked.
            case LinkType::unlinkedIfLinkedNotPresent:
                found = (trackIndexToFind == trackIndexes.getUsedTrackIndex());
                break;
            // Linked only, if present.
            case LinkType::linkedOnly:
                found = trackIndexes.isLinked() && (trackIndexToFind == trackIndexes.getLinkedTrackIndex().getValue());
                break;
            case LinkType::unlinkedOnly:
                found = !trackIndexes.isLinked() && (trackIndexToFind == trackIndexes.getUnlinkedTrackIndex());
                break;
            default:
                jassertfalse;       // Not managed? Should never happen.
                break;
        }

        if (found) {
            channelIndexes.push_back(channelIndex);
        }
        ++channelIndex;
    }

    return channelIndexes;
}

bool Pattern::isSpecialTrackUsed(const bool speedTrack, const int specialTrackIndexToFind, const LinkType linkType) const noexcept
{
    const auto currentSpecialTrackIndexes = speedTrack ? speedTrackLinkOrIndex : eventTrackLinkOrIndex;

    switch (linkType) {
        case LinkType::unlinkedIfLinkedNotPresent:
            return (specialTrackIndexToFind == currentSpecialTrackIndexes.getUsedTrackIndex());
        case LinkType::linkedOnly:
            return currentSpecialTrackIndexes.isLinked() && (specialTrackIndexToFind == currentSpecialTrackIndexes.getLinkedTrackIndex().getValue());
        case LinkType::unlinkedOnly:
            return !currentSpecialTrackIndexes.isLinked() && (specialTrackIndexToFind == currentSpecialTrackIndexes.getUnlinkedTrackIndex());
        default:
            jassertfalse;       // Not managed? Should never happen.
            return false;
    }
}


bool Pattern::operator==(const Pattern& rhs) const noexcept
{
    return (color == rhs.color) &&
           (trackLinksOrIndexes == rhs.trackLinksOrIndexes) &&
           (speedTrackLinkOrIndex == rhs.speedTrackLinkOrIndex) &&
           (eventTrackLinkOrIndex == rhs.eventTrackLinkOrIndex);
}

bool Pattern::operator!=(const Pattern& rhs) const noexcept
{
    return !(rhs == *this);
}

bool Pattern::areEqualMusically(const Pattern& other) const noexcept
{
    return (trackLinksOrIndexes == other.trackLinksOrIndexes) &&
           (speedTrackLinkOrIndex == other.speedTrackLinkOrIndex) &&
           (eventTrackLinkOrIndex == other.eventTrackLinkOrIndex);
}

bool Pattern::operator<(const Pattern& rhs) const noexcept
{
    if (speedTrackLinkOrIndex != rhs.speedTrackLinkOrIndex) {
        return (speedTrackLinkOrIndex < rhs.speedTrackLinkOrIndex);
    }
    if (eventTrackLinkOrIndex != rhs.eventTrackLinkOrIndex) {
        return (eventTrackLinkOrIndex < rhs.eventTrackLinkOrIndex);
    }
    if (color != rhs.color) {
        return (color < rhs.color);
    }

    return (trackLinksOrIndexes < rhs.trackLinksOrIndexes);
}


// TrackIdAndLinkedTrackId
// ==================================================================

Pattern::TrackIndexAndLinkedTrackIndex::TrackIndexAndLinkedTrackIndex(const int pTrackIndex, const OptionalInt pLinkedTrackIndex) noexcept :
        trackIndex(pTrackIndex),
        linkedTrackIndex(pLinkedTrackIndex)
{
}

std::vector<Pattern::TrackIndexAndLinkedTrackIndex> Pattern::TrackIndexAndLinkedTrackIndex::buildFromTrackIds(const std::vector<int>& trackIndexes) noexcept
{
    std::vector<TrackIndexAndLinkedTrackIndex> trackIdsAndLinkedTrackIds;
    trackIdsAndLinkedTrackIds.reserve(trackIndexes.size());

    for (const auto trackIndex : trackIndexes) {
        trackIdsAndLinkedTrackIds.emplace_back(trackIndex);
    }

    return trackIdsAndLinkedTrackIds;
}

/** @return the normal track index, or linked one if present. */
int Pattern::TrackIndexAndLinkedTrackIndex::getUsedTrackIndex() const noexcept
{
    return linkedTrackIndex.isPresent() ? linkedTrackIndex.getValue() : trackIndex;
}

int Pattern::TrackIndexAndLinkedTrackIndex::getUnlinkedTrackIndex() const
{
    return trackIndex;
}

const OptionalInt& Pattern::TrackIndexAndLinkedTrackIndex::getLinkedTrackIndex() const
{
    return linkedTrackIndex;
}

bool Pattern::TrackIndexAndLinkedTrackIndex::isValid() const noexcept
{
    return (trackIndex >= 0) && (linkedTrackIndex.isAbsent() || (linkedTrackIndex.getValue() >= 0));
}

bool Pattern::TrackIndexAndLinkedTrackIndex::isLinked() const noexcept
{
    return linkedTrackIndex.isPresent();
}

bool Pattern::TrackIndexAndLinkedTrackIndex::operator==(const TrackIndexAndLinkedTrackIndex& rhs) const noexcept
{
    return (trackIndex == rhs.trackIndex) &&
           (linkedTrackIndex == rhs.linkedTrackIndex);
}

bool Pattern::TrackIndexAndLinkedTrackIndex::operator!=(const TrackIndexAndLinkedTrackIndex& rhs) const noexcept
{
    return !(rhs == *this);
}

bool Pattern::TrackIndexAndLinkedTrackIndex::operator<(const TrackIndexAndLinkedTrackIndex& rhs) const noexcept
{
    if (trackIndex != rhs.trackIndex) {
        return (trackIndex < rhs.trackIndex);
    }

    if (linkedTrackIndex.isPresent() && rhs.linkedTrackIndex.isPresent()) {
        return (linkedTrackIndex.getValue() < rhs.linkedTrackIndex.getValue());
    }
    if (linkedTrackIndex.isAbsent() && rhs.linkedTrackIndex.isAbsent()) {
        return false;
    }
    if (linkedTrackIndex.isPresent()) {
        return false;
    }

    return true;
}

}   // namespace arkostracker
