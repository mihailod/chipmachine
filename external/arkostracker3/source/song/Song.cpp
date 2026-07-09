#include "Song.h"

#include "../business/instrument/GenericInstrumentGenerator.h"
#include "../utils/NumberUtil.h"
#include "../utils/PsgValues.h"
#include "subsong/SubsongConstants.h"

namespace arkostracker 
{

Song::Song(juce::String pName, juce::String pAuthor, juce::String pComposer, juce::String pComments, const bool pCreateDefaultData,
    const juce::int64 pCreationDate, const juce::int64 pModificationDate) noexcept:
        songMutex(),
        name(std::move(pName)),
        author(std::move(pAuthor)),
        composer(std::move(pComposer)),
        comments(std::move(pComments)),
        creationDate(pCreationDate),
        modificationDate(pModificationDate),
        arpeggios(true, pCreateDefaultData),         // Creates a default item?
        pitches(false, pCreateDefaultData),          // Idem?

        subsongs(),
        subsongsMutex(),

        instruments(),
        instrumentsMutex()
{
    if (pCreateDefaultData) {
        // Subsong 0.
        std::vector<Psg> psgs;
        psgs.emplace_back();
        auto subsong = std::make_unique<Subsong>(SubsongConstants::defaultFirstSubsongName, SubsongConstants::defaultSpeed, PsgFrequency::defaultReplayFrequencyHz,
        SubsongConstants::defaultDigiChannel, SubsongConstants::defaultPrimaryHighlight, SubsongConstants::defaultSecondaryHighlight,
                                                 0, 0, psgs, SidPlayerCapability::buildDefault(), true);
        subsongs.push_back(std::move(subsong));

        // Instrument 0 (empty). The arpeggio/pitch are created in the initialization list.
        addInstrument(Instrument::buildEmptyPsgInstrument());
    }
}

Song::Song(const Song& source) noexcept :
        songMutex(),
        name(source.name),
        author(source.author),
        composer(source.composer),
        comments(source.comments),
        creationDate(source.creationDate),
        modificationDate(source.modificationDate),
        arpeggios(source.arpeggios),
        pitches(source.pitches),
        subsongs(),
        subsongsMutex(),
        instruments(),
        instrumentsMutex()
{
    // Copies the Subsongs.
    {
        const std::lock_guard lock(source.subsongsMutex);        // Lock the source data!

        subsongs.reserve(source.subsongs.size());
        for (const auto& sourceSubsong: source.subsongs) {
            subsongs.push_back(std::make_unique<Subsong>(*sourceSubsong));
        }
    }

    // Copies the Instruments.
    {
        const std::lock_guard lock(source.instrumentsMutex);        // Lock the source data!

        instruments.reserve(source.instruments.size());
        for (const auto& sourceInstrument: source.instruments) {
            instruments.push_back(std::make_unique<Instrument>(*sourceInstrument));
        }
    }
}

std::unique_ptr<Song> Song::buildDefaultSong(const bool addFirstInstrument) noexcept
{
    auto song = std::make_unique<Song>(juce::translate("Untitled"), juce::translate("Unknown"), juce::translate("Unknown"),
                                       juce::String(), true);
    if (addFirstInstrument) {
        auto instrument = GenericInstrumentGenerator::generateInstrumentForDecreasingVolume(juce::translate("Beep"));
        song->addInstrument(std::move(instrument));
    }
    return song;
}

void Song::addEmptyInstrumentAndExpressions() noexcept
{
    // Instrument 0 (empty).
    addInstrument(Instrument::buildEmptyPsgInstrument());

    // Expressions.
    arpeggios.addExpression(Expression::buildArpeggio(juce::translate("Empty"), true));
    pitches.addExpression(Expression::buildPitch(juce::translate("Empty"), true));
}

ExpressionHandler& Song::getExpressionHandler(const bool isArpeggio) noexcept
{
    return isArpeggio ? arpeggios : pitches;
}

const ExpressionHandler& Song::getConstExpressionHandler(const bool isArpeggio) const noexcept
{
    return isArpeggio ? arpeggios : pitches;
}

std::vector<std::pair<juce::String, Id>> Song::getSubsongNamesAndIds() const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    std::vector<std::pair<juce::String, Id>> namesAndIds;
    namesAndIds.reserve(subsongs.size());

    for (const auto& subsong : subsongs) {
        namesAndIds.emplace_back(subsong->getName(), subsong->getId());
    }

    return namesAndIds;
}

Subsong::Metadata Song::getSubsongMetadata(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto* subsong = findSubsong_noLock(subsongId);
    if (subsong == nullptr) {
        jassertfalse;       // Not found!
        return Subsong::Metadata::buildDefault();
    }

    return subsong->getMetadata();
}

std::vector<Psg> Song::getSubsongPsgs(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto* subsong = findSubsong_noLock(subsongId);
    if (subsong == nullptr) {
        jassertfalse;       // Not found!
        return { };
    }

    return subsong->getPsgs();
}

int Song::getSubsongPsgCount(const Id& subsongId) const noexcept
{
    return static_cast<int>(getSubsongPsgs(subsongId).size());
}

void Song::performOnConstSubsong(const Id& subsongId, const std::function<void(const Subsong&)>& function) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!
    const auto* subsong = findSubsong_noLock(subsongId);
    if (subsong == nullptr) {
        jassertfalse;           // Subsong doesn't exist anymore! But can happen when loading a new Song.
        return;
    }

    function(*subsong);
}

void Song::performOnSubsong(const Id& subsongId, const std::function<void(Subsong&)>& function) noexcept
{
    // Couldn't use the trick not to duplicate the same code as above...
    const std::lock_guard lock(subsongsMutex);        // Lock!
    auto* subsong = findSubsong_noLock(subsongId);
    if (subsong == nullptr) {
        jassertfalse;           // Subsong doesn't exist anymore!
        return;
    }

    function(*subsong);
}

const Subsong* Song::findSubsong_noLock(const Id& subsongId) const noexcept
{
    const auto findIterator = std::find_if(subsongs.cbegin(), subsongs.cend(), [&] (const std::unique_ptr<Subsong>& subsong) {
        return (subsong->getId() == subsongId);
    });
    return (findIterator == subsongs.cend()) ? nullptr : findIterator->get();
}

Subsong* Song::findSubsong_noLock(const Id& subsongId) noexcept
{
    return const_cast<Subsong*>(static_cast<const Song&>(*this).findSubsong_noLock(subsongId));        // See Effective C++ by Scott Meyer, item 3.     // NOLINT(*)
}

OptionalLong Song::findSubsongIndex_noLock(const Id& subsongId) const noexcept
{
    const auto findIterator = std::find_if(subsongs.cbegin(), subsongs.cend(), [&] (const std::unique_ptr<Subsong>& subsong) {
        return (subsong->getId() == subsongId);
    });

    if (findIterator == subsongs.cend()) {
        return { };
    }

    return std::distance(subsongs.cbegin(), findIterator);
}

const Instrument* Song::findInstrument(const Id& id) const noexcept
{
    const auto findIterator = std::find_if(instruments.cbegin(), instruments.cend(), [&] (const std::unique_ptr<Instrument>& instrument) {
        return (instrument->getId() == id);
    });
    return (findIterator == instruments.cend()) ? nullptr : findIterator->get();
}

Instrument* Song::findInstrument_noLock(const Id& id) noexcept
{
    return const_cast<Instrument*>(this->findInstrument(id));        // See Effective C++ by Scott Meyer, item 3.    // NOLINT(*)
}

const Instrument* Song::findInstrumentFromIndex_noLock(const int instrumentIndex) const noexcept
{
    if (instrumentIndex >= static_cast<int>(instruments.size())) {
        return nullptr;
    }

    return instruments.at(static_cast<size_t>(instrumentIndex)).get();
}

int Song::getSubsongCount() const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!
    return static_cast<int>(subsongs.size());
}

bool Song::doesSubsongIdExist(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!
    const auto* subsong = findSubsong_noLock(subsongId);
    return (subsong != nullptr);
}

void Song::addSubsong(std::unique_ptr<Subsong> subsong) noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    subsongs.push_back(std::move(subsong));
}

bool Song::insertSubsong(const int subsongIndexToInsertAt, std::unique_ptr<Subsong> subsong) noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    subsongs.insert(subsongs.begin() + subsongIndexToInsertAt, std::move(subsong));

    return true;
}

std::unique_ptr<Subsong> Song::deleteSubsong(const Id& subsongId) noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto foundIndexOptional = findSubsongIndex_noLock(subsongId);
    if (foundIndexOptional.isAbsent()) {
        jassertfalse;           // Subsong not found!
        return nullptr;
    }
    const auto foundIndex = foundIndexOptional.getValue();

    // Extracts the Subsong, then deletes the now dangling pointer.
    auto deletedSubsong = std::move(subsongs.at(static_cast<size_t>(foundIndex)));
    subsongs.erase(subsongs.begin() + static_cast<int>(foundIndex));

    jassert(!subsongs.empty());         // Dangerous if there is no more Subsongs!

    return deletedSubsong;
}

void Song::replaceSubsong(const Id& subsongId, std::unique_ptr<Subsong> subsong) noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto foundIndexOptional = findSubsongIndex_noLock(subsongId);
    if (foundIndexOptional.isAbsent()) {
        jassertfalse;           // Subsong not found!
        return;
    }
    const auto foundIndex = foundIndexOptional.getValue();

    subsongs.at(static_cast<size_t>(foundIndex)) = std::move(subsong);
}

Id Song::getFirstSubsongId() const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    jassert(!subsongs.empty());
    return subsongs.at(0U)->getId();
}

std::vector<Id> Song::getSubsongIds() const noexcept
{
    std::vector<Id> subsongIds;

    const std::lock_guard lock(subsongsMutex);        // Lock!

    subsongIds.reserve(subsongs.size());
    for (const auto& subsong : subsongs) {
        subsongIds.push_back(subsong->getId());
    }

    return subsongIds;
}

OptionalValue<Id> Song::getSubsongIdFromHash(const int idHash) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    for (const auto& subsong : subsongs) {
        if (const auto id = subsong->getId(); static_cast<int>(id.hash()) == idHash) {
            return id;
        }
    }

    return { };
}

OptionalValue<Id> Song::getSubsongId(const int subsongIndex) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    if ((subsongIndex >= 0) && (subsongIndex < static_cast<int>(subsongs.size()))) {
        return subsongs.at(static_cast<size_t>(subsongIndex))->getId();
    }

    return { };
}

bool Song::doesSubsongExist(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    for (const auto& subsong : subsongs) {
        if (subsong->getId() == subsongId) {
            return true;
        }
    }

    return false;
}

OptionalInt Song::getSubsongIndex(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto indexLong = findSubsongIndex_noLock(subsongId);
    return indexLong.isPresent() ? OptionalInt(static_cast<int>(indexLong.getValue())) : OptionalInt();
}

OptionalValue<Id> Song::getSubsongIdIfSubsongDeleted(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto subsongCount = static_cast<int>(subsongs.size());
    if (subsongCount <= 1) {
        return { };             // Cannot delete, not enough Subsongs.
    }

    const auto foundIndexOptional = findSubsongIndex_noLock(subsongId);
    if (foundIndexOptional.isAbsent()) {
        jassertfalse;           // Subsong not found!
        return { };
    }
    const auto subsongIndexToDelete = static_cast<int>(foundIndexOptional.getValue());

    // Can the next Subsong is reachable?
    auto subsongIndexToGoTo = subsongIndexToDelete + 1;
    if (subsongIndexToGoTo >= subsongCount) {
        // Not reachable. Then use the previous one. It is always possible since the size has been checked before.
        subsongIndexToGoTo = subsongIndexToDelete - 1;
    }

    return subsongs.at(static_cast<size_t>(subsongIndexToGoTo))->getId();
}

std::pair<Location, Location> Song::getLoopStartAndPastEndPositions(const Id& subsongId) const noexcept
{
    std::pair loopStartAndEndPosition(0, 0);
    performOnConstSubsong(subsongId, [&](const Subsong& subsong) noexcept {
        loopStartAndEndPosition = subsong.getLoopStartAndEndPosition();
    });

    const auto loopStartLocation = Location(subsongId, loopStartAndEndPosition.first);
    const auto pastEndLocation = Location(subsongId, loopStartAndEndPosition.second + 1);

    return { loopStartLocation, pastEndLocation };
}


// ========================================================================

juce::String Song::getTitle() const noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    return name;
}

juce::String Song::getAuthor() const noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    return author;
}

juce::String Song::getComposer() const noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    return composer;
}

juce::String Song::getComments() const noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    return comments;
}

void Song::setTitle(const juce::String& newTitle) noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    name = newTitle;
}

void Song::setAuthor(const juce::String& newAuthor) noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    author = newAuthor;
}

void Song::setComposer(const juce::String& newComposer) noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    composer = newComposer;
}

void Song::setComments(const juce::String& newComments) noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    comments = newComments;
}

juce::int64 Song::getCreationDateMs() const noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    return creationDate;
}

juce::int64 Song::getModificationDateMs() const noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    return modificationDate;
}

void Song::resetDatesMs(const juce::int64 creationDateMs, const juce::int64 modificationDataMs) noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    creationDate = creationDateMs;
    modificationDate = (modificationDataMs == 0) ? creationDateMs : modificationDataMs;
}

void Song::updateModificationDate() noexcept
{
    const std::lock_guard lock(songMutex);        // Lock!
    modificationDate = juce::Time::currentTimeMillis();
}

int Song::getChannelCount(const Id& subsongId) const noexcept
{
    return getPsgCount(subsongId) * PsgValues::channelCountPerPsg;
}

int Song::getPsgCount(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto* subsong = findSubsong_noLock(subsongId);
    if (subsong == nullptr) {
        jassertfalse;           // Subsong not found!
        return 1;
    }

    return subsong->getPsgCount();
}

float Song::getReplayFrequencyHz(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto* subsong = findSubsong_noLock(subsongId);
    if (subsong == nullptr) {
        jassertfalse;           // Subsong not found!
        return PsgFrequency::defaultReplayFrequencyHz;
    }

    return subsong->getReplayFrequencyHz();
}

int Song::getDigiChannel(const Id& subsongId) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    const auto* subsong = findSubsong_noLock(subsongId);
    if (subsong == nullptr) {
        jassertfalse;           // Subsong not found!
        return 1;
    }

    return subsong->getDigiChannel();
}
/*StartEnd Song::getLoopStartAndEndLocation(const Id& subsongId) const noexcept
{
    std::lock_guard<std::mutex> lock(subsongsMutex);        // Lock!

    const auto& subsong = *subsongs.at(static_cast<size_t>(subsongIndex));

    auto startEndPositions = subsong.getLoopStartAndEndPosition();
    // The start is easy.
    auto startLocation = Location(subsongIndex, startEndPositions.first);
    // The end must take in account the height of the position.
    const auto endPosition = startEndPositions.second;
    const auto endPositionHeight = subsong.getPositionHeight(endPosition);
    jassert(endPositionHeight > 0);
    auto endLocation = Location(subsongIndex, endPosition, endPositionHeight - 1);      // The line is the last one.

    return { startLocation, endLocation };
}*/


// Instrument.
// =======================================

int Song::getInstrumentCount() const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!
    return static_cast<int>(instruments.size());
}

OptionalId Song::getInstrumentId(const int index) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    if (index >= static_cast<int>(instruments.size())) {
        return { };
    }

    return instruments.at(static_cast<size_t>(index))->getId();
}

Id Song::getFirstRealInstrumentId() const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!
    jassert(!instruments.empty());

    if (instruments.size() > 1) {
        return instruments.at(1)->getId();
    }
    return instruments.at(0)->getId();
}

Id Song::getLastInstrumentId() const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!
    jassert(!instruments.empty());

    return (*instruments.crbegin())->getId();
}

OptionalInt Song::getInstrumentIndex(const Id& instrumentId) const noexcept
{
    const auto findIterator = std::find_if(instruments.cbegin(), instruments.cend(), [&] (const std::unique_ptr<Instrument>& instrument) {
        return (instrument->getId() == instrumentId);
    });
    if (findIterator == instruments.cend()) {
        return { };
    }

    return static_cast<int>(std::distance(instruments.cbegin(), findIterator));
}

std::vector<Id> Song::getInstrumentIds(const bool skip0th) const noexcept
{
    std::vector<Id> subsongIds;

    const std::lock_guard lock(instrumentsMutex);        // Lock!

    subsongIds.reserve(instruments.size());
    for (const auto& instrument : instruments) {
        subsongIds.push_back(instrument->getId());
    }

    if (!skip0th || subsongIds.empty()) {   // Empty test as a security.
        return subsongIds;
    }

    // Skips the 0th.
    return std::vector(subsongIds.cbegin() + 1, subsongIds.cend());
}

OptionalId Song::findInstrumentIdFromEvent(const int event) const noexcept
{
    // No strategy for now. The event matches the instrument index.
    return getInstrumentId(event);
}

juce::String Song::getInstrumentName(const Id& id) const noexcept
{
    juce::String instrumentName;
    performOnConstInstrument(id, [&](const Instrument& instrument) noexcept {
        instrumentName = instrument.getName();
    });

    return instrumentName;
}

juce::String Song::getInstrumentNameFromIndex(const int instrumentIndex) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    const auto* instrument = findInstrumentFromIndex_noLock(instrumentIndex);
    if (instrument == nullptr) {
        jassertfalse;           // Instrument doesn't exist!
        return { };
    }

    return instrument->getName();
}

std::unordered_map<int, juce::uint32> Song::buildInstrumentToColor() const noexcept
{
    std::unordered_map<int, juce::uint32> instrumentToColor;

    const std::lock_guard lock(instrumentsMutex);        // Lock!

    // Fills the table with each instrument color.
    auto instrumentIndex = 0;
    for (const auto& instrument : instruments) {
        const auto& color = instrument->getArgbColor();
        instrumentToColor.insert(std::make_pair(instrumentIndex, color));

        ++instrumentIndex;
    }

    return instrumentToColor;
}

void Song::performOnConstInstrument(const Id& id, const std::function<void(const Instrument&)>& function) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    const auto* instrument = findInstrument(id);
    if (instrument == nullptr) {
        jassertfalse;           // Instrument doesn't exist!
        return;
    }

    function(*instrument);
}

void Song::performOnConstInstrumentFromIndex(const int instrumentIndex, const std::function<void(const Instrument&)>& function) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    if (instrumentIndex >= static_cast<int>(instruments.size())) {
        jassertfalse;           // Out of bounds!
        return;
    }

    const auto& instrument = instruments.at(static_cast<size_t>(instrumentIndex));

    function(*instrument);
}

void Song::performOnInstrument(const Id& id, const std::function<void(Instrument&)>& function) noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    auto* instrument = findInstrument_noLock(id);
    if (instrument == nullptr) {
        jassertfalse;           // Instrument doesn't exist!
        return;
    }

    function(*instrument);
}

void Song::performOnConstPsgInstrument(const Id& id, const std::function<void(const Instrument&, const PsgPart&)>& function) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    const auto* instrument = findInstrument(id);
    if (instrument == nullptr) {
        jassertfalse;           // Instrument doesn't exist!
        return;
    }

    if (instrument->getType() != InstrumentType::psgInstrument) {
        jassertfalse;           // Not a PSG Instrument!
        return;
    }

    const auto& psgPart = instrument->getConstPsgPart();
    function(*instrument, psgPart);
}

void Song::performOnConstPsgInstrumentFromIndex(const int instrumentIndex, const std::function<void(const Instrument&, const PsgPart&)>& function) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    if (instrumentIndex >= static_cast<int>(instruments.size())) {
        jassertfalse;           // Out of bounds!
        return;
    }
    const auto& instrument = instruments.at(static_cast<size_t>(instrumentIndex));

    if (instrument->getType() != InstrumentType::psgInstrument) {
        jassertfalse;           // Not a PSG Instrument!
        return;
    }

    const auto& psgPart = instrument->getConstPsgPart();
    function(*instrument, psgPart);
}

void Song::performOnConstSampleInstrument(const Id& id, const std::function<void(const Instrument&, const SamplePart&)>& function) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    const auto* instrument = findInstrument(id);
    if (instrument == nullptr) {
        jassertfalse;           // Instrument doesn't exist!
        return;
    }

    if (instrument->getType() != InstrumentType::sampleInstrument) {
        jassertfalse;           // Not a Sample Instrument!
        return;
    }

    const auto& samplePart = instrument->getConstSamplePart();
    function(*instrument, samplePart);
}

void Song::performOnConstSampleInstrumentFromIndex(const int instrumentIndex, const std::function<void(const Instrument&, const SamplePart&)>& function) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    if (instrumentIndex >= static_cast<int>(instruments.size())) {
        jassertfalse;           // Out of bounds!
        return;
    }
    const auto& instrument = instruments.at(static_cast<size_t>(instrumentIndex));

    if (instrument->getType() != InstrumentType::sampleInstrument) {
        jassertfalse;           // Not a Sample Instrument!
        return;
    }

    const auto& samplePart = instrument->getConstSamplePart();
    function(*instrument, samplePart);
}

int Song::addInstrument(std::unique_ptr<Instrument> instrument) noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    const auto index = static_cast<int>(instruments.size());
    instruments.push_back(std::move(instrument));
    return index;
}

void Song::removeInstrument(const Id& instrumentId) noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!
    auto instrumentIndex = 0;
    auto foundInstrumentIndex = -1;
    for (auto& instrument : instruments) {
        if (instrument->getId() == instrumentId) {
            foundInstrumentIndex = instrumentIndex;
            break;
        }
        ++instrumentIndex;
    }

    if (foundInstrumentIndex >= 0) {
        instruments.erase(instruments.cbegin() + foundInstrumentIndex, instruments.cbegin() + foundInstrumentIndex + 1);
    } else {
        jassertfalse;       // Instrument not found!
    }
}

void Song::performOnInstruments(const std::function<void(std::vector<std::unique_ptr<Instrument>>&)>& lockedOperation) noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!
    lockedOperation(instruments);
}

void Song::performOnConstInstruments(const std::function<void(const std::vector<std::unique_ptr<Instrument>>&)>& lockedOperation) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!
    lockedOperation(instruments);
}

/**
 * @return true if the instrument is read-only (i.e. it is the first one). If not found, returns false.
 * @param instrumentId the ID of the Instrument. May not exist.
 */
bool Song::isReadOnlyInstrument(const Id& instrumentId) const noexcept
{
    const std::lock_guard lock(instrumentsMutex);        // Lock!

    const auto instrumentIndex = getInstrumentIndex(instrumentId);
    if (instrumentIndex.isAbsent()) {
        jassertfalse;       // Searching for a non-existing Instrument?
        return true;
    }

    return (instrumentIndex == 0);
}


// Cells.
// =======================================

Cell Song::getCell(const Location& location, const int channelIndex) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    // Valid Subsong?
    const auto* subsong = findSubsong_noLock(location.getSubsongId());
    if (subsong == nullptr) {
        jassertfalse;           // Subsong doesn't exist!
        return { };
    }

    // We can get the Cell. The position is checked internally.
    return subsong->getCell(location.getPosition(), location.getLine(), channelIndex);
}

SpecialCell Song::getSpecialCell(const bool speedTrack, const Location& location) const noexcept
{
    const std::lock_guard lock(subsongsMutex);        // Lock!

    // Valid Subsong?
    const auto* subsong = findSubsong_noLock(location.getSubsongId());
    if (subsong == nullptr) {
        jassertfalse;           // Subsong doesn't exist!
        static const auto emptySpecialCell = SpecialCell::buildEmptySpecialCell();
        return emptySpecialCell;
    }

    // We can get the SpecialCell. The position is checked internally.
    return subsong->getSpecialCell(speedTrack, location.getPosition(), location.getLine());
}

}   // namespace arkostracker
