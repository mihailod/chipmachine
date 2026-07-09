#include "MidiTrackReader.h"
#include "../../utils/NumberUtil.h"

namespace arkostracker 
{

MidiTrackReader::MidiTrackReader(const short pTimeFormat, const int pPpq, const int pDrumChannel, const bool pImportVelocities) noexcept :
    timeFormat(pTimeFormat),
    ppq(pPpq),
    programChangeToInstrumentIndex(),
    drumIndexToInstrumentIndex(),
    highestInstrument(0),
    cellIndexToTimeSignature(),
    cellIndexToBpm(),
    instrumentIndexToMidiInstrument(),
    lastCellIndex(0),
    drumChannel(pDrumChannel),
    importVelocities(pImportVelocities)
{
}

std::map<int, Cell> MidiTrackReader::readMidiTrack(const juce::MidiMessageSequence& midiMessageSequence, const bool isConductorTrack) noexcept
{
    std::map<int, Cell> cellIndexToCell;
    auto currentProgramChange = 0;

    // Browses each midi event.
    for (auto midiEventHolder : midiMessageSequence) {
        auto midiMessage = midiEventHolder->message;

        // To what Cell index this event can be related to?
        const auto timeStamp = midiMessage.getTimeStamp();
        const auto cellIndex = static_cast<int>(timeStamp / static_cast<double>(ppq));

        // What kind of event? If conductor Track, notes and PC events are ignored.
        if (!isConductorTrack && midiMessage.isNoteOn(false)) {
            onNoteOnFound(cellIndexToCell, midiMessage, cellIndex, currentProgramChange);
        } else if (!isConductorTrack && midiMessage.isProgramChange()) {
            onProgramChangeFound(midiMessage, currentProgramChange);
        } else if (midiMessage.isTimeSignatureMetaEvent()) {
            onTimeSignatureFound(midiMessage, cellIndex);
        } else if (midiMessage.isTempoMetaEvent()) {
            onTempoFound(midiMessage, cellIndex);
        }
    }

    return cellIndexToCell;
}

void MidiTrackReader::onNoteOnFound(std::map<int, Cell>& cellIndexToCell, const juce::MidiMessage& midiMessage, const int cellIndex,
    const int programChange) noexcept
{
    jassert(midiMessage.isNoteOn(false));

    // Nothing to do if the slot is already used.
    if (cellIndexToCell.find(cellIndex) != cellIndexToCell.cend()) {
        return;
    }

    updateLastCellIndex(cellIndex);         // This cell is maybe the largest one in the song.

    const int noteNumber = midiMessage.getNoteNumber();

    // Is it a drum? Special case.
    if (isDrum(midiMessage)) {
        onNoteOnFoundForDrum(cellIndexToCell, midiMessage, cellIndex);
        return;
    }

    // Does an Instrument already exist about this Program Change?
    int instrumentIndex;
    auto iterator = programChangeToInstrumentIndex.find(programChange);
    if (iterator != programChangeToInstrumentIndex.cend()) {
        // The PC is already linked to an Instrument.
        instrumentIndex = iterator->second;
    } else {
        // This PC has never been met before. Creates a new entry with a new Instrument.
        instrumentIndex = ++highestInstrument;
        programChangeToInstrumentIndex.emplace(programChange, instrumentIndex);

        // What type of AY instrument to create?
        auto instrumentType = findMidiInstrumentDataForNote(programChange);
        instrumentIndexToMidiInstrument.insert(std::make_pair(instrumentIndex, instrumentType));
    }

    encodeCell(cellIndexToCell, cellIndex, noteNumber, instrumentIndex, midiMessage);
}

void MidiTrackReader::encodeCell(std::map<int, Cell>& cellIndexToCell, int cellIndex, const int noteNumber, const int instrumentIndex, const juce::MidiMessage& midiMessage) const noexcept
{
    Cell cell(Note::buildNote(noteNumber), instrumentIndex);
    
    // Encodes the velocity, if wanted.
    if (importVelocities) {
        const auto velocity = static_cast<int>(midiMessage.getVelocity());
        auto volume4bits = NumberUtil::toLog(velocity, 0x7f, 0xf);
        volume4bits = NumberUtil::correctNumber(volume4bits, 1, 0xf);       // Low values can return 0, we don't want this.
        cell.addEffect(Effect::volume, volume4bits);
    }

    cellIndexToCell.emplace(cellIndex, cell);
}

void MidiTrackReader::onNoteOnFoundForDrum(std::map<int, Cell>& cellIndexToCell, const juce::MidiMessage& midiMessage, int cellIndex) noexcept
{
    jassert(midiMessage.getChannel() == drumChannel);
    jassert(midiMessage.isNoteOn());

    const int noteIndex = midiMessage.getNoteNumber();

    // Does an instrument exist for this drum note?
    int instrumentIndex;
    auto iterator = drumIndexToInstrumentIndex.find(noteIndex);
    if (iterator != drumIndexToInstrumentIndex.cend()) {
        // The drum is already linked to an Instrument.
        instrumentIndex = iterator->second;
    } else {
        // This drum note has never been met before. Creates a new entry with a new Instrument.
        instrumentIndex = ++highestInstrument;
        drumIndexToInstrumentIndex.emplace(noteIndex, instrumentIndex);

        // What type of drum instrument to create?
        auto instrumentType = findMidiInstrumentDataForDrum(noteIndex);
        instrumentIndexToMidiInstrument.insert(std::make_pair(instrumentIndex, instrumentType));
    }
    
    encodeCell(cellIndexToCell, cellIndex, 4 * 12, instrumentIndex, midiMessage);          // The note is only a convention...
}

void MidiTrackReader::onProgramChangeFound(const juce::MidiMessage& midiMessage, int& currentProgramChange) noexcept
{
    jassert(midiMessage.isProgramChange());

    // Only stores it for now. It will be used when a note is found.
    // Two reasons:
    // - Some PC may be declared, but not used.
    // - It helps the management of the drums.
    currentProgramChange = midiMessage.getProgramChangeNumber();
}

void MidiTrackReader::onTimeSignatureFound(const juce::MidiMessage& midiMessage, const int cellIndex) noexcept
{
    jassert(midiMessage.isTimeSignatureMetaEvent());

    // Nothing to do if the slot is already used.
    if (cellIndexToTimeSignature.find(cellIndex) != cellIndexToTimeSignature.cend()) {
        return;
    }

    // Gets the time signature.
    std::pair<int, int> timeSignature;
    midiMessage.getTimeSignatureInfo(timeSignature.first, timeSignature.second);
    // Stores it.
    cellIndexToTimeSignature.emplace(cellIndex, timeSignature);
}

void MidiTrackReader::onTempoFound(const juce::MidiMessage& midiMessage, int cellIndex) noexcept
{
    jassert(midiMessage.isTempoMetaEvent());

    // Nothing to do if the slot is already used.
    if (cellIndexToBpm.find(cellIndex) != cellIndexToBpm.cend()) {
        return;
    }

    updateLastCellIndex(cellIndex);         // This cell is maybe the largest one in the song.

    const double tickLengthSeconds = midiMessage.getTempoMetaEventTickLength(timeFormat);
    // Rather empirical, but works!
    const auto bpm = 60.0 / (tickLengthSeconds * static_cast<double>(timeFormat));

    cellIndexToBpm.emplace(cellIndex, static_cast<int>(bpm));
}

const std::map<int, int>& MidiTrackReader::getCellIndexToBpm() const noexcept
{
    return cellIndexToBpm;
}

int MidiTrackReader::getLastCellIndex() const noexcept
{
    return lastCellIndex;
}

void MidiTrackReader::updateLastCellIndex(const int cellIndex) noexcept
{
    // Updates only if relevant.
    if (cellIndex > lastCellIndex) {
        lastCellIndex = cellIndex;
    }
}

std::vector<int> MidiTrackReader::buildPatternHeights() const noexcept
{
    // Builds the raw patterns.
    auto rawPatternHeightsAndSignatureAdvisedHeight = buildRawPatternHeights();

    const int maximumPracticalHeight = 96;

    // They might not be practical (too short). Try to regroup them into 48 lines at least.
    std::vector<int> patternHeights;
    auto accumulatedHeight = 0;
    auto lastIdealHeight = 64;       // Only a fallback value, should never be used.
    for (auto rawPatternHeightAndSignatureAdvisedHeight : rawPatternHeightsAndSignatureAdvisedHeight) {
        const auto rawPatternHeight = rawPatternHeightAndSignatureAdvisedHeight.first;
        const auto signatureAdvisedHeight = rawPatternHeightAndSignatureAdvisedHeight.second;
        const auto idealHeight = findIdealHeight(signatureAdvisedHeight);
        lastIdealHeight = idealHeight;

        if (rawPatternHeight >= idealHeight) {
            // The read pattern is high enough. It should be encoded as-is
            // But can it be mixed with the accumulated height?
            if ((accumulatedHeight + rawPatternHeight) <= maximumPracticalHeight) {
                // Yes, it can.
                patternHeights.emplace_back(accumulatedHeight + rawPatternHeight);
                accumulatedHeight = 0;
            } else {
                // No, they can't be mixed. Encodes them separately.
                patternHeights.emplace_back(accumulatedHeight);
                patternHeights.emplace_back(rawPatternHeight);
                accumulatedHeight = 0;
            }
        } else {
            // The read pattern is not quite high enough.
            // But maybe the accumulated height is high enough.
            accumulatedHeight += rawPatternHeight;
            if (accumulatedHeight >= idealHeight) {
                patternHeights.emplace_back(accumulatedHeight);
                accumulatedHeight = 0;
            }
        }
    }

    // Some heights remaining?
    if (accumulatedHeight > 0) {
        // Use the ideal height, cleaner than just the accumulated which will create half-finished patterns.
        patternHeights.emplace_back(lastIdealHeight);
    }

    return patternHeights;
}

std::vector<std::pair<int, int>> MidiTrackReader::buildRawPatternHeights() const noexcept
{
    std::vector<std::pair<int, int>> patternHeightAndSignatureHeights;

    // This is a raw pass, builds a simple list of heights. Encodes one as soon as the advised height is reached, or a new time signature is found.

    auto timeSignatureNumerator = 4;         // Default values.
    auto timeSignatureDenominator = 4;
    auto advisedHeight = calculatePatternAdvisedHeight(timeSignatureNumerator, timeSignatureDenominator);

    // Browses through each cell. This is inefficient, but clearly more simple!
    int currentPatternHeight = 0;
    for (int cellIndex = 0; cellIndex <= lastCellIndex; ++cellIndex) {
        // Is there a time signature here?
        auto iterator = cellIndexToTimeSignature.find(cellIndex);
        if (iterator != cellIndexToTimeSignature.cend()) {
            // New time signature.
            timeSignatureNumerator = iterator->second.first;
            timeSignatureDenominator = iterator->second.second;
            advisedHeight = calculatePatternAdvisedHeight(timeSignatureNumerator, timeSignatureDenominator);
            // Encodes the current Pattern, unless it was empty.
            if (currentPatternHeight > 0) {
                patternHeightAndSignatureHeights.emplace_back(currentPatternHeight, advisedHeight);
                currentPatternHeight = 0;
            }
        }

        // If we have reached the advised height, encodes the Pattern.
        if (++currentPatternHeight >= advisedHeight) {
            patternHeightAndSignatureHeights.emplace_back(currentPatternHeight, advisedHeight);
            currentPatternHeight = 0;
        }
    }

    // Finally, encodes the last pattern.
    if (currentPatternHeight > 0) {
        patternHeightAndSignatureHeights.emplace_back(currentPatternHeight, advisedHeight);
    }

    return patternHeightAndSignatureHeights;
}

int MidiTrackReader::calculatePatternAdvisedHeight(const int timeSignatureNumerator, const int timeSignatureDenominator) noexcept
{
    auto lineCount = timeSignatureNumerator * timeSignatureDenominator;
    return NumberUtil::correctNumber(lineCount, 8, 128);
}

int MidiTrackReader::findIdealHeight(const int signatureHeight) noexcept
{
    // Tries to have a large pattern, but not too much.
    for (auto multiplier = 8; multiplier >= 1; multiplier /= 2) {
        const auto multipliedHeight = (signatureHeight * multiplier);
        if ((multipliedHeight >= 8) && (multipliedHeight <= 64)) {      // Arbitrary : not too large, not too small.
            return multipliedHeight;
        }
    }

    // Fallback, don't change it.
    return signatureHeight;
}

const std::map<int, MidiTrackReader::MidiInstrument>& MidiTrackReader::getInstrumentIndexToMidiInstrumentType() const noexcept
{
    return instrumentIndexToMidiInstrument;
}

MidiTrackReader::MidiInstrument MidiTrackReader::findMidiInstrumentDataForDrum(const int noteNumber) noexcept
{
    // Only a few AY instruments can be created.
    MidiInstrumentType midiInstrumentType;
    switch (noteNumber) {
    case 35:
    case 36:
        midiInstrumentType = MidiInstrumentType::drumBassdrum;
        break;
    case 38:
    case 40:
        midiInstrumentType = MidiInstrumentType::drumSnare;
        break;
    case 42:
    case 46:
    case 54:
        midiInstrumentType = MidiInstrumentType::drumHihat;
        break;
    case 49:
    case 51:
    case 52:
    case 55:
    case 57:
    case 59:
        midiInstrumentType = MidiInstrumentType::drumCrash;
        break;
    case 41:
    case 45:
        midiInstrumentType = MidiInstrumentType::drumTomLow;
        break;
    case 47:
    case 48:
        midiInstrumentType = MidiInstrumentType::drumTomMedium;
        break;
    case 43:
    case 50:
        midiInstrumentType = MidiInstrumentType::drumTomHigh;
        break;
    default:
        midiInstrumentType = MidiInstrumentType::drumGeneric;
        break;
    }

    // Finds the possible MIDI name of the drum.
    auto* genericMidiName = juce::MidiMessage::getRhythmInstrumentName(noteNumber);
    auto drumName = (genericMidiName != nullptr) ? genericMidiName : juce::translate("Generic drum");

    return { midiInstrumentType, drumName };
}


MidiTrackReader::MidiInstrument MidiTrackReader::findMidiInstrumentDataForNote(const int programChange) noexcept
{
    // A normal instrument. The Program change determines what kind of instrument to generate.
    MidiInstrumentType midiInstrumentType;
    switch (programChange + 1) {               // +1 because Juce goes from 0-127, but MIDI ids from 1-128. See https://www.midi.org/specifications-old/item/gm-level-1-sound-set
        // Long sound?
        case 17:            // Organs.
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 30:            // Guitars.
        case 31:
        case 34:            // Basses.
        case 35:
        case 36:
        case 39:
        case 40:
        case 41:            // Strings.
        case 42:
        case 43:
        case 44:
        case 45:
        case 49:
        case 50:
        case 51:
        case 52:
        case 53:
        case 54:
        case 55:
        case 57:            // Horns.
        case 58:
        case 59:
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:            // Saxs.
        case 66:
        case 67:
        case 68:
        case 69:
        case 70:
        case 71:
        case 72:
        case 73:
        case 74:
        case 75:
        case 76:
        case 77:
        case 78:
        case 79:
        case 80:
        case 81:            // Leads
        case 82:
        case 83:
        case 84:
        case 85:
        case 86:
        case 87:
        case 88:
        case 89:            // Pads.
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
            midiInstrumentType = MidiInstrumentType::longSound;
            break;
        default:
            midiInstrumentType = MidiInstrumentType::shortSound;
            break;
    }

    // Finds the possible MIDI name of the PC.
    auto* instrumentMidiName = juce::MidiMessage::getGMInstrumentName(programChange);
    auto instrumentName = (instrumentMidiName != nullptr) ? instrumentMidiName : juce::translate("Generic instrument");

    return { midiInstrumentType, instrumentName };
}

bool MidiTrackReader::isDrum(const juce::MidiMessage& midiMessage) const noexcept
{
    return (midiMessage.getChannel() == drumChannel);
}

void MidiTrackReader::setLastCellIndex(const int newLastCellIndex) noexcept
{
    lastCellIndex = newLastCellIndex;
}

void MidiTrackReader::addCellIndexToTimeSignature(int cellIndex, const std::pair<int, int>& timeSignature) noexcept
{
    cellIndexToTimeSignature.insert(std::pair(cellIndex, timeSignature));
}


}   // namespace arkostracker

