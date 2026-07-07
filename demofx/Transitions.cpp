#include "Transitions.h"

// Pulled in for the full Icon definition (the screenshot widget the effects
// animate). Icon lives in ChipMachine.h; the transitions only touch its public
// state (color, rec, mosaic/star fields).
#include "../src/ChipMachine.h"

#include <algorithm>
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

// Shuffle a fresh random reveal order for the mosaic effect onto the screenshot
// icon: tileRank[idx] gives each tile's position in the reveal sequence.
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
    icon->mosaicGridW = gw;
    icon->mosaicGridH = gh;
    icon->tileRank = std::move(rank);
}

// Effect: dissolve the current shot to black tile-by-tile in random order, swap,
// then build the next shot up from black the same way.
void ScreenshotTransitions::transitionMosaic()
{
    float secs = mosaicSeconds;
    icon->color = Color(0xffffffff);
    setupMosaicOrder();
    icon->mosaicMode = true;
    icon->revealProgress = 1.0f;
    Tween::make()
        .to(icon->revealProgress, 0.0f)
        .seconds(secs)
        .onComplete([=]() {
            if (shotCount() <= currentShot) {
                LOGD("Shot went away!");
                icon->mosaicMode = false;
                return;
            }
            swapToCurrentShot();
            setupMosaicOrder();
            icon->mosaicMode = true;
            icon->revealProgress = 0.0f;
            Tween::make()
                .to(icon->revealProgress, 1.0f)
                .seconds(secs)
                .onComplete([=]() {
                    // Back to a plain quad so the next effect renders normally.
                    icon->mosaicMode = false;
                });
        });
}

// Sample a starGrid x starGrid grid of pixels from bm and store them as stars
// (home position + color) on the screenshot icon. Fully transparent pixels
// (e.g. a logo's background) become no star.
void ScreenshotTransitions::setupStars(const image::bitmap& bm)
{
    int gw = starGrid, gh = starGrid;
    int bw = bm.width(), bh = bm.height();
    auto& stars = icon->stars;
    stars.clear();
    icon->starGridW = gw;
    icon->starGridH = gh;
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

// Effect: the outgoing shot's pixels break into stars and fly toward the viewer
// (fading out), then the next shot's pixels decelerate back from the scatter to
// reform the image.
void ScreenshotTransitions::transitionStarfield()
{
    float secs = starSeconds;
    float zMin = starDepthMin;
    icon->color = Color(0xffffffff);

    // In phase: start the new shot fully scattered/faded and collapse it home.
    auto startIn = [=]() {
        if (shotCount() <= currentShot) {
            LOGD("Shot went away!");
            icon->starMode = false;
            return;
        }
        swapToCurrentShot();
        setupStars(shotBitmap(currentShot));
        icon->starMode = true;
        icon->starZ = zMin;
        icon->starAlpha = 0.0f;
        Tween::make()
            .to(icon->starZ, 1.0f)
            .to(icon->starAlpha, 1.0f)
            .seconds(secs)
            .onComplete([=]() { icon->starMode = false; });
    };

    bool haveOut = outgoingShot >= 0 && outgoingShot < shotCount();
    if (!haveOut) {
        // Nothing to scatter out from (first shot of a song) -- just collapse in.
        startIn();
        return;
    }
    setupStars(shotBitmap(outgoingShot));
    icon->starMode = true;
    icon->starZ = 1.0f;
    icon->starAlpha = 1.0f;
    Tween::make()
        .to(icon->starZ, zMin)
        .to(icon->starAlpha, 0.0f)
        .seconds(secs)
        .onComplete([=]() { startIn(); });
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

    // A fresh transition owns the icon: clear any special render mode left over
    // from an interrupted effect so a stale particle cloud can't linger.
    icon->starMode = false;
    icon->mosaicMode = false;

    // Cycle through the available transition effects so successive shots animate
    // differently (zoom, fade, mosaic, starfield, ...). Append new effects here
    // and they slot into the rotation automatically.
    static const std::vector<void (ScreenshotTransitions::*)()> effects = {
        &ScreenshotTransitions::transitionZoom,
        &ScreenshotTransitions::transitionFade,
        &ScreenshotTransitions::transitionMosaic,
        &ScreenshotTransitions::transitionStarfield,
    };
    auto effect = effects[currentEffect % effects.size()];
    currentEffect = (currentEffect + 1) % (int)effects.size();
    (this->*effect)();
}

} // namespace chipmachine
