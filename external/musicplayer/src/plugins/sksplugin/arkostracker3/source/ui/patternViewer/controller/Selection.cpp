#include "Selection.h"

#include <juce_core/juce_core.h>

#include "../../../song/tracks/TrackConstants.h"

namespace arkostracker 
{

Selection::Selection() noexcept :
        empty(true),
        startLine(0),
        endLine(0),
        startCursorLocation(),
        endCursorLocation(),
        grown(false),
        createdByMouse(false)
{
}

Selection::Selection(const int pLine, const CursorLocation& pXLocation, const bool pCreatedByMouse) noexcept :
        empty(false),
        startLine(pLine),
        endLine(pLine),
        startCursorLocation(pXLocation),
        endCursorLocation(pXLocation),
        grown(false),
        createdByMouse(pCreatedByMouse)
{
    correctOnAreas();
}

Selection::Selection(const int pStartLine, const CursorLocation& pStartXLocation, const int pEndLine, const CursorLocation& pEndXLocation,
                     const bool pCreatedByMouse, const bool pGrown) noexcept :
        empty(false),
        startLine(pStartLine),
        endLine(pEndLine),
        startCursorLocation(pStartXLocation),
        endCursorLocation(pEndXLocation),
        grown(pGrown),
        createdByMouse(pCreatedByMouse)
{
    jassert((startLine >= 0) && (startLine <= 127));
    jassert((endLine >= 0) && (endLine <= 127));
    correctOnAreas();
}

Selection Selection::buildForLineTrack(int startLine, int endLine) noexcept
{
    return { startLine, CursorLocation::onLineTrack(), endLine, CursorLocation::onLineTrack(), false, false };
}

Selection Selection::buildForWholePattern() noexcept
{
    return {
        0, CursorLocation::onTrack(0, CellCursorRank::first),
        TrackConstants::lastPossibleIndex, CursorLocation::onEventTrack(SpecialCellCursorRank::last),
        false, false
    };
}

Selection Selection::withEnd(int pEndLine, const CursorLocation& pEndXLocation) const noexcept
{
    if (empty) {
        jassertfalse;           // Setting the end to an empty selection is quite strange.
        return { pEndLine, pEndXLocation, pEndLine, pEndXLocation, createdByMouse, grown };
    }

    auto newSelection = Selection(startLine, startCursorLocation, pEndLine, pEndXLocation, createdByMouse, grown);
    newSelection.correctOnAreas();
    return newSelection;
}

Selection Selection::withCreator(bool pCreatedByMouse) const noexcept
{
    if (empty) {
        jassertfalse;           // Setting the creator to an empty selection is quite strange.
    }

    return { startLine, startCursorLocation, endLine, endCursorLocation, pCreatedByMouse, grown };
}

bool Selection::isPresent() const noexcept
{
    return !empty;
}

bool Selection::isEmpty() const noexcept
{
    return empty;
}

int Selection::getTopLine() const noexcept
{
    if (empty) {
        jassertfalse;           // Should have been tested earlier.
    }

    return std::min(startLine, endLine);
}

int Selection::getBottomLine() const noexcept
{
    if (empty) {
        jassertfalse;           // Should have been tested earlier.
    }

    return std::max(startLine, endLine);
}
/*
int Selection::getStartLine() const noexcept
{
    if (empty) {
        jassertfalse;           // Should have been tested earlier.
    }

    return startLine;
}*/

int Selection::getEndLine() const noexcept
{
    if (empty) {
        jassertfalse;           // Should have been tested earlier.
    }

    return endLine;
}

bool Selection::containsLine(const int lineIndex) const noexcept
{
    if (empty) {
        return false;
    }

    const auto topLine = getTopLine();
    const auto bottomLine = getBottomLine();
    return ((lineIndex >= topLine) && (lineIndex <= bottomLine));
}

const CursorLocation& Selection::getLeftCursorLocation() const noexcept
{
    jassert(!empty);        // The emptiness should have been tested before calling this method.

    return (startCursorLocation < endCursorLocation) ? startCursorLocation : endCursorLocation;
}

CursorLocation& Selection::getLeftCursorLocationNotConst() noexcept
{
    // To avoid duplicating the same code as above, for a non-const method. See Effective C++ by Scott Meyer, item 3.
    return const_cast<CursorLocation&>(static_cast<const Selection&>(*this).getLeftCursorLocation()); // NOLINT(*-pro-type-const-cast)
}

const CursorLocation& Selection::getRightCursorLocation() const noexcept
{
    jassert(!empty);        // The emptiness should have been tested before calling this method.

    return (startCursorLocation < endCursorLocation) ? endCursorLocation : startCursorLocation;
}

CursorLocation& Selection::getRightCursorLocationNotConst() noexcept
{
    // To avoid duplicating the same code as above, for a non-const method. See Effective C++ by Scott Meyer, item 3.
    return const_cast<CursorLocation&>(static_cast<const Selection&>(*this).getRightCursorLocation()); // NOLINT(*-pro-type-const-cast)
}

CellCursorRank Selection::getLeftRankAsCellCursorRank() const noexcept
{
    const auto& cursorLocation = getLeftCursorLocation();
    jassert(cursorLocation.getTrackType() == TrackType::normal);

    return cursorLocation.getRankAsCellCursorRank();
}

CellCursorRank Selection::getRightRankAsCellCursorRank() const noexcept
{
    const auto& cursorLocation = getRightCursorLocation();
    jassert(cursorLocation.getTrackType() == TrackType::normal);

    return cursorLocation.getRankAsCellCursorRank();
}

const CursorLocation& Selection::getStartLocation() const noexcept
{
    jassert(!empty);            // Start location doesn't mean anything!
    return startCursorLocation;
}

const CursorLocation& Selection::getEndLocation() const noexcept
{
    jassert(!empty);            // End location doesn't mean anything!
    return endCursorLocation;
}

bool Selection::isSingle() const noexcept
{
    // Simple case first. Must not be empty, or on different lines.
    if (empty || (startLine != endLine)) {
        return false;
    }

    // As we now "enlarge" areas (selecting an effect digit will spread from the effect number to the right-most digit),
    // We cannot simply compare for the start/end location.
    // A simple solution is to put both start/end location "to the left" and compare them.
    const auto cursorLocation1 = startCursorLocation.withAreaCorrection(true);
    const auto cursorLocation2 = endCursorLocation.withAreaCorrection(true);
    return (cursorLocation1 == cursorLocation2);
}

bool Selection::isCreatedByMouse() const noexcept
{
    return createdByMouse;
}

bool Selection::isGrown() const noexcept
{
    return grown;
}

int Selection::getHeight() const noexcept
{
    if (empty) {
        return 0;
    }

    return getBottomLine() - getTopLine() + 1;
}

bool Selection::operator==(const Selection& rhs) const
{
    // If both are empty, no need to compare the other elements, they are unused.
    if (empty && rhs.empty) {
        return true;
    }

    // The "createdByMouse" and "grown" are NOT taken in account the comparison, there is no need.
    return (empty == rhs.empty) &&
           (startLine == rhs.startLine) &&
           (endLine == rhs.endLine) &&
           (startCursorLocation == rhs.startCursorLocation) &&
           (endCursorLocation == rhs.endCursorLocation);
}

bool Selection::operator!=(const Selection& rhs) const
{
    return !(rhs == *this);
}

Selection Selection::extractForTrack(const int channelIndex) const noexcept
{
    if (empty) {
        return { };
    }

    // At least one of the location must be on a music track. We consider the line cannot be selected.
    const auto& leftCursorLocation = getLeftCursorLocation();
    const auto& rightCursorLocation = getRightCursorLocation();
    const auto leftTrackType = leftCursorLocation.getTrackType();
    const auto rightTrackType = rightCursorLocation.getTrackType();
    if ((leftCursorLocation.getTrackType() != TrackType::normal) && (rightTrackType != TrackType::normal)) {
        return { };
    }

    const auto unreachableChannelIndex = channelIndex + 99999;

    // What is the minimum channel index?
    int minimumChannelIndex;            // NOLINT(*-init-variables)
    switch (leftTrackType) {
        case TrackType::line:
            minimumChannelIndex = -1;
            break;
        case TrackType::normal:
            minimumChannelIndex = leftCursorLocation.getChannelIndex();
            break;
        default:
            jassertfalse;           // Should never happen.
        case TrackType::speed:
        case TrackType::event:
            minimumChannelIndex = unreachableChannelIndex;      // The selected channel will not be inside the selection.
            break;
    }
    // No selection if the channel index is too much "to the left" of the selection.
    if (channelIndex < minimumChannelIndex) {
        return { };
    }

    // What is the maximum channel index ?
    int maximumChannelIndex;        // NOLINT(*-init-variables)
    switch (rightTrackType) {
        case TrackType::line:
            maximumChannelIndex = -1;
            break;
        case TrackType::normal:
            maximumChannelIndex = rightCursorLocation.getChannelIndex();
            break;
        default:
            jassertfalse;           // Should never happen.
        case TrackType::speed:
        case TrackType::event:
            maximumChannelIndex = unreachableChannelIndex;
            break;
    }
    // If selection is after the maximum, there is nothing to select.
    if (channelIndex > maximumChannelIndex) {
        return { };
    }

    // Simple case first: if the selection is inside the min/max EXCLUDED, the full track is selected.
    if ((minimumChannelIndex < channelIndex) && (channelIndex < maximumChannelIndex)) {
        return { startLine, CursorLocation::onTrack(channelIndex, CellCursorRank::first),
                 endLine, CursorLocation::onTrack(channelIndex, CellCursorRank::last), createdByMouse, grown };
    }

    // Finds the left rank.
    auto leftRank = CellCursorRank::first;                          // If minimumChannel < index, this default value is used.
    jassert(minimumChannelIndex <= channelIndex);          // Abnormal, that was tested before!
    if (minimumChannelIndex == channelIndex) {
        leftRank = leftCursorLocation.getRankAsCellCursorRank();
    }

    // Finds the right rank.
    auto rightRank = CellCursorRank::last;                          // If maximumChannel > index, this default value is used.
    jassert(channelIndex <= maximumChannelIndex);          // Abnormal, that was tested before!
    if (channelIndex == maximumChannelIndex) {
        rightRank = rightCursorLocation.getRankAsCellCursorRank();
    }
    jassert(leftRank <= rightRank);

    return { startLine, CursorLocation::onTrack(channelIndex, leftRank),
             endLine, CursorLocation::onTrack(channelIndex, rightRank), createdByMouse, grown };
}

Selection Selection::extractForSpecialTrack(const bool isSpeedTrack) const noexcept
{
    if (empty) {
        return { };
    }

    const auto& leftCursorLocation = getLeftCursorLocation();
    const auto& rightCursorLocation = getRightCursorLocation();
    const auto leftTrackType = leftCursorLocation.getTrackType();
    const auto rightTrackType = rightCursorLocation.getTrackType();

    // Simple case first:
    // If right is not in a special track, there is no selection.
    if ((rightTrackType != TrackType::speed) && (rightTrackType != TrackType::event)) {
        return { };
    }
    // We are sure there is a selection in a special track.

    auto isSpeedTrackIncluded = false;
    auto isEventTrackIncluded = false;

    // Checks the left track first.
    switch (leftTrackType) {
        default:
            jassertfalse;           // Should never happen.
        case TrackType::line:
        case TrackType::normal:
        case TrackType::speed:
            isSpeedTrackIncluded = true;
            break;
        case TrackType::event:
            isEventTrackIncluded = true;
            break;
    }
    // Checks the right track first.
    switch (rightTrackType) {
        default:
        case TrackType::line:
        case TrackType::normal:
            jassertfalse;               // Not supposed to happen, tested before.
        case TrackType::speed:
            isSpeedTrackIncluded = true;
            break;
        case TrackType::event:
            isEventTrackIncluded = true;
            break;
    }

    if ((isSpeedTrack && isSpeedTrackIncluded) || (!isSpeedTrack && isEventTrackIncluded)) {
        auto leftXLocation = CursorLocation::onSpecialTrack(isSpeedTrack, SpecialCellCursorRank::first);
        auto rightXLocation = CursorLocation::onSpecialTrack(isSpeedTrack, SpecialCellCursorRank::last);
        return { startLine, leftXLocation, endLine, rightXLocation, createdByMouse, grown };
    }

    return { };
}

Selection Selection::enlargeSelectionIfNeeded() const noexcept
{
    // If empty, no change.
    if (empty) {
        return *this;
    }

    // Do we need to grow? If start/end are on the same channel, no need.
    const auto startTrackType = startCursorLocation.getTrackType();
    if (startTrackType == endCursorLocation.getTrackType()) {
        // Same types.
        switch (startTrackType) {
            default:
                jassertfalse;           // Not supposed to happen.
            case TrackType::line:
            case TrackType::event:
            case TrackType::speed:
                return *this;
            case TrackType::normal:
                // Same channel? If yes, don't change, but only if "not grown yet". If grown, keeps grown, managed below.
                if (!grown && (startCursorLocation.getChannelIndex() == endCursorLocation.getChannelIndex())) {
                    return *this;
                }
        }
    }

    return growSelectionHorizontally();
}

Selection Selection::growSelectionHorizontally() const noexcept
{
    // The start/end may be special, or "normal" but on different channel.
    // Grows the selection, making sure to keep the ordering.
    const auto leftToRight = (startCursorLocation < endCursorLocation);         // We know they are not on the same channel.
    auto newStartCursorLocation = growCursorLocation(startCursorLocation, leftToRight);      // If left to right, the start must "grow" to the left.
    auto newEndCursorLocation = growCursorLocation(endCursorLocation, !leftToRight);

    return { startLine, newStartCursorLocation, endLine, newEndCursorLocation, createdByMouse, true };      // Grown, from now on.
}

Selection Selection::growHeight() const noexcept
{
    return { 0, startCursorLocation,
        TrackConstants::lastPossibleIndex, endCursorLocation,
        createdByMouse, true };      // Grown, from now on.
}

CursorLocation Selection::growCursorLocation(const CursorLocation& inputCursorLocation, const bool growToLeft) noexcept
{
    switch (inputCursorLocation.getTrackType()) {
        default:
        case TrackType::event:
            return CursorLocation::onEventTrack(growToLeft ? SpecialCellCursorRank::first : SpecialCellCursorRank::last);
        case TrackType::speed:
            return CursorLocation::onSpeedTrack(growToLeft ? SpecialCellCursorRank::first : SpecialCellCursorRank::last);
        case TrackType::line:
            return inputCursorLocation;
        case TrackType::normal:
            return CursorLocation::onTrack(inputCursorLocation.getChannelIndex(), growToLeft ? CellCursorRank::first : CellCursorRank::last);
    }
}

void Selection::correctOnAreas() noexcept
{
    // I don't like using pointers, but it was needed.
    auto* leftCursorLocation = &getLeftCursorLocationNotConst();
    *leftCursorLocation = leftCursorLocation->withAreaCorrection(true);

    auto* rightCursorLocation = &getRightCursorLocationNotConst();
    *rightCursorLocation = rightCursorLocation->withAreaCorrection(false);
}

std::vector<int> Selection::getChannelIndexes(const int channelCount) const noexcept
{
    jassert(channelCount > 0);

    std::vector<int> channelIndexes;

    // If the left cursor is not a Normal Track, then there can be no channel index.
    const auto& leftCursorLocation = getLeftCursorLocation();
    if (leftCursorLocation.getTrackType() != TrackType::normal) {
        return channelIndexes;
    }
    channelIndexes.reserve(static_cast<size_t>(channelCount));

    const auto& rightCursorLocation = getRightCursorLocation();
    auto endChannelIndex = channelCount - 1;        // Default value.
    switch (rightCursorLocation.getTrackType()) {
        case TrackType::line:
        default:
            jassertfalse;           // Shouldn't happen.
            break;
        case TrackType::speed:
        case TrackType::event:
            // EndChannelIndex set above is fine.
            break;
        case TrackType::normal:
            endChannelIndex = rightCursorLocation.getChannelIndex();
            break;
    }

    for (auto channelIndex = leftCursorLocation.getChannelIndex(); channelIndex <= endChannelIndex; ++channelIndex) {
        channelIndexes.push_back(channelIndex);
    }

    return channelIndexes;
}

bool Selection::containsSpecialTrack(const bool isSpeedTrack) const noexcept
{
    const auto selection = extractForSpecialTrack(isSpeedTrack);        // Nice little trick to use our previous method.
    return !selection.isEmpty();
}

bool Selection::containsSpecialTrack() const noexcept
{
    return containsSpecialTrack(false) || containsSpecialTrack(true);
}

OptionalInt Selection::getLeftStartChannelIndexIfOnTrack() const noexcept
{
    const auto& leftCursorLocation = getLeftCursorLocation();
    return (leftCursorLocation.getTrackType() == TrackType::normal) ? leftCursorLocation.getChannelIndex() : OptionalInt();
}

bool Selection::isSingleChannel() const noexcept
{
    // Special Tracks?
    if (containsSpecialTrack(false) && containsSpecialTrack(true)) {
        return false;
    }

    // Music Track.
    return !empty && (startCursorLocation.getChannelIndex() == endCursorLocation.getChannelIndex());
}

bool Selection::isValid(const int height) const noexcept
{
    return empty || ((startLine < height) && (endLine < height));
}

}   // namespace arkostracker
