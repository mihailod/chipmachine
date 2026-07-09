#include "CursorLocation.h"

#include <unordered_map>

#include <juce_core/juce_core.h>

namespace arkostracker 
{

CursorLocation::CursorLocation() noexcept :
        trackType(TrackType::normal),
        index(0),
        rank(static_cast<int>(CellCursorRank::cursorOnNote))
{
}

CursorLocation::CursorLocation(const TrackType pTrackType, const int pIndex, const int pRank) noexcept :
        trackType(pTrackType),
        index(pIndex),
        rank(pRank)
{
}

CursorLocation CursorLocation::onTrack(int channelIndex, CellCursorRank rank) noexcept
{
    return { TrackType::normal, channelIndex, static_cast<int>(rank) };
}

CursorLocation CursorLocation::onSpeedTrack(SpecialCellCursorRank rank) noexcept
{
    return { TrackType::speed, 0, static_cast<int>(rank) };
}

CursorLocation CursorLocation::onEventTrack(SpecialCellCursorRank rank) noexcept
{
    return { TrackType::event, 0, static_cast<int>(rank) };
}

CursorLocation CursorLocation::onLineTrack() noexcept
{
    return { TrackType::line, 0, 0 };
}

CursorLocation CursorLocation::onSpecialTrack(const bool isSpeedTrack, const SpecialCellCursorRank rank) noexcept
{
    return isSpeedTrack ? onSpeedTrack(rank) : onEventTrack(rank);
}

TrackType CursorLocation::getTrackType() const noexcept
{
    return trackType;
}

int CursorLocation::getChannelIndex() const noexcept
{
    return index;
}

int CursorLocation::getRank() const noexcept
{
    return rank;
}

CellCursorRank CursorLocation::getRankAsCellCursorRank() const noexcept
{
    jassert(trackType == TrackType::normal);        // This method was wrongly called!
    return static_cast<CellCursorRank>(rank);
}

SpecialCellCursorRank CursorLocation::getRankAsSpecialCursorRank() const noexcept
{
    jassert((trackType == TrackType::speed) || (trackType == TrackType::event));        // This method was wrongly called!
    return static_cast<SpecialCellCursorRank>(rank);
}

bool CursorLocation::isMusicTrack() const noexcept
{
    return trackType == TrackType::normal;
}

std::pair<OptionalInt, bool> CursorLocation::getEffectIndexIfRankOnEffect() const noexcept
{
    if (!isMusicTrack()) {
        return { };
    }

    switch (getRankAsCellCursorRank()) {
        case CellCursorRank::cursorOnEffect1Number:
            return { 0, true };
        case CellCursorRank::cursorOnEffect2Number:
            return { 1, true };
        case CellCursorRank::cursorOnEffect3Number:
            return { 2, true };
        case CellCursorRank::cursorOnEffect4Number:
            return { 3, true };
        case CellCursorRank::cursorOnNote: [[fallthrough]];
        case CellCursorRank::cursorOnInstrumentDigit2: [[fallthrough]];
        case CellCursorRank::cursorOnInstrumentDigit1: [[fallthrough]];
        case CellCursorRank::count:
        default:
            return { };
        case CellCursorRank::cursorOnEffect1Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect1Digit1:
            return { 0, false };
        case CellCursorRank::cursorOnEffect2Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect2Digit1:
            return { 1, false };
        case CellCursorRank::cursorOnEffect3Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect3Digit1:
            return { 2, false };
        case CellCursorRank::cursorOnEffect4Digit3: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit2: [[fallthrough]];
        case CellCursorRank::cursorOnEffect4Digit1:
            return { 3, false };
    }
}

OptionalValue<std::pair<int, CellCursorRank>> CursorLocation::getChannelIndexAndRankIfOnTrack() const noexcept
{
    if (trackType != TrackType::normal) {
        return { };
    }

    return {std::make_pair(index, static_cast<CellCursorRank>(rank)) };
}

bool CursorLocation::operator==(const CursorLocation& rhs) const
{
    return trackType == rhs.trackType &&
           index == rhs.index &&
           rank == rhs.rank;
}

bool CursorLocation::operator!=(const CursorLocation& rhs) const
{
    return !(rhs == *this);
}

bool CursorLocation::operator<(const CursorLocation& rhs) const
{
    // The track type enumeration is ordered.
    if (static_cast<int>(trackType) != static_cast<int>(rhs.trackType)) {
        return static_cast<int>(trackType) < static_cast<int>(rhs.trackType);
    }
    // Same track type.
    switch (trackType) {
        default:
            jassertfalse;
        case TrackType::line:
            return false;                // Shouldn't really happen...
        case TrackType::speed:
        case TrackType::event:
            return (rank < rhs.rank);
        case TrackType::normal:
            if (index != rhs.index) {
                return (index < rhs.index);
            }
            // Same channel.
            return rank < rhs.rank;
    }
}

CursorLocation CursorLocation::withEffectCorrection() const noexcept
{
    if (trackType != TrackType::normal) {
        return *this;
    }

    // Map of the current rank to a new rank. Only needed values are encoded.
    static const std::unordered_map<CellCursorRank, CellCursorRank> rankToNewRank = {
            { CellCursorRank::cursorOnEffect1Digit1, CellCursorRank::cursorOnEffect1Number },
            { CellCursorRank::cursorOnEffect1Digit2, CellCursorRank::cursorOnEffect1Number },
            { CellCursorRank::cursorOnEffect1Digit3, CellCursorRank::cursorOnEffect1Number },

            { CellCursorRank::cursorOnEffect2Digit1, CellCursorRank::cursorOnEffect2Number },
            { CellCursorRank::cursorOnEffect2Digit2, CellCursorRank::cursorOnEffect2Number },
            { CellCursorRank::cursorOnEffect2Digit3, CellCursorRank::cursorOnEffect2Number },

            { CellCursorRank::cursorOnEffect3Digit1, CellCursorRank::cursorOnEffect3Number },
            { CellCursorRank::cursorOnEffect3Digit2, CellCursorRank::cursorOnEffect3Number },
            { CellCursorRank::cursorOnEffect3Digit3, CellCursorRank::cursorOnEffect3Number },

            { CellCursorRank::cursorOnEffect4Digit1, CellCursorRank::cursorOnEffect4Number },
            { CellCursorRank::cursorOnEffect4Digit2, CellCursorRank::cursorOnEffect4Number },
            { CellCursorRank::cursorOnEffect4Digit3, CellCursorRank::cursorOnEffect4Number },
    };

    const auto currentRank = getRankAsCellCursorRank();
    auto iterator = rankToNewRank.find(currentRank);
    if (iterator == rankToNewRank.cend()) {
        return *this;       // Not found, no change!
    }

    // Change.
    return { trackType, index, static_cast<int>(iterator->second) };
}

CursorLocation CursorLocation::withAreaCorrection(const bool goToLeft) const noexcept
{
    if (trackType != TrackType::normal) {           // NOTE: for now, seems only useful for Normal tracks. Special tracks don't have the digit granularity (on display).
        return *this;
    }

    // Map of the current rank to a new rank. Only needed values are encoded.
    static const std::unordered_map<CellCursorRank, CellCursorRank> rankToNewRankForLeft = {
            { CellCursorRank::cursorOnInstrumentDigit1, CellCursorRank::cursorOnInstrumentDigit2 },

            { CellCursorRank::cursorOnEffect1Digit1, CellCursorRank::cursorOnEffect1Number },
            { CellCursorRank::cursorOnEffect1Digit2, CellCursorRank::cursorOnEffect1Number },
            { CellCursorRank::cursorOnEffect1Digit3, CellCursorRank::cursorOnEffect1Number },

            { CellCursorRank::cursorOnEffect2Digit1, CellCursorRank::cursorOnEffect2Number },
            { CellCursorRank::cursorOnEffect2Digit2, CellCursorRank::cursorOnEffect2Number },
            { CellCursorRank::cursorOnEffect2Digit3, CellCursorRank::cursorOnEffect2Number },

            { CellCursorRank::cursorOnEffect3Digit1, CellCursorRank::cursorOnEffect3Number },
            { CellCursorRank::cursorOnEffect3Digit2, CellCursorRank::cursorOnEffect3Number },
            { CellCursorRank::cursorOnEffect3Digit3, CellCursorRank::cursorOnEffect3Number },

            { CellCursorRank::cursorOnEffect4Digit1, CellCursorRank::cursorOnEffect4Number },
            { CellCursorRank::cursorOnEffect4Digit2, CellCursorRank::cursorOnEffect4Number },
            { CellCursorRank::cursorOnEffect4Digit3, CellCursorRank::cursorOnEffect4Number },
    };
    static const std::unordered_map<CellCursorRank, CellCursorRank> rankToNewRankForRight = {
            { CellCursorRank::cursorOnInstrumentDigit2, CellCursorRank::cursorOnInstrumentDigit1 },

            { CellCursorRank::cursorOnEffect1Number, CellCursorRank::cursorOnEffect1Digit1 },
            { CellCursorRank::cursorOnEffect1Digit3, CellCursorRank::cursorOnEffect1Digit1 },
            { CellCursorRank::cursorOnEffect1Digit2, CellCursorRank::cursorOnEffect1Digit1 },

            { CellCursorRank::cursorOnEffect2Number, CellCursorRank::cursorOnEffect2Digit1 },
            { CellCursorRank::cursorOnEffect2Digit3, CellCursorRank::cursorOnEffect2Digit1 },
            { CellCursorRank::cursorOnEffect2Digit2, CellCursorRank::cursorOnEffect2Digit1 },

            { CellCursorRank::cursorOnEffect3Number, CellCursorRank::cursorOnEffect3Digit1 },
            { CellCursorRank::cursorOnEffect3Digit3, CellCursorRank::cursorOnEffect3Digit1 },
            { CellCursorRank::cursorOnEffect3Digit2, CellCursorRank::cursorOnEffect3Digit1 },

            { CellCursorRank::cursorOnEffect4Number, CellCursorRank::cursorOnEffect4Digit1 },
            { CellCursorRank::cursorOnEffect4Digit3, CellCursorRank::cursorOnEffect4Digit1 },
            { CellCursorRank::cursorOnEffect4Digit2, CellCursorRank::cursorOnEffect4Digit1 },
    };

    const auto& rankToNewRank = goToLeft ? rankToNewRankForLeft : rankToNewRankForRight;

    const auto currentRank = getRankAsCellCursorRank();
    auto iterator = rankToNewRank.find(currentRank);
    if (iterator == rankToNewRank.cend()) {
        return *this;       // Not found, no change!
    }

    // Change.
    return { trackType, index, static_cast<int>(iterator->second) };
}

}   // namespace arkostracker
