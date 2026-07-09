#include "GenerateInstrument.h"

#include "../../controllers/SongController.h"
#include "../../player/SongPlayer.h"
#include "../../ui/tools/streamedMusicAnalyzer/InstrumentStreamExporter.h"

namespace arkostracker
{

std::unique_ptr<Instrument> GenerateInstrument::generateInstrument(const SongController& songController, const Id& subsongId, const int positionIndex,
    const int firstLine, const int lastLine, const int channelIndex,
    const juce::String& name, const bool encodeWithPeriods) noexcept
{
    const auto psgCount = songController.getPsgCount(subsongId);
    const auto channelCount = PsgValues::getChannelCount(psgCount);
    if (channelIndex >= channelCount) {
        jassertfalse;           // Channel count invalid!
        return nullptr;
    }
    const auto targetPsg = PsgValues::getPsgIndex(channelIndex);

    SongPlayer songPlayer(songController.getConstSong());

    const Location startLocation(subsongId, positionIndex, firstLine);
    const Location pastEndLocation(subsongId, positionIndex, lastLine + 1);
    songPlayer.play(startLocation, startLocation, pastEndLocation, true, true, false);

    std::vector<PsgRegisters> psgRegistersFrames;

    // Reads the part of the song, stacks up the registers.
    songPlayer.setOfflineSongEndCountBeforeMuting(1);
    while (!songPlayer.hasOfflineSongEndCountReached()) {
        // Plays one tick on all the PSGs of the Song (even if only fewer PSGs are needed), but keeps only the result of the target one.
        for (auto psgIndex = 0; psgIndex < psgCount; ++psgIndex) {
            const auto& [tempPsgRegisters, sampleData] = songPlayer.getNextRegisters(psgIndex);
            if (psgIndex == targetPsg) {
                psgRegistersFrames.emplace_back(*tempPsgRegisters);
            }
        }
    }

    // Retrieves the PSG info.
    const auto psg = songController.getPsgs(subsongId).at(static_cast<size_t>(targetPsg));
    const auto psgFrequencyHz = psg.getPsgFrequency();
    const auto referenceFrequencyHz = psg.getReferenceFrequency();

    return InstrumentStreamExporter::exportStreamAsInstrument(name, channelIndex, psgRegistersFrames, false, encodeWithPeriods,
        psgFrequencyHz, psgFrequencyHz, referenceFrequencyHz);
}

} // namespace arkostracker
