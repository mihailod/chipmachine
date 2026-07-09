#pragma once

#include "../../../../song/tracks/Track.h"

namespace arkostracker
{

class CellContext;

/** Optimizes the Track by normalizing each cell, and removing all useless effects (several volumes, etc.). */
class TrackOptimizer
{
public:
    /** Prevents instantiation. */
    TrackOptimizer() = delete;

    /**
     * Optimizes the track.
     * @param track the track to optimize.
     */
    static void optimizeTrack(Track& track) noexcept;

private:
    static const std::set<Effect> allPitchEffects;
    static const std::set<Effect> allArpeggiosEffects;
    static const std::set<Effect> volumeInAndOutEffects;

    static void managePitch(CellContext& currentContext, Cell& cell, Effect effect) noexcept;
    static void manageVolume(CellContext& currentContext, Cell& cell, Effect effect) noexcept;
    static void manageSpeed(CellContext& currentContext, Cell& cell, Effect effect) noexcept;
    static void manageArpeggio(CellContext& currentContext, Cell& cell, Effect effect) noexcept;
    static void manageReset(CellContext& currentContext, Cell& cell) noexcept;
    static void managePitchTable(CellContext& currentContext, Cell& cell) noexcept;

    /** Invalidates all the Pitch effects, except the given ones. */
    static void invalidatePitchesExcept(CellContext& context, const std::set<Effect>& effectsToIgnore = { }) noexcept;
    /** Invalidates all the Volume effects, except the given ones. */
    static void invalidateVolumesExcept(CellContext& context, const std::set<Effect>& effectsToIgnore = { }) noexcept;
    /** Invalidates all the effects, except the given ones. */
    static void invalidateEffectExcept(CellContext& context, const std::set<Effect>& effectsToInvalidate, const std::set<Effect>& effectsToIgnore) noexcept;
};

}   // namespace arkostracker
