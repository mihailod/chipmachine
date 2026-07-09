#include "TrackOptimizer.h"

#include "../../../../utils/PsgValues.h"
#include "../CellOperations.h"
#include "../context/model/CellContext.h"

namespace arkostracker
{

const std::set<Effect> TrackOptimizer::allPitchEffects = {         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
    Effect::pitchDown,
    Effect::pitchUp,
    Effect::fastPitchDown,
    Effect::fastPitchUp,
    Effect::pitchGlide,
};

const std::set<Effect> TrackOptimizer::allArpeggiosEffects = {         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
    Effect::arpeggioTable,
    Effect::arpeggio3Notes,
    Effect::arpeggio4Notes,
};

const std::set<Effect> TrackOptimizer::volumeInAndOutEffects = {         // NOLINT(cert-err58-cpp, *-statically-constructed-objects)
    Effect::volumeIn,
    Effect::volumeOut,
};

void TrackOptimizer::optimizeTrack(Track& track) noexcept
{
    CellContext currentContext;

    for (auto cellIndex = 0; cellIndex < Track::getSize(); ++cellIndex) {
        constexpr NormalizeCell normalizeCell;

        // Normalizes the cells.
        auto cell = track.getCell(cellIndex);

        if (const auto normalizedCell = normalizeCell(cell); normalizedCell != nullptr) {
            // Any change?
            track.setCell(cellIndex, *normalizedCell);
            cell = *normalizedCell;
        }

        // If a note is present, many effects stops.
        if (cell.isNoteAndInstrument()) {
            currentContext.resetEffects({ Effect::volumeOut, Effect::volumeIn });
            currentContext.resetEffects(allPitchEffects);
            // After a note, there is no telling what is the speed of the new instrument.
            currentContext.invalidateEffects({ Effect::forceInstrumentSpeed, Effect::forceArpeggioSpeed, Effect::forcePitchTableSpeed });
        } else if (cell.isNote()) {
            // Legato.
            currentContext.resetEffects(allPitchEffects);
        }

        // Reset.
        manageReset(currentContext, cell);

        // Volumes. Order matters, Set Volume is first.
        manageVolume(currentContext, cell, Effect::volume);
        manageVolume(currentContext, cell, Effect::volumeOut);
        manageVolume(currentContext, cell, Effect::volumeIn);

        // Pitches. Order shouldn't matter, since only one pitch per line is authorized.
        managePitch(currentContext, cell, Effect::pitchUp);
        managePitch(currentContext, cell, Effect::pitchDown);
        managePitch(currentContext, cell, Effect::fastPitchUp);
        managePitch(currentContext, cell, Effect::fastPitchDown);
        managePitch(currentContext, cell, Effect::pitchGlide);

        // Speeds
        manageSpeed(currentContext, cell, Effect::forceInstrumentSpeed);
        manageSpeed(currentContext, cell, Effect::forceArpeggioSpeed);
        manageSpeed(currentContext, cell, Effect::forcePitchTableSpeed);

        // Arpeggios.
        manageArpeggio(currentContext, cell, Effect::arpeggioTable);
        manageArpeggio(currentContext, cell, Effect::arpeggio3Notes);
        manageArpeggio(currentContext, cell, Effect::arpeggio4Notes);

        // Pitch table.
        managePitchTable(currentContext, cell);

        // Finally, normalizes the cell again.
        if (const auto finalNormalizedCell = normalizeCell(cell); finalNormalizedCell != nullptr) {
            // Any change?
            cell = *finalNormalizedCell;
        }

        track.setCell(cellIndex, cell);
    }
}

void TrackOptimizer::manageVolume(CellContext& currentContext, Cell& cell, const Effect effect) noexcept
{
    if (const auto foundEffect = cell.find(effect); foundEffect.isPresent()) {
        const auto value = foundEffect.getValue().getEffectLogicalValue();
        if (currentContext.getCurrentVolumeValueFromEffect(effect) == value) {
            const auto found = cell.removeEffect(effect);
            jassert(found); (void)found;
        } else {
            currentContext.setEffectValue(effect, value);
        }
        if (effect == Effect::volume) {
            // When a Set Volume is set, a i/o to 0 could be optimized.
            currentContext.resetEffects(volumeInAndOutEffects);
        } else {
            if (value == 0) {
                // If i/o 0, all the volume effects are considered 0 to be further optimized.
                currentContext.resetEffects(volumeInAndOutEffects);
            } else {
                // Invalidates all other volumes.
                invalidateVolumesExcept(currentContext, { effect });
            }
        }
    }
}

void TrackOptimizer::managePitch(CellContext& currentContext, Cell& cell, const Effect effect) noexcept
{
    if (const auto foundPitchEffect = cell.find(effect); foundPitchEffect.isPresent()) {
        const auto pitch = foundPitchEffect.getValue().getEffectLogicalValue();
        if (currentContext.getCurrentPitchValueFromEffect(effect) == pitch) {
            const auto found = cell.removeEffect(effect);
            jassert(found); (void)found;
        } else {
            currentContext.setEffectValue(effect, pitch);
        }

        if (pitch == 0) {
            // If u/d... 0, all the pitch effects are considered 0 to be further optimized.
            currentContext.resetEffects(allPitchEffects);
        } else {
            // Invalidates all other pitches.
            invalidatePitchesExcept(currentContext, { effect });
        }
    }
}

void TrackOptimizer::manageSpeed(CellContext& currentContext, Cell& cell, const Effect effect) noexcept
{
    if (const auto foundSpeedEffect = cell.find(effect); foundSpeedEffect.isPresent()) {
        const auto value = foundSpeedEffect.getValue().getEffectLogicalValue();
        if (currentContext.getCurrentSpeedValueFromEffect(effect) == value) {
            const auto found = cell.removeEffect(effect);
            jassert(found); (void)found;
        } else {
            currentContext.setEffectValue(effect, value);
        }
    }
}

void TrackOptimizer::manageArpeggio(CellContext& currentContext, Cell& cell, const Effect effect) noexcept
{
    if (const auto foundEffect = cell.find(effect); foundEffect.isPresent()) {
        const auto value = foundEffect.getValue().getEffectLogicalValue();

        if (value == 0) {
            // Only stored. Cannot be optimized, because on every use, the Arp is restarted.
            // Except when stopped, no need to stop it more than once.
            if (currentContext.getCurrentArpeggioValueFromEffect(effect) == 0) {
                const auto found = cell.removeEffect(effect);
                jassert(found); (void)found;
            }
            currentContext.setEffectValue(effect, value);
            // If arp table/3/4 = 0, all the arpeggio effects are considered 0 to be further optimized.
            currentContext.resetEffects(allArpeggiosEffects);
        } else {
            currentContext.setEffectValue(effect, value);
            // Invalidates all other arpeggio effects.
            invalidateEffectExcept(currentContext, allArpeggiosEffects, { effect });
            // After an Arp, the force Arp speed becomes unknown.
            currentContext.invalidateEffect(Effect::forceArpeggioSpeed);
        }
    }
}

void TrackOptimizer::managePitchTable(CellContext& currentContext, Cell& cell) noexcept
{
    constexpr auto effect = Effect::pitchTable;
    if (const auto foundEffect = cell.find(effect); foundEffect.isPresent()) {
        const auto value = foundEffect.getValue().getEffectLogicalValue();

        if (value == 0) {
            // Only stored. Cannot be optimized, because on every use, the Pitch Table is restarted.
            // Except when stopped, no need to stop it more than once.
            if (currentContext.getCurrentPitchTableValue() == 0) {
                const auto found = cell.removeEffect(effect);
                jassert(found); (void)found;
            }
            currentContext.resetEffect(effect);
        }
        currentContext.setEffectValue(effect, value);
        // Like an Arp, the force Pitch speed becomes unknown after a Pitch Table is set.
        currentContext.invalidateEffect(Effect::forcePitchTableSpeed);
    }
}

void TrackOptimizer::manageReset(CellContext& currentContext, Cell& cell) noexcept
{
    if (const auto foundEffect = cell.find(Effect::reset); foundEffect.isPresent()) {
        const auto invertedVolume = foundEffect.getValue().getEffectLogicalValue();
        const auto newVolume = PsgValues::maximumVolumeNoHard - invertedVolume;

        // Is the reset effect useful?
        if ((currentContext.getCurrentVolume() == newVolume)
            && (currentContext.getCurrentPitchValueFromEffect(Effect::pitchDown) == 0)
            && (currentContext.getCurrentPitchValueFromEffect(Effect::pitchUp) == 0)
            && (currentContext.getCurrentPitchValueFromEffect(Effect::fastPitchUp) == 0)
            && (currentContext.getCurrentPitchValueFromEffect(Effect::fastPitchDown) == 0)
            && (currentContext.getCurrentPitchValueFromEffect(Effect::pitchGlide) == 0)
            && (currentContext.getCurrentVolumeValueFromEffect(Effect::volumeIn) == 0)
            && (currentContext.getCurrentVolumeValueFromEffect(Effect::volumeOut) == 0)
            && (currentContext.getCurrentArpeggioValueFromEffect(Effect::arpeggioTable) == 0)
            && (currentContext.getCurrentArpeggioValueFromEffect(Effect::arpeggio3Notes) == 0)
            && (currentContext.getCurrentArpeggioValueFromEffect(Effect::arpeggio4Notes) == 0)
            && (currentContext.getCurrentPitchTableValue() == 0)
        ) {
            const auto found = cell.removeEffect(Effect::reset);
            jassert(found); (void)found;
        } else {
            currentContext.setVolume(newVolume);
            currentContext.resetEffects(volumeInAndOutEffects);
            currentContext.resetEffects(allPitchEffects);
            currentContext.resetEffects(allArpeggiosEffects);
            currentContext.resetEffect(Effect::pitchTable);
        }
    }
}

void TrackOptimizer::invalidatePitchesExcept(CellContext& context, const std::set<Effect>& effectsToIgnore) noexcept
{
    invalidateEffectExcept(context, allPitchEffects, effectsToIgnore);
}

void TrackOptimizer::invalidateVolumesExcept(CellContext& context, const std::set<Effect>& effectsToIgnore) noexcept
{
    static const std::set effectsToInvalidate = {
        Effect::volumeIn,
        Effect::volumeOut,
        Effect::volume,
    };

    invalidateEffectExcept(context, effectsToInvalidate, effectsToIgnore);
}

void TrackOptimizer::invalidateEffectExcept(CellContext& context, const std::set<Effect>& effectsToInvalidate, const std::set<Effect>& effectsToIgnore) noexcept
{
    for (const auto effect : effectsToInvalidate) {
        if (effectsToIgnore.find(effect) == effectsToIgnore.cend()) {
            context.invalidateEffect(effect);
        }
    }
}

} // namespace arkostracker
