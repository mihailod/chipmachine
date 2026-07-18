#include "Subsong.h"

#include "../../business/song/validation/CheckLoopStartEnd.h"
#include "../../utils/NumberUtil.h"
#include "../../utils/PsgValues.h"
#include "../TrackLocation.h"
#include "SubsongConstants.h"

namespace arkostracker 
{

// Subsong Metadata.
// =======================================================================

Subsong::Metadata::Metadata(juce::String pName, const int pInitialSpeed, const float pReplayFrequencyHz, const int pDigiChannel, const int pHighlightSpacing,
    const int pSecondaryHighlight, const int pLoopStartPosition, const int pEndPosition, const SidPlayerCapability& pSidPlayerCapability) noexcept :
        name(std::move(pName)),
        initialSpeed(pInitialSpeed),
        replayFrequencyHz(pReplayFrequencyHz),
        digiChannel(pDigiChannel),
        highlightSpacing(pHighlightSpacing),
        secondaryHighlight(pSecondaryHighlight),
        loopStartPosition(pLoopStartPosition),
        endPosition(pEndPosition),
        sidPlayerCapability(pSidPlayerCapability)
{
}

Subsong::Metadata Subsong::Metadata::buildDefault() noexcept
{
    return {
        juce::translate("Untitled"), SubsongConstants::defaultSpeed, PsgFrequency::defaultReplayFrequencyHz,
        SubsongConstants::defaultDigiChannel, SubsongConstants::defaultPrimaryHighlight, SubsongConstants::defaultSecondaryHighlight,
        0, 0, SidPlayerCapability::buildForCpc()
    };
}

bool Subsong::Metadata::operator==(const Metadata& rhs) const
{
    return name == rhs.name &&
           initialSpeed == rhs.initialSpeed &&
           juce::exactlyEqual(replayFrequencyHz, rhs.replayFrequencyHz) &&
           digiChannel == rhs.digiChannel &&
           highlightSpacing == rhs.highlightSpacing &&
           secondaryHighlight == rhs.secondaryHighlight &&
           loopStartPosition == rhs.loopStartPosition &&
           endPosition == rhs.endPosition;
}

bool Subsong::Metadata::operator!=(const Metadata& rhs) const
{
    return !(rhs == *this);
}

const juce::String& Subsong::Metadata::getName() const
{
    return name;
}

int Subsong::Metadata::getInitialSpeed() const
{
    return initialSpeed;
}

float Subsong::Metadata::getReplayFrequencyHz() const
{
    return replayFrequencyHz;
}

int Subsong::Metadata::getDigiChannel() const
{
    return digiChannel;
}

int Subsong::Metadata::getHighlightSpacing() const
{
    return highlightSpacing;
}

int Subsong::Metadata::getSecondaryHighlight() const
{
    return secondaryHighlight;
}

int Subsong::Metadata::getLoopStartPosition() const
{
    return loopStartPosition;
}

int Subsong::Metadata::getEndPosition() const
{
    return endPosition;
}

SidPlayerCapability Subsong::Metadata::getSidPlayerCapability() const
{
    return sidPlayerCapability;
}


// Subsong.
// =======================================================================

Subsong::Subsong(juce::String pName, const int pInitialSpeed, const float pReplayFrequencyHz, const int pDigiChannel,
                 const int pHighlightSpacing, const int pSecondaryHighlight,
                 const int pLoopStartPosition, const int pEndPosition, std::vector<Psg> pPsgs,
                 const SidPlayerCapability& pSidPlayerCapability, const bool pCreateDefaultData) noexcept :
        id(),
        name(std::move(pName)),
        psgs(std::move(pPsgs)),
        sidPlayerCapability(pSidPlayerCapability),
        initialSpeed(pInitialSpeed),
        replayFrequencyHz(pReplayFrequencyHz),
        digiChannel(pDigiChannel),
        highlightSpacing(pHighlightSpacing),
        secondaryHighlight(pSecondaryHighlight),
        loopStartPosition(pLoopStartPosition),
        endPosition(pEndPosition),

        patterns(),
        positions(),
        tracks(),
        speedTracks(),
        eventTracks()
{
    jassert(!psgs.empty());         // Illegal!

    if (pCreateDefaultData) {
        // Creates as many Tracks as there are channels.
        const auto channelCount = PsgValues::getChannelCount(static_cast<int>(psgs.size()));
        std::vector<int> trackIndexes;
        trackIndexes.reserve(static_cast<size_t>(channelCount));

        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            tracks.emplace_back();
            trackIndexes.push_back(channelIndex);
        }
        speedTracks.emplace_back();
        eventTracks.emplace_back();

        patterns.emplace_back(trackIndexes, 0, 0, LookAndFeelConstants::defaultPatternColor);

        positions.emplace_back(0, TrackConstants::defaultPositionHeight, Position::firstMarkerName, LookAndFeelConstants::defaultMarkerColor,
                               std::unordered_map<int, int>());
    }
}

const juce::String& Subsong::getName() const noexcept
{
    return name;
}

void Subsong::setName(const juce::String& newName) noexcept
{
    name = newName;
}

int Subsong::getLength() const noexcept
{
    jassert(!positions.empty());

    return static_cast<int>(positions.size());
}

int Subsong::getPsgCount() const noexcept
{
    return static_cast<int>(psgs.size());
}

int Subsong::getChannelCount() const noexcept
{
    return PsgValues::getChannelCount(getPsgCount());
}

float Subsong::getReplayFrequencyHz() const noexcept
{
    return replayFrequencyHz;
}

void Subsong::setReplayFrequency(const float newReplayFrequencyHz) noexcept
{
    replayFrequencyHz = NumberUtil::correctNumber(newReplayFrequencyHz, PsgFrequency::minimumReplayFrequencyHz, PsgFrequency::maximumReplayFrequencyHz);
}

int Subsong::getDigiChannel() const noexcept
{
    return digiChannel;
}

void Subsong::setDigiChannel(const int newDigiChannel) noexcept
{
    jassert(newDigiChannel >= 0);
    digiChannel = newDigiChannel;
}

void Subsong::setHighlightSpacings(const int primary, const int secondary) noexcept
{
    highlightSpacing = primary;
    secondaryHighlight = secondary;
}

void Subsong::setInitialSpeed(const int newInitialSpeed) noexcept
{
    jassert(newInitialSpeed >= 0);
    initialSpeed = newInitialSpeed;
}

void Subsong::setPosition(const int positionIndex, const Position& positionData) noexcept
{
    positions.at(static_cast<size_t>(positionIndex)) = positionData;
}

Subsong::Metadata Subsong::getMetadata() const noexcept
{
    return { name,
            initialSpeed,
            replayFrequencyHz,
            digiChannel,
            highlightSpacing,
            secondaryHighlight,
            loopStartPosition,
            endPosition,
            sidPlayerCapability,
    };
}

Id Subsong::getId() const noexcept
{
    return id;
}

bool Subsong::operator==(const Subsong& rhs) const
{
    return (id == rhs.id);
}

bool Subsong::operator!=(const Subsong& rhs) const
{
    return !(rhs == *this);
}

bool Subsong::isMusicallyEqual(const Subsong& other) const
{
    return (psgs == other.psgs)
            && (sidPlayerCapability == other.sidPlayerCapability)
            && (initialSpeed == other.initialSpeed)
            && juce::exactlyEqual(replayFrequencyHz, other.replayFrequencyHz)
            && (digiChannel == other.digiChannel)
            && (highlightSpacing == other.highlightSpacing)
            && (secondaryHighlight == other.secondaryHighlight)
            && (loopStartPosition == other.loopStartPosition)
            && (endPosition == other.endPosition)
            && (patterns == other.patterns)
            && (positions == other.positions)
            && (tracks == other.tracks)
            && (speedTracks == other.speedTracks)
            && (eventTracks == other.eventTracks)
    ;
}

int Subsong::addPosition(const Position& position) noexcept
{
    const auto index = static_cast<int>(positions.size());
    addPositions(index, false, { position });
    return index;
}

void Subsong::addPosition(const int insertionIndex, const bool insertAfter, const Position& position) noexcept
{
    addPositions(insertionIndex, insertAfter, { position });
}

void Subsong::addPositions(const int insertionIndex, const bool insertAfter, const std::vector<Position>& positionsToInsert) noexcept
{
    jassert((insertionIndex >= 0) && (insertionIndex <= static_cast<int>(positions.size())));

    const auto indexToUse = insertionIndex + (insertAfter ? 1 : 0);
    positions.insert(positions.begin() + indexToUse, positionsToInsert.cbegin(), positionsToInsert.cend());
    // Uses the original index to shift the loop.
    shiftStartLoopAndEnd(insertionIndex, static_cast<int>(positionsToInsert.size()));
}

void Subsong::removePositions(const int index, const bool deleteAfter, const int positionCountToRemove) noexcept
{
    jassert((index >= 0) && ((index + positionCountToRemove) <= static_cast<int>(positions.size())));

    const auto indexToUse = index + (deleteAfter ? 1 : 0);
    positions.erase(positions.begin() + indexToUse, positions.begin() + indexToUse + positionCountToRemove);
    shiftStartLoopAndEnd(index, -positionCountToRemove);
}

void Subsong::setPositions(const std::vector<Position>& newPositions) noexcept
{
    positions = newPositions;
}

void Subsong::removeLastPatterns(const int count) noexcept
{
    jassert(patterns.size() >= static_cast<size_t>(count));

    // I love C++. See https://stackoverflow.com/questions/44037804/does-vectorerase-not-work-with-reverse-iterators
    patterns.erase((patterns.rbegin() + count).base(), patterns.rbegin().base());
}

void Subsong::removeLastTracks(const int count) noexcept
{
    jassert(tracks.size() >= static_cast<size_t>(count));

    tracks.erase((tracks.rbegin() + count).base(), tracks.rbegin().base());
}

void Subsong::removeLastSpecialTracks(const bool speedTrack, const int count) noexcept
{
    auto& specialTracks = speedTrack ? speedTracks : eventTracks;
    specialTracks.erase((specialTracks.rbegin() + count).base(), specialTracks.rbegin().base());
}

void Subsong::removePositions(const std::set<int>& positionsToRemove) noexcept
{
    // Makes sure to delete from the END to the BEGINNING!
    for (auto iterator = positionsToRemove.rbegin(); iterator != positionsToRemove.rend(); ++iterator) {
        const auto index = *iterator;
        shiftStartLoopAndEnd(index, -1);

        positions.erase(positions.begin() + index);
    }
}

Position Subsong::getPosition(const int positionIndex) const noexcept
{
    jassert(positionIndex < static_cast<int>(positions.size()));
    return positions.at(static_cast<size_t>(positionIndex));
}

std::vector<Position> Subsong::getPositions(const bool untilLoopEnd) const noexcept
{
    if (!untilLoopEnd) {
        return positions;
    }

    return { positions.cbegin(), positions.cbegin() + endPosition + 1 };
}

std::vector<Pattern> Subsong::getPatterns() const noexcept
{
    return patterns;
}

std::vector<Track> Subsong::getTracks() const noexcept
{
    return tracks;
}

std::vector<SpecialTrack> Subsong::getSpecialTracks(const bool isSpeedTrack) const noexcept
{
    return isSpeedTrack ? speedTracks : eventTracks;
}

const Position& Subsong::getPositionRef(const int position) const noexcept
{
    jassert(position < static_cast<int>(positions.size()));
    return positions.at(static_cast<size_t>(position));
}

bool Subsong::doesPositionExist(const int positionIndex) const noexcept
{
    return (positionIndex < static_cast<int>(positions.size()));
}

int Subsong::getPositionHeight(const int positionIndex) const noexcept
{
    if (static_cast<size_t>(positionIndex) >= positions.size()) {
        jassertfalse;           // Position doesn't exist!
        return TrackConstants::defaultPositionHeight;
    }

    const auto& position = positions.at(static_cast<size_t>(positionIndex));
    return position.getHeight();
}

void Subsong::setPositionMarkerData(const int positionIndex, const juce::String& text, const juce::uint32 color) noexcept
{
    auto& position = positions.at(static_cast<size_t>(positionIndex));
    position.setMarkerNameAndColour(text, color);
}

std::pair<juce::String, juce::uint32> Subsong::getPositionMarker(const int positionIndex) const noexcept
{
    const auto& position = positions.at(static_cast<size_t>(positionIndex));
    return { position.getMarkerName(), position.getMarkerColor() };
}

void Subsong::setLoopAndEndStartPosition(const OptionalInt newLoopStartPosition, const OptionalInt newEndPosition) noexcept
{
    // Checks the new loop.
    const auto newStartAndEnd = CheckLoopStartEnd::checkLoopStartEnd(loopStartPosition, endPosition, getLength(), newLoopStartPosition, newEndPosition);
    loopStartPosition = newStartAndEnd.first;
    endPosition = newStartAndEnd.second;
}

void Subsong::setLoop(const Loop& loop) noexcept
{
    setLoopAndEndStartPosition(loop.getStartIndex(), loop.getEndIndex());
}

int Subsong::getLoopStartPosition() const noexcept
{
    return loopStartPosition;
}

int Subsong::getEndPosition() const noexcept
{
    return endPosition;
}

Loop Subsong::getLoop() const noexcept
{
    return Loop(loopStartPosition, endPosition, true);
}

int Subsong::getTrackCount() const noexcept
{
    return static_cast<int>(tracks.size());
}

std::vector<Psg> Subsong::getPsgs() const noexcept
{
    return psgs;
}

const std::vector<Psg>& Subsong::getPsgRefs() const noexcept
{
    return psgs;
}

void Subsong::setPsgs(std::vector<Psg> newPsgs) noexcept
{
    jassert(!newPsgs.empty());      // Illegal!
    psgs = std::move(newPsgs);
}

SidPlayerCapability Subsong::getSidPlayerCapability() const noexcept
{
    return sidPlayerCapability;
}

void Subsong::setSidPlayerCapability(const SidPlayerCapability& newSidPlayerCapability) noexcept
{
    sidPlayerCapability = newSidPlayerCapability;
}

void Subsong::shiftStartLoopAndEnd(const int index, const int shiftCount) noexcept
{
    if (shiftCount == 0) {
        jassertfalse;
        return;         // Sanity check.
    }

    if (shiftCount > 0) {
        if (index < loopStartPosition) {
            loopStartPosition += shiftCount;
        }
        if (index <= endPosition) {
            endPosition += shiftCount;
        }
    } else {
        if (index < loopStartPosition) {
            const auto newLoopStartPosition = loopStartPosition + shiftCount;
            loopStartPosition = std::max(index, newLoopStartPosition);      // Cannot go less than the index.
        }
        if (index < endPosition) {
            const auto newLoopEndPosition = endPosition + shiftCount;
            endPosition = std::max(index, newLoopEndPosition);      // Cannot go less than the index.
        }
    }
}

const Pattern& Subsong::getPatternRef(const int position) const noexcept
{
    jassert(position < static_cast<int>(positions.size()));
    const auto patternIndex = positions.at(static_cast<size_t>(position)).getPatternIndex();

    jassert(patternIndex < static_cast<int>(patterns.size()));
    return patterns.at(static_cast<size_t>(patternIndex));
}

Pattern Subsong::getPatternFromIndex(const int patternIndex) const noexcept
{
    jassert(patternIndex < static_cast<int>(patterns.size()));
    return patterns.at(static_cast<size_t>(patternIndex));
}

juce::uint32 Subsong::getPatternColor(const int patternIndex) const noexcept
{
    jassert(patternIndex < static_cast<int>(patterns.size()));

    return patterns.at(static_cast<size_t>(patternIndex)).getArgbColor();
}

bool Subsong::doesPatternExist(const int patternIndex) const noexcept
{
    return (patternIndex < static_cast<int>(patterns.size()));
}

int Subsong::getPatternCount() const noexcept
{
    return static_cast<int>(patterns.size());
}

int Subsong::addPattern(const Pattern& pattern) noexcept
{
    const auto index = getPatternCount();
    patterns.push_back(pattern);
    return index;
}

void Subsong::setPattern(const int patternIndex, const Pattern& pattern) noexcept
{
    jassert(patternIndex < static_cast<int>(patterns.size()));
    patterns.at(static_cast<size_t>(patternIndex)) = pattern;
}

void Subsong::setPatterns(const std::vector<Pattern>& newPatterns) noexcept
{
    jassert(!newPatterns.empty());
    patterns = newPatterns;
}


// ====================================================

const Track& Subsong::getConstTrackRefFromPosition(const int positionIndex, const int channelIndex) const noexcept
{
    const auto& pattern = getPatternRef(positionIndex);
    const auto trackIndex = pattern.getCurrentTrackIndex(channelIndex);

    return getTrackRefFromIndex(trackIndex);
}

Track& Subsong::getTrackRefFromPosition(const int positionIndex, const int channelIndex) noexcept
{
    const auto& pattern = getPatternRef(positionIndex);
    const auto trackIndex = pattern.getCurrentTrackIndex(channelIndex);

    return getTrackRefFromIndex(trackIndex);
}

const Track& Subsong::getTrackRefFromIndex(const int trackIndex) const noexcept
{
    jassert(trackIndex < static_cast<int>(tracks.size()));
    return tracks.at(static_cast<size_t>(trackIndex));
}

const SpecialTrack& Subsong::getSpecialTrackRefFromPosition(const int positionIndex, const bool speedTrack) const noexcept
{
    const auto& pattern = getPatternRef(positionIndex);
    const auto specialTrackIndex = pattern.getCurrentSpecialTrackIndex(speedTrack);

    return getSpecialTrackRefFromIndex(specialTrackIndex, speedTrack);
}
SpecialTrack& Subsong::getSpecialTrackRefFromPosition(const int positionIndex, const bool speedTrack) noexcept
{
    // To avoid duplicating the same code as above, for a non-const method. See Effective C++ by Scott Meyer, item 3.
    return const_cast<SpecialTrack&>(static_cast<const Subsong&>(*this).getSpecialTrackRefFromPosition(positionIndex, speedTrack)); // NOLINT(*-pro-type-const-cast)
}

const SpecialTrack& Subsong::getSpecialTrackRefFromIndex(const int specialTrackIndex, const bool speedTrack) const noexcept
{
    const auto& specialTracks = speedTrack ? speedTracks : eventTracks;

    jassert(specialTrackIndex < static_cast<int>(specialTracks.size()));
    return specialTracks.at(static_cast<size_t>(specialTrackIndex));
}

SpecialTrack& Subsong::getSpecialTrackRefFromIndex(const int specialTrackIndex, const bool speedTrack) noexcept
{
    // To avoid duplicating the same code as above, for a non-const method. See Effective C++ by Scott Meyer, item 3.
    return const_cast<SpecialTrack&>(static_cast<const Subsong&>(*this).getSpecialTrackRefFromIndex(specialTrackIndex, speedTrack)); // NOLINT(*-pro-type-const-cast)
}

int Subsong::getSpecialTrackCount(const bool speedTrack) const noexcept
{
    const auto& specialTracks = speedTrack ? speedTracks : eventTracks;
    return static_cast<int>(specialTracks.size());
}

std::pair<int, int> Subsong::getLoopStartAndEndPosition() const noexcept
{
    return { loopStartPosition, endPosition };
}


// Tracks.
// ==========================================================

Track& Subsong::getTrackRefFromIndex(const int trackIndex) noexcept
{
    return tracks.at(static_cast<size_t>(trackIndex));
}

int Subsong::addTrack(const Track& track) noexcept
{
    const auto trackIndex = static_cast<int>(tracks.size());

    tracks.push_back(track);

    return trackIndex;
}

Cell Subsong::insertCellAt(const int trackIndex, const int cellIndex, const Cell& cellToInsert) noexcept
{
    auto& track = getTrackRefFromIndex(trackIndex);
    return track.insertCellAt(cellIndex, cellToInsert);
}

Cell Subsong::removeCellAt(const int trackIndex, const int cellIndex, const Cell& lastCell) noexcept
{
    auto& track = getTrackRefFromIndex(trackIndex);
    return track.removeCellAt(cellIndex, lastCell);
}

void Subsong::setTrackName(const int positionIndex, const int channelIndex, const juce::String& newName) noexcept
{
    auto& track = getTrackRefFromPosition(positionIndex, channelIndex);
    track.setName(newName);
}

void Subsong::setTrackColor(const int positionIndex, const int channelIndex, const OptionalValue<juce::uint32>& newColor) noexcept
{
    auto& track = getTrackRefFromPosition(positionIndex, channelIndex);
    track.setColor(newColor);
}

LinkState Subsong::getTrackLinkState(const int positionIndex, const int channelIndex) const noexcept
{
    const auto& pattern = getPatternRef(positionIndex);
    const auto trackLinkedAndUnlinkedIndexes = pattern.getTrackIndexAndLinkedTrackIndex(channelIndex);
    if (trackLinkedAndUnlinkedIndexes.isLinked()) {
        return LinkState::link;
    }

    // Is this Track referred to via a Link? Not very efficient, but browses all the Positions first.
    const auto trackIndex = trackLinkedAndUnlinkedIndexes.getUnlinkedTrackIndex();
    const auto trackLocations = findTrackUse(trackIndex, LinkType::linkedOnly);
    // Browses the result, excluding our own position and channel. As soon as one is found, stops.
    for (const auto& trackLocation : trackLocations) {
        if ((positionIndex != trackLocation.getPositionIndex()) || (channelIndex != trackLocation.getChannelIndex())) {
            return LinkState::linkedTo;
        }
    }

    return LinkState::none;
}

std::vector<TrackLocation> Subsong::findTrackUse(const int trackIndex, const LinkType linkType) const noexcept
{
    std::vector<TrackLocation> locations;

    const auto positionCount = getLength();
    for (auto positionIndex = 0; positionIndex < positionCount; ++positionIndex) {
        const auto& pattern = getPatternRef(positionIndex);
        const auto channelIndexes = pattern.getWhereTrackUsed(trackIndex, linkType);

        for (const auto& channelIndex : channelIndexes) {
            locations.emplace_back(positionIndex, channelIndex);
        }
    }

    return locations;
}


// Special Tracks.
// ==========================================================

int Subsong::addSpecialTrack(const bool isSpeedTrack, const SpecialTrack& specialTrack) noexcept
{
    // What list?
    auto& specialTracks = isSpeedTrack ? speedTracks : eventTracks;
    const auto specialTrackIndex = static_cast<int>(specialTracks.size());

    specialTracks.push_back(specialTrack);
    return specialTrackIndex;
}

void Subsong::setCellFromSpecialTrackIndex(const bool isSpeedTrack, const int specialTrackIndex, const int lineIndex, const SpecialCell& cell) noexcept
{
    jassert((lineIndex >= 0) && (lineIndex < TrackConstants::maximumCapacity));
    auto& specialTracks = isSpeedTrack ? speedTracks : eventTracks;
    jassert(specialTrackIndex < static_cast<int>(specialTracks.size()));

    auto& specialTrack = specialTracks.at(static_cast<size_t>(specialTrackIndex));
    specialTrack.setCell(lineIndex, cell);
}

SpecialCell Subsong::getSpecialCell(const bool speedTrack, const int position, const int lineIndex) const noexcept
{
    jassert(position >= 0);
    jassert((lineIndex >= 0) && (lineIndex < TrackConstants::maximumCapacity));

    // Does the position exist?
    if (!doesPositionExist(position)) {
        static const auto emptySpecialCell = SpecialCell::buildEmptySpecialCell();
        return emptySpecialCell;
    }

    const auto& specialTrack = getSpecialTrackRefFromPosition(position, speedTrack);
    return specialTrack.getCell(lineIndex);
}

SpecialCell Subsong::insertSpecialCellAt(const bool speedTrack, const int specialTrackIndex, const int cellIndex, const SpecialCell& specialCellToInsert) noexcept
{
    auto& specialTrack = getSpecialTrackRefFromIndex(specialTrackIndex, speedTrack);
    return specialTrack.insertCellAt(cellIndex, specialCellToInsert);
}

SpecialCell Subsong::removeSpecialCellAt(const bool speedTrack, const int specialTrackIndex, const int cellIndex, const SpecialCell& lastSpecialCell) noexcept
{
    auto& specialTrack = getSpecialTrackRefFromIndex(specialTrackIndex, speedTrack);
    return specialTrack.removeCellAt(cellIndex, lastSpecialCell);
}

void Subsong::setSpecialTrackName(const int positionIndex, const bool speedTrack, const juce::String& newName) noexcept
{
    auto& specialTrack = getSpecialTrackRefFromPosition(positionIndex, speedTrack);
    specialTrack.setName(newName);
}

LinkState Subsong::getSpecialTrackLinkState(const bool speedTrack, const int positionIndex) const noexcept
{
    const auto& pattern = getPatternRef(positionIndex);
    const auto specialTrackLinkedAndUnlinkedIndexes = pattern.getSpecialTrackIndexAndLinkedTrackIndex(speedTrack);
    if (specialTrackLinkedAndUnlinkedIndexes.isLinked()) {
        return LinkState::link;
    }

    // Is this SpecialTrack referred to via a Link? Not very efficient, but browses all the Positions first.
    const auto specialTrackIndex = specialTrackLinkedAndUnlinkedIndexes.getUnlinkedTrackIndex();
    const auto specialTrackLocations = findSpecialTrackUse(speedTrack, specialTrackIndex, LinkType::linkedOnly);
    // Browses the result, excluding our own position and channel. As soon as one is found, stops.
    for (const auto& specialTrackLocation : specialTrackLocations) {
        if ((positionIndex != specialTrackLocation.getPositionIndex()) || (speedTrack != specialTrackLocation.isSpeedTrack())) {
            return LinkState::linkedTo;
        }
    }

    return LinkState::none;
}

std::vector<SpecialTrackLocation> Subsong::findSpecialTrackUse(const bool speedTrack, const int specialTrackIndex, const LinkType linkType) const noexcept
{
    std::vector<SpecialTrackLocation> locations;

    const auto positionCount = getLength();
    for (auto positionIndex = 0; positionIndex < positionCount; ++positionIndex) {
        const auto& pattern = getPatternRef(positionIndex);
        if (pattern.isSpecialTrackUsed(speedTrack, specialTrackIndex, linkType)) {
            locations.emplace_back(speedTrack, positionIndex);
        }
    }

    return locations;
}


// Cells.
// ==========================================================

Cell Subsong::getCell(const int position, const int line, const int channelIndex) const noexcept
{
    jassert(position >= 0);
    jassert((line >= 0) && (line < TrackConstants::maximumCapacity));
    jassert(channelIndex >= 0);

    // Valid channel index?
    if (channelIndex >= getChannelCount()) {
        return { };
    }

    // Does the position exist?
    if (!doesPositionExist(position)) {
        return { };
    }

    const auto& track = getConstTrackRefFromPosition(position, channelIndex);
    return track.getCell(line);
}

void Subsong::setCell(const int position, const int line, const int channelIndex, const Cell& cell) noexcept
{
    auto& track = getTrackRefFromPosition(position, channelIndex);
    track.setCell(line, cell);
}

void Subsong::setCellFromTrackIndex(const int trackIndex, const int line, const Cell& cell) noexcept
{
    jassert((line >= 0) && (line < TrackConstants::maximumCapacity));
    jassert(trackIndex < static_cast<int>(tracks.size()));

    auto& track = tracks.at(static_cast<size_t>(trackIndex));
    track.setCell(line, cell);
}


// Special Cells.
// ==========================================================

void Subsong::setSpecialCell(const bool isSpeedTrack, const int position, const int line, const SpecialCell& specialCell) noexcept
{
    auto& specialTrack = getSpecialTrackRefFromPosition(position, isSpeedTrack);
    specialTrack.setCell(line, specialCell);
}

void Subsong::setSpecialCellFromTrackIndex(const bool isSpeedTrack, const int specialTrackIndex, const int lineIndex, const SpecialCell& specialCell) noexcept
{
    auto& specialTrack = getSpecialTrackRefFromIndex(specialTrackIndex, isSpeedTrack);
    specialTrack.setCell(lineIndex, specialCell);
}

}   // namespace arkostracker
