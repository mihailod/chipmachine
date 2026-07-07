#ifndef CHIPMACHINE_TRANSITIONS_H
#define CHIPMACHINE_TRANSITIONS_H

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

#include <grappix/rectangle.h>
#include <image/bitmap.h>

namespace grappix {
class RenderTarget;
}

namespace chipmachine {

class Icon;

// Drives the animated transitions between the screenshots shown in
// ChipMachine's screenshot area. Owns the effect rotation and the
// current/outgoing shot indices, and renders through an Icon (which knows how
// to draw a partially-revealed / scattered image). Kept decoupled from
// ChipMachine via a few callbacks:
//   shotCount  -> number of screenshots currently available
//   shotBitmap -> pixels of screenshot i (for CPU sampling, e.g. starfield)
//   updateArea -> recompute the icon's full-size on-screen rectangle
//
// To add a new effect: write a private transitionX() and append
// &ScreenshotTransitions::transitionX to the `effects` list in next().
class ScreenshotTransitions
{
public:
    ScreenshotTransitions() = default;

    void configure(Icon& icon, std::function<int()> shotCount,
                   std::function<const image::bitmap&(int)> shotBitmap,
                   std::function<void()> updateArea);

    // Restart the rotation from before the first shot, then animate it in. Used
    // when a fresh set of screenshots has just been loaded.
    void restart();
    // Advance to (and animate in) the next shot, cycling the effect list.
    void next();
    // True once the current shot has been showing for at least `ms`.
    bool idleFor(uint64_t ms) const;

    // Tunables. Each *Seconds is one direction of the transition (out phase,
    // then in phase), sized to match so the effects feel consistent.
    float fadeSeconds = 1.0f;
    float zoomSeconds = 1.0f;
    float mosaicSeconds = 1.0f;
    int mosaicGrid = 10;          // mosaic tiles: NxN
    float starSeconds = 1.0f;
    int starGrid = 64;            // starfield pixels sampled: NxN
    float starDepthMin = 0.04f;   // smallest starZ = maximum scatter (f = 1/z)
    float copperSeconds = 1.0f;
    int copperStrips = 16;        // image split into this many horizontal strips
    float warpSeconds = 1.0f;
    int warpColumns = 64;         // image split into this many vertical columns

private:
    // Individual transition effects, cycled by next().
    void transitionFade();
    void transitionZoom();
    void transitionMosaic();
    void transitionStarfield();
    void transitionCopperWipe();
    void transitionSineWarp();
    // Effect frame renderers, installed as the icon's custom renderer while the
    // matching effect runs. They draw into the icon's rect/texture.
    void renderMosaic(std::shared_ptr<grappix::RenderTarget> target);
    void renderStars(std::shared_ptr<grappix::RenderTarget> target);
    void renderCopper(std::shared_ptr<grappix::RenderTarget> target,
                      uint32_t delta);
    void renderSineWarp(std::shared_ptr<grappix::RenderTarget> target,
                        uint32_t delta);
    // Reshuffle the mosaic tile reveal order.
    void setupMosaicOrder();
    // Sample a grid of stars (positions + colors) from bm.
    void setupStars(const image::bitmap& bm);
    // Swap in the current shot's bitmap and return its full-size rectangle.
    grappix::Rectangle swapToCurrentShot();

    // Batched solid-colour quad renderer. Effects that draw thousands of small
    // coloured quads per frame (the starfield's particles, the copper bars)
    // accumulate them with pushQuad() and emit them in a single draw call via
    // drawColorQuads(), instead of thousands of RenderTarget::rectangle() calls
    // -- that per-call driver overhead was long enough to hitch the scroller.
    void pushQuad(float x, float y, float w, float h, uint32_t argb);
    void drawColorQuads(std::shared_ptr<grappix::RenderTarget> target);

    // Batched textured quad renderer, for effects that draw many sub-rects of
    // the same texture with the same tint per frame (mosaic tiles, copper
    // strips, sine-warp columns). Accumulate with pushTexQuad(), emit with one
    // draw call via drawTexQuads().
    void pushTexQuad(float x, float y, float w, float h, float s0, float t0,
                     float s1, float t1);
    void drawTexQuads(std::shared_ptr<grappix::RenderTarget> target,
                      uint32_t texId, uint32_t color);

    Icon* icon = nullptr;
    std::function<int()> shotCount;
    std::function<const image::bitmap&(int)> shotBitmap;
    std::function<void()> updateArea;

    int currentShot = -1;
    int outgoingShot = -1;   // shot displayed before the current transition
    int currentEffect = 0;   // position in the effect rotation
    uint64_t setShotAt = 0;  // when the current shot began showing

    // Mosaic runtime state (read by renderMosaic).
    int mosaicGridW = 0;
    int mosaicGridH = 0;
    float revealProgress = 1.0f;   // 0 = all black, 1 = whole image shown
    std::vector<int> tileRank;     // per-tile position in the shuffled reveal

    // Starfield runtime state (read by renderStars).
    struct Star
    {
        float hx, hy;      // home offset from rect center, as a fraction [-0.5,0.5]
        uint32_t color;    // sampled pixel, already in 0xAARRGGBB order
    };
    int starGridW = 0;
    int starGridH = 0;
    float starZ = 1.0f;        // depth: 1 = image intact, ->0 = flown at viewer
    float starAlpha = 1.0f;    // global fade of the star cloud
    std::vector<Star> stars;

    // Copper-wipe runtime state (read by renderCopper).
    float copperShift = 0.0f;        // strip horizontal slide, fraction of width
    float copperImageAlpha = 1.0f;   // image opacity over the bars
    float copperPhase = 0.0f;        // animated copper colour cycle

    // Sine-warp runtime state (read by renderSineWarp).
    float warpAmp = 0.0f;            // displacement amplitude, fraction of height
    float warpPhase = 0.0f;          // animated wave phase (oscillation)

    // Reused CPU-side vertex scratch for the batched renderers, kept across
    // frames to avoid reallocating. quadVerts: x,y,r,g,b,a per vertex.
    // texVerts: x,y,u,v per vertex. 6 vertices per quad.
    std::vector<float> quadVerts;
    std::vector<float> texVerts;
};

} // namespace chipmachine

#endif // CHIPMACHINE_TRANSITIONS_H
