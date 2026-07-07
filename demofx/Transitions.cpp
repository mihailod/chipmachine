#include "Transitions.h"

// Pulled in for the full Icon definition (the screenshot widget the effects
// animate). Icon lives in ChipMachine.h; the transitions only touch its public
// state (color, rec, mosaic/star fields).
#include "../src/ChipMachine.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

#include <coreutils/log.h>
#include <coreutils/utils.h>
#include <grappix/grappix.h>
#include <tween/tween.h>

using namespace grappix;
using tween::Tween;

namespace chipmachine {

void ScreenshotTransitions::configure(
    Icon& ic, std::function<int()> count,
    std::function<const image::bitmap&(int)> bitmap,
    std::function<void()> update)
{
    icon = &ic;
    shotCount = std::move(count);
    shotBitmap = std::move(bitmap);
    updateArea = std::move(update);
}

bool ScreenshotTransitions::idleFor(uint64_t ms) const
{
    return setShotAt < utils::getms() - ms;
}

// A full-size screenshot rectangle collapsed to a single pixel at its center --
// the start/end point of the zoom transition.
static grappix::Rectangle collapsedRect(const grappix::Rectangle& full)
{
    return grappix::Rectangle(full.x + full.w / 2, full.y + full.h / 2, 1.0f,
                              1.0f);
}

// Mid-transition step shared by every effect: swap in the current shot's bitmap
// and (re)compute its full-size rectangle. Returns that rectangle. Assumes the
// caller has already checked that currentShot is still valid.
grappix::Rectangle ScreenshotTransitions::swapToCurrentShot()
{
    icon->setBitmap(shotBitmap(currentShot), true);
    updateArea();
    return icon->rec;
}

// Effect: fade the current shot out, swap, then fade the next shot in.
void ScreenshotTransitions::transitionFade()
{
    float secs = fadeSeconds;
    Tween::make()
        .to(icon->color, Color(0x00000000))
        .seconds(secs)
        .onComplete([=]() {
            if (shotCount() <= currentShot) {
                LOGD("Shot went away!");
                return;
            }
            swapToCurrentShot();
            Tween::make().to(icon->color, Color(0xffffffff)).seconds(secs);
        });
}

// Effect: shrink the current shot down to a pixel, swap, then grow the next
// shot back up from a pixel to full size.
void ScreenshotTransitions::transitionZoom()
{
    float secs = zoomSeconds;
    // The zoom animates the rectangle only, so make sure the shot is opaque in
    // case a preceding fade left the color mid-transition.
    icon->color = Color(0xffffffff);
    auto& rec = icon->rec;
    auto target = collapsedRect(rec);
    Tween::make()
        .to(rec.x, target.x)
        .to(rec.y, target.y)
        .to(rec.w, target.w)
        .to(rec.h, target.h)
        .seconds(secs)
        .onComplete([=]() {
            if (shotCount() <= currentShot) {
                LOGD("Shot went away!");
                return;
            }
            // Start the new shot collapsed at its own full-size center so the
            // zoom-in lands exactly on it.
            grappix::Rectangle full = swapToCurrentShot();
            icon->rec = collapsedRect(full);
            auto& r = icon->rec;
            Tween::make()
                .to(r.x, full.x)
                .to(r.y, full.y)
                .to(r.w, full.w)
                .to(r.h, full.h)
                .seconds(secs);
        });
}

// Shuffle a fresh random reveal order for the mosaic effect: tileRank[idx] gives
// each tile's position in the reveal sequence.
void ScreenshotTransitions::setupMosaicOrder()
{
    int gw = mosaicGrid, gh = mosaicGrid;
    int total = gw * gh;
    std::vector<int> order(total);
    std::iota(order.begin(), order.end(), 0);
    static std::mt19937 rng{ std::random_device{}() };
    std::shuffle(order.begin(), order.end(), rng);
    std::vector<int> rank(total);
    for (int i = 0; i < total; i++)
        rank[order[i]] = i;
    mosaicGridW = gw;
    mosaicGridH = gh;
    tileRank = std::move(rank);
}

// Draws the image as a mosaicGridW x mosaicGridH grid of tiles. A tile is shown
// (textured) once its position in the shuffled reveal order, tileRank[idx], is
// below the reveal threshold; hidden tiles are simply not drawn, so the
// starfield shows through. revealProgress in [0,1] scales how many are shown.
void ScreenshotTransitions::renderMosaic(std::shared_ptr<RenderTarget> target)
{
    Texture* tex = icon->getTexture();
    if (!tex || mosaicGridW <= 0 || mosaicGridH <= 0 ||
        (int)tileRank.size() != mosaicGridW * mosaicGridH)
        return;
    auto& rec = icon->rec;
    uint32_t color = (uint32_t)icon->color;
    int total = mosaicGridW * mosaicGridH;
    int revealed = (int)(revealProgress * total + 0.5f);
    if (revealed < 0) revealed = 0;
    if (revealed > total) revealed = total;
    float tw = rec.w / mosaicGridW;
    float th = rec.h / mosaicGridH;
    for (int ty = 0; ty < mosaicGridH; ty++) {
        for (int tx = 0; tx < mosaicGridW; tx++) {
            int idx = ty * mosaicGridW + tx;
            if (tileRank[idx] >= revealed) continue;
            float x = rec.x + tx * tw;
            float y = rec.y + ty * th;
            float s0 = (float)tx / mosaicGridW;
            float s1 = (float)(tx + 1) / mosaicGridW;
            // Texture is uploaded vertically flipped and the draw quad maps
            // screen-top to t=1, so mirror the vertical UV band to keep each
            // tile in the same place it occupies in the whole image.
            float t0 = 1.0f - (float)(ty + 1) / mosaicGridH;
            float t1 = 1.0f - (float)ty / mosaicGridH;
            float uvs[8] = { s0, t0, s1, t0, s0, t1, s1, t1 };
            target->draw(*tex, x, y, tw, th, uvs, color);
        }
    }
}

// Effect: dissolve the current shot to black tile-by-tile in random order, swap,
// then build the next shot up from black the same way.
void ScreenshotTransitions::transitionMosaic()
{
    float secs = mosaicSeconds;
    icon->color = Color(0xffffffff);
    setupMosaicOrder();
    revealProgress = 1.0f;
    icon->customRender = [this](std::shared_ptr<RenderTarget> t, uint32_t) {
        renderMosaic(t);
    };
    Tween::make()
        .to(revealProgress, 0.0f)
        .seconds(secs)
        .onComplete([=]() {
            if (shotCount() <= currentShot) {
                LOGD("Shot went away!");
                icon->customRender = nullptr;
                return;
            }
            swapToCurrentShot();
            setupMosaicOrder();
            revealProgress = 0.0f;
            Tween::make()
                .to(revealProgress, 1.0f)
                .seconds(secs)
                .onComplete([=]() {
                    // Back to a plain quad so the next effect renders normally.
                    icon->customRender = nullptr;
                });
        });
}

// Sample a starGrid x starGrid grid of pixels from bm and store them as stars
// (home position + color). Fully transparent pixels (e.g. a logo's background)
// become no star.
void ScreenshotTransitions::setupStars(const image::bitmap& bm)
{
    int gw = starGrid, gh = starGrid;
    int bw = bm.width(), bh = bm.height();
    stars.clear();
    starGridW = gw;
    starGridH = gh;
    if (bw <= 0 || bh <= 0) return;
    stars.reserve(gw * gh);
    for (int gy = 0; gy < gh; gy++) {
        for (int gx = 0; gx < gw; gx++) {
            float u = (gx + 0.5f) / gw;
            float v = (gy + 0.5f) / gh;
            int px = std::min(bw - 1, (int)(u * bw));
            int py = std::min(bh - 1, (int)(v * bh));
            uint32_t p = bm[px + py * bw];
            if (((p >> 24) & 0xff) == 0) continue;   // transparent -> no star
            // Bitmap is RGBA in memory (0xAABBGGRR); Color wants 0xAARRGGBB.
            uint32_t col = (p & 0xff000000) | ((p & 0xff) << 16) |
                           (p & 0x0000ff00) | ((p >> 16) & 0xff);
            stars.push_back({ u - 0.5f, v - 0.5f, col });
        }
    }
}

// Draws the sampled pixels as a 3D starfield. Each star sits at home offset
// (hx,hy) (fraction of the rect, from center) at rest; the projection factor
// f = 1/starZ blows them radially outward as starZ shrinks toward the viewer,
// and starAlpha fades the whole cloud. At starZ=1, f=1 and the stars tile the
// rect, reproducing the image; as starZ->0 they streak off toward the edges.
void ScreenshotTransitions::renderStars(std::shared_ptr<RenderTarget> target)
{
    if (stars.empty() || starGridW <= 0 || starGridH <= 0) return;
    auto& rec = icon->rec;
    float cx = rec.x + rec.w * 0.5f;
    float cy = rec.y + rec.h * 0.5f;
    float f = starZ > 0.0001f ? 1.0f / starZ : 10000.0f;
    // Particle size grows slower than the positional spread so the cloud opens
    // gaps between particles as it explodes, instead of the tiles staying
    // edge-to-edge. At rest (f=1) sizeF=1 so the image tiles solid.
    const float sizeGrow = 0.1f;
    float sizeF = 1.0f + (f - 1.0f) * sizeGrow;
    float sw = (rec.w / starGridW) * sizeF;
    float sh = (rec.h / starGridH) * sizeF;
    float scrW = (float)target->width();
    float scrH = (float)target->height();
    for (auto& s : stars) {
        float sx = cx + s.hx * rec.w * f;
        float sy = cy + s.hy * rec.h * f;
        if (sx + sw < 0 || sx - sw > scrW || sy + sh < 0 || sy - sh > scrH)
            continue;
        float a = ((s.color >> 24) & 0xff) / 255.0f * starAlpha;
        if (a <= 0.004f) continue;
        uint32_t col = ((uint32_t)(a * 255.0f) << 24) | (s.color & 0x00ffffff);
        target->rectangle(sx - sw * 0.5f, sy - sh * 0.5f, sw, sh, col);
    }
}

// Effect: the outgoing shot's pixels break into stars and fly toward the viewer
// (fading out), then the next shot's pixels decelerate back from the scatter to
// reform the image.
void ScreenshotTransitions::transitionStarfield()
{
    float secs = starSeconds;
    float zMin = starDepthMin;
    icon->color = Color(0xffffffff);
    icon->customRender = [this](std::shared_ptr<RenderTarget> t, uint32_t) {
        renderStars(t);
    };

    // In phase: start the new shot fully scattered/faded and collapse it home.
    auto startIn = [=]() {
        if (shotCount() <= currentShot) {
            LOGD("Shot went away!");
            icon->customRender = nullptr;
            return;
        }
        swapToCurrentShot();
        setupStars(shotBitmap(currentShot));
        starZ = zMin;
        starAlpha = 0.0f;
        Tween::make()
            .to(starZ, 1.0f)
            .to(starAlpha, 1.0f)
            .seconds(secs)
            .onComplete([=]() { icon->customRender = nullptr; });
    };

    bool haveOut = outgoingShot >= 0 && outgoingShot < shotCount();
    if (!haveOut) {
        // Nothing to scatter out from (first shot of a song) -- just collapse in.
        startIn();
        return;
    }
    setupStars(shotBitmap(outgoingShot));
    starZ = 1.0f;
    starAlpha = 1.0f;
    Tween::make()
        .to(starZ, zMin)
        .to(starAlpha, 0.0f)
        .seconds(secs)
        .onComplete([=]() { startIn(); });
}

// HSV (0..1 each) -> 0xAARRGGBB, full alpha. Used for the copper palette.
static uint32_t hsv(float h, float s, float v)
{
    h = (h - std::floor(h)) * 6.0f;
    float f = h - std::floor(h);
    float p = v * (1 - s), q = v * (1 - f * s), t = v * (1 - (1 - f) * s);
    float r, g, b;
    switch (((int)h) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
    return 0xff000000u | ((uint32_t)(r * 255) << 16) |
           ((uint32_t)(g * 255) << 8) | (uint32_t)(b * 255);
}

// Colour of a copper scanline at vertical fraction v, animated by `phase`: a
// drifting hue down the rect (rainbow) with sine "metallic" highlight bands
// moving through it -- the classic Amiga copper-bar look.
static uint32_t copperColor(float v, float phase)
{
    const float bands = 7.0f;    // highlight bands down the rect
    const float hueCycles = 1.3f;
    float lum = 0.35f + 0.65f *
                (0.5f + 0.5f * std::sin((v * bands - phase) * 6.2831853f));
    return hsv(v * hueCycles + phase * 0.25f, 0.85f, lum);
}

// Copper bars fill the rect; on top, the image is drawn as `copperStrips`
// horizontal strips shifted horizontally by copperShift (alternating direction)
// and faded by copperImageAlpha. copperPhase animates the bars' colour cycle.
void ScreenshotTransitions::renderCopper(
    std::shared_ptr<RenderTarget> target, uint32_t delta)
{
    auto& rec = icon->rec;
    Texture* tex = icon->getTexture();
    if (!tex) return;

    copperPhase += delta * 0.00035f;   // colour cycles per millisecond

    // 1) Copper bars fill the rect, one thin band per few scanlines.
    const float step = 3.0f;
    for (float yy = 0; yy < rec.h; yy += step) {
        float v = (yy + step * 0.5f) / rec.h;
        float h = std::min(step, rec.h - yy);
        target->rectangle(rec.x, rec.y + yy, rec.w, h, copperColor(v, copperPhase));
    }

    // 2) Image strips over the bars.
    int strips = copperStrips > 0 ? copperStrips : 16;
    float a = copperImageAlpha;
    if (a <= 0.004f) return;
    uint32_t col = ((uint32_t)(a * 255.0f) << 24) | 0x00ffffff;
    float stripH = rec.h / strips;
    for (int s = 0; s < strips; s++) {
        float dx = ((s % 2 == 0) ? 1.0f : -1.0f) * copperShift * rec.w;
        float y = rec.y + s * stripH;
        // Texture is uploaded flipped, so mirror the vertical UV band.
        float t0 = 1.0f - (float)(s + 1) / strips;
        float t1 = 1.0f - (float)s / strips;
        float uvs[8] = { 0, t0, 1, t0, 0, t1, 1, t1 };
        target->draw(*tex, rec.x + dx, y, rec.w, stripH, uvs, col);
    }
}

// Effect: the current image splits into horizontal strips that slide out and
// fade, revealing cycling rainbow copper bars; then the new image fades back in
// over the settled bars. (Amiga Copper / Blitter wipe.)
void ScreenshotTransitions::transitionCopperWipe()
{
    float secs = copperSeconds;
    icon->color = Color(0xffffffff);
    copperShift = 0.0f;
    copperImageAlpha = 1.0f;
    icon->customRender = [this](std::shared_ptr<RenderTarget> t, uint32_t d) {
        renderCopper(t, d);
    };
    // Out: slide the current image's strips out (alternating sides) and fade
    // them, exposing the copper bars underneath.
    Tween::make()
        .to(copperShift, 0.35f)
        .to(copperImageAlpha, 0.0f)
        .seconds(secs)
        .onComplete([=]() {
            if (shotCount() <= currentShot) {
                LOGD("Shot went away!");
                icon->customRender = nullptr;
                return;
            }
            swapToCurrentShot();
            copperShift = 0.0f;
            copperImageAlpha = 0.0f;
            // In: fade the new image in over the still-cycling copper bars.
            Tween::make()
                .to(copperImageAlpha, 1.0f)
                .seconds(secs)
                .onComplete([=]() { icon->customRender = nullptr; });
        });
}

// Draws the image as `warpColumns` full-height vertical columns, each shifted
// vertically by a sine wave running along the x axis (amplitude warpAmp as a
// fraction of the rect height, animated by warpPhase). At warpAmp=0 the columns
// line up into the flat image; as it grows the image bends into an oscillating
// ribbon.
void ScreenshotTransitions::renderSineWarp(std::shared_ptr<RenderTarget> target,
                                           uint32_t delta)
{
    Texture* tex = icon->getTexture();
    if (!tex) return;
    auto& rec = icon->rec;
    uint32_t color = (uint32_t)icon->color;

    warpPhase += delta * 0.004f;   // oscillation speed
    const float waves = 2.5f;      // sine periods across the width
    int cols = warpColumns > 0 ? warpColumns : 64;
    float colW = rec.w / cols;
    float amp = warpAmp * rec.h;
    for (int i = 0; i < cols; i++) {
        float xf = (i + 0.5f) / cols;   // column-center fraction across width
        float dy = amp * std::sin(xf * waves * 6.2831853f + warpPhase);
        float s0 = (float)i / cols;
        float s1 = (float)(i + 1) / cols;
        // Full-height column: t spans 0..1 (already upright, like the default
        // quad), only the horizontal UV band is sliced.
        float uvs[8] = { s0, 0, s1, 0, s0, 1, s1, 1 };
        target->draw(*tex, rec.x + i * colW, rec.y + dy, colW, rec.h, uvs, color);
    }
}

// Effect: warp the current image's columns with a growing sine wave until it is
// an oscillating ribbon, swap the texture, then damp the wave back down so the
// new image settles flat. (Amiga vector-grid / sine distortion.)
void ScreenshotTransitions::transitionSineWarp()
{
    float secs = warpSeconds;
    const float ampMax = 0.5f;   // peak displacement, fraction of height
    icon->color = Color(0xffffffff);
    warpAmp = 0.0f;
    icon->customRender = [this](std::shared_ptr<RenderTarget> t, uint32_t d) {
        renderSineWarp(t, d);
    };
    // Out: ramp the amplitude up until the image deforms into a ribbon.
    Tween::make()
        .to(warpAmp, ampMax)
        .seconds(secs)
        .onComplete([=]() {
            if (shotCount() <= currentShot) {
                LOGD("Shot went away!");
                icon->customRender = nullptr;
                return;
            }
            swapToCurrentShot();
            // In: damp the amplitude back to zero so the new image settles flat.
            Tween::make()
                .to(warpAmp, 0.0f)
                .seconds(secs)
                .onComplete([=]() { icon->customRender = nullptr; });
        });
}

void ScreenshotTransitions::restart()
{
    currentShot = -1;
    next();
}

void ScreenshotTransitions::next()
{
    setShotAt = utils::getms();
    if (shotCount() <= 0) return;

    outgoingShot = currentShot;   // index currently displayed (-1 if none)
    currentShot++;
    if (currentShot >= shotCount()) currentShot = 0;

    // A fresh transition owns the icon: clear any custom renderer left over from
    // an interrupted effect so a stale particle cloud can't linger.
    icon->customRender = nullptr;

    // Cycle through the available transition effects so successive shots animate
    // differently (zoom, fade, mosaic, starfield, copper, ...). Append new
    // effects here and they slot into the rotation automatically.
    static const std::vector<void (ScreenshotTransitions::*)()> effects = {
        &ScreenshotTransitions::transitionZoom,
        &ScreenshotTransitions::transitionFade,
        &ScreenshotTransitions::transitionMosaic,
        &ScreenshotTransitions::transitionStarfield,
        &ScreenshotTransitions::transitionCopperWipe,
        &ScreenshotTransitions::transitionSineWarp,
    };
    auto effect = effects[currentEffect % effects.size()];
    currentEffect = (currentEffect + 1) % (int)effects.size();
    (this->*effect)();
}

} // namespace chipmachine
