#include "SubsongBuilder.h"

#include "../../../../utils/ColorUtil.h"
#include "../../../../utils/MapUtil.h"
#include "../../../../utils/PsgValues.h"
#include "PatternUnoptimizer.h"

namespace arkostracker 
{

SubsongBuilder::SubsongBuilder(const bool pIsNative) noexcept :
        isNative(pIsNative),
        errorReport(),
        title(),
        initialSpeed(6),
        replayFrequencyHz(PsgFrequency::defaultReplayFrequencyHz),
        digiChannel(1),
        highlightSpacing(4),
        secondaryHighlight(4),

        loopStartIndex(0),
        endIndex(0),

        psgs(),
        sidPlayerCapability(SidPlayerCapability::buildDefault()),

        currentTrackIndex(-1),
        currentTrack(),
        indexToTrack(),

        currentSpeedTrackIndex(-1),
        currentSpeedTrack(),
        indexToSpeedTrack(),

        currentEventTrackIndex(-1),
        currentEventTrack(),
        indexToEventTrack(),

        positions(),
        currentPositionHeight(),
        currentPositionPatternIndex(0),
        currentPositionMarkerName(),
        currentPositionMarkerColor(),
        currentPositionChannelToTransposition(),

        currentPatternIndex(0),
        currentPatternChannelIndexToTrack(),
        currentPatternSpeedIndex(0),
        currentPatternEventIndex(0),
        currentPatternColor(LookAndFeelConstants::defaultPatternColor),
        indexToPattern(),
        browsedRawOriginalPatternToIndex(),

        currentCellIndex(0),
        currentCell()
{
}

void SubsongBuilder::setMetadata(juce::String newTitle, const int newInitialSpeed, const float newReplayFrequencyHz, const int newDigiChannel,
                                 const int newHighlightSpacing, const int newSecondaryHighlight) noexcept
{
    title = std::move(newTitle);
    initialSpeed = newInitialSpeed;
    replayFrequencyHz = newReplayFrequencyHz;
    digiChannel = newDigiChannel;
    highlightSpacing = newHighlightSpacing;
    secondaryHighlight = newSecondaryHighlight;
}

void SubsongBuilder::setLoop(const int newStartIndex, const int newEndIndex) noexcept
{
    loopStartIndex = newStartIndex;
    endIndex = newEndIndex;
}

void SubsongBuilder::addPsg(const Psg& psg) noexcept
{
    psgs.push_back(psg);
}

void SubsongBuilder::setSidPlayerCapability(const SidPlayerCapability& newSidPlayerCapability) noexcept
{
    sidPlayerCapability = newSidPlayerCapability;
}

std::unique_ptr<Subsong> SubsongBuilder::buildSubsong() noexcept
{
    // There must be at least one PSG, but as a convenience, one is created if not present.
    if (psgs.empty()) {
        addError("A subsong must have at least one PSG!");
        return nullptr;
    }

    auto subsong = std::make_unique<Subsong>(title, initialSpeed, replayFrequencyHz, digiChannel, highlightSpacing, secondaryHighlight,
                                             loopStartIndex, endIndex, psgs, sidPlayerCapability);

    // Checks for holes in the Patterns. This MUST be done before the "unoptimization" is performed, as Tracks will be duplicated, so we need to
    // know all the Patterns.
    checkAndRepairHolesInPatterns();
    // Some formats (such as STarKos) may declare Tracks in the Patterns but do not encode them (as an optimization). They must be created.
    createReferencedInPatternsButNotPresentTracks();

    // BEFORE encoding the tracks, parses the patterns to remove ALL duplication (i.e. one track is only used ONCE).
    // However, this is NOT done for AT3 format, else linked Tracks will be removed.
    if (!isNative) {
        PatternUnoptimizer::unoptimize(indexToPattern, indexToTrack, indexToSpeedTrack, indexToEventTrack);
    }

    // Writes the Tracks.
    auto success = encodeTracks(*subsong);
    // Writes the speed/event tracks.
    success = success && encodeSpecialTracks(true, *subsong);
    success = success && encodeSpecialTracks(false, *subsong);
    success = success && encodePatterns(*subsong);
    success = success && encodePositions(*subsong);

    return success ? std::move(subsong) : nullptr;
}

const ErrorReport& SubsongBuilder::getErrorReport() const noexcept
{
    return errorReport;
}

void SubsongBuilder::addWarning(const juce::String& text, const OptionalInt lineNumber) noexcept
{
    errorReport.addWarning(text, lineNumber);
}

void SubsongBuilder::addError(const juce::String& text, const OptionalInt lineNumber) noexcept
{
    errorReport.addError(text, lineNumber);
}

bool SubsongBuilder::isOk() const noexcept
{
    return errorReport.isOk();
}

bool SubsongBuilder::areErrors() const noexcept
{
    return !isOk();
}


// Tracks.
// ==============================================

void SubsongBuilder::startNewTrack(const int trackIndex) noexcept
{
    // The track must not exist yet. Should never happen.
    if (indexToTrack.find(trackIndex) != indexToTrack.cend()) {
        addError("The track " + juce::String(trackIndex) + " already exist!");
        jassertfalse;
        return;
    }

    // Initializes the Track.
    currentTrackIndex = trackIndex;
    currentTrack = Track();
}

void SubsongBuilder::finishCurrentTrack() noexcept
{
    jassert(indexToTrack.find(currentTrackIndex) == indexToTrack.cend());       // Track already stored??

    // Stores the Track.
    indexToTrack.insert(std::make_pair(currentTrackIndex, currentTrack));
    currentTrackIndex = -1;
}

void SubsongBuilder::setCellOnTrack(const int trackIndex, const int cellIndex, const Cell& cell) noexcept
{
    jassert(trackIndex != currentTrackIndex);           // Must NOT be the same as the currently edited Track!!

    if (const auto iterator = indexToTrack.find(trackIndex); iterator != indexToTrack.cend()) {
        // The track exists.
        auto& track = iterator->second;
        track.setCell(cellIndex, cell);
    } else {
        // The track does not exist.
        Track track;
        track.setCell(cellIndex, cell);
        indexToTrack.insert(std::make_pair(trackIndex, track));
    }
}

void SubsongBuilder::setTrackMetadata(const int trackIndex, const bool readOnly, const juce::String& name, const juce::uint32 argbColor) noexcept
{
    jassert(trackIndex != currentTrackIndex);           // Must NOT be the same as the currently edited Track!!

    if (const auto iterator = indexToTrack.find(trackIndex); iterator != indexToTrack.cend()) {
        auto& track = iterator->second;
        fillTrackMetadata(track, readOnly, name, argbColor);
    } else {
        // The track does not exist.
        Track track;
        fillTrackMetadata(track, readOnly, name, argbColor);
        indexToTrack.insert(std::make_pair(trackIndex, track));
    }
}

void SubsongBuilder::fillTrackMetadata(Track& track, const bool readOnly, const juce::String& name, const juce::uint32 argbColor) noexcept
{
    track.setName(name);
    track.setReadOnly(readOnly);
    if (argbColor != 0) {
        const auto color = ColorUtil::setAlphaToFull(argbColor) ; // Makes sure alpha is full.
        track.setColor(color);
    }
}

bool SubsongBuilder::encodeTracks(Subsong& subsong) noexcept
{
    // There MUST be at least one Tracks.
    if (indexToTrack.empty()) {
        addError("There are no tracks, abnormal.");
        return false;
    }

    // Fills the holes with empty Tracks.
    MapUtil::fillHolesInMap<Track>(0, indexToTrack, [] {
        return Track();
    });

    for (const auto& [trackIndex, track] : indexToTrack) {
        const auto returnedTrackIndex = subsong.addTrack(track);
        jassert(returnedTrackIndex == trackIndex);            // The map is ordered! Critical error!
        (void)returnedTrackIndex;
    }

    return isOk();
}

bool SubsongBuilder::encodeSpecialTracks(const bool isSpeedTrack, Subsong& subsong) noexcept
{
    auto& indexToSpecialTrack = isSpeedTrack ? indexToSpeedTrack :  indexToEventTrack;

    // There MUST be at least one Special Track.
    if (indexToSpecialTrack.empty()) {
        addError("There are no special tracks, abnormal.");
        return false;
    }

    // Fills the holes with empty Special Tracks.
    MapUtil::fillHolesInMap<SpecialTrack>(0, indexToSpecialTrack, [] {
        return SpecialTrack();
    });

    for (const auto& [specialTrackIndex, specialTrack] : indexToSpecialTrack) {
        const auto returnedTrackIndex = subsong.addSpecialTrack(isSpeedTrack, specialTrack);
        jassert(returnedTrackIndex == specialTrackIndex);            // The map is ordered! Critical error!
        (void)returnedTrackIndex;
    }

    return isOk();
}

bool SubsongBuilder::encodePatterns(Subsong& subsong) noexcept
{
    // There MUST be patterns.
    if (indexToPattern.empty()) {
        addError("No Patterns! Abnormal.");
        jassertfalse;
        return false;
    }

    // There must NOT be any holes in the patterns!
    jassert(MapUtil::findHolesInKeys(indexToPattern, 0).empty());

    // Now encodes the Patterns.
    for (const auto& [patternIndex, pattern] : indexToPattern) {

        // The Tracks must exist.
        const auto channelCount = pattern.getChannelCount();
        for (auto channelIndex = 0; channelIndex < channelCount; ++channelIndex) {
            if (!checkTrackExist(pattern.getTrackIndexAndLinkedTrackIndex(channelIndex), patternIndex, juce::translate("Track"))) {
                return false;
            }
        }
        // The Special Tracks must exist too.
        if (!checkTrackExist(pattern.getSpecialTrackIndexAndLinkedTrackIndex(true), patternIndex, juce::translate("Speed Track"))) {
            return false;
        }
        if (!checkTrackExist(pattern.getSpecialTrackIndexAndLinkedTrackIndex(false), patternIndex, juce::translate("Event Track"))) {
            return false;
        }

        const auto returnedPatternIndex = subsong.addPattern(pattern);
        jassert(returnedPatternIndex == patternIndex);            // The map is ordered! Critical error!
        (void)returnedPatternIndex;
    }

    return isOk();
}

bool SubsongBuilder::checkTrackExist(const Pattern::TrackIndexAndLinkedTrackIndex& trackIndexes, const int patternIndex, const juce::String& typeName) noexcept
{
    if (indexToTrack.find(trackIndexes.getUnlinkedTrackIndex()) == indexToTrack.cend()) {
        addError("The Pattern " + juce::String(patternIndex) + " declared a " + typeName + " " + juce::String(trackIndexes.getUnlinkedTrackIndex())
            + ", but it doesn't exist!");
        jassertfalse;
        return false;
    }
    const auto linkedTrackIndex = trackIndexes.getLinkedTrackIndex();
    if (linkedTrackIndex.isPresent() && (indexToTrack.find(trackIndexes.getLinkedTrackIndex().getValue()) == indexToTrack.cend())) {
        addError("The Pattern " + juce::String(patternIndex) + " declared a Linked " + typeName + " " + juce::String(linkedTrackIndex.getValue())
            + ", but it doesn't exist!");
        jassertfalse;
        return false;
    }

    return true;
}

void SubsongBuilder::checkAndRepairHolesInPatterns() noexcept
{
    // If there is any hole in the Patterns, duplicates any another one.
    const auto holeIndexes = MapUtil::findHolesInKeys(indexToPattern, 0);
    if (!holeIndexes.empty()) {
        // Gets any Pattern. We have checked before there were Patterns.
        const auto& patternToDuplicate = indexToPattern.cbegin()->second;       // Selects the first one.
        for (const auto holeIndex : holeIndexes) {
            indexToPattern.insert(std::make_pair(holeIndex, patternToDuplicate));
        }
    }
}

void SubsongBuilder::createReferencedInPatternsButNotPresentTracks() noexcept
{
    for (const auto& [_, pattern] : indexToPattern) {

        for (auto channelIndex = 0, channelCount = pattern.getChannelCount(); channelIndex < channelCount; ++channelIndex) {
            const auto trackIndexes = pattern.getTrackIndexAndLinkedTrackIndex(channelIndex);
            // Checks/creates the unlink and possible link track index.
            if (indexToTrack.find(trackIndexes.getUnlinkedTrackIndex()) == indexToTrack.cend()) {
                indexToTrack.insert(std::make_pair(trackIndexes.getUnlinkedTrackIndex(), Track()));
            }
            if (const auto linkedTrackIndex = trackIndexes.getLinkedTrackIndex(); linkedTrackIndex.isPresent()) {
                if (indexToTrack.find(linkedTrackIndex.getValue()) == indexToTrack.cend()) {
                    indexToTrack.insert(std::make_pair(linkedTrackIndex.getValue(), Track()));
                }
            }
        }

        // The same with Special Tracks.
        createSpecialTrackIfNotPresentInPattern(pattern, true);
        createSpecialTrackIfNotPresentInPattern(pattern, false);
    }
}

void SubsongBuilder::createSpecialTrackIfNotPresentInPattern(const Pattern& pattern, const bool isSpeedTrack) noexcept
{
    auto& indexToSpecialTrack = isSpeedTrack ? indexToSpeedTrack : indexToEventTrack;
    const auto& specialTrackIndexes = pattern.getSpecialTrackIndexAndLinkedTrackIndex(isSpeedTrack);

    // Checks/creates the unlink and possible link Special Track index.
    if (indexToSpecialTrack.find(specialTrackIndexes.getUnlinkedTrackIndex()) == indexToSpecialTrack.cend()) {
        indexToSpecialTrack.insert(std::make_pair(specialTrackIndexes.getUnlinkedTrackIndex(), SpecialTrack()));
    }
    if (const auto linkedTrackIndex = specialTrackIndexes.getLinkedTrackIndex(); linkedTrackIndex.isPresent()) {
        if (indexToSpecialTrack.find(linkedTrackIndex.getValue()) == indexToSpecialTrack.cend()) {
            indexToSpecialTrack.insert(std::make_pair(linkedTrackIndex.getValue(), SpecialTrack()));
        }
    }
}

bool SubsongBuilder::encodePositions(Subsong& subsong) noexcept
{
    auto positionIndex = 0;
    for (const auto& position : positions) {
        // The pattern must exist.
        const auto patternIndex = position.getPatternIndex();
        if (indexToPattern.find(patternIndex) == indexToPattern.cend()) {
            addError("The Position " + juce::String(positionIndex) + " declared a Pattern " + juce::String(patternIndex) + ", but it doesn't exist!");
            jassertfalse;
            return false;
        }

        // Adds the position, but if the format is not AT3, adds a Start marker on the first position.
        const auto returnedPositionIndex = (!isNative && positionIndex == 0)
                ? subsong.addPosition(position.withMarkerName(Position::firstMarkerName))
                : subsong.addPosition(position);
        jassert(returnedPositionIndex == positionIndex);
        (void)returnedPositionIndex;

        ++positionIndex;
    }

    // Encodes the loop.
    subsong.setLoopAndEndStartPosition(loopStartIndex, endIndex);

    return isOk();
}

// Special Tracks.
// ==============================================
void SubsongBuilder::startNewSpecialTrack(const bool isSpeedTrack, const int specialTrackIndex) noexcept
{
    if (isSpeedTrack) {
        startNewSpeedTrack(specialTrackIndex);
    } else {
        startNewEventTrack(specialTrackIndex);
    }
}

void SubsongBuilder::setCurrentSpecialTrackValue(const bool isSpeedTrack, const int cellIndex, const int value) noexcept
{
    if (isSpeedTrack) {
        setCurrentSpeedTrackValue(cellIndex, value);
    } else {
        setCurrentEventTrackValue(cellIndex, value);
    }
}

void SubsongBuilder::finishCurrentSpecialTrack(const bool isSpeedTrack) noexcept
{
    if (isSpeedTrack) {
        finishCurrentSpeedTrack();
    } else {
        finishCurrentEventTrack();
    }
}

void SubsongBuilder::setSpecialCellOnTrack(const bool isSpeedTrack, int specialTrackIndex, const int cellIndex, const SpecialCell& specialCell) noexcept
{
    auto& indexToSpecialTrack = isSpeedTrack ? indexToSpeedTrack : indexToEventTrack;

    if (const auto iterator = indexToSpecialTrack.find(specialTrackIndex); iterator != indexToSpecialTrack.cend()) {
        // The Special Track exists.
        auto& specialTrack = iterator->second;
        specialTrack.setCell(cellIndex, specialCell);
    } else {
        // The Special Track does not exist.
        SpecialTrack specialTrack;
        specialTrack.setCell(cellIndex, specialCell);
        indexToSpecialTrack.insert(std::make_pair(specialTrackIndex, specialTrack));
    }
}

void SubsongBuilder::setSpecialTrackMetadata(const bool isSpeedTrack, const int specialTrackIndex, const bool readOnly,
    const juce::String& name, const juce::uint32 argbColor) noexcept
{
    auto& indexToSpecialTrack = isSpeedTrack ? indexToSpeedTrack : indexToEventTrack;

    if (const auto iterator = indexToSpecialTrack.find(specialTrackIndex); iterator != indexToSpecialTrack.cend()) {
        auto& specialTrack = iterator->second;
        specialTrack.setReadOnly(readOnly);
        specialTrack.setName(name);
        if (argbColor != 0) {
            const auto color = ColorUtil::setAlphaToFull(argbColor) ; // Makes sure alpha is full.
            specialTrack.setColor(color);
        }
    } else {
        // The Special Track does not exist.
        SpecialTrack specialTrack;
        specialTrack.setReadOnly(readOnly);
        specialTrack.setName(name);
        indexToSpecialTrack.insert(std::make_pair(specialTrackIndex, specialTrack));
    }
}


// Speed Tracks.
// ==============================================

void SubsongBuilder::startNewSpeedTrack(const int speedTrackIndex) noexcept
{
    jassert(currentSpeedTrackIndex < 0);            // Track already started?

    // The track must not exist yet. Should never happen.
    if (indexToSpeedTrack.find(speedTrackIndex) != indexToSpeedTrack.cend()) {
        addError("The Speed track " + juce::String(speedTrackIndex) + " already exist!");
        jassertfalse;
        return;
    }

    // Initializes the track.
    currentSpeedTrackIndex = speedTrackIndex;
    currentSpeedTrack = SpecialTrack();
}

void SubsongBuilder::setCurrentSpeedTrackValue(const int cellIndex, const int speed) noexcept
{
    jassert(currentSpeedTrackIndex >= 0);            // No Track started?
    currentSpeedTrack.setCell(cellIndex, SpecialCell::buildSpecialCell(speed));
}

void SubsongBuilder::finishCurrentSpeedTrack() noexcept
{
    jassert(currentSpeedTrackIndex >= 0);            // No Track started?
    jassert(indexToSpeedTrack.find(currentSpeedTrackIndex) == indexToSpeedTrack.cend());       // Track already stored??

    // Stores the SpeedTrack.
    indexToSpeedTrack.insert(std::make_pair(currentSpeedTrackIndex, currentSpeedTrack));
    currentSpeedTrackIndex = -1;
}

void SubsongBuilder::setSpeedToTrack(int speedTrackIndex, const int cellIndex, const int speed) noexcept
{
    jassert(speedTrackIndex != currentSpeedTrackIndex);           // Must NOT be the same as the currently edited Track!!

    if (const auto iterator = indexToSpeedTrack.find(speedTrackIndex); iterator != indexToSpeedTrack.cend()) {
        // The track exists.
        auto& speedTrack = iterator->second;
        speedTrack.setCell(cellIndex, SpecialCell::buildSpecialCell(speed));
    } else {
        // The track does not exist.
        SpecialTrack specialTrack;
        specialTrack.setCell(cellIndex, SpecialCell::buildSpecialCell(speed));
        indexToSpeedTrack.insert(std::make_pair(speedTrackIndex, specialTrack));
    }
}


// Event Tracks.
// ==============================================

void SubsongBuilder::startNewEventTrack(const int eventTrackIndex) noexcept
{
    jassert(currentEventTrackIndex < 0);            // Track already started?

    // The track must not exist yet. Should never happen.
    if (indexToEventTrack.find(eventTrackIndex) != indexToEventTrack.cend()) {
        addError("The Event track " + juce::String(eventTrackIndex) + " already exist!");
        jassertfalse;
        return;
    }

    // Initializes the track.
    currentEventTrackIndex = eventTrackIndex;
    currentEventTrack = SpecialTrack();
}

void SubsongBuilder::setCurrentEventTrackValue(const int cellIndex, const int event) noexcept
{
    jassert(currentEventTrackIndex >= 0);            // No Track started?
    currentEventTrack.setCell(cellIndex, SpecialCell::buildSpecialCell(event));
}

void SubsongBuilder::finishCurrentEventTrack() noexcept
{
    jassert(currentEventTrackIndex >= 0);            // No Track started?W
    jassert(indexToEventTrack.find(currentEventTrackIndex) == indexToEventTrack.cend());       // Track already stored??

    // Stores the EventTrack.
    indexToEventTrack.insert(std::make_pair(currentEventTrackIndex, currentEventTrack));
    currentEventTrackIndex = -1;
}


// Cells.
// ==============================================

void SubsongBuilder::startNewCell(const int cellIndex) noexcept
{
    // Initializes the Cell.
    currentCellIndex = cellIndex;
    currentCell = Cell();
}

void SubsongBuilder::finishCurrentCell() noexcept
{
    // No need to encode an empty Cell.
    if (currentCell.isEmpty()) {
        return;
    }

    currentTrack.setCell(currentCellIndex, currentCell);
}

void SubsongBuilder::setCurrentCellNote(const int note) noexcept
{
    currentCell.setNote(Note::buildNote(note));
}

void SubsongBuilder::setCurrentCellInstrument(const int instrument) noexcept
{
    currentCell.setInstrument(instrument);
}

void SubsongBuilder::removeCurrentCellInstrument() noexcept
{
    currentCell.setInstrument({ });
}

void SubsongBuilder::setCurrentCellRst() noexcept
{
    currentCell.setNote(Note::buildRst());
    currentCell.setInstrument(0);
}

void SubsongBuilder::addCurrentCellEffect(const Effect effect, const int logicalValue) noexcept
{
    const auto success = currentCell.addEffect(effect, logicalValue);
    if (!success) {
        jassertfalse;
        addWarning("Too many effects for one cell.");
    }
}

void SubsongBuilder::setCurrentCellEffectRawValue(const int effectIndex, const Effect effect, const int rawValue) noexcept
{
    const auto success = currentCell.setEffect(effectIndex, CellEffect(effect, rawValue));
    if (!success) {
        jassertfalse;
        addWarning("Unable to write an effect.");
    }
}


// Positions.
// ==============================================

void SubsongBuilder::startNewPosition() noexcept
{
    // Initializes the values.
    currentPositionHeight = TrackConstants::defaultPositionHeight;
    currentPositionPatternIndex = -1;
    currentPositionMarkerName = juce::String();
    currentPositionMarkerColor = 0xff000000;
    currentPositionChannelToTransposition.clear();
}

void SubsongBuilder::setCurrentPositionHeight(const int newHeight) noexcept
{
    currentPositionHeight = newHeight;
}

void SubsongBuilder::setCurrentPositionTransposition(const int channelIndex, const int transposition) noexcept
{
    if (transposition == 0) {
        // 0 are not encoded. This will remove a previously encoded transposition.
        currentPositionChannelToTransposition.erase(channelIndex);
        return;
    }
    currentPositionChannelToTransposition[channelIndex] = transposition;
}

void SubsongBuilder::setCurrentPositionPatternIndex(const int patternIndex) noexcept
{
    jassert(patternIndex >= 0);         // Abnormal!
    currentPositionPatternIndex = patternIndex;
}

void SubsongBuilder::finishCurrentPosition() noexcept
{
    // Pattern index is mandatory!
    if (currentPositionPatternIndex < 0) {
        addError("Position " + juce::String(positions.size()) + " has no pattern index set!");
        return;
    }

    positions.emplace_back(currentPositionPatternIndex, currentPositionHeight, currentPositionMarkerName, currentPositionMarkerColor,
                           currentPositionChannelToTransposition);
}

void SubsongBuilder::addPositions(const std::vector<Position>& positionsToAdd) noexcept
{
    positions.insert(positions.cend(), positionsToAdd.cbegin(), positionsToAdd.cend());
}


// Patterns.
// ==============================================

void SubsongBuilder::startNewPattern(const int patternIndex) noexcept
{
    currentPatternIndex = patternIndex;
    currentPatternChannelIndexToTrack.clear();
    currentPatternSpeedIndex = 0;
    currentPatternEventIndex = 0;
    currentPatternColor = LookAndFeelConstants::defaultPatternColor;
}

void SubsongBuilder::setCurrentPatternTrackIndex(const int channelIndex, const int trackIndex) noexcept
{
    currentPatternChannelIndexToTrack[channelIndex] = trackIndex;
}

void SubsongBuilder::setCurrentPatternSpeedTrackIndex(const int speedTrackIndex) noexcept
{
    currentPatternSpeedIndex = speedTrackIndex;
}

void SubsongBuilder::setCurrentPatternEventTrackIndex(const int eventTrackIndex) noexcept
{
    currentPatternEventIndex = eventTrackIndex;
}

void SubsongBuilder::finishCurrentPattern() noexcept
{
    // The pattern must not already exist!
    if (indexToPattern.find(currentPatternIndex) != indexToPattern.cend()) {
        addError("Pattern " + juce::String(currentPatternIndex) + " has already been created!");
        return;
    }

    // Creates the Pattern. The channel count must match.
    const auto channelCount = PsgValues::getChannelCount(static_cast<int>(psgs.size()));
    if (static_cast<int>(currentPatternChannelIndexToTrack.size()) != channelCount) {
        addError("Pattern " + juce::String(currentPatternIndex) + " does not have the right amount of channels!");
        return;
    }
    std::vector<int> trackIndexes;
    trackIndexes.reserve(currentPatternChannelIndexToTrack.size());
    for (const auto& entry : currentPatternChannelIndexToTrack) {
        trackIndexes.push_back(entry.second);       // The input map is ordered, so all should be fine.
    }

    // Stores the Pattern.
    const auto pattern = Pattern(trackIndexes, currentPatternSpeedIndex, currentPatternEventIndex, currentPatternColor);
    indexToPattern.emplace(currentPatternIndex, pattern);
}

void SubsongBuilder::addPatterns(const std::vector<Pattern>& patterns) noexcept
{
    jassert(indexToPattern.empty());            // This method must not be used along any other creation method!

    // Performs some checks. The channel count must match.
    const auto channelCount = PsgValues::getChannelCount(static_cast<int>(psgs.size()));

    auto index = 0;
    for (const auto& pattern : patterns) {
        if (pattern.getChannelCount() != channelCount) {
            addError("Pattern " + juce::String(index) + " does not have the right amount of channels!");
            return;
        }

        indexToPattern.emplace(index, pattern);

        ++index;
    }
}

void SubsongBuilder::setPatternSpeedTrackIndex(const int patternIndex, const int speedTrackIndex) noexcept
{
    jassert(static_cast<size_t>(patternIndex) < indexToPattern.size());

    const auto oldPattern = indexToPattern.at(patternIndex);
    const auto newPattern = oldPattern.withSpeedTrackIndex(speedTrackIndex);
    indexToPattern[patternIndex] = newPattern;
}

std::pair<bool, int> SubsongBuilder::checkRawOriginalPatternAndStoreIfDifferent(const Pattern& pattern) noexcept
{
    // Does the same Pattern exist?
    const auto iterator = browsedRawOriginalPatternToIndex.find(pattern);
    if (iterator == browsedRawOriginalPatternToIndex.cend()) {
        // The pattern doesn't exist. Creates a new entry.
        // Finds a non-used pattern index.
        const auto newPatternIndex = MapUtil::findNextHoleInKeys(indexToPattern, 0);
        browsedRawOriginalPatternToIndex.insert(std::make_pair(pattern, newPatternIndex));
        return { true, newPatternIndex };
    }

    // The pattern already exist!
    return { false, iterator->second };
}


}   // namespace arkostracker

