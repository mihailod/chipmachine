#pragma once

#include "../../../../song/instrument/psg/PsgInstrumentCell.h"
#include "BaseItemOptimizer.h"

namespace arkostracker 
{

class Instrument;

/**
 * Optimizes the given Instrument.
 *
 * In order:
 * - Transfers the instrument retrig, if wanted.
 * - Remove the cells past the end.
 * - If softOnly & volume = 0, convert to noSoftNoHard with volume 0, remove all pitch, noise and arp as well (well... use an empty cell).
 * - If not looping, remove last cell(s) with noSoftNoHard if volume 0, reduce the endIndex as well.
 * - Removes cycles at the loop.
 * - If the cells are all duplicates on X steps, increase the speed accordingly and remove the duplicates.
 */
class InstrumentOptimizer final : BaseItemOptimizer<Instrument, PsgInstrumentCell>
{
public:
    /**
     * Constructor.
     * @param instrument the Instrument.
     * @param transferInstrumentRetrig if true, remove the Instrument Retrig and put it in the Instrument itself. Some exporters require that.
     * Can be only performed if flatten is true.
     * @param flatten should be true for exports. It will apply (and delete) the auto-loops. Use false for non-export, but some optimizations will be skipped.
     */
    InstrumentOptimizer(Instrument& instrument, bool transferInstrumentRetrig, bool flatten) noexcept;

    /** Optimizes the Instrument. It will be modified. Nothing happens if the Instrument is not the right type or cannot be optimized. */
    void optimize() noexcept;

private:

    // BaseItemOptimizer method implementations.
    // ============================================
    Loop getLoop() noexcept override;
    void setLoop(const Loop& loop) noexcept override;
    int getLength() noexcept override;
    int getSpeed() noexcept override;
    void setSpeed(int speed) noexcept override;
    void removeCells(int cellIndex, int cellCountToRemove) noexcept override;
    PsgInstrumentCell getCell(int cellIndex) noexcept override;
    bool areCellsMusicallyEqual(const PsgInstrumentCell& left, const PsgInstrumentCell& right) noexcept override;

    /** Flattens the Instrument. It will remove the auto-loops. */
    void flattenInstrument() noexcept;

    /** Removes the Software Only with volume 0 Cells, if any. */
    void convertMuteCells() noexcept;

    /** If not looping, removes the last mute Cells, if any, sets the endIndex accordingly. */
    void removeLastMuteCells() noexcept;

    /** If looping, if all the looping cells are mute, remove them and the loop. */
    void removeMuteLoop() noexcept;

    bool transferInstrumentRetrig;
    bool flatten;
};

}   // namespace arkostracker
