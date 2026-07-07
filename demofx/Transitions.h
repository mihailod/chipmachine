#ifndef CHIPMACHINE_TRANSITIONS_H
#define CHIPMACHINE_TRANSITIONS_H

#include <cstdint>
#include <functional>

#include <grappix/rectangle.h>
#include <image/bitmap.h>

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

private:
    // Individual transition effects, cycled by next().
    void transitionFade();
    void transitionZoom();
    void transitionMosaic();
    void transitionStarfield();
    // Reshuffle the mosaic tile reveal order onto the icon.
    void setupMosaicOrder();
    // Sample a grid of stars (positions + colors) from bm onto the icon.
    void setupStars(const image::bitmap& bm);
    // Swap in the current shot's bitmap and return its full-size rectangle.
    grappix::Rectangle swapToCurrentShot();

    Icon* icon = nullptr;
    std::function<int()> shotCount;
    std::function<const image::bitmap&(int)> shotBitmap;
    std::function<void()> updateArea;

    int currentShot = -1;
    int outgoingShot = -1;   // shot displayed before the current transition
    int currentEffect = 0;   // position in the effect rotation
    uint64_t setShotAt = 0;  // when the current shot began showing
};

} // namespace chipmachine

#endif // CHIPMACHINE_TRANSITIONS_H
