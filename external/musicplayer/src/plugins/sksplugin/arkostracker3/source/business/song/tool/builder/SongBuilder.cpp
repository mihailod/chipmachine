#include "SongBuilder.h"

#include "../../../../utils/Base64Util.h"
#include "../../../../utils/MapUtil.h"
#include "../../../../utils/NoteUtil.h"
#include "../../validation/CorrectExpression.h"
#include "../../validation/CorrectInstrument.h"

namespace arkostracker 
{

const int SongBuilder::sampleMaximumSizeInBytes = 128 * 1024;

SongBuilder::SongBuilder(const bool pIsNative) noexcept :
        isNative(pIsNative),
        errorReport(std::make_unique<ErrorReport>()),
        currentSubsongIndex(0),
        currentInstrumentIndex(-1),
        currentExpressionIndex(0),
        currentSubsongBuilder(),
        instrumentStarted(),
        currentPsgPart(),
        currentInstrumentType(InstrumentType::psgInstrument),
        currentInstrumentTitle(),
        currentInstrumentArgbColor(),
        currentSampleInstrumentFrequencyHz(PsgFrequency::defaultSampleFrequencyHz),
        currentSampleInstrumentLoop(),
        currentSampleInstrumentVolumeRatio(1.0F),
        currentSampleInstrumentSample(),
        currentExpressionIsArpeggio(true),
        currentExpression(true, "fake", false),         // Fake Expression at first.
        currentExpressionLoop(0, 0, false),
        songTitle(),
        author(),
        composer(),
        comments(),
        creationDate(juce::Time::currentTimeMillis()),
        modificationDate(juce::Time::currentTimeMillis()),
        subsongIndexToBuilder(),
        indexToInstrument(),
        indexToArpeggio(),
        indexToPitch(),
        psgInstrumentCell()
{
    // By default, creates first Instruments and Expressions.
    auto instrument = Instrument::buildEmptyPsgInstrument();
    instrument->getPsgPart().setSoundEffectExported(false);
    indexToInstrument.insert(std::make_pair(0, std::move(instrument)));

    const Expression emptyArpeggio(true, "Empty", true);
    indexToArpeggio.insert(std::make_pair(0, emptyArpeggio));

    const Expression emptyPitch(false, "Empty", true);
    indexToPitch.insert(std::make_pair(0, emptyPitch));
}

void SongBuilder::addWarning(const juce::String& text, const OptionalInt lineNumber) noexcept
{
    errorReport->addWarning(text, lineNumber);
}

void SongBuilder::addError(const juce::String& text, const OptionalInt lineNumber) noexcept
{
    errorReport->addError(text, lineNumber);
}

bool SongBuilder::isOk() const noexcept
{
    if (!errorReport->isOk()) {
        return false;
    }

    // Checks the report of the subsongs.
    for (const auto& entry : subsongIndexToBuilder) {
        if (!entry.second->isOk()) {
            return false;
        }
    }

    return true;
}

bool SongBuilder::areErrors() const noexcept
{
    return !isOk();
}

void SongBuilder::setSongMetaData(juce::String newTitle, juce::String newAuthor, juce::String newComposer, juce::String newComments,
                                  const juce::int64 newCreationDate, const juce::int64 newModificationDate) noexcept
{
    songTitle = std::move(newTitle);
    author = std::move(newAuthor);
    composer = std::move(newComposer);
    comments = std::move(newComments);
    creationDate = newCreationDate;
    modificationDate = newModificationDate;
}

std::pair<std::unique_ptr<Song>, std::unique_ptr<ErrorReport>> SongBuilder::buildSong() noexcept
{
    // If already critical errors, no need to go further.
    if (!errorReport->isOk()) {
        return std::make_pair(nullptr, std::move(errorReport));
    }

    auto song = std::make_unique<Song>(songTitle, author, composer, comments, false, creationDate, modificationDate);

    // Encodes each Subsong. All must be present.
    auto success = encodeSubsongs(*song);
    success = success && encodeInstruments(*song);
    success = success && encodeExpression(*song, true);
    success = success && encodeExpression(*song, false);

    // ALSO makes sure there are no holes in the Subsongs, tracks, arpeggios, pitches, instruments.
    return std::make_pair(success ? std::move(song) : nullptr, std::move(errorReport));
}

bool SongBuilder::encodeSubsongs(Song& song) noexcept
{
    // There must be at least one Subsong!
    if (subsongIndexToBuilder.empty()) {
        addError("There must be at least one Subsong!");
        return false;
    }

    // There must not be any holes in the Subsongs.
    const auto hole = MapUtil::areKeysContiguous(subsongIndexToBuilder, 0);
    if (hole.isPresent()) {
        addError("Subsong is missing: " + juce::String(hole.getValue()));
        return false;
    }

    for (const auto& entry : subsongIndexToBuilder) {
        auto& subsongBuilder = *entry.second;
        auto subsong = subsongBuilder.buildSubsong();

        if (subsong != nullptr) {
            song.addSubsong(std::move(subsong));
        }

        // Mixes the Error Reports.
        errorReport->addFrom(subsongBuilder.getErrorReport());
    }

    return isOk();
}

bool SongBuilder::encodeInstruments(Song& song) noexcept
{
    jassert(!instrumentStarted);             // Last instrument not finished!

    // There MUST be at least one Instrument.
    if (indexToInstrument.empty()) {
        addError("There are no instruments, abnormal.");
        return false;
    }

    // Fills the holes with empty Instruments.
    MapUtil::fillHolesInMapUniquePtr<Instrument>(0, indexToInstrument, []() noexcept {
        return Instrument::buildEmptyPsgInstrument();
    });

    for (auto& [instrumentIndex, instrumentInMapPtr] : indexToInstrument) {
        auto instrumentInMap = std::move(instrumentInMapPtr);

        if (instrumentIndex < 0) {
            addError("Illegal instrument index: " + juce::String(instrumentIndex));
        }

        // Corrects the Instrument.
        CorrectInstrument::correctInstrument(*instrumentInMap);

        const auto returnedInstrumentIndex = song.addInstrument(std::move(instrumentInMap));
        jassert(instrumentIndex == returnedInstrumentIndex); (void)returnedInstrumentIndex;          // Must be equal!
    }
    indexToInstrument.clear();      // Security, as the map items are moved.

    return isOk();
}

bool SongBuilder::encodeExpression(Song& song, const bool isArpeggio) noexcept
{
    auto& indexToExpression = isArpeggio ? indexToArpeggio : indexToPitch;
    auto& expressionHandler = song.getExpressionHandler(isArpeggio);

    // There MUST be at least one Expression.
    if (indexToExpression.empty()) {
        addError("There are no expressions, abnormal.");
        return false;
    }

    // Fills the holes with empty Expression.
    MapUtil::fillHolesInMap<Expression>(0, indexToExpression, [&] {
        return Expression(isArpeggio, "Empty", true);
    });

    CorrectExpression correctExpression;

    for (auto& entry : indexToExpression) {
        const auto expressionIndex = entry.first;
        auto expression = entry.second;

        // Corrects the Expression.
        correctExpression.correctExpression(expression);

        const auto returnedExpressionIndex = expressionHandler.addExpression(expression);
        jassert(expressionIndex == returnedExpressionIndex);          // Must be equal!
        (void)expressionIndex;
        (void)returnedExpressionIndex;
    }

    return isOk();
}


// Subsongs.
// ==================================================

SubsongBuilder& SongBuilder::startNewSubsong(const int subsongIndex) noexcept
{
    // The slot must be free.
    if (subsongIndexToBuilder.find(subsongIndex) != subsongIndexToBuilder.cend()) {
        addError("Subsong " + juce::String(subsongIndex) + " already exists!");
        jassertfalse;
        return *subsongIndexToBuilder.find(subsongIndex)->second;       // Only not compile... This case is a dead-end.
    }

    currentSubsongIndex = subsongIndex;
    currentSubsongBuilder = std::make_unique<SubsongBuilder>(isNative);

    return *currentSubsongBuilder;
}

void SongBuilder::finishCurrentSubsong() noexcept
{
    jassert(currentSubsongBuilder != nullptr);          // Builder not created??

    // Stores the Subsong Builder.
    subsongIndexToBuilder[currentSubsongIndex] = std::move(currentSubsongBuilder);

    currentSubsongBuilder = nullptr;
}

SubsongBuilder& SongBuilder::getCurrentSubsongBuilder() noexcept
{
    jassert(currentSubsongBuilder != nullptr);      // Ouch! No subsong builder is being currently edited!
    return *currentSubsongBuilder;
}


// Instruments.
// ==================================================================

int SongBuilder::findNextInstrumentIndex() noexcept
{
    return MapUtil::findNextHoleInKeys(indexToInstrument, 0);
}

void SongBuilder::startNewPsgInstrument(const int instrumentIndex) noexcept
{
    startInstrument(instrumentIndex, InstrumentType::psgInstrument);
}

void SongBuilder::startNewSampleInstrument(const int instrumentIndex) noexcept
{
    startInstrument(instrumentIndex, InstrumentType::sampleInstrument);
}

void SongBuilder::startInstrument(const int instrumentIndex, const InstrumentType instrumentType) noexcept
{
    jassert(!instrumentStarted);             // Instrument already started!

    // The slot must be free.
    if (indexToInstrument.find(instrumentIndex) != indexToInstrument.cend()) {
        addError("Instrument " + juce::String(instrumentIndex) + " already exists!");
        jassertfalse;
        return;
    }
    // Instrument 0 cannot be created.
    if (instrumentIndex == 0) {
        addError("Instrument 0 cannot be created!");
        jassertfalse;
        return;
    }

    instrumentStarted = true;
    currentInstrumentTitle = juce::String();
    currentInstrumentIndex = instrumentIndex;
    currentPsgPart = std::make_unique<PsgPart>();
    currentInstrumentType = instrumentType;
    currentInstrumentArgbColor = Instrument::defaultColor;
    currentSampleInstrumentFrequencyHz = PsgFrequency::defaultSampleFrequencyHz;
    currentSampleInstrumentLoop = Loop();
    currentSampleInstrumentVolumeRatio = 1.0F;
    currentSampleInstrumentSample = nullptr;
}

void SongBuilder::finishCurrentInstrument() noexcept
{
    jassert(instrumentStarted); (void)instrumentStarted;              // Instrument was not started!

    std::unique_ptr<Instrument> newInstrument;

    // What kind of Instrument?
    switch (currentInstrumentType) {
        case InstrumentType::psgInstrument:
            jassert(currentPsgPart != nullptr);

            newInstrument = Instrument::buildPsgInstrument(currentInstrumentTitle, *currentPsgPart, currentInstrumentArgbColor);
            currentPsgPart = nullptr;
            break;
        case InstrumentType::sampleInstrument: {
            jassert(currentSampleInstrumentSample != nullptr);  // No sample created??

            // Stores the sample into the Sample Manager.
            const auto samplePart = std::make_unique<SamplePart>(currentSampleInstrumentSample, currentSampleInstrumentLoop, currentSampleInstrumentVolumeRatio,
                                                             currentSampleInstrumentFrequencyHz);
            newInstrument = Instrument::buildSampleInstrument(currentInstrumentTitle, *samplePart, currentInstrumentArgbColor);
            break;
        }
        default:
            jassertfalse;       // Never supposed to happen.
            break;
    }

    instrumentStarted = false;
    indexToInstrument[currentInstrumentIndex] = std::move(newInstrument);
    currentInstrumentIndex = -1;
}

void SongBuilder::setInstrument(const int instrumentIndex, std::unique_ptr<Instrument> instrument) noexcept
{
    jassert(!instrumentStarted);                                // An Instrument is already started! Dangerous!
    jassert(instrumentIndex != currentInstrumentIndex);         // Dangerous!
    jassert(instrumentIndex > 0);

    indexToInstrument[instrumentIndex] = std::move(instrument);
}

void SongBuilder::setInstruments(const int startIndex, std::vector<std::unique_ptr<Instrument>>& instruments) noexcept
{
    jassert(!instrumentStarted);                                // An Instrument is already started! Dangerous!

    auto instrumentIndex = startIndex;
    for (auto& instrument : instruments) {
        indexToInstrument[instrumentIndex++] = std::move(instrument);
    }
}

void SongBuilder::setCurrentInstrumentName(const juce::String& name) noexcept
{
    currentInstrumentTitle = name;
}

void SongBuilder::setCurrentInstrumentColor(const juce::uint32& argbColor) noexcept
{
    currentInstrumentArgbColor = argbColor;
}

void SongBuilder::setCurrentPsgInstrumentSpeed(const int speed) noexcept
{
    currentPsgPart->setSpeed(speed);
}

void SongBuilder::setCurrentPsgInstrumentSoundEffectExported(const bool exported) noexcept
{
    currentPsgPart->setSoundEffectExported(exported);
}

void SongBuilder::setCurrentPsgInstrumentRetrig(const bool retrig) noexcept
{
    currentPsgPart->setInstrumentRetrig(retrig);
}

void SongBuilder::setCurrentPsgInstrumentMainLoop(const int startIndex, const int endIndex, const bool isLooping)
{
    currentPsgPart->setMainLoop(Loop(startIndex, endIndex, isLooping));
}

void SongBuilder::startNewPsgInstrumentCell() noexcept
{
    psgInstrumentCell = PsgInstrumentCell();
}

void SongBuilder::finishCurrentPsgInstrumentCell() noexcept
{
    jassert(currentPsgPart != nullptr);
    currentPsgPart->addCell(psgInstrumentCell);
}

void SongBuilder::setCurrentPsgInstrumentCellLink(const PsgInstrumentCellLink link)
{
    psgInstrumentCell = psgInstrumentCell.withLink(link);
}

void SongBuilder::setCurrentPsgInstrumentCellVolume(const int volume)
{
    psgInstrumentCell = psgInstrumentCell.withVolume(volume);
}

void SongBuilder::setCurrentPsgInstrumentCellNoise(const int noise)
{
    psgInstrumentCell = psgInstrumentCell.withNoise(noise);
}

void SongBuilder::setCurrentPsgInstrumentCellPrimaryPeriod(const int period)
{
    psgInstrumentCell = psgInstrumentCell.withPrimaryPeriod(period);
}

void SongBuilder::setCurrentPsgInstrumentCellPrimaryArpeggio(const int arpeggio)
{
    const auto noteInOctave = NoteUtil::getNoteInOctave(arpeggio);
    const auto octave = NoteUtil::getOctaveFromNote(arpeggio);
    psgInstrumentCell = psgInstrumentCell.withPrimaryArpeggioNoteInOctave(noteInOctave);
    psgInstrumentCell = psgInstrumentCell.withPrimaryArpeggioOctave(octave);
}

void SongBuilder::setCurrentPsgInstrumentCellPrimaryPitch(const int pitch)
{
    psgInstrumentCell = psgInstrumentCell.withPrimaryPitch(pitch);
}

void SongBuilder::setCurrentPsgInstrumentCellSecondaryPeriod(const int period)
{
    psgInstrumentCell = psgInstrumentCell.withSecondaryPeriod(period);
}

void SongBuilder::setCurrentPsgInstrumentCellSecondaryArpeggio(const int arpeggio)
{
    const auto noteInOctave = NoteUtil::getNoteInOctave(arpeggio);
    const auto octave = NoteUtil::getOctaveFromNote(arpeggio);
    psgInstrumentCell = psgInstrumentCell.withSecondaryArpeggioNoteInOctave(noteInOctave);
    psgInstrumentCell = psgInstrumentCell.withSecondaryArpeggioOctave(octave);
}

void SongBuilder::setCurrentPsgInstrumentCellSecondaryPitch(const int pitch)
{
    psgInstrumentCell = psgInstrumentCell.withSecondaryPitch(pitch);
}

void SongBuilder::setCurrentPsgInstrumentCellRatio(const int ratio)
{
    psgInstrumentCell = psgInstrumentCell.withRatio(ratio);
}

void SongBuilder::setCurrentPsgInstrumentCellHardwareEnvelope(const int envelope)
{
    psgInstrumentCell = psgInstrumentCell.withEnvelope(envelope);
}

void SongBuilder::setCurrentPsgInstrumentCellRetrig(const bool retrig)
{
    psgInstrumentCell = psgInstrumentCell.withRetrig(retrig);
}

void SongBuilder::setCurrentPsgInstrumentCell(const PsgInstrumentCell& cell)
{
    psgInstrumentCell = cell;
}

void SongBuilder::setCurrentSampleInstrumentFrequencyHz(const int frequencyHz)
{
    currentSampleInstrumentFrequencyHz = frequencyHz;
}

void SongBuilder::setCurrentSampleInstrumentLoop(const Loop& loop)
{
    currentSampleInstrumentLoop = loop;
}

void SongBuilder::setCurrentSampleInstrumentVolumeRatio(const float volumeRatio)
{
    currentSampleInstrumentVolumeRatio = volumeRatio;
}

void SongBuilder::setCurrentSampleInstrumentSample(const juce::MemoryBlock& sampleData8BitsUnsigned)
{
    // If too large, the sample data should not be accepted.
    if (sampleData8BitsUnsigned.getSize() > static_cast<size_t>(sampleMaximumSizeInBytes)) {
        addError(juce::translate("The sample instrument is too large: ") + juce::String(sampleData8BitsUnsigned.getSize()));
        return;
    }

    // Creates a sample.
    currentSampleInstrumentSample = std::make_shared<Sample>(sampleData8BitsUnsigned);
}

void SongBuilder::setCurrentSampleInstrumentSample(const juce::String& sampleData8BitsUnsignedBase64)
{
    const auto stringPointer = sampleData8BitsUnsignedBase64.toUTF8();
    const auto utf8Size = juce::CharPointer_UTF8::getBytesRequiredFor(sampleData8BitsUnsignedBase64.getCharPointer());

    // If too large, the sample data should not be accepted.
    if (utf8Size > static_cast<size_t>(sampleMaximumSizeInBytes)) {
        addError(juce::translate("The sample instrument is too large: ") + juce::String(utf8Size));
        return;
    }

    // Source to InputStream. The pointer can be directly used, UTF8 is compatible with ASCII.
    const juce::MemoryBlock sourceMemoryBlock(stringPointer, utf8Size);
    juce::MemoryInputStream inputStream(sourceMemoryBlock, true);

    // Decodes the Base-64 stream.
    juce::MemoryOutputStream outputStream;
    auto success = Base64Util::decodeFromBase64(inputStream, outputStream);
    if (!success) {
        addError(juce::translate("The data could not be extract from the given Base64 string, for instrument index : ") + juce::String(currentInstrumentIndex));
        return;
    }

    // Creates a sample.
    const auto memoryBlock = outputStream.getMemoryBlock();
    currentSampleInstrumentSample = std::make_shared<Sample>(memoryBlock);
}


// Expressions.
// ==================================================

void SongBuilder::startNewExpression(const bool isArpeggio, const int expressionIndex, const juce::String& title) noexcept
{
    const auto& indexToExpression = isArpeggio ? indexToArpeggio : indexToPitch;

    // The slot must be free.
    if (indexToExpression.find(expressionIndex) != indexToExpression.cend()) {
        addError("Expression " + juce::String(expressionIndex) + " already exists!");
        jassertfalse;
        return;
    }
    // Expression 0 cannot be created.
    if (expressionIndex == 0) {
        addError("Expression 0 cannot be created!");
        jassertfalse;
        return;
    }

    // Resets the current Expression.
    currentExpressionIsArpeggio = isArpeggio;
    currentExpressionIndex = expressionIndex;
    currentExpression = Expression(isArpeggio, title, false);
    currentExpressionLoop = Loop();
}

void SongBuilder::finishCurrentExpression() noexcept
{
    // There must have been at least one value!
    if (currentExpression.getLength() <= 0) {
        addError("Expression " + juce::String(currentExpressionIndex) + " has not been added values!");
        jassertfalse;
        return;
    }

    // More convenient to set it at the end! Thus can be done even before adding the values.
    currentExpression.setLoop(currentExpressionLoop);

    // Stores the Expression.
    auto& indexToExpression = currentExpressionIsArpeggio ? indexToArpeggio : indexToPitch;
    indexToExpression[currentExpressionIndex] = currentExpression;
}

void SongBuilder::setCurrentExpressionSpeed(const int speed) noexcept
{
    currentExpression.setSpeed(speed);
}

void SongBuilder::setCurrentExpressionLoop(const int startIndex, const int endIndex) noexcept
{
    currentExpressionLoop = Loop(startIndex, endIndex, true);
}

void SongBuilder::addCurrentExpressionValue(const int value) noexcept
{
    currentExpression.addValue(value);
}

void SongBuilder::setExpressions(const bool isArpeggio, const int startIndex, const std::vector<std::unique_ptr<Expression>>& expressions) noexcept
{
    auto& indexToExpression = isArpeggio ? indexToArpeggio : indexToPitch;
    auto index = startIndex;
    for (const auto& expression : expressions) {
        indexToExpression[index++] = *expression;
    }
}

bool SongBuilder::doesExpressionExist(const bool isArpeggio, const int expressionIndex) const noexcept
{
    const auto& indexToExpression = isArpeggio ? indexToArpeggio : indexToPitch;
    return (indexToExpression.find(expressionIndex) != indexToExpression.cend());
}

}   // namespace arkostracker
