#pragma once

// Tracker pattern feed for the libxmp-backed plugins (med, mgt, musx, fnk,
// coco). They all drive libxmp the same way, so the row capture lives here
// instead of being copy-pasted five times.
//
// Include this AFTER the plugin's own `extern "C" { #include "xmp.h" }` block
// (and after its BUILDING_STATIC define) -- xmp.h has an include guard, so the
// one below is a no-op in that position and only serves standalone use.

#include "chipplayer.h"
#include "tracker_util.h"

extern "C" {
#include "xmp.h"
}

#include <algorithm>

namespace musix::tracker {

// Watches a libxmp context for row changes. Call capture() between short
// xmp_play_buffer() slices; it returns true (and fills `out`) exactly once per
// row the player enters.
class XmpRowWatcher
{
public:
    void reset() { lastPattern = lastRow = -1; }

    bool capture(xmp_context ctx, int frameOffset, TrackerRow& out)
    {
        xmp_frame_info fi;
        xmp_get_frame_info(ctx, &fi);
        if (fi.pattern == lastPattern && fi.row == lastRow) { return false; }
        lastPattern = fi.pattern;
        lastRow = fi.row;

        xmp_module_info mi;
        xmp_get_module_info(ctx, &mi);
        struct xmp_module* mod = mi.mod;
        if (mod == nullptr || fi.pattern < 0 || fi.pattern >= mod->pat ||
            fi.row < 0) {
            return false;
        }
        struct xmp_pattern* pat = mod->xxp[fi.pattern];
        if (pat == nullptr || fi.row >= pat->rows) { return false; }

        out = TrackerRow{};
        out.frameOffset = frameOffset;
        out.pattern = static_cast<int16_t>(fi.pattern);
        out.row = static_cast<int16_t>(fi.row);
        out.numRows = static_cast<int16_t>(fi.num_rows);
        out.channels =
            static_cast<int8_t>(std::min(mod->chn, kTrackerChannels));

        for (int c = 0; c < out.channels; c++) {
            // A pattern is a list of track indices, one per channel; the events
            // live in the tracks.
            int trk = pat->index[c];
            if (trk < 0 || trk >= mod->trk) { continue; }
            struct xmp_track* t = mod->xxt[trk];
            if (t == nullptr || fi.row >= t->rows) { continue; }
            const struct xmp_event& e = t->event[fi.row];
            auto& cell = out.cells[c];

            if (e.note == XMP_KEY_OFF) {
                setNoteText(cell, "===");
            } else if (e.note == XMP_KEY_CUT) {
                setNoteText(cell, "^^^");
            } else if (e.note == XMP_KEY_FADE) {
                setNoteText(cell, "~~~");
            } else if (e.note > 0) {
                // libxmp stores note+1, with internal note 0 == C-0.
                setNote(cell, e.note - 1);
            }
            setInstrument(cell, e.ins);
            setEffect(cell, e.fxt, e.fxp);
        }
        return true;
    }

private:
    int lastPattern = -1;
    int lastRow = -1;
};

} // namespace musix::tracker
