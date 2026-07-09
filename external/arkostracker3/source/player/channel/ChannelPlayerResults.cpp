#include "ChannelPlayerResults.h"

namespace arkostracker 
{

ChannelPlayerResults::ChannelPlayerResults(const OptionalInt pNewPlayedNote, const CellEffects& pEffects, OptionalId pInstrumentId,
                                           std::unique_ptr<ChannelOutputRegisters> pChannelOutputRegisters, SamplePlayInfo pSamplePlayInfo,
                                           const int pPlayedIndexInInstrument, const std::vector<Cell>& pOriginalCells) noexcept :
        newPlayedNote(pNewPlayedNote),
        effects(pEffects),
        instrumentId(std::move(pInstrumentId)),
        channelOutputRegisters(std::move(pChannelOutputRegisters)),
        samplePlayInfo(std::move(pSamplePlayInfo)),
        playedIndexInInstrument(pPlayedIndexInInstrument),
        cells(pOriginalCells),
        speed(),
        tick()
{
}

ChannelPlayerResults::ChannelPlayerResults() noexcept :
        newPlayedNote(),
        effects(),
        instrumentId(),
        channelOutputRegisters(std::make_unique<ChannelOutputRegisters>()),
        samplePlayInfo(SamplePlayInfo::buildNoSample()),
        playedIndexInInstrument(0),
        cells(),
        speed(),
        tick()
{
}

bool ChannelPlayerResults::isNewNotePlayed() const noexcept
{
    return newPlayedNote.isPresent();
}

OptionalInt ChannelPlayerResults::getNewPlayedNote() const noexcept
{
    return newPlayedNote;
}

bool ChannelPlayerResults::isEffectDeclared() const noexcept
{
    return !effects.isEmpty();
}

OptionalId ChannelPlayerResults::getInstrumentId() const noexcept
{
    return instrumentId;
}

const ChannelOutputRegisters& ChannelPlayerResults::getChannelOutputRegisters() const noexcept
{
    return *channelOutputRegisters;
}

const SamplePlayInfo& ChannelPlayerResults::getSamplePlayInfo() const noexcept
{
    return samplePlayInfo;
}

int ChannelPlayerResults::getPlayedIndexInInstrument() const noexcept
{
    return playedIndexInInstrument;
}

const CellEffects& ChannelPlayerResults::getEffects() const noexcept
{
    return effects;
}

const std::vector<Cell>& ChannelPlayerResults::getCells() const noexcept
{
    return cells;
}

OptionalInt ChannelPlayerResults::getSpeed() const noexcept
{
    return speed;
}

void ChannelPlayerResults::setSpeed(const int pSpeed) noexcept
{
    speed = pSpeed;
}

int ChannelPlayerResults::getTick() const noexcept
{
    return tick;
}

void ChannelPlayerResults::setTick(const int pTick) noexcept
{
    tick = pTick;
}

bool ChannelPlayerResults::isPsgDataEqual(const ChannelPlayerResults& other) const noexcept
{
    return ((newPlayedNote == other.newPlayedNote)
            && (effects == other.effects)
            && (instrumentId == other.instrumentId)
            && (*channelOutputRegisters == *other.channelOutputRegisters)
            // originalCell, cells and speed not compared... Probably not useful.
    );
}

}   // namespace arkostracker
