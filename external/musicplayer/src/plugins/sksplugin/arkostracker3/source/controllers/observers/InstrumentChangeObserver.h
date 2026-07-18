#pragma once

namespace arkostracker 
{

class Id;

/** Observers an Instrument, or all of them. */
class InstrumentChangeObserver
{
public:
    /** Flags about what has changed. */
    enum What : unsigned char
    {
        name = 1U << 0U,
        color = 1U << 1U,
        /** Loop start and/or end, or toggle, retrig, auto-spread, etc. */
        other = 1U << 2U
    };

    /** Destructor. */
    virtual ~InstrumentChangeObserver() = default;

    /**
     * Called when an Instrument has changed. It does not include the Instrument being deleted/moved.
     * @param id the ID of the Instrument.
     * @param whatChanged the bit of what has changed.
     */
    virtual void onInstrumentChanged(const Id& id, unsigned int whatChanged) = 0;

    /**
     * Called when an PSG Instrument Cell has changed.
     * @param id the ID of the Instrument.
     * @param cellIndex the index of the Cell that has changed.
     * @param mustRefreshAllAfterToo if true, the change has consequence on the next items. For example, a modified Link with auto-spread will
     * have all the envelope of the >= cellIndex possibly impacted.
     */
    virtual void onPsgInstrumentCellChanged(const Id& id, int cellIndex, bool mustRefreshAllAfterToo) = 0;

    /** Called when multiple Instruments have changed (moved, deleted, etc.). */
    virtual void onInstrumentsInvalidated() = 0;
};

}   // namespace arkostracker
