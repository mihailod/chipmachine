#pragma once

#include "../../song/instrument/Instrument.h"
#include "../../utils/Id.h"

namespace arkostracker
{

class SongController;

/** Generates an Instrument from a part of the Subsong. */
class GenerateInstrument final
{
public:
    /** Prevents instantiation. */
    GenerateInstrument() = delete;

    /**
     * Constructor.
     * @param songController the SongController, to access the Song and for notification.
     * @param subsongId the id of the Subsong. Must be valid.
     * @param positionIndex the position index to play.
     * @param firstLine the first line to generate from.
     * @param lastLine the first line to generate from.
     * @param channelIndex the channel index. Must be valid.
     * @param name the name of the new instrument.
     * @param encodeWithPeriods true to encode with periods, false with notes+shift.
     */
    static std::unique_ptr<Instrument> generateInstrument(const SongController& songController, const Id& subsongId, int positionIndex,
        int firstLine, int lastLine, int channelIndex,
        const juce::String& name, bool encodeWithPeriods) noexcept;
};

} // namespace arkostracker
