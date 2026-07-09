#include "Stk128Importer.h"

#include "../../utils/MemoryBlockUtil.h"
#include "Stk128Constants.h"
#include "SubsongImporter.h"
#include "../common/AmsdosHeaderUtil.h"

namespace arkostracker 
{

const juce::String Stk128Importer::extensionStk = "128";                // NOLINT(cert-err58-cpp, *-statically-constructed-objects)

Stk128Importer::Stk128Importer() noexcept :
        songBuilder(),
        instrumentsWithSFlagIndexes()
{
}


// SongImporter method implementations.
// =======================================

bool Stk128Importer::doesFormatMatch(juce::InputStream& inputStream, const juce::String& extension) const noexcept
{
    // The extension MUST be .128!
    if (extension != extensionStk) {
        return false;
    }

    // There is not much we can do to make sure it is a valid Soundtrakker file, as there is no header.

    // The song should be at least the size till the Patterns.
    return (inputStream.getTotalLength() >= Stk128Constants::offsetPatternsData);
}

std::unique_ptr<SongImporter::Result> Stk128Importer::loadSong(juce::InputStream& inputStream, const ImportConfiguration& /*configuration*/) noexcept
{
    songBuilder = std::make_unique<SongBuilder>();

    // Parses the Song, removes the possible Amsdos header.
    const auto originalMusicData = MemoryBlockUtil::fromInputStream(inputStream);
    const auto musicData = AmsdosHeaderUtil::removeHeaderIfNeeded(originalMusicData);
    parse(musicData);

    // Gets the result from the Builder and returns it.
    auto [song, report] = songBuilder->buildSong();

    return std::make_unique<Result>(std::move(song), std::move(report));
}


// =======================================

void Stk128Importer::parse(const juce::MemoryBlock& musicData) noexcept
{
    // Resets the variables.
    instrumentsWithSFlagIndexes.clear();

    bool success;        // NOLINT(*-init-variables)
    const auto title = MemoryBlockUtil::extractString(musicData, Stk128Constants::offsetSongName, Stk128Constants::songNameSize, success);
    if (!success) {
        songBuilder->addError("Unable to read the name of the song.");
        return;
    }

    // First, reads the Instruments, Arpeggios and Patterns.
    success = success && readAndCreateAllInstruments(musicData);
    success = success && readAndCreateAllArpeggios(musicData);
    success = success && readAndCreateSubsong(musicData);

    if (!success) {
        return;
    }

    songBuilder->setSongMetaData(title, juce::String(), juce::String(), juce::translate("Converted with Arkos Tracker."));
}

bool Stk128Importer::readAndCreateAllInstruments(const juce::MemoryBlock& musicData) noexcept
{
    bool success;           // NOLINT(*-init-variables)
    for (auto instrumentIndex = 0; instrumentIndex < Stk128Constants::instrumentCount; ++instrumentIndex) {
        success = readAndCreateInstrument(instrumentIndex, musicData);

        // Stops in case of failure.
        if (!success) {
            songBuilder->addError("Unable to read the instrument " + juce::String(instrumentIndex));
            break;
        }
    }

    return success;
}

bool Stk128Importer::readAndCreateInstrument(const int soundtrakkerInstrumentIndex, const juce::MemoryBlock& memoryBlock) noexcept
{
    jassert(songBuilder != nullptr);            // Else, boom!

    const auto destinationInstrumentIndex = soundtrakkerInstrumentIndex + 1;               // + 1 because the instrument 0 is reserved for AT!

    // Gets the offset on the data of the instrument.
    const auto offsetInstrumentData = Stk128Constants::offsetInstruments + (Stk128Constants::instrumentSize * soundtrakkerInstrumentIndex);

    // Gets the title.
    bool success;    // NOLINT(*-init-variables)
    const auto title = MemoryBlockUtil::extractString(memoryBlock, Stk128Constants::offsetInstrumentNames + (soundtrakkerInstrumentIndex * Stk128Constants::instrumentNameSize),
                                                      Stk128Constants::instrumentNameSize, success);
    if (!success) {
        songBuilder->addError("Unable to read the name for the instrument " + juce::String(soundtrakkerInstrumentIndex));
        return false;
    }

    auto currentVolume = 0;
    auto currentNoise = 0;

    // Reads the repeat/length flags.
    const auto rawRepeatPositionAndFlags = MemoryBlockUtil::extractUnsignedChar(memoryBlock, offsetInstrumentData + Stk128Constants::offsetRepeatFlagsInInstrument, success);
    int repeatPosition;    // NOLINT(*-init-variables)
    if (!success) {
        songBuilder->addWarning("Unable to read the repeat Position for the instrument " + juce::String(soundtrakkerInstrumentIndex));
        repeatPosition = 0;
    } else {
        repeatPosition = static_cast<int>(rawRepeatPositionAndFlags & 0x1fU);
    }

    // The bit 7 forces the period to "0". We can't emulate this, but we setting the sound to off does not work well on drums.
    // What we do is that if no pitch is set, we set the sound to off. If there is a pitch, the sound is set to on, and the period is forced.
    // The bit 6 (N/S) forces the volume to 15 whenever the Instrument is read. This doesn't exist in AT, but we can trick it anyway.
    // For now we only store the index of such Instruments.
    const auto forceSoundToOffIfPossible = ((rawRepeatPositionAndFlags & 128U) != 0U);
    const auto forceFullVolume = ((rawRepeatPositionAndFlags & 64U) != 0U);
    if (forceFullVolume) {
        instrumentsWithSFlagIndexes.insert(destinationInstrumentIndex);
    }

    auto repeatLength = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryBlock, offsetInstrumentData + Stk128Constants::offsetRepeatLengthInInstrument, success));
    if (!success || (repeatLength > 32)) {
        songBuilder->addWarning("Repeat length is invalid: " + juce::String(repeatLength) + "for the instrument " + juce::String(soundtrakkerInstrumentIndex));
        repeatLength = 0;
    }
    const auto repeatEnd = repeatPosition + repeatLength - 1;      // Invalid if repeatLength = 0, but we don't use it in that case because there is no loop if repeatLength = 0.
    // The looping must be within boundaries.
    if ((repeatPosition + repeatLength) > 32) {             // 32, not 31 because the length is actually +1. (because 0 means "no loop").
        songBuilder->addError("The repeat position and length are invalid for the instrument " + juce::String(soundtrakkerInstrumentIndex));
        return false;
    }
    const auto isLooping = (repeatLength > 0);

    // Starts creating the Instrument.
    songBuilder->startNewPsgInstrument(destinationInstrumentIndex);
    songBuilder->setCurrentInstrumentName(title);
    songBuilder->setCurrentPsgInstrumentSpeed(0);            // Always 0.

    // The loop of Soundtrakker is strange: it makes one full go, then loop on a part.
    // We have to store the cells to loop, to put them at the end of the instrument.
    std::vector<PsgInstrumentCell> loopableCells;

    // Reads the instrument data itself. There are always only 16 items.
    for (int cellIndex = 0; cellIndex < 32; ++cellIndex) {
        //int volume = MemoryBlockUtil::extractChar
        const auto encodedVolumeAndSound = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryBlock, offsetInstrumentData + Stk128Constants::offsetVolumeTableInInstrument + cellIndex,
                                                                         success));
        if (!success) {
            songBuilder->addError("Unable to read the volume for the instrument " + juce::String(soundtrakkerInstrumentIndex));
            return false;
        }
        const auto encodedNoise = static_cast<int>(MemoryBlockUtil::extractUnsignedChar(memoryBlock, offsetInstrumentData + Stk128Constants::offsetNoiseTableInInstrument + cellIndex, success));
        if (!success) {
            songBuilder->addError("Unable to read the volume for the instrument " + juce::String(soundtrakkerInstrumentIndex));
            return false;
        }

        // Should we encode these values? Bit 7 to 1 means "ignore", in which case the last value is used.
        if ((static_cast<unsigned int>(encodedVolumeAndSound) & 128U) == 0U) {
            currentVolume = static_cast<int>(static_cast<unsigned int>(encodedVolumeAndSound) & 0xfU);
        }
        if ((static_cast<unsigned int>(encodedNoise) & 128U) == 0U) {
            currentNoise = static_cast<int>(static_cast<unsigned int>(encodedNoise) & 0x1fU);
        }

        // Reads the Tone.
        int tone = MemoryBlockUtil::extractSignedWord(memoryBlock, offsetInstrumentData + Stk128Constants::offsetToneTableInInstrument + cellIndex * 2,
                                                      success);    // * 2 because two bytes encoded.

        // Encodes the line.
        // A way to simulate the "R/A" flag of the Soundtrakker: if "A" flag, the sound is off, unless there is a tone, in which case the tone is forced as a software period (and not as a pitch).
        // That way, we simulate the "period to 0 + tone".
        PsgInstrumentCellLink link = PsgInstrumentCellLink::softOnly;
        auto softwarePeriod = 0;         // "auto" by default.
        if (forceSoundToOffIfPossible) {
            if (tone == 0) {
                // No tone. We cut the sound.
                link = PsgInstrumentCellLink::noSoftNoHard;
            } else {
                // There is a tone. Sound on, and the period is forced to the tone (and the tone is set to off).
                softwarePeriod = tone;
                tone = 0;
            }
        }

        // Creates the Cell. The period may be "auto" or not. The pitch is inverted, because more user-friendly this way in AT.
        songBuilder->startNewPsgInstrumentCell();
        songBuilder->setCurrentPsgInstrumentCellLink(link);
        songBuilder->setCurrentPsgInstrumentCellVolume(currentVolume);
        songBuilder->setCurrentPsgInstrumentCellNoise(currentNoise);
        songBuilder->setCurrentPsgInstrumentCellPrimaryPeriod(softwarePeriod);
        songBuilder->setCurrentPsgInstrumentCellPrimaryPitch(-tone);
        songBuilder->finishCurrentPsgInstrumentCell();

        // In case of a looping mode, if this cell is within the loop section, we create and store the Cell.
        if ((isLooping) && (cellIndex >= repeatPosition) && (cellIndex <= repeatEnd)) {
            // Creates the Cell.
            const auto loopCell = PsgInstrumentCell::buildSoftwareCell(currentVolume, currentNoise, 0, 0, tone,
                                                                 (link == PsgInstrumentCellLink::softOnly));
            loopableCells.push_back(loopCell);
        }
    }

    // Encodes the repeat and end index of the Instrument.
    if (!isLooping) {
        // If there is no loop, simply loops the last Cell.
        const auto lastIndex = 31;
        songBuilder->setCurrentPsgInstrumentMainLoop(lastIndex, lastIndex, true);      // Always looping on STK.
    } else {
        // If there is a loop, the Cells within the loop are added to the end, and we loop on them.
        for (const PsgInstrumentCell& cell : loopableCells) {
            songBuilder->startNewPsgInstrumentCell();
            songBuilder->setCurrentPsgInstrumentCellLink(cell.getLink());
            songBuilder->setCurrentPsgInstrumentCellVolume(cell.getVolume());
            songBuilder->setCurrentPsgInstrumentCellNoise(cell.getNoise());
            songBuilder->setCurrentPsgInstrumentCellPrimaryPeriod(cell.getPrimaryPeriod());
            songBuilder->setCurrentPsgInstrumentCellPrimaryPitch(cell.getPrimaryPitch());
            songBuilder->finishCurrentPsgInstrumentCell();

            if (!success) {
                songBuilder->addError("Unable to create the instrument cell for the loop.");
                return false;
            }
        }
        // Loops just after the "normal" sound, for how long it was defined.
        songBuilder->setCurrentPsgInstrumentMainLoop(32, 32 + static_cast<int>(loopableCells.size()) - 1, true);
    }

    // Stores the Instrument.
    songBuilder->finishCurrentInstrument();
    
    return songBuilder->isOk();
}

bool Stk128Importer::readAndCreateAllArpeggios(const juce::MemoryBlock& musicData) const noexcept
{
    bool success;           // NOLINT(*-init-variables)
    for (auto arpeggioIndex = 0; arpeggioIndex < Stk128Constants::arpeggioCount; ++arpeggioIndex) {
        success = readAndCreateArpeggio(arpeggioIndex, musicData);

        // Stops in case of failure.
        if (!success) {
            songBuilder->addError("Unable to read the arpeggio " + juce::String(arpeggioIndex));
            break;
        }
    }

    return success;
}

bool Stk128Importer::readAndCreateArpeggio(const int originalArpeggioIndex, const juce::MemoryBlock& musicData) const noexcept
{
    const auto destinationArpeggioIndex = originalArpeggioIndex + 1;      // +1 because the Arpeggio 0 is reserved in AT3.

    // Gets the offset on the data of the arpeggio.
    const auto offsetArpeggioData = Stk128Constants::offsetArpeggios + (Stk128Constants::arpeggioSize * originalArpeggioIndex);

    songBuilder->startNewExpression(true, destinationArpeggioIndex, "Arpeggio " + juce::String(destinationArpeggioIndex));

    // Speed is always 0.
    // For the looping, loops the whole arpeggio. This is a raw solution, but in Soundtrakker, the Arpeggio length depends on the sound that uses it,
    // which we can't do (and don't want to).
    songBuilder->setCurrentExpressionSpeed(0);

    // Reads all the values and encodes them.
    bool success;    // NOLINT(*-init-variables)
    for (auto i = 0; i < Stk128Constants::arpeggioSize; ++i) {
        const auto arpeggioValue = MemoryBlockUtil::extractSignedByte(musicData, offsetArpeggioData + i, success);
        if (!success) {
            songBuilder->addError("Error while reading the arpeggio value of offset " + juce::String(i) + "for the arpeggio " + juce::String(originalArpeggioIndex));
            return false;
        }
        songBuilder->addCurrentExpressionValue(arpeggioValue);
    }
    songBuilder->setCurrentExpressionLoop(0, Stk128Constants::arpeggioSize - 1);

    // Stores the Arpeggio.
    songBuilder->finishCurrentExpression();

    return songBuilder->isOk();
}

bool Stk128Importer::readAndCreateSubsong(const juce::MemoryBlock& musicData) const noexcept
{
    auto& subsongBuilder = songBuilder->startNewSubsong(0);

    // Lets another object builds the Tracks.
    SubsongImporter subsongImporter(*songBuilder, subsongBuilder, musicData, instrumentsWithSFlagIndexes);
    subsongImporter.parse();

    songBuilder->finishCurrentSubsong();

    return songBuilder->isOk();
}

ImportedFormat Stk128Importer::getFormat() noexcept
{
    return ImportedFormat::soundtrakker128;
}


}   // namespace arkostracker

