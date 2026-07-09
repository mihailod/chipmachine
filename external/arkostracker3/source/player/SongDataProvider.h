#pragma once

#include "../business/model/Loop.h"
#include "../business/song/tool/context/model/LineContext.h"
#include "../song/CellLocationInPosition.h"     // Warning about removal, but false positive.
#include "../song/instrument/InstrumentType.h"
#include "../song/instrument/psg/LowLevelPsgInstrumentCell.h"
#include "../song/instrument/sample/Sample.h"
#include "../song/sid/SidPlayerCapability.h"

namespace arkostracker 
{

class Location;

/**
 * Pure abstract for objects that can get Song data (Cells, Instrument data, Expression data, etc.)
 * without having classes knowing anything about a Song.
 */
class SongDataProvider
{
public:
    /** Holder of the speed/loop data of a PSG Instrument. */
    class PsgInstrumentMetadata
    {
    public:
        PsgInstrumentMetadata(const int pSpeed, const int pLoopStartIndex, const int pEndIndex, const bool pIsLooping) :
                speed(pSpeed),
                loopStartIndex(pLoopStartIndex),
                endIndex(pEndIndex),
                isLooping(pIsLooping)
        {
        }

        const int speed;
        const int loopStartIndex;
        const int endIndex;
        const bool isLooping;
    };

    /** Holds the speed/loop of an Expression. */
    class ExpressionMetadata
    {
    public:
        ExpressionMetadata(const int pSpeed, const int pLoopStartIndex, const int pEndIndex) :
                speed(pSpeed),
                loopStartIndex(pLoopStartIndex),
                endIndex(pEndIndex)
        {
        }

        const int speed;
        const int loopStartIndex;
        const int endIndex;
    };

    /** Holds one frame data for a PSG Instrument. */
    class PsgInstrumentFrameData
    {
    public:
        PsgInstrumentFrameData(const Loop& pLoop, const int pSpeed, const bool pIsInstrumentRetrig, const LowLevelPsgInstrumentCell& pCell) :
                loop(pLoop),
                speed(pSpeed),
                isInstrumentRetrig(pIsInstrumentRetrig),
                cell(pCell)
        {
        }

        /** @return an empty frame. */
        static PsgInstrumentFrameData buildEmpty() noexcept
        {
            return { Loop(), 0, false, LowLevelPsgInstrumentCell::getEmptyCell() };
        }

        const Loop loop;
        const int speed;
        const bool isInstrumentRetrig;
        const LowLevelPsgInstrumentCell cell;
    };

    /** Holds one frame data for a Sample Instrument. */
    class SampleInstrumentFrameData
    {
    public:
        /**
         * @param pLoop the loop.
         * @param pFrequencyHz the frequency of the replay, in Hz.
         * @param pAmplificationRatio the amplification ratio.
         * @param pSample may be null (unlikely).
         */
        SampleInstrumentFrameData(const Loop& pLoop, const int pFrequencyHz, const float pAmplificationRatio, std::shared_ptr<Sample> pSample) :
            loop(pLoop),
            frequencyHz(pFrequencyHz),
            amplificationRatio(pAmplificationRatio),
            sample(std::move(pSample))
        {
        }

        Loop loop;
        int frequencyHz;
        float amplificationRatio;
        const std::shared_ptr<Sample> sample;
    };


    /** Destructor. */
    virtual ~SongDataProvider() = default;

    /**
     * @return the Instrument ID from an index, if any. This locks all the Instruments, so be as quick as possible.
     * @param instrumentIndex the Instrument index.
     */
    virtual OptionalId getInstrumentIdFromAudioThread(int instrumentIndex) const noexcept = 0;

    /**
     * @return the type of Instrument. If not existing, a PSG type in returned.
     * This locks all the Instruments, so be as quick as possible.
     * @param instrumentId the Instrument ID, if found.
     */
    virtual InstrumentType getInstrumentTypeFromAudioThread(const OptionalId& instrumentId) const noexcept = 0;

    /**
     * @return the ID of the Expression from its index, if any.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionIndex the expression index. May be invalid.
     */
    virtual OptionalId getExpressionIdFromAudioThread(bool isArpeggio, int expressionIndex) const noexcept = 0;

    /**
     * @return the loop and speed of an expression. If not existing, 0 values are returned.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the expression ID. May be invalid.
     */
    virtual ExpressionMetadata getExpressionMetadataFromAudioThread(bool isArpeggio, const OptionalId& expressionId) const noexcept = 0;

    /**
     * @return data of one frame of a PSG Instrument. If not existing, default values are returned.
     * @param instrumentId the instrument ID. May be invalid.
     * @param cellIndex the cell index. May be invalid.
     */
    virtual PsgInstrumentFrameData getPsgInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId, int cellIndex) const noexcept = 0;

    /**
     * @return data of one frame of a Sample Instrument. If not existing, default values are returned.
     * @param instrumentId the instrument ID. May be invalid.
     */
    virtual SampleInstrumentFrameData getSampleInstrumentFrameDataFromAudioThread(const OptionalId& instrumentId) const noexcept = 0;

    /**
     * @return a value from an Expression. If not existing, a default value is returned.
     * @param isArpeggio true if Arpeggio, false if Pitch.
     * @param expressionId the expression ID. May be invalid or absent.
     * @param cellIndex the cell index. May be invalid.
     */
    virtual int getExpressionValueFromAudioThread(bool isArpeggio, const OptionalId& expressionId, int cellIndex) const noexcept = 0;

    /**
     * @return the PSG frequency and the reference frequency.
     * @param channelIndexInSong the channel in the song (may be >2).
     */
    virtual std::pair<int, float> getPsgFrequencyFromChannelFromAudioThread(int channelIndexInSong) const noexcept = 0;

    /**
     * @return the transposition for a channel, from the currently played position. It is safe: 0 is returned in case the channel doesn't exist anymore.
     * @param channelIndexInSong the channel in the song (may be >2).
     */
    virtual int getTranspositionFromAudioThread(int channelIndexInSong) const noexcept = 0;


    // Effect context.
    // ===========================================

    /** If false, the method from Effect Contex must not be called. The editor should use it, not the TUs for example. */
    virtual bool isEffectContextEnabled() const noexcept = 0;

    /**
     * Determines the effect context from what is being played. There may be no data.
     * @param location from where to determine the context.
     */
    virtual LineContext determineEffectContextFromAudioThread(CellLocationInPosition location) const noexcept = 0;

    /**
     * Determines the effect context what is being played, according that the currently playing location and the given channel. There may be none.
     * @param channelIndexInSong the channel in the song (may be >2).
     */
    virtual LineContext determineEffectContextFromAudioThread(int channelIndexInSong) const noexcept = 0;


    // SID.
    // ===========================================

    /** @return the SID player capability. */
    virtual SidPlayerCapability getSidPlayerCapability() const noexcept = 0;
};

}   // namespace arkostracker
