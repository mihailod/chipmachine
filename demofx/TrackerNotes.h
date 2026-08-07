#ifndef TRACKERNOTES_H
#define TRACKERNOTES_H

#include <musicplayer/src/chipplayer.h>

#include <coreutils/log.h>
#include <grappix/grappix.h>

#include <cmath>
#include <cstdio>
#include <deque>
#include <string>
#include <utility>
#include <vector>

namespace demofx {

// A ProTracker-style pattern display for the formats whose players can say
// which row they are on (see musix::TrackerRow): the four leftmost channels
// scroll up the screen, newest row at the bottom, so the notes you are hearing
// right now sit just above the scroller and everything above them is what has
// already played.
//
// Deliberately a BACKGROUND layer: drawn just in front of the starfield and
// behind every piece of UI, in a translucent colour that fades out towards the
// top of the screen, so it never competes with the song info or the scroll.
class TrackerNotes
{
public:
    explicit TrackerNotes(grappix::RenderTarget& target) : target(target)
    {
        // The stock font shader throws the colour's alpha away
        // (gl_FragColor = vec4(color.rgb, texture.a)), which is fine for opaque
        // UI text and useless here -- the whole point of this layer is that it
        // is see-through. Same trick the scroller uses for its gradient.
        try {
            program = grappix::get_program(grappix::FONT_PROGRAM).clone();
            program.setFragmentSource(fontShaderF);
            haveProgram = true;
        } catch (grappix::shader_exception& e) {
            LOGD("TRACKER NOTES SHADER FAILED TO COMPILE: %s", e.what());
        }
    }

    // Builds the pattern font. Non-distance-map (a pixel font wants nearest-ish
    // sampling, not a signed distance field) at a 4x-of-design-size raster so
    // it stays sharp when the layout scales it down.
    void setFont(const std::string& path)
    {
        font = grappix::Font(path, 44, 512);
        if (haveProgram) { font.set_program(program); }
        charW1 = 0.0F;
        haveFont = true;
    }

    [[nodiscard]] bool hasFont() const { return haveFont; }

    void addRow(const musix::TrackerRow& r)
    {
        rows.push_front(formatRow(r));
        while (rows.size() > kMaxRows) {
            rows.pop_back();
        }
    }

    // The rows already decoded but not yet heard, nearest first. Replaced whole
    // every frame; re-formatted only when the run actually changes, since the
    // same handful of rows is handed to us over and over between row changes.
    void setUpcoming(const std::vector<musix::TrackerRow>& next)
    {
        bool same = next.size() == aheadKey.size();
        for (size_t i = 0; same && i < next.size(); i++) {
            same = next[i].pattern == aheadKey[i].first &&
                   next[i].row == aheadKey[i].second;
        }
        if (same) { return; }
        ahead.clear();
        aheadKey.clear();
        ahead.reserve(next.size());
        aheadKey.reserve(next.size());
        for (auto const& r : next) {
            ahead.push_back(formatRow(r));
            aheadKey.emplace_back(r.pattern, r.row);
        }
    }

    void clearRows()
    {
        rows.clear();
        ahead.clear();
        aheadKey.clear();
    }

    [[nodiscard]] bool empty() const { return rows.empty(); }

    // `fraction` is how far playback is between the row that is sounding and the
    // next one (0..1); it slides the whole column smoothly instead of stepping
    // once per row. Everything else comes from the knobs below.
    void draw(float fraction)
    {
        if (!on || !haveFont || rows.empty()) { return; }

        float rowH = rowHeight();
        if (rowH < 4.0F) { return; }

        // rows[i] is i rows above the sounding one, ahead[j] is j+1 rows below,
        // and the fraction offset slides the whole grid so the hand-over is
        // continuous: ahead[0] takes rows[0]'s place exactly as it starts
        // sounding.
        //
        // The play cursor does NOT move with its row. It is a fixed line and
        // the pattern scrolls through it, the way a tracker with smooth
        // scrolling behaves -- rows brighten as they approach it and dim as
        // they leave, peaking at lineY. Highlighting rows[0] instead made the
        // bright line (and its slab) climb a whole row and snap back once per
        // row, which read as a vertical shake against the smooth scroll.
        // The geometry below puts rows[0]'s centre half a row below the cursor
        // at fraction 0 and half a row above it at fraction 1, so it is the
        // brightest row for exactly its own duration.
        float x = leftMargin();
        auto s = textScale();
        float half = rowH * 0.5F;

        int above = static_cast<int>(lineY / rowH) + 2;
        if (above > static_cast<int>(rows.size())) {
            above = static_cast<int>(rows.size());
        }
        int below =
            static_cast<int>((static_cast<float>(target.height()) - lineY) /
                             rowH) +
            2;
        if (below > static_cast<int>(ahead.size())) {
            below = static_cast<int>(ahead.size());
        }

        // The cursor slab. Pinned to lineY -- see above.
        if (barAlpha > 0.0F) {
            target.rectangle(0, lineY - half, static_cast<float>(target.width()),
                             rowH, withAlpha(hlColor, alpha * barAlpha));
        }

        // Draws one row, shaded by how close its centre is to the cursor: at
        // the cursor it is hlColor at full opacity, a row away it is already
        // the plain colour at whatever the distance fade allows.
        auto drawRow = [&](const std::string& text, float y, float baseA) {
            float d = std::fabs((y + half) - lineY) / rowH;
            float hot = d < 1.0F ? (1.0F - d) : 0.0F;
            float a = baseA + (1.0F - baseA) * hot;
            target.text(font, text, x, y, withAlpha(mix(color, hlColor, hot),
                                                    alpha * a),
                        s);
        };

        // Already played: upwards from the cursor, fading out at the top.
        for (int i = 0; i < above; i++) {
            float y = lineY - (static_cast<float>(i) + fraction) * rowH;
            if (y + rowH < 0.0F) { break; }
            drawRow(rows[i], y, fade(static_cast<float>(i) + fraction, above) * dim);
        }

        // Not yet heard: downwards, fading in as they rise. These pass behind
        // the scroller on their way up, which is what makes them appear to
        // scroll in from the bottom of the screen rather than pop into being.
        for (int j = 0; j < below; j++) {
            float d = static_cast<float>(j) + 1.0F - fraction;
            drawRow(ahead[j], lineY + d * rowH, fade(d, below) * dim * dimAhead);
        }

        // Rows scroll off and their cached vertex buffers with them; without
        // this the per-font text cache would grow for the length of the song.
        font.update_cache();
    }

    // --- knobs (set from lua, see ChipMachine::setVariable) -----------------
    // The play cursor: a FIXED line that the pattern scrolls through. Rows
    // above it have played, rows below it are about to, and whichever row is
    // crossing it is the one you are hearing.
    float lineY = 0;
    // Multiplier on the width-fitted text size; 1.0 exactly fills the screen.
    float sizeScale = 1.0F;
    // Opacity of the current row; the rest fade from `dim` times this to 0.
    float alpha = 0.75F;
    float dim = 0.8F;
    // Extra dimming for the rows that have not played yet -- they cross the
    // scroller, so they want to be quieter than the ones above the play line.
    float dimAhead = 0.6F;
    // Row height, as a multiple of the character cell width. Departure Mono's
    // glyph box is 2:1 (tall and narrow), so anything below ~1.6 overlaps.
    float lineSpacing = 1.8F;
    // Opacity of the slab behind the current row, relative to `alpha`.
    float barAlpha = 0.12F;
    uint32_t hlColor = 0xffffffff;
    uint32_t color = 0xff70b0ff;
    bool on = true;

private:
    // Widest line the layout has to fit: a two-digit row number, a gap, then
    // four "C-4 01 A0F" cells with a gap after each.
    static constexpr int kColumns = 2 + 2 + musix::kTrackerChannels * 11;
    static constexpr size_t kMaxRows = 128;

    // Distance-from-the-play-line falloff, in rows. Squared so the column
    // dissolves into the starfield instead of ending on a hard edge.
    static float fade(float distance, int span)
    {
        if (span <= 0) { return 0.0F; }
        float t = distance / static_cast<float>(span);
        if (t > 1.0F) { t = 1.0F; }
        return (1.0F - t) * (1.0F - t);
    }

    // RGB blend; the alpha channel is set separately by withAlpha().
    static uint32_t mix(uint32_t a, uint32_t b, float t)
    {
        if (t <= 0.0F) { return a; }
        if (t >= 1.0F) { return b; }
        uint32_t out = 0;
        for (int shift = 0; shift <= 16; shift += 8) {
            auto ca = static_cast<float>((a >> shift) & 0xff);
            auto cb = static_cast<float>((b >> shift) & 0xff);
            out |= static_cast<uint32_t>(ca + (cb - ca) * t) << shift;
        }
        return out | (a & 0xff000000);
    }

    static uint32_t withAlpha(uint32_t c, float a)
    {
        if (a < 0.0F) { a = 0.0F; }
        if (a > 1.0F) { a = 1.0F; }
        auto base = static_cast<float>((c >> 24) & 0xff) / 255.0F;
        auto v = static_cast<uint32_t>(base * a * 255.0F);
        return (c & 0x00ffffff) | (v << 24);
    }

    // One draw call per row is the whole reason the cells are baked into a
    // single padded string here: a pattern screen is ~30 rows, and drawing note
    // / instrument / effect separately would be 400 draw calls a frame.
    static std::string formatRow(const musix::TrackerRow& r)
    {
        char line[8 + musix::kTrackerChannels * 16];
        int n = snprintf(line, sizeof(line), "%02X  ", r.row & 0xff);
        for (int c = 0; c < musix::kTrackerChannels; c++) {
            const char* note = "---";
            const char* inst = "--";
            const char* fx = "---";
            if (c < r.channels) {
                if (r.cells[c].note[0] != 0) { note = r.cells[c].note; }
                if (r.cells[c].inst[0] != 0) { inst = r.cells[c].inst; }
                if (r.cells[c].fx[0] != 0) { fx = r.cells[c].fx; }
            } else {
                // Channel the module does not have: leave the column blank
                // rather than pretending it is an empty row.
                note = "   ";
                inst = "  ";
                fx = "   ";
            }
            n += snprintf(line + n, sizeof(line) - n, "%-3s %-2s %-3s ", note,
                          inst, fx);
        }
        return { line };
    }

    // Character advance at scale 1, measured across a run so the per-glyph
    // offsets at the ends do not skew it (the font is monospaced).
    float charWidth1()
    {
        if (charW1 <= 0.0F) {
            const char* probe = "00000000000000000000";
            charW1 = static_cast<float>(font.get_width(probe, 1.0F)) / 19.0F;
        }
        return charW1;
    }

    float textScale()
    {
        float cw = charWidth1();
        if (cw <= 0.0F) { return 1.0F; }
        float avail = static_cast<float>(target.width()) - 2.0F * margin();
        return (avail / (static_cast<float>(kColumns) * cw)) * sizeScale;
    }

    // Row height is a multiple of the CHARACTER CELL, not of the font's own
    // line height: grappix stores that metric in a /100 unit and returns it
    // through an integer vec2i, so get_size().y truncates to 0 for any sane
    // size. Deriving it from the advance is both correct and the right knob for
    // a tracker anyway -- ProTracker's rows are exactly one 8x8 cell tall.
    float rowHeight() { return charWidth1() * textScale() * lineSpacing; }

    float margin() const { return static_cast<float>(target.width()) * 0.02F; }

    float leftMargin()
    {
        // Centre whatever the text actually measures, so a sizeScale below 1
        // does not leave the block hugging the left edge.
        float w = static_cast<float>(kColumns) * charWidth1() * textScale();
        return (static_cast<float>(target.width()) - w) / 2.0F;
    }

    grappix::RenderTarget& target;
    grappix::Font font;
    grappix::Program program;
    bool haveProgram = false;
    bool haveFont = false;
    float charW1 = 0.0F;

    std::deque<std::string> rows;
    std::vector<std::string> ahead;
    std::vector<std::pair<int16_t, int16_t>> aheadKey;

    const std::string fontShaderF = R"(
        #ifdef GL_ES
        precision mediump float;
        #endif
        uniform vec4 color;
        uniform sampler2D sTexture;
        varying vec2 UV;

        void main() {
            gl_FragColor = vec4(color.rgb, color.a * texture2D(sTexture, UV).a);
        }
    )";
};

} // namespace demofx

#endif // TRACKERNOTES_H
