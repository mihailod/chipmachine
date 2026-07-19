#include "ChipMachine.h"
#include "Icons.h"
#include "version.h"
#ifdef __APPLE__
#    include "macnative/FileOpenHandler.h"
#endif
#include <coreutils/environment.h>
#include <coreutils/format.h>
#include <coreutils/searchpath.h>
#include <grappix/window.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <map>
#include <random>
#include <set>
#include <thread>
#ifdef _WIN32
#    include <ShellApi.h>
#endif

using namespace grappix;
using tween::Tween;

void initYoutube(sol::state&);

std::string compressWhitespace(std::string&& m)
{
    replace(begin(m), end(m), '\n', ' ');
    auto last = unique(begin(m), end(m),
                       [](char a, char b) { return (a | b) <= 0x20; });
    m.resize(distance(begin(m), last));
    return m;
}

std::string compressWhitespace(std::string const& text)
{
    return compressWhitespace(std::string(text));
}

namespace chipmachine {

// Decode a list of image files in parallel into CPU-side bitmaps, applying the
// pure-black -> transparent keying every logo/screenshot uses (so images
// exported on a black background show the starfield through; real RGBA alpha is
// preserved either way). Result index i corresponds to paths[i]; a failed
// decode leaves an empty bitmap (width 0) there.
//
// Startup used to decode all ~117 logos serially (~2.6s of the ~3.1s to splash).
// stb_image decodes are independent and stateless, and the keying is a local
// pixel loop with no shared state or GL calls, so this fans the work across all
// cores. GL texture upload still happens later, on the render thread.
static std::vector<image::bitmap> decodeLogosParallel(
    const std::vector<std::string>& paths)
{
    std::vector<image::bitmap> out(paths.size());
    if (paths.empty())
        return out;

    unsigned hw = std::thread::hardware_concurrency();
    unsigned nthreads =
        std::min<unsigned>(paths.size(), hw ? hw : 4u);

    std::atomic<size_t> next{ 0 };
    auto worker = [&]() {
        for (;;) {
            size_t i = next.fetch_add(1, std::memory_order_relaxed);
            if (i >= paths.size())
                break;
            try {
                auto bm = image::load_image(paths[i]);
                for (auto& px : bm)
                    if ((px & 0xffffff) == 0)
                        px &= 0xffffff;
                out[i] = std::move(bm);
            } catch (image::image_exception& e) {
                LOGW("Could not decode logo %s", paths[i]);
            }
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(nthreads);
    for (unsigned t = 0; t < nthreads; t++)
        pool.emplace_back(worker);
    for (auto& th : pool)
        th.join();
    return out;
}

const std::vector<FilterOption> ChipMachine::filterOptions = {
    { "[no filter, search all]", {} },
    { "Amiga", { AMIGA, PROTRACKER, SOUNDTRACKER, UADE, TRACKER } },
    // Children in chronological order (VCS 1977 first), not by corpus size.
    // "Atari" is the house logo (Atari.png) for the group row itself.
    { "Atari", {}, {
        { "Atari VCS Console", { ATARIVCS } },
        { "Atari 8bit Computers (XL/XE)", { POKEY } },
        { "Atari 7800 Console", { ATARI7800 } },
        { "Atari ST/STE/TT", { ATARI } },
        { "Atari Falcon", { ATARIFALCON } },
        { "Atari Lynx", { ATARILYNX } },
        { "Atari Jaguar", { ATARIJAGUAR } },
    }, "Atari" },
    { "Commodore 64 (SID)", { SID, STR } },
    { "Commodore 16/116/+4 (TED)", { PRG } },
    { "Commodore VIC-20", { VIC20 } },
    { "ZX Spectrum 16K/48K (Beeper)", { ZXBEEPER } },
    { "ZX Spectrum 128K (AY)", { ZXAY, SPECTRUM } },
    { "IBM PC (Trackers/DAWs)", { FASTTRACKER, IMPULSETRACKER, SCREAMTRACKER, PCTRACKER, PC } },
    { "IBM PC (AdLib/OPL)", { ADPLUG } },
    { "MSX", { MSX } },
    { "Amstrad CPC", { AMSTRAD } },
    { "Sam Coupe", { SAMCOUPE } },
    { "Acorn Archimedes", { ACORN } },
    { "Apple", {}, {
        { "Original Apple Mac", { APPLEMAC } },
        { "Apple IIGS", { APPLE } },
        { "Mac OS", { MACOS } },
        { "iOS", { IOS } },
    }, "Apple" },
    { "Sony", {}, {
        { "Sony PlayStation 1/2", { PLAYSTATION, PLAYSTATION2 } },
        { "Sony PlayStation 3", { PS3 } },
        { "Sony PSP", { PSP } },
    }, "Sony" },
    { "Nintendo", {}, {
        { "Nintendo NES", { NES } },
        { "Nintendo SNES", { SNES } },
        { "Nintendo GameBoy/GBA", { GAMEBOY, GBA } },
        { "Nintendo 64", { NINTENDO64 } },
        { "Nintendo DS", { NDS } },
        { "Nintendo 3DS", { N3DS } },
        { "Nintendo GameCube", { GAMECUBE } },
        { "Nintendo Wii", { WII } },
        { "Nintendo Virtual Boy", { VIRTUALBOY } },
    }, "Nintendo" },
    { "Microsoft", {}, {
        { "Microsoft Xbox", { XBOX } },
        { "Microsoft Xbox 360", { XBOX360 } },
    }, "Microsoft" },
    { "Sega", {}, {
        { "Sega 8bit", { SEGAMS } },
        { "Sega 16bit/32X/Saturn", { SEGA, MEGADRIVE, SATURN } },
        { "Sega Dreamcast", { DREAMCAST } },
    }, "Sega" },
    { "Japanese Computers", {}, {
        { "PC-98", { JPFM } },
        { "X68000", { JPX68000 } },
        { "FM Towns", { JPFMTOWNS } },
    }, "Japanese Computers" },
    { "PC Engine/TurboGrafx-16", { HES } },
    { "WonderSwan", { WONDERSWAN } },
    { "Neo Geo Pocket", { NEOGEOPOCKET } },
    { "Arcade", { ARCADE } },
    // "Other" is too generic a byte-slug to outrank a specific ext logo
    // elsewhere (see platformSlugForByte's specificPlatform check), so this
    // group-less leaf row still needs its own explicit logo, same as the
    // three rows below.
    { "Other Platforms", { OTHER }, {}, "Other" },
    // Rendered audio with no hardware identity: BotB allgear/remix/wildchip/
    // fakebit compos, standalone demoscene MP3s, untagged chipmusic.org tracks.
    // Not "unclassified" -- the source itself declares no platform.
    // MP3/OGG/Radio/Podcast are "nonHardware" in platformSlugForByte, so their
    // byte resolves to no slug at all -- these rows need an explicit `logo` or
    // the TAB filter screen shows nothing when they're highlighted.
    { "MP3/OGG (no platform)", { MP3, OGG }, {}, "MP3-OGG" },
    // No "YouTube (no platform)" row: a capture whose pouet tag names no
    // hardware (Animation/Video, mIRC, Alambik, Wild, JavaScript, ...) resolves
    // to OTHER, so it lands in the Other Platforms drill alongside the
    // "Youtube (Wild)" / "Youtube (JavaScript)" groups that already live there.
    // The YOUTUBE byte is consequently never produced (see formatToByte).
    { "Podcasts", { PODCAST }, {}, "Podcasts" },
    { "Radio Stations", { RADIO }, {}, "Radio" }
};

// Base color for a format byte. Shared by the now-playing list (renderSong)
// and the TAB filter screen so platforms keep a consistent color everywhere.
static uint32_t formatColor(int f)
{
    static const std::map<uint32_t, uint32_t> colors = {
        { NOT_SET, 0xffff00ff }, { PLAYLIST, 0xffffff88 },
        { OTHER, 0xffdd3355 },
        { ARCADE, 0xffb060e0 },
        { HES, 0xffee7766 },
        { NES, 0xffe05555 },     { SNES, 0xff9a7bd0 },
        { GAMEBOY, 0xff9bbc0f },  { GBA, 0xff9bbc0f },
        { NINTENDO64, 0xff4466cc },
        { NDS, 0xff55ccbb },     { SEGAMS, 0xff66aaee },
        { SEGA, 0xff3377dd },    { MEGADRIVE, 0xff3377dd },
        { DREAMCAST, 0xffee8844 }, { SATURN, 0xff4488cc },
        { WONDERSWAN, 0xff88ccaa }, { PLAYSTATION, 0xffbbbbbb },
        { PLAYSTATION2, 0xffbbbbbb },
        { N3DS, 0xffcc4477 },     { GAMECUBE, 0xff7755bb },
        { WII, 0xffddddee },      { PS3, 0xff8899bb },
        { PSP, 0xff6677aa },      { XBOX, 0xff33bb44 },
        { XBOX360, 0xff55cc66 },
        { SID, 0xffcc8844 },     { PRG, 0xffbb66cc },
        { VIC20, 0xff55bbdd },    { NEOGEOPOCKET, 0xffdd4444 },
        { ZXBEEPER, 0xffff88dd }, { ZXAY, 0xffbb88ff }, { SPECTRUM, 0xffbb88ff },
        { MSX, 0xff66ddaa },     { AMSTRAD, 0xff44aadd },
        { ACORN, 0xff88dd55 },   { SAMCOUPE, 0xffdd66aa },
        { ATARI, 0xffcccc33 },   { POKEY, 0xffee7711 },
        // The Atari family. Each needs its own entry: formatColor does a range
        // lookup, so without one they would inherit VIRTUALBOY's red.
        { ATARIVCS, 0xffcc7733 },   // the VCS woodgrain
        { ATARI7800, 0xffdd9955 },
        { ATARIFALCON, 0xffaacc66 },
        { ATARILYNX, 0xffdddd55 },
        { ATARIJAGUAR, 0xffbb8833 },
        { MP3, 0xff88ff88 },
        { APPLE, 0xff66cccc },
        { APPLEMAC, 0xffaaaaaa }, { MACOS, 0xff88bbcc }, { IOS, 0xffcccccc },
        // The VB's red-LED display. Needs its own entry: formatColor does a
        // range lookup, so without one it would inherit IOS's grey.
        { VIRTUALBOY, 0xffee1122 },
        { M3U, 0xffaaddaa },     { RADIO, 0xffff7722 },
        { YOUTUBE, 0xffff0000 },
        { PODCAST, 0xff22bbff },
        { PC, 0xffcccccc },      { JPFM, 0xffff66cc },
        { JPX68000, 0xffff8844 }, { JPFMTOWNS, 0xffcc66ff },
        { ADPLUG, 0xffe8c040 },
        { AMIGA, 0xff6666cc },
        { SCREAMTRACKER, 0xffaaccee }, { PCTRACKER, 0xffaaccee },
        { PRODUCT, 0xffff88cc }, { 255, 0xff00ffff }
    };
    auto it = --colors.upper_bound((uint32_t)f);
    return it->second;
}

// Vary a base color by an evenly-spaced position t in [0,1) -- this sub-format's
// slot among the distinct formats present in the active platform filter. Because
// the slots are evenly spaced, two-format platforms separate as widely as
// many-format ones. Spreads hue generously plus a brightness/saturation gradient
// (so even desaturated base colors stay distinguishable).
static uint32_t shiftColorBySpread(uint32_t argb, float t)
{
    uint32_t a = (argb >> 24) & 0xff;
    float r = ((argb >> 16) & 0xff) / 255.f;
    float g = ((argb >> 8) & 0xff) / 255.f;
    float b = (argb & 0xff) / 255.f;
    float mx = r > g ? (r > b ? r : b) : (g > b ? g : b);
    float mn = r < g ? (r < b ? r : b) : (g < b ? g : b);
    float v = mx, d = mx - mn;
    float s = mx <= 0.f ? 0.f : d / mx;
    float h = 0.f;
    if (d > 0.f) {
        if (mx == r) h = (g - b) / d + (g < b ? 6.f : 0.f);
        else if (mx == g) h = (b - r) / d + 2.f;
        else h = (r - g) / d + 4.f;
        h *= 60.f;
    }
    h += (t - 0.5f) * 200.f; // +-100 deg, evenly spread across the formats
    if (h < 0.f) h += 360.f;
    if (h >= 360.f) h -= 360.f;
    v *= 0.72f + 0.28f * (1.f - t); // brightness gradient over the spread
    // Lift saturation to a vivid floor (still varied by t) so the hue steps read
    // as distinct colours even when the platform's base is a washed-out tint
    // (e.g. the ZX lavender) -- otherwise adjacent formats look like the same
    // muted shade. Only raises saturation, never lowers an already-vivid base.
    float sfloor = 0.55f + 0.30f * t;
    if (s < sfloor) s = sfloor;
    if (v > 1.f) v = 1.f;
    if (s > 1.f) s = 1.f;
    float cc = v * s;
    float x = cc * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f));
    float m = v - cc;
    float rr = 0, gg = 0, bb = 0;
    switch ((int)(h / 60.f) % 6) {
    case 0: rr = cc; gg = x; break;
    case 1: rr = x; gg = cc; break;
    case 2: gg = cc; bb = x; break;
    case 3: gg = x; bb = cc; break;
    case 4: rr = x; bb = cc; break;
    default: rr = cc; bb = x; break;
    }
    auto q = [](float f) -> uint32_t {
        int v = (int)((f) * 255.f + 0.5f);
        return v < 0 ? 0 : (v > 255 ? 255 : v);
    };
    return (a << 24) | (q(rr + m) << 16) | (q(gg + m) << 8) | q(bb + m);
}

void ChipMachine::renderSong(grappix::Rectangle const& rec, int y,
                             uint32_t index, bool hilight)
{
    Color c;
    std::string text;

    auto res = iquery->getResult(index);
    auto parts = utils::split(res, "\t");
    int f = std::stol(parts[3]) & 0xff;
    long fullIndex = std::stol(parts[2]);
    bool isShow = fullIndex >= MusicDatabase::PODCAST_SHOW_INDEX;

    if (isShow) {
        // Podcast show row: a drillable group, shown like a folder.
        text = utils::format("> %s", parts[0]);
    } else if (f == PLAYLIST || f == PRODUCT) {
        if (parts[1] == nullptr || parts[1][0] == '\0')
            text = utils::format("<%s>", parts[0]);
        else
            text = utils::format("<%s / %s>", parts[0], parts[1]);
    } else {
        if (parts[1] == nullptr || parts[1][0] == '\0')
            text = parts[0];
        else
            // Empty composer stays title-only (above); a non-empty but cryptic
            // placeholder ("?"/"<?>"/...) folds to the readable "Uncredited
            // Composer" for display, matching the info panel. DISPLAY ONLY.
            text = utils::format("%s / %s", parts[0], displayComposer(parts[1]));
    }
    uint32_t base = formatColor(f);
    if (fullIndex >= MusicDatabase::OTHER_PLATFORM_INDEX) {
        // Other Platforms group row: the OTHER byte is one flat deep red, so
        // give each sub-platform its own colour, spread evenly across all groups
        // (in alphabetical order) so the list reads as a rainbow, not a wall.
        // The songs inside a group keep the normal per-format colour below.
        int gid = (int)(fullIndex - MusicDatabase::OTHER_PLATFORM_INDEX);
        int n = (int)musicDatabase.otherPlatforms().size();
        base = shiftColorBySpread(base, n > 0 ? (gid + 0.5f) / (float)n : 0.5f);
    } else if (musicDatabase.hasFormatFilter() && f != PLAYLIST && f != PRODUCT) {
        // Inside a platform filter, vary the hue per sub-format/extension so the
        // different formats in the result list are distinguishable. The variation
        // is spread evenly across however many formats the platform has, so a
        // 2-format platform separates as widely as a 20-format one. General
        // (unfiltered) search keeps a single flat platform color as before.
        // Other-platform songs share one format each, so a whole drilled-in
        // platform reads as a single per-format colour, as before.
        float t = musicDatabase.formatSpread(fullIndex);
        if (t >= 0.f) base = shiftColorBySpread(base, t);
    }
    c = Color(base) * 0.75f;

    if (hilight) {
        static uint32_t markStartcolor = 0;
        if (markStartcolor != c) {
            markStartcolor = c;
            markColor = c;
            markTween = Tween::make()
                            .sine()
                            .repeating()
                            .from(markColor, hilightColor)
                            .seconds(1.0);
            markTween.start();
        }
        c = markColor;
    }

    grappix::screen.text(listFont, text, rec.x, rec.y, c,
                         resultFieldTemplate.scale);
}

ChipMachine::ChipMachine(utils::path const& wd, RemoteLoader& rl,
                         MusicPlayerList& mpl, MusicDatabase& mdb,
                         sol::state& _lua)
    : workDir(wd), remoteLoader(rl), player(mpl), musicDatabase(mdb), lua(_lua),
      currentScreen(MAIN_SCREEN), eq(SpectrumAnalyzer::eq_slots),
      eqLeft(SpectrumAnalyzer::eq_slots), eqRight(SpectrumAnalyzer::eq_slots),
      eqMono(SpectrumAnalyzer::eq_slots),
      starEffect(screen), scrollEffect(screen)
{
    isShuttingDown = false; // Safe initialization state

    screen.setTitle(PROGRAM_NAME " " VERSION_STR);

    // Font pool for the main scroller is populated from the config
    // (Settings.scroll[4] = folder) via loadScrollFonts(). Seed a sensible
    // default here so the scroller still has a font even if that entry is
    // missing from lua; the config load overrides it moments later.
    loadScrollFonts("data/fontsmainscroll");

#ifdef ENABLE_TELNET
    telnet = std::make_unique<TelnetInterface>(player);
    telnet->start();
#endif

    nextInfoField.setAlign(1.0);
    nextField.align = 1.0;

    // Load the big centred icon shown from launch until indexing finishes, as
    // early in setup() as possible so it appears the instant the app opens.
    try {
        auto ip = workDir / "data" / "misc" / "icon.png";
        auto bm = image::load_image(ip.string());
        // Key out the pure-black background so the starfield shows through,
        // same as paused.png/muted.png below and the platform logos.
        for (auto& px : bm)
            if ((px & 0xffffff) == 0) px &= 0xffffff;
        startupIcon.setBitmap(bm, true);
        updateStartupIconArea();
    } catch (image::image_exception& e) {
        LOGD("Failed to load icon.png startup splash");
    }

    screenShotIcon = Icon(image::bitmap(8, 8), 100, 100);
    mainScreen.add(&screenShotIcon);
    // Give the transition driver access to the screenshot icon, the loaded
    // screenshot bitmaps, and the geometry recompute.
    transitions.configure(
        screenShotIcon, [this] { return (int)screenshots.size(); },
        [this](int i) -> const image::bitmap& { return screenshots[i].bm; },
        [this] { updateScreenshotArea(); });

    // Idle splash: same effect rotation, but its own icon + picture set, drawn
    // centred and large (see updateSplashArea). Rendered manually in render()
    // only while idle, so it is NOT added to mainScreen.
    splashTransitions.configure(
        splashIcon, [this] { return (int)splashShots.size(); },
        [this](int i) -> const image::bitmap& { return splashShots[i].bm; },
        [this] { updateSplashArea(); });
    // Run the splash's transitions at 2x speed (half the per-effect duration).
    // The per-song `transitions` keeps its default 1.0s timings.
    splashTransitions.fadeSeconds = 0.5f;
    splashTransitions.zoomSeconds = 0.5f;
    splashTransitions.mosaicSeconds = 0.5f;
    splashTransitions.starSeconds = 0.5f;
    splashTransitions.copperSeconds = 0.5f;
    splashTransitions.warpSeconds = 0.5f;
    splashTransitions.cardFlipSeconds = 0.5f;
    splashTransitions.cubeSeconds = 0.5f;

    mainScreen.add(&prevInfoField);
    mainScreen.add(&currentInfoField);
    mainScreen.add(&nextInfoField);
    mainScreen.add(&outsideInfoField);

    mainScreen.add(&xinfoField);
    mainScreen.add(&nextField);
    mainScreen.add(&timeField);
    mainScreen.add(&lengthField);
    mainScreen.add(&songField);

    iquery = musicDatabase.createQuery();

    searchField.setPrompt("#");
    // Added first so the platform-logo preview draws beneath the search field
    // and result list (it only occupies the right-hand screenshot slot anyway).
    searchScreen.add(&searchLogoIcon);
    searchScreen.add(&searchField);
    searchField.visible(false);

    searchScreen.add(&topStatus);
    topStatus.visible(false);

    searchScreen.add(&sourceStatus);
    sourceStatus.visible(false);

    overlay.add(&toastField);

    Resources::getInstance().load<image::bitmap>(
        (Environment::getCacheDir() / "favicon.png").string(),
        [=](std::shared_ptr<image::bitmap> bitmap) {
            favIcon = Icon(heart_icon, favPos.x, favPos.y, favPos.w, favPos.h);
        },
        heart_icon);

    float ww = volume_icon.width() * 15;
    float hh = volume_icon.height() * 10;
    volPos = { ((float)screen.width() - ww) / 2.0f,
               ((float)screen.height() - hh) / 2.0f, ww, hh };
    volumeIcon = Icon(volume_icon, volPos.x, volPos.y, volPos.w, volPos.h);

    // Load the big "paused" glyph shown while paused. Missing file is not fatal:
    // the overlay is simply skipped when the icon has no texture.
    try {
        auto mp = workDir / "data" / "misc" / "paused.png";
        auto bm = image::load_image(mp.string());
        // Key out the pure-black background so the starfield shows through, the
        // same way platform logos are made transparent (see loadPlatformScreenshots).
        for (auto& px : bm)
            if ((px & 0xffffff) == 0) px &= 0xffffff;
        pausedIcon.setBitmap(bm, true);
    } catch (image::image_exception& e) {
        LOGD("Failed to load paused.png overlay");
    }

    // Load the "muted" glyph shown centred when the volume is turned down to
    // silence (in place of the volume bars). Black background keyed to alpha.
    try {
        auto mp = workDir / "data" / "misc" / "muted.png";
        auto bm = image::load_image(mp.string());
        for (auto& px : bm)
            if ((px & 0xffffff) == 0) px &= 0xffffff;
        mutedIcon.setBitmap(bm, true);
    } catch (image::image_exception& e) {
        LOGD("Failed to load muted.png overlay");
    }

    setupCommands();
    setupRules();

    initLua();
    layoutScreen();

    filterField = searchField;
    searchScreen.add(&filterField);
    filterField.visible(false);
    filterField.color = 0xff55ff55;

    mainScreen.add(&favIcon);
    favIcon.color = Color(favColor);

    netIcon = Icon(net_icon, 2, 2, 8 * 3, 5 * 3);
    mainScreen.add(&netIcon);
    netIcon.visible(false);
    showVolume = 0;

    // LIFETIME GUARD GATE ENFORCED HERE:
    player.setAudioCallback(
        [this](int16_t* ptr, int size) { 
            if (!isShuttingDown) {
                fft.addAudio(ptr, size); 
            }
        });

    musicBarsWidth = spectrumWidth;
    musicBars.setup(musicBarsWidth, spectrumHeight);

    LOGD("WORKDIR %s", workDir.string());

    // Preload per-platform logos (and warn about any that are missing) so they
    // are ready to rotate into the screenshot area when a song plays.
    loadPlatformScreenshots();
    // Preload per-extension screenshots; reports extensions not covered by a
    // platform logo. Must run after loadPlatformScreenshots().
    loadExtensionScreenshots();
    // Build the deduplicated splash picture set from the logos just loaded.
    loadSplashScreenshots();

    musicDatabase.initFromLuaAsync(this->workDir);

    if (musicDatabase.busy()) {
        indexingDatabase = true;
    }

    screenSize = screen.size();
    resizeDelay = 0;

    auto listrec =
        grappix::Rectangle(topLeft.x, topLeft.y + 30 * searchField.scale,
                           screen.width() - topLeft.x,
                           downRight.y - topLeft.y - searchField.scale * 30);
    songList =
        VerticalList(listrec, numLines,
                     [=](grappix::Rectangle& rec, int y, uint32_t index,
                         bool hilight) { renderSong(rec, y, index, hilight); });

    searchScreen.add(&songList);

    commandList = VerticalList(
        listrec, numLines,
        [=](grappix::Rectangle& rec, int y, uint32_t index, bool hilight) {
            if (index < matchingCommands.size()) {
                auto cmd = matchingCommands[index];
                // No selection highlight here: the whole help menu fits on one
                // screen, so there is nothing to navigate to (the `hilight` flag
                // is unused). Alternate the green shade per logical group so
                // adjacent command sets are visually distinct: even groups use
                // the base green, odd groups a lighter one.
                bool oddGroup =
                    index < matchingGroup.size() && (matchingGroup[index] & 1);
                uint32_t c = oddGroup ? 0xaaaaffaa : 0xaa00cc00;
                int cmdPos = rec.w * 0.6;
                std::string displayName = cmd->name;
                for (char& ch : displayName) {
                    if (ch == '_') ch = ' ';
                }
                // Push the row down by the accumulated group-divider gaps before
                // it (half a row per divider), so logical sets read apart without
                // dividers being selectable rows of their own.
                float py = rec.y + (index < matchingGap.size()
                                        ? rec.h * matchingGap[index]
                                        : 0.0f);
                // Grow the font with the taller dedicated help area (see
                // commandFontFactor in updateLists), so the roomier help screen
                // reads bigger.
                float fscale = resultFieldTemplate.scale * commandFontFactor;
                grappix::screen.text(listFont, displayName, rec.x, py, c,
                                     fscale);
                grappix::screen.text(listFont, cmd->shortcut, rec.x + cmdPos,
                                     py, 0xffffffff, fscale * 0.8);
            }
        });

    clearCommand(); // fills matchingCommands (bindable commands only)
    commandList.setTotal(matchingCommands.size());

    updateLists();

    commandScreen.add(&commandField);
    commandScreen.add(&commandList);

    commandTitle.setFont(font);
    commandTitle.color = 0xffffffaa;
    commandTitle.scale = searchField.scale;
    commandTitle.visible(true);
    commandTitle.setText(PROGRAM_NAME " " VERSION_STR " HELP MENU (CTRL+H)");
    commandScreen.add(&commandTitle);

    mainFilterField.setFont(font);
    mainFilterField.visible(true);
    mainFilterField.setText("");
    mainScreen.add(&mainFilterField);

    // Highlighted-platform logo drawn (dimmed) centred BEHIND the filter list,
    // so navigating the TAB screen previews which platform is selected. Added
    // first so it renders beneath the title and the two-column list.
    filterLogoIcon.color = 0x50ffffff;
    advancedScreen.add(&filterLogoIcon);

    advancedTitle.setFont(font);
    advancedTitle.color = 0xffffffaa;
    advancedTitle.scale = searchField.scale;
    advancedTitle.visible(true);
    advancedTitle.setText("PLATFORM FILTER");
    advancedScreen.add(&advancedTitle);

    // The key hint rides on the title's line in a smaller, dimmer font (same
    // treatment as the source-DB tag next to the format line).
    advancedHint.setFont(font);
    advancedHint.color = 0xffffff66;
    advancedHint.visible(true);
    advancedHint.setText("      ARROWS navigate   ENTER apply/drill   ESC go back");
    advancedScreen.add(&advancedHint);
    // layoutScreen() already ran (it only had advancedTitle's pos/scale to go
    // on); now that the title carries its font and text, its width is real, so
    // park the hint against it.
    positionAdvancedHint();

    // The filter screen lays its entries out in two columns (column-major: the
    // left column holds the first half, the right column the rest) so all
    // platforms fit without scrolling. Up/Down still walk the single selection
    // index (down the left column, then down the right). visibleItems is set to
    // the item count so the list renders every entry (and never scrolls).
    advancedList = VerticalList(
        listrec, (int)filterOptions.size(),
        [=](grappix::Rectangle& rec, int y, uint32_t index, bool hilight) {
            auto const& opts = currentFilterOptions();
            if (index >= opts.size()) return;
            auto const& opt = opts[index];
            // Inherit the platform's color (see formatColor / renderSong); the
            // "[No Filter]" entry has no single format, so render white. A group
            // ("Nintendo"/"Sony") borrows the colour of its first child platform.
            uint8_t colorByte =
                !opt.matchedFormats.empty()
                    ? opt.matchedFormats[0]
                    : (!opt.children.empty() &&
                       !opt.children[0].matchedFormats.empty()
                           ? opt.children[0].matchedFormats[0]
                           : 0);
            uint32_t c =
                (colorByte == 0) ? 0xffffffff : formatColor(colorByte);
            if (hilight) {
                // The pulse animates baseColor <-> hilightColor. hilightColor is
                // white, so a white base ("[Show All]") would pulse white<->white
                // = no visible glow. Pulse it toward a grey instead so its
                // selection reads as clearly as the coloured rows.
                Color target =
                    (c == 0xffffffff) ? Color(0xff707070) : hilightColor;
                static uint32_t markStartcolor = 0;
                if (markStartcolor != c) {
                    markStartcolor = c;
                    markColor = c;
                    markTween = Tween::make()
                                    .sine()
                                    .repeating()
                                    .from(markColor, target)
                                    .seconds(1.0);
                    markTween.start();
                }
                c = markColor;
            }
            std::string label = opt.name;
            uint8_t fmt0 =
                opt.matchedFormats.empty() ? 0 : opt.matchedFormats[0];
            // A drilled-in submenu lays its (short) list out in a single column
            // aligned to the top-level's left column, rather than two columns.
            bool drilled = (activeFilterOptions != nullptr);
            // Prefix the Podcasts entry with the number of distinct shows, e.g.
            // "9 Podcasts  [1,497 episodes]".
            if (fmt0 == PODCAST && podcastShowCount > 0)
                label = utils::format("%d %s", podcastShowCount, opt.name);
            // Prefix the Other Platforms entry with the number of distinct
            // sub-platforms, e.g. "23 Other Platforms  [N tunes]".
            if (fmt0 == OTHER && otherPlatformCount > 0)
                label = utils::format("%d %s", otherPlatformCount, opt.name);
            // Same for the Arcade entry, e.g. "6 Arcade  [N tunes]".
            if (fmt0 == ARCADE && arcadePlatformCount > 0)
                label = utils::format("%d %s", arcadePlatformCount, opt.name);
            // A group ("Nintendo"/"Sony"): prefix with its sub-platform count,
            // e.g. "8 Nintendo  [N tunes]".
            if (!opt.children.empty())
                label = utils::format("%d %s", (int)opt.children.size(),
                                      opt.name);
            {
                int cnt = filterOptionCount(opt);
                if (fmt0 == RADIO) {
                    // Each radio entry IS one station, so just count-prefix the
                    // name ("10 Radio Stations") -- no "[N streams]" bracket.
                    label = utils::format("%s %s", withCommas(cnt), opt.name);
                } else {
                    // Count unit by platform: "[No Filter]" spans everything
                    // (tunes + podcasts + radio) so it counts in "items";
                    // podcasts in episodes, everything else in tunes.
                    const char* unit =
                        (opt.matchedFormats.empty() && opt.children.empty())
                            ? "items"
                            : (fmt0 == PODCAST ? "episodes" : "tunes");
                    label += utils::format("  [%s %s]", withCommas(cnt), unit);
                }
            }

            // Drill cue: a leading "> " marks rows that open ANOTHER browse level
            // rather than applying a filter to land on the tune list. Mirrors the
            // "> " renderSong already puts on the Other/Arcade/Podcast folder rows
            // (isShow branch). It shows on the level-1 aggregator/browse entries --
            // the groups with children (Nintendo/Sony/Atari/Apple/Microsoft/Sega/
            // Japanese Computers) and the System-B drill rows (Arcade/Other/
            // Podcasts) -- and on every row inside a drilled-in submenu, since each
            // sub-platform drills on to its filtered tunes (the next level). Plain
            // top-level platforms, [No Filter], MP3/OGG and Radio stay bare.
            bool drillCue = !opt.children.empty() || drilled ||
                            fmt0 == ARCADE || fmt0 == OTHER || fmt0 == PODCAST;
            if (drillCue) label = "> " + label;

            float lineH = advancedArea.h / (float)numLines;
            float px, py;
            if (drilled) {
                // Single column, aligned to the top-level's left column.
                px = advancedArea.x;
                py = advancedArea.y + lineH * ((int)index + 1);
            } else {
                int rows = ((int)opts.size() + 1) / 2;
                int col = (int)index / rows;
                int row = (int)index % rows;
                // Leave the top row of the right column empty so "[No Filter]"
                // (alone at the top of the left column) stands out: shift the
                // right column down by one row.
                row += col;
                float colW = advancedArea.w / 2.0f;
                px = advancedArea.x + col * colW;
                py = advancedArea.y + lineH * (row + 1);
            }
            grappix::screen.text(listFont, label, px, py, c,
                                 resultFieldTemplate.scale * 0.9f);
        });
    advancedList.setTotal(filterOptions.size());
    advancedList.setVisible((int)filterOptions.size());
    advancedList.setArea(advancedArea); // match the layout area (scissor clip)
    advancedScreen.add(&advancedList);

    // --- Formats screen ------------------------------------------------------
    // Highlighted-extension logo, dimmed and centred behind the list (added
    // first so it renders beneath the title and rows), same as the TAB screen.
    formatLogoIcon.color = 0x50ffffff;
    formatScreen.add(&formatLogoIcon);

    formatTitle.setFont(font);
    formatTitle.color = 0xffffffaa;
    formatTitle.scale = searchField.scale;
    formatTitle.visible(true);
    formatTitle.setText("FORMAT FILTER [TYPE TO NARROW]");
    formatScreen.add(&formatTitle);

    formatHint.setFont(font);
    formatHint.color = 0xffffff66;
    formatHint.visible(true);
    // Single column -> UP/DOWN only (LEFT/RIGHT are ignored here; see the noop
    // binding in setupRules). The 2-column Platforms screen keeps "ARROWS".
    formatHint.setText("      UP/DOWN navigate   ENTER apply   ESC go back");
    formatScreen.add(&formatHint);
    positionFormatHint();

    // A single scrolling column: "EXT   count   name" per row. Unlike the TAB
    // list (which sets visibleItems == size and never scrolls), there are
    // hundreds of extensions, so it scrolls like the search results (numLines).
    formatList = VerticalList(
        listrec, numLines,
        [=](grappix::Rectangle& rec, int y, uint32_t index, bool hilight) {
            auto const& groups = musicDatabase.extensionGroups();
            // Row 0 is the clear-filter entry (like the TAB screen's [no filter]);
            // the narrowed group at visible position g renders on row g+1.
            bool noFilter = (index == 0);
            if (!noFilter && (index - 1) >= formatVisibleGroups.size()) return;
            uint32_t c;
            std::string ext, cnt, name;
            if (noFilter) {
                c = 0xffffffff; // white; no single platform
                name = "[no filter, search all]";
            } else {
                auto const& g = groups[formatVisibleGroups[index - 1]];
                // Same platform colouring as the TAB filter and results list.
                c = formatColor(g.platform);
                ext = "." + g.ext;
                for (auto& ch : ext) ch = (char)toupper((unsigned char)ch);
                cnt = withCommas(g.count);
                name = g.name;
            }
            if (hilight) {
                // A white base ("[no filter]") pulses toward grey so its
                // selection reads; coloured rows pulse toward white.
                Color target =
                    (c == 0xffffffff) ? Color(0xff707070) : hilightColor;
                static uint32_t markStartcolor = 0;
                if (markStartcolor != c) {
                    markStartcolor = c;
                    markColor = c;
                    markTween = Tween::make()
                                    .sine()
                                    .repeating()
                                    .from(markColor, target)
                                    .seconds(1.0);
                    markTween.start();
                }
                c = markColor;
            }
            float scale = resultFieldTemplate.scale * 0.9f;
            if (noFilter) {
                grappix::screen.text(listFont, name, rec.x, rec.y, c, scale);
                return;
            }
            // Three columns at fixed fractions of the row width.
            grappix::screen.text(listFont, ext, rec.x, rec.y, c, scale);
            grappix::screen.text(listFont, cnt, rec.x + rec.w * 0.16f, rec.y, c,
                                 scale);
            grappix::screen.text(listFont, name, rec.x + rec.w * 0.34f, rec.y, c,
                                 scale);
        });
    formatList.setTotal(0); // populated when the screen is opened (see commands)
    formatList.setVisible(numLines);
    formatList.setArea(listrec);
    formatScreen.add(&formatList);

    // --- Databases screen ----------------------------------------------------
    // Same single-column layout as Formats, one row per source collection.
    databaseLogoIcon.color = 0x50ffffff;
    databaseScreen.add(&databaseLogoIcon);

    databaseTitle.setFont(font);
    databaseTitle.color = 0xffffffaa;
    databaseTitle.scale = searchField.scale;
    databaseTitle.visible(true);
    databaseTitle.setText("DATABASE FILTER");
    databaseScreen.add(&databaseTitle);

    databaseHint.setFont(font);
    databaseHint.color = 0xffffff66;
    databaseHint.visible(true);
    databaseHint.setText("      UP/DOWN navigate   ENTER apply   ESC go back");
    databaseScreen.add(&databaseHint);
    positionDatabaseHint();

    databaseList = VerticalList(
        listrec, numLines,
        [=](grappix::Rectangle& rec, int y, uint32_t index, bool hilight) {
            auto const& groups = musicDatabase.databaseGroups();
            // Row 0 is the clear-filter entry; collection g renders on row g+1.
            bool noFilter = (index == 0);
            if (!noFilter && (index - 1) >= groups.size()) return;
            uint32_t c;
            std::string id, cnt, name;
            if (noFilter) {
                c = 0xffffffff;
                name = "[no filter, search all]";
            } else {
                auto const& g = groups[index - 1];
                c = formatColor(g.platform);
                id = g.id;
                cnt = withCommas(g.count);
                name = g.name;
            }
            if (hilight) {
                Color target =
                    (c == 0xffffffff) ? Color(0xff707070) : hilightColor;
                static uint32_t markStartcolor = 0;
                if (markStartcolor != c) {
                    markStartcolor = c;
                    markColor = c;
                    markTween = Tween::make()
                                    .sine()
                                    .repeating()
                                    .from(markColor, target)
                                    .seconds(1.0);
                    markTween.start();
                }
                c = markColor;
            }
            float scale = resultFieldTemplate.scale * 0.9f;
            if (noFilter) {
                grappix::screen.text(listFont, name, rec.x, rec.y, c, scale);
                return;
            }
            // Three columns: id / count / display name (ids run a touch longer
            // than extensions, so the columns sit slightly wider than Formats').
            grappix::screen.text(listFont, id, rec.x, rec.y, c, scale);
            grappix::screen.text(listFont, cnt, rec.x + rec.w * 0.18f, rec.y, c,
                                 scale);
            grappix::screen.text(listFont, name, rec.x + rec.w * 0.36f, rec.y, c,
                                 scale);
        });
    databaseList.setTotal(0); // populated when the screen is opened
    databaseList.setVisible(numLines);
    databaseList.setArea(listrec);
    databaseScreen.add(&databaseList);

    // --- Plugins screen ------------------------------------------------------
    // Same single-column layout as Databases, one row per registered ChipPlugin.
    pluginLogoIcon.color = 0x50ffffff;
    pluginScreen.add(&pluginLogoIcon);

    pluginTitle.setFont(font);
    pluginTitle.color = 0xffffffaa;
    pluginTitle.scale = searchField.scale;
    pluginTitle.visible(true);
    pluginTitle.setText("PLUGIN FILTER [TYPE TO NARROW]");
    pluginScreen.add(&pluginTitle);

    pluginHint.setFont(font);
    pluginHint.color = 0xffffff66;
    pluginHint.visible(true);
    pluginHint.setText("      UP/DOWN navigate   ENTER apply   ESC go back");
    pluginScreen.add(&pluginHint);
    positionPluginHint();

    pluginList = VerticalList(
        listrec, numLines,
        [=](grappix::Rectangle& rec, int y, uint32_t index, bool hilight) {
            auto const& groups = musicDatabase.pluginGroups();
            // Row 0 is the clear-filter entry; the narrowed plugin at visible
            // position g renders on row g+1.
            bool noFilter = (index == 0);
            if (!noFilter && (index - 1) >= pluginVisibleGroups.size()) return;
            uint32_t c;
            std::string cnt, name;
            if (noFilter) {
                c = 0xffffffff;
                name = "[no filter, search all]";
            } else {
                auto const& g = groups[pluginVisibleGroups[index - 1]];
                c = formatColor(g.platform);
                cnt = withCommas(g.count);
                name = g.name;
            }
            if (hilight) {
                Color target =
                    (c == 0xffffffff) ? Color(0xff707070) : hilightColor;
                static uint32_t markStartcolor = 0;
                if (markStartcolor != c) {
                    markStartcolor = c;
                    markColor = c;
                    markTween = Tween::make()
                                    .sine()
                                    .repeating()
                                    .from(markColor, target)
                                    .seconds(1.0);
                    markTween.start();
                }
                c = markColor;
            }
            float scale = resultFieldTemplate.scale * 0.9f;
            if (noFilter) {
                grappix::screen.text(listFont, name, rec.x, rec.y, c, scale);
                return;
            }
            // Two columns: song count, then the plugin's full name().
            grappix::screen.text(listFont, cnt, rec.x, rec.y, c, scale);
            grappix::screen.text(listFont, name, rec.x + rec.w * 0.18f, rec.y, c,
                                 scale);
        });
    pluginList.setTotal(0); // populated when the screen is opened
    pluginList.setVisible(numLines);
    pluginList.setArea(listrec);
    pluginScreen.add(&pluginList);

    scrollText = "INITIAL_TEXT";
    scrollEffect.set("scortest",
        "Just type to search"
        " . . TAB to cycle filters"
        " . . UP+DOWN/ENTER to navigate/play"
        " . . CTRL+H for help . . . "
    );
    starEffect.fadeIn();
    }

ChipMachine::~ChipMachine()
{
    // 1. Immediately drop the atomic gate block to reject processing calls
    isShuttingDown = true;

    // 2. Erase the functional reference stored within the active player structure
    player.setAudioCallback(nullptr);

#ifdef ENABLE_TELNET
    if (telnet) telnet->stop();
#endif
}

void ChipMachine::setScrolltext(std::string const& txt)
{
    scrollEffect.set("scrolltext", txt);
}

std::string ChipMachine::appendFormatInfo(std::string const& text,
                                          SongInfo const& info)
{
    if (info.format.empty()) return text;

    // "Platform - Name (EXT)" plus, if listed, "<trackers> - <description>".
    // Use the pre-resolved currentSongExt rather than re-deriving from
    // info.format: by the time this runs, both callers have already overwritten
    // info.format with the describeFormat() DISPLAY string ("Apple IIGS -
    // SoundSmith (W)"), which no longer resolves to a key. currentSongExt was set
    // from the RAW format just before, via songExtension()/resolveExtension().
    std::string fmt = info.format;
    auto desc = musicDatabase.describeExtension(currentSongExt);
    if (!desc.empty()) fmt += " ... " + desc;

    // Dots give a clean gap between sections and before the line repeats.
    if (text.empty()) return "... " + fmt + " ...";
    return text + " ... " + fmt + " ...";
}

void ChipMachine::initLua()
{
    lua["set_var"] = sol::overload(
        [=](std::string const& name, uint32_t index, std::string const& val) {
            setVariable(name, index, val);
        },
        [=](std::string const& name, uint32_t index, double val) {
            setVariable(name, index, std::to_string(val));
        },
        [=](std::string const& name, uint32_t index, uint32_t val) {
            setVariable(name, index, std::to_string(val));
        });
}

void ChipMachine::layoutScreen()
{
    LOGD("LAYOUT SCREEN");
    currentTween.finish();
    currentTween = Tween();

    lua["on_layout"](screen.width(), screen.height(),
                     screen.getPPI() < 0 ? 100 : screen.getPPI());

    utils::File f(workDir / "lua" / "screen.lua");

    lua["SCREEN_WIDTH"] = screen.width();
    lua["SCREEN_HEIGHT"] = screen.height();
    lua["SCREEN_PPI"] = screen.getPPI() < 0 ? 100 : screen.getPPI();

    Resources::getInstance().load<std::string>(
        f.getName(), [=](std::shared_ptr<std::string> contents) {
            lua.script(*contents);
            lua.script(R"(
            for a,b in pairs(Settings) do
                if type(b) == 'table' then
                    for a1,b1 in ipairs(b) do
                        set_var(a, a1, b1)
                    end
                else
                    set_var(a, 0, b)
                end
            end
        )");
        });

    starEffect.resize(screen.width(), screen.height());
    scrollEffect.resize(screen.width(), 300);
    updateStartupIconArea();
    musicBarsWidth = stereoSpectrum ? spectrumWidth : spectrumWidth * 2;
    musicBars.setup(musicBarsWidth, spectrumHeight);
    updateScreenshotArea();

    searchField.setFont(font);
    commandField.pos = searchField.pos;
    commandField.scale = searchField.scale;
    commandField.cursorH = searchField.cursorH;
    commandField.cursorW = searchField.cursorW;

    advancedTitle.pos = { (float)topLeft.x, (float)topLeft.y };
    advancedTitle.scale = searchField.scale;
    positionAdvancedHint();

    formatTitle.pos = { (float)topLeft.x, (float)topLeft.y };
    formatTitle.scale = searchField.scale;
    positionFormatHint();

    databaseTitle.pos = { (float)topLeft.x, (float)topLeft.y };
    databaseTitle.scale = searchField.scale;
    positionDatabaseHint();

    pluginTitle.pos = { (float)topLeft.x, (float)topLeft.y };
    pluginTitle.scale = searchField.scale;
    positionPluginHint();

    // y is reclaimed (moved up) in updateLists(); x/scale here.
    commandTitle.pos = { (float)topLeft.x, topLeft.y * 0.90f };
    commandTitle.scale = searchField.scale;

    favIcon.setArea(favPos);

    float ww = volume_icon.width() * 15;
    float hh = volume_icon.height() * 10;
    volPos = { ((float)screen.width() - ww) / 2.0f,
               ((float)screen.height() - hh) / 2.0f, ww, hh };
    volumeIcon.setArea(volPos);

    // No title-marquee rebuild needed here: updateTitleMarquee() recomputes the
    // scroll extent from the live layout every frame, so a resize/maximize is
    // picked up automatically.
}

// Bounce-scroll the song title AND composer each frame when they are too wide to
// fit, so the whole text becomes visible instead of being cut off. Recomputes the
// overflow from each field's CURRENT width and the content bounds every call, so
// it stays correct across window resizes (the previous baked-tween version kept
// the distance captured at song start). Position is derived purely from
// titleMarqueePhase[], so a field can never get "stuck" scrolled off to one side.
void ChipMachine::updateTitleMarquee(uint32_t delta)
{
    if (!titleMarqueeActive)
        return;

    // The format field (index 2) is never scrolled, so its x is the un-scrolled
    // left/base x shared by the whole info column.
    float baseX = currentInfoField[2].pos.x;
    float avail = downRight.x - topLeft.x - 20;
    float gscale = screen.height() / 576.0f;

    // 0 = title, 1 = composer. Each field scrolls on its own phase so a long
    // title and a long composer bounce independently at the same visual speed.
    for (int i = 0; i < 2; i++) {
        int overflow = currentInfoField.getWidth(i) - (int)avail;

        if (overflow <= 20) {
            // Fits (or nearly): pin it to the base, no scrolling.
            currentInfoField[i].pos.x = baseX;
            titleMarqueePhase[i] = 0.0f;
            continue;
        }

        titleMarqueePhase[i] += delta / 1000.0f;

        // Constant on-screen scroll speed, independent of window size. The sweep
        // time is distance / velocity, and the velocity scales with the window
        // (gscale) exactly as the glyphs and the overflow distance do -- so
        // growing the window increases the distance and the speed together,
        // leaving the perceived scroll rate unchanged. (A fixed cycle time made
        // bigger windows, which overflow more pixels, appear to crawl -- the same
        // bug we fixed on the bottom scroller.) gscale matches Scroller.h.
        float speed = 330.0f * gscale;                            // px/sec; raise to go faster
        float cycle = 2.0f * overflow / speed;                    // seconds per out-and-back
        if (cycle < 1.0f) cycle = 1.0f;                           // gentle floor for near-fitting text
        float p = std::fmod(titleMarqueePhase[i], cycle) / cycle; // 0..1
        // Easing shape 0 -> 1 -> 0 over one cycle, dwelling briefly at each end.
        float ease = 0.5f - 0.5f * std::cos(p * 6.28318530718f);
        currentInfoField[i].pos.x = baseX - ease * overflow;
    }
}

void ChipMachine::play(SongInfo const& si)
{
    player.addSong(si);
    player.nextSong();
}

void ChipMachine::updateFavorite()
{
    auto favorites = musicDatabase.getPlaylist(currentPlaylistName);
    auto favsong =
        find_if(favorites.begin(), favorites.end(), [&](SongInfo const& song) {
            return (song.path == currentInfo.path &&
                    (currentTune == song.starttune ||
                     (currentTune == currentInfo.starttune &&
                      song.starttune == -1)));
        });
    isFavorite = (favsong != favorites.end());
    uint32_t alpha = isFavorite ? 0xff : 0x00;
    favIcon.color = Color(favColor | (alpha << 24));
}

void ChipMachine::updateScreenshotArea()
{
    int bm_w = screenShotIcon.getTextureWidth();
    int bm_h = screenShotIcon.getTextureHeight();
    if (bm_w == 0 || bm_h == 0) return;

    auto w = screen.width() * 0.45;
    auto h = screen.height() * 0.45;

    float d = (float)h / bm_h;
    float d2 = (float)w / bm_w;
    if (d2 < d) d = d2;

    float final_w = bm_w * d;
    float final_h = bm_h * d;

    // Horizontal: right edge anchored near the window edge (unchanged).
    float x = screen.width() - final_w - (screen.width() * 0.05);
    // Vertical: anchor every image's CENTER on the same line (the middle of the
    // 0.45-height slot), so pictures of differing heights/aspect ratios share a
    // common center of gravity and read as aligned, instead of pinning the top
    // edge (which left short/wide logos floating high and tall shots hanging low).
    float centerY = topLeft.y + screen.height() * 0.1 + h * 0.5;
    float y = centerY - final_h * 0.5;

    screenShotIcon.setArea(grappix::Rectangle(x, y, final_w, final_h));
}

// Fits searchLogoIcon's current texture into the playback screenshot slot (see
// updateScreenshotArea): inside the 0.45x0.45 box, right-anchored, centred on
// the same line. No-op when the icon has no texture.
void ChipMachine::positionSearchLogo()
{
    int bm_w = searchLogoIcon.getTextureWidth();
    int bm_h = searchLogoIcon.getTextureHeight();
    if (bm_w == 0 || bm_h == 0) return;
    auto w = screen.width() * 0.45;
    auto h = screen.height() * 0.45;
    float d = (float)h / bm_h;
    float d2 = (float)w / bm_w;
    if (d2 < d) d = d2;
    float final_w = bm_w * d;
    float final_h = bm_h * d;
    float x = screen.width() - final_w - (screen.width() * 0.05);
    float centerY = topLeft.y + screen.height() * 0.1 + h * 0.5;
    float y = centerY - final_h * 0.5;
    searchLogoIcon.setArea(grappix::Rectangle(x, y, final_w, final_h));
}

// Loads a remote artwork URL (podcast show image) into searchLogoIcon. The
// download runs async on the web worker; the decoded bitmap is handed to the
// render thread via pendingSearchLogo (setBitmap touches GL). searchLogoUrl
// tracks the desired image so a stale download that finishes after the cursor
// moved on is discarded.
void ChipMachine::loadSearchArtwork(const std::string& url)
{
    if (url.empty() || noImages) {
        searchLogoIcon.clear();
        searchLogoUrl = "";
        return;
    }
    // Already showing (or fetching) this exact image -> nothing to do.
    if (url == searchLogoUrl) return;
    searchLogoUrl = url;
    webutils::Web::getInstance().getFile(url, [this, url](utils::File f) {
        if (searchLogoUrl != url) return; // cursor moved to another row
        if (!f) return;
        try {
            auto bm = image::load_image(f.getName());
            // Key out pure black, matching the other logo/screenshot paths.
            for (auto& px : bm)
                if ((px & 0xffffff) == 0) px &= 0xffffff;
            pendingSearchLogoBm = bm;
            pendingSearchLogoUrl = url;
            // Publish to the render thread (which uploads the texture); pairs
            // with the acquire-load in update().
            pendingSearchLogo.store(true, std::memory_order_release);
        } catch (image::image_exception& e) {
            LOGD("Failed to load podcast artwork %s", url.c_str());
        }
    });
}

void ChipMachine::updateSearchLogo()
{
    // Preview the highlighted song's platform/extension logo in the screenshot
    // slot so the user sees which platform it resolves to before playing it.
    int i = (currentScreen == SEARCH_SCREEN) ? songList.selected() : -1;
    if (i < 0 || iquery->numHits() <= 0) {
        searchLogoIcon.clear();
        searchLogoUrl = "";
        return;
    }
    SongInfo song = musicDatabase.getSongInfo(iquery->getIndex(i));

    // Podcast SHOW rows carry no hardware platform; preview the show's own
    // artwork (a remote image) instead, loaded asynchronously into the slot.
    static const std::string kPodcastShowPrefix = "podcastshow::";
    if (utils::startsWith(song.path, kPodcastShowPrefix)) {
        int rowid = std::atoi(song.path.c_str() + kPodcastShowPrefix.size());
        loadSearchArtwork(musicDatabase.getPodcastShowArtwork(rowid));
        return;
    }

    // Radio stations carry their station logo URL in the INFO metadata field
    // (getSongScreenshots uses the same value as the screenshot); preview it in
    // the slot, loaded asynchronously like podcast artwork.
    if (utils::startsWith(song.path, "radio::")) {
        loadSearchArtwork(song.metadata[SongInfo::INFO]);
        return;
    }

    // Non-podcast rows use a local platform/extension logo; cancel any pending
    // artwork request so a late podcast download can't overwrite this logo.
    searchLogoUrl = "";
    std::string ext = MusicDatabase::resolveExtension(song);
    std::string platform = MusicDatabase::platformScreenshotName(song);
    std::string format = utils::toLower(song.format);
    // Other-platform / Arcade GROUP rows on the drill screen carry the
    // sub-platform only in their TITLE ("Vectrex", "ColecoVision",
    // "Arcade (Capcom)"); their format is the flat "Other"/"Arcade", which would
    // resolve to the generic icon (or generic cabinet). Feed the title in as the
    // format so pickPlatformOrExtLogo resolves the per-platform ("<row>.png",
    // matched case-insensitively) or per-board (arcadeSubLogos) logo. Normalise
    // the two arcade names that don't match arcadeSubLogos 1:1 (getOtherPlatforms).
    if (utils::startsWith(song.path, "otherplatform::") ||
        utils::startsWith(song.path, "othergroup::")) {
        std::string name = utils::toLower(song.title);
        if (name == "arcade (other)") name = "arcade";
        else if (name == "arcade (neo geo)") name = "neo geo";
        format = name;
    }
    const image::bitmap* bm = pickPlatformOrExtLogo(ext, platform, format);
    if (!bm) {
        searchLogoIcon.clear();
        return;
    }
    searchLogoIcon.setBitmap(*bm);
    positionSearchLogo();
}

void ChipMachine::updateFilterLogo()
{
    // Preview the highlighted platform/category behind the TAB filter list.
    if (currentScreen != ADVANCED_SCREEN) {
        filterLogoIcon.clear();
        return;
    }
    int i = advancedList.selected();
    auto const& opts = currentFilterOptions();
    const image::bitmap* bm = nullptr;
    if (i >= 0 && i < (int)opts.size()) {
        // The entry's first matched format byte names its platform ("[No Filter]"
        // matches nothing -> no logo). A group borrows its first child's
        // platform. Map it to the per-platform logo slug.
        auto const& opt = opts[i];
        // A group with a house logo ("Atari") shows the family mark rather than
        // borrowing its first child's ("Atari VCS"). Falls through to the borrow
        // when the image isn't installed, so naming one costs nothing until the
        // artwork lands.
        if (!opt.logo.empty()) bm = findPlatformShot(opt.logo);
        std::vector<uint8_t> const& fmts =
            !opt.matchedFormats.empty()
                ? opt.matchedFormats
                : (!opt.children.empty() ? opt.children[0].matchedFormats
                                         : opt.matchedFormats);
        if (bm == nullptr && !fmts.empty()) {
            std::string slug = MusicDatabase::platformScreenshotSlug(fmts[0]);
            bm = findPlatformShot(slug);
        }
    }
    if (!bm) {
        filterLogoIcon.clear();
        return;
    }
    filterLogoIcon.setBitmap(*bm);

    // Fit inside a centred box (same fraction as the splash), keeping aspect.
    int bm_w = bm->width();
    int bm_h = bm->height();
    float w = screen.width() * 0.5f;
    float h = screen.height() * 0.5f;
    float d = h / bm_h;
    float d2 = w / bm_w;
    if (d2 < d) d = d2;
    float final_w = bm_w * d;
    float final_h = bm_h * d;
    float x = (screen.width() - final_w) * 0.5f;
    float y = (screen.height() - final_h) * 0.5f;
    filterLogoIcon.setArea(grappix::Rectangle(x, y, final_w, final_h));
}

void ChipMachine::rebuildFormatVisible()
{
    auto const& groups = musicDatabase.extensionGroups();
    formatVisibleGroups.clear();
    // No query -> every group, in the database's own (count-sorted) order.
    // Otherwise keep a group if the (lowercased) query is a substring of either
    // its extension or its description name -- matching what the row displays.
    for (int i = 0; i < (int)groups.size(); i++) {
        if (formatFilterText.empty()) {
            formatVisibleGroups.push_back(i);
            continue;
        }
        std::string ext = utils::toLower(groups[i].ext);
        std::string name = utils::toLower(groups[i].name);
        if (ext.find(formatFilterText) != std::string::npos ||
            name.find(formatFilterText) != std::string::npos)
            formatVisibleGroups.push_back(i);
    }
    // Row 0 is the [no filter] entry, then one row per surviving group.
    formatList.setTotal((int)formatVisibleGroups.size() + 1);
    if (formatList.selected() >= (int)formatVisibleGroups.size() + 1)
        formatList.select(0);
    // Echo the query in the title so the user can see what they've typed.
    formatTitle.setText(formatFilterText.empty()
                            ? "FORMAT FILTER [TYPE TO NARROW]"
                            : "FORMAT FILTER [TYPE TO NARROW]  " +
                                  formatFilterText);
    // The query is echoed into the title, so it grows as the user types -- push
    // the key hint back out past the new title width so they never overlap.
    positionFormatHint();
    updateFormatLogo();
}

void ChipMachine::rebuildPluginVisible()
{
    auto const& groups = musicDatabase.pluginGroups();
    pluginVisibleGroups.clear();
    // No query -> every plugin, in the database's own (count-sorted) order.
    // Otherwise keep a plugin if the (lowercased) query is a substring of its
    // name -- the only thing the row displays besides the count.
    for (int i = 0; i < (int)groups.size(); i++) {
        if (pluginFilterText.empty()) {
            pluginVisibleGroups.push_back(i);
            continue;
        }
        std::string name = utils::toLower(groups[i].name);
        if (name.find(pluginFilterText) != std::string::npos)
            pluginVisibleGroups.push_back(i);
    }
    // Row 0 is the [no filter] entry, then one row per surviving plugin.
    pluginList.setTotal((int)pluginVisibleGroups.size() + 1);
    if (pluginList.selected() >= (int)pluginVisibleGroups.size() + 1)
        pluginList.select(0);
    // Echo the query in the title so the user can see what they've typed.
    pluginTitle.setText(pluginFilterText.empty()
                            ? "PLUGIN FILTER [TYPE TO NARROW]"
                            : "PLUGIN FILTER [TYPE TO NARROW]  " +
                                  pluginFilterText);
    // The query is echoed into the title, so it grows as the user types -- push
    // the key hint back out past the new title width so they never overlap.
    positionPluginHint();
    updatePluginLogo();
}

void ChipMachine::updateFormatLogo()
{
    // Preview the highlighted extension's logo behind the Formats list.
    if (currentScreen != FORMAT_SCREEN) {
        formatLogoIcon.clear();
        return;
    }
    auto const& groups = musicDatabase.extensionGroups();
    // Row 0 is [no filter] (no logo); the narrowed group at visible position g
    // is at list index g+1 -- map through formatVisibleGroups to the real group.
    int i = formatList.selected() - 1;
    const image::bitmap* bm = nullptr;
    if (i >= 0 && i < (int)formatVisibleGroups.size()) {
        auto const& g = groups[formatVisibleGroups[i]];
        // The per-extension screenshot (mod.png, sid.png, ...) if present, else
        // the row's platform logo. Same resolver the now-playing screen uses.
        std::string slug = MusicDatabase::platformScreenshotSlug(g.platform);
        bm = pickPlatformOrExtLogo(g.ext, slug, "");
    }
    if (!bm) {
        formatLogoIcon.clear();
        return;
    }
    formatLogoIcon.setBitmap(*bm);

    // Fit inside a centred box (same fraction as the TAB filter logo).
    int bm_w = bm->width();
    int bm_h = bm->height();
    float w = screen.width() * 0.5f;
    float h = screen.height() * 0.5f;
    float d = h / bm_h;
    float d2 = w / bm_w;
    if (d2 < d) d = d2;
    float final_w = bm_w * d;
    float final_h = bm_h * d;
    float x = (screen.width() - final_w) * 0.5f;
    float y = (screen.height() - final_h) * 0.5f;
    formatLogoIcon.setArea(grappix::Rectangle(x, y, final_w, final_h));
}

void ChipMachine::centerLogoIcon(Icon& icon, const image::bitmap& bm)
{
    icon.setBitmap(bm);
    int bm_w = bm.width();
    int bm_h = bm.height();
    if (bm_w <= 0 || bm_h <= 0) return;
    float w = screen.width() * 0.5f;
    float h = screen.height() * 0.5f;
    float d = h / bm_h;
    float d2 = w / bm_w;
    if (d2 < d) d = d2;
    float final_w = bm_w * d;
    float final_h = bm_h * d;
    icon.setArea(grappix::Rectangle((screen.width() - final_w) * 0.5f,
                                    (screen.height() - final_h) * 0.5f, final_w,
                                    final_h));
}

void ChipMachine::updateDatabaseLogo()
{
    // Preview the highlighted collection's logo behind the list.
    if (currentScreen != DATABASE_SCREEN) {
        databaseLogoIcon.clear();
        databaseLogoUrl = "";
        return;
    }
    auto const& groups = musicDatabase.databaseGroups();
    // Row 0 is [no filter] (no logo); collection g is at list index g+1.
    int i = databaseList.selected() - 1;
    if (i < 0 || i >= (int)groups.size()) {
        databaseLogoIcon.clear();
        databaseLogoUrl = "";
        return;
    }
    auto const& g = groups[i];
    // Podcasts (and other platformless collections) have no hardware logo -- show
    // the collection's own artwork instead, loaded remotely, exactly as the
    // search screen previews a podcast show.
    if (g.platform == PODCAST) {
        loadDatabaseArtwork(musicDatabase.getPodcastShowArtwork(g.rowid));
        return;
    }
    databaseLogoUrl = ""; // cancel any pending remote load from a prior row
    std::string slug = MusicDatabase::platformScreenshotSlug(g.platform);
    const image::bitmap* bm = pickPlatformOrExtLogo("", slug, "");
    if (!bm) {
        databaseLogoIcon.clear();
        return;
    }
    centerLogoIcon(databaseLogoIcon, *bm);
}

void ChipMachine::loadDatabaseArtwork(const std::string& url)
{
    if (url.empty() || noImages) {
        databaseLogoIcon.clear();
        databaseLogoUrl = "";
        return;
    }
    if (url == databaseLogoUrl) return; // already showing / fetching this one
    databaseLogoUrl = url;
    webutils::Web::getInstance().getFile(url, [this, url](utils::File f) {
        if (databaseLogoUrl != url) return; // cursor moved to another row
        if (!f) return;
        try {
            auto bm = image::load_image(f.getName());
            for (auto& px : bm)
                if ((px & 0xffffff) == 0) px &= 0xffffff; // key out pure black
            pendingDatabaseLogoBm = bm;
            pendingDatabaseLogoUrl = url;
            // Hand to the render thread (GL upload happens there; see update()).
            pendingDatabaseLogo.store(true, std::memory_order_release);
        } catch (image::image_exception& e) {
            LOGD("Failed to load database artwork %s", url.c_str());
        }
    });
}

void ChipMachine::updatePluginLogo()
{
    // Preview the highlighted plugin's modal-platform logo behind the list.
    if (currentScreen != PLUGIN_SCREEN) {
        pluginLogoIcon.clear();
        return;
    }
    auto const& groups = musicDatabase.pluginGroups();
    // Row 0 is [no filter] (no logo); the narrowed plugin at visible position g
    // is at list index g+1 -- map through pluginVisibleGroups to the real group.
    int i = pluginList.selected() - 1;
    if (i < 0 || i >= (int)pluginVisibleGroups.size()) {
        pluginLogoIcon.clear();
        return;
    }
    auto const& g = groups[pluginVisibleGroups[i]];
    std::string slug = MusicDatabase::platformScreenshotSlug(g.platform);
    const image::bitmap* bm = pickPlatformOrExtLogo("", slug, "");
    if (!bm) {
        pluginLogoIcon.clear();
        return;
    }
    centerLogoIcon(pluginLogoIcon, *bm);
}

// Positions the idle-splash picture centred on the screen, scaled to fill
// splashSizeFraction of the smaller screen dimension while preserving aspect.
void ChipMachine::updateSplashArea()
{
    int bm_w = splashIcon.getTextureWidth();
    int bm_h = splashIcon.getTextureHeight();
    if (bm_w == 0 || bm_h == 0) return;

    // Fit the picture inside a splashSizeFraction x splashSizeFraction box of the
    // screen (as a fraction of each axis), keeping its aspect ratio.
    float w = screen.width() * splashSizeFraction;
    float h = screen.height() * splashSizeFraction;

    float d = h / bm_h;
    float d2 = w / bm_w;
    if (d2 < d) d = d2;

    float final_w = bm_w * d;
    float final_h = bm_h * d;

    float x = (screen.width() - final_w) * 0.5f;
    // Centred both horizontally and vertically.
    float y = (screen.height() - final_h) * 0.5f;

    splashIcon.setArea(grappix::Rectangle(x, y, final_w, final_h));
}

void ChipMachine::updateStartupIconArea()
{
    int bm_w = startupIcon.getTextureWidth();
    int bm_h = startupIcon.getTextureHeight();
    if (bm_w == 0 || bm_h == 0) return;

    // Fit the icon inside a startupIconSizeFraction x startupIconSizeFraction
    // box of the screen (as a fraction of each axis), keeping its aspect ratio.
    float w = screen.width() * startupIconSizeFraction;
    float h = screen.height() * startupIconSizeFraction;

    float d = h / bm_h;
    float d2 = w / bm_w;
    if (d2 < d) d = d2;

    float final_w = bm_w * d;
    float final_h = bm_h * d;

    float x = (screen.width() - final_w) * 0.5f;
    float y = (screen.height() - final_h) * 0.5f;

    startupIcon.setArea(grappix::Rectangle(x, y, final_w, final_h));
}

// Ping-pong-scrolls the welcome banner across the top of the splash. Uses the
// same font/scale/colour as the song title (currentInfoField[0]) and the same
// bounce math as updateTitleMarquee, but spans the full window width. The banner
// text is (re)built when the indexed song total becomes known.
void ChipMachine::updateSplashWelcome(uint32_t delta)
{
    if (totalSongs <= 0) return; // not indexed yet -- nothing to announce

    auto& title = currentInfoField[0];

    // (Re)build the text and adopt the title font when the total changes.
    if (splashWelcomeSongs != totalSongs) {
        splashWelcomeField.setFont(font);
        splashWelcomeField.setText(
            PROGRAM_NAME " " VERSION_STR
            " - EXPLORE & LISTEN TO " +
            withCommas(totalSongs) +
            " RETRO TUNES");
        splashWelcomeSongs = totalSongs;
        splashWelcomePhase = 0.0f;
    }
    // Track the live title styling every frame (lua may set it after startup).
    splashWelcomeField.scale = title.scale;
    splashWelcomeField.color = title.color;
    splashWelcomeField.align = 0.0f; // left-anchored; x is the left edge

    float gscale = screen.height() / 576.0f;
    // Span (almost) the whole window width, with a small margin each side.
    float margin = 10.0f * gscale;
    float baseX = margin;
    float avail = screen.width() - 2.0f * margin;
    // Sit a good way below the top edge: the title font's rounded glyphs
    // overshoot the em-box, so a small margin clips their tops. Anchoring to a
    // fraction of the height keeps this correct as the window (and the
    // proportional font) resizes.
    splashWelcomeField.pos.y = screen.height() * 0.16f;

    int overflow = splashWelcomeField.getWidth() - (int)avail;
    if (overflow <= 0) {
        // Fits -- pin to the left margin, no scroll.
        splashWelcomeField.pos.x = baseX;
        splashWelcomePhase = 0.0f;
        return;
    }

    splashWelcomePhase += delta / 1000.0f;
    // Constant on-screen speed regardless of window size (matches the title
    // marquee): distance and velocity both scale with gscale.
    float speed = 330.0f * gscale;             // px/sec
    float cycle = 2.0f * overflow / speed;     // seconds per out-and-back
    if (cycle < 1.0f) cycle = 1.0f;
    float p = std::fmod(splashWelcomePhase, cycle) / cycle;   // 0..1
    // Ease 0 -> 1 -> 0 over one cycle, dwelling briefly at each end.
    float ease = 0.5f - 0.5f * std::cos(p * 6.28318530718f);
    splashWelcomeField.pos.x = baseX - ease * overflow;
}

// Builds splashShots from every loaded platform + extension picture, collapsing
// duplicates so the splash shows each distinct thing only once. Two kinds of
// duplicate are removed: pixel-identical images (platform aliases / the
// case-insensitive Console sub-logos), and same-SUBJECT images that merely look
// alike -- e.g. the arcade "vgz-arcade" board logo and the "Arcade" platform
// logo are different files but both read as "arcade", so only one is kept.
void ChipMachine::loadSplashScreenshots()
{
    splashShots.clear();
    std::set<uint64_t> seen;
    std::set<std::string> seenSubjects;
    // Normalised subject of a logo, used to drop near-duplicates of the same
    // thing: lowercased, with the arcade "vgz-" board prefix stripped. So
    // "vgz-arcade" and "Arcade" both map to "arcade".
    auto subjectOf = [](const std::string& key) {
        std::string s = utils::toLower(key);
        if (s.rfind("vgz-", 0) == 0) s = s.substr(4);
        return s;
    };
    // Cheap content fingerprint (dimensions + FNV-1a over the pixels). Collisions
    // would only drop a picture, never crash, so a 64-bit hash is plenty.
    auto fingerprint = [](const image::bitmap& bm) -> uint64_t {
        uint64_t h = 1469598103934665603ull;
        auto mix = [&](uint64_t v) { h = (h ^ v) * 1099511628211ull; };
        mix((uint64_t)bm.width());
        mix((uint64_t)bm.height());
        int n = bm.size();
        for (int i = 0; i < n; i++)
            mix((uint64_t)bm[i]);
        return h;
    };
    auto add = [&](const std::string& kind, const std::string& key,
                   const image::bitmap& bm) {
        if (bm.width() <= 0 || bm.height() <= 0) return;
        if (!seenSubjects.insert(subjectOf(key)).second) return; // same subject
        if (seen.insert(fingerprint(bm)).second)
            splashShots.emplace_back(kind + key, bm);
    };
    // Platform logos deliberately kept out of the splash. The splash rotation is
    // a showcase of REAL hardware -- actual computer/console companies and their
    // machines -- so anything that is a software platform, a media/format bucket,
    // a fantasy console, or a generic catch-all is excluded here. The keys are
    // the logo file basenames (see loadPlatformScreenshots' directory scan).
    static const std::set<std::string> splashExcludePlatforms = {
        // Generic / catch-all buckets, not a specific machine.
        "Other", "Wild", "Custom Hardware", "Japanese Computers",
        // Software platforms / language runtimes / players (mirror the code's own
        // isNonHardwareTag classification in MusicDatabase.cpp).
        "Java", "JavaScript", "Flash", "MIRC", "Alambik", "Animation-Video",
        // Operating systems, not hardware (Apple/PC hardware is shown separately
        // via "Original Apple Mac", "Apple IIGS", "PC", "PocketPC", ...).
        "Mac OS", "iOS",
        // Non-machine meta rows (mirror isMetaSubPlatform). The Easter Egg logo
        // has two platformShots keys -- the file basename ("EasterEgg") and the
        // aliased display name ("Easter Egg!", created in loadPlatformScreenshots)
        // -- and the alias sorts first, so BOTH must be excluded.
        "Browser", "Calculator", "EasterEgg", "Easter Egg!",
        // Fantasy / virtual consoles -- emulated abstractions, not real silicon.
        "Pico-8", "Tic-80", "Microw8",
        "Virtual - Fantasy Platforms - Consoles",
        // Company / house logos: the individual machines already appear, so the
        // bare corporate mark adds nothing to a hardware showcase.
        "Microsoft", "Apple", "Sony", "SEGA", "Nintendo", "Atari",
        // Media/category buckets, not hardware (TAB filter house logos --
        // see filterOptions). "MP3-OGG"/"Radio"/"Podcasts" don't overlap
        // splashExcludeExtensions ("mp3"/"ogg" there are ext-shot keys, these
        // are platform-shot keys).
        "MP3-OGG", "Radio", "Podcasts",
        // Generic device-category buckets rather than a named product.
        "Mobile", "Pinball", "PC AdLib", "PC", "PocketPC", "PocketPCBLUE",
        // Obscure / niche machines the user chose to drop from the showcase.
        "KC-85", "BK-0010-11M", "Vector-06c", "Thomson", "Pokemon Mini",
        "Arcade", "Raspberry Pi", "TI-8x Calculator",
    };
    // Extension logos excluded from the splash: streamable/plain-audio formats
    // that aren't a "venerable retro platform" worth showcasing here.
    static const std::set<std::string> splashExcludeExtensions = {
        "aac", "aif", "aiff", "m4a", "opus", "mp2", "ac3", "mpeg", "mp3", "ogg",
        // Generic arcade board mark (the named makers below stay); the "Arcade"
        // platform logo is likewise excluded above.
        "vgz-arcade",
    };
    // Extensions first (more specific artwork), then platforms.
    for (auto& [key, bm] : extensionShots) {
        if (splashExcludeExtensions.count(key)) continue;
        add("ext:", key, bm);
    }
    for (auto& [key, bm] : platformShots) {
        if (splashExcludePlatforms.count(key)) continue;
        add("platform:", key, bm);
    }
    // Randomise the display order so each launch shows a different sequence
    // (the maps above iterate in a fixed, sorted order otherwise).
    std::shuffle(splashShots.begin(), splashShots.end(),
                 std::mt19937{ std::random_device{}() });
    LOGD("Splash: %d unique pictures from %d platform + %d extension logos",
         (int)splashShots.size(), (int)platformShots.size(),
         (int)extensionShots.size());
}

bool ChipMachine::noImages = false;
bool ChipMachine::debugMode = false;

// Base thumbnail URL ("https://img.youtube.com/vi/<id>/") for a YouTube
// watch/short URL, or "" if it isn't a YouTube link. Used as the screenshot
// fallback for YouTube songs: if the "better" external screenshot we matched
// (pouet, etc.) is dead (403/404) we drop to the video's own thumbnail rather
// than the platform logo -- which is often wrong now that YouTube items classify
// to a platform. Guarantees every YouTube item shows a real, on-topic picture.
static std::string youtubeThumbBase(const std::string& url)
{
    if (url.find("youtube.com/") == std::string::npos &&
        url.find("youtu.be/") == std::string::npos)
        return "";
    std::string id;
    auto v = url.find("v=");
    if (v != std::string::npos)
        id = url.substr(v + 2, 11);
    else {
        auto sl = url.rfind('/');
        if (sl != std::string::npos && sl + 1 < url.size())
            id = url.substr(sl + 1, 11);
    }
    if (id.size() != 11) return "";
    return "https://img.youtube.com/vi/" + id + "/";
}

void ChipMachine::loadScreenshot(const std::string& shot)
{
    // --donotloadimages: never attempt any screenshot download.
    if (noImages) {
        screenShotIcon.clear();
        screenshots.clear();
        currentScreenshot = "";
        return;
    }

    // Platform of the playing song (classified from its raw format when
    // currentInfo was set), used to pick the per-platform logo.
    std::string slug = currentSongPlatform;

    // Called from Playstarted (immediate) and from the Playing poll (late arrival).
    if (shot == "") {
        // Song has no screenshot/cover art — rotate the extension/platform logo.
        // Keep currentScreenshot empty so the Playing poll can still upgrade to a
        // real screenshot URL that arrives late. Avoid rebuilding (and re-fading)
        // only when we are already showing the SAME fallback set: same extension
        // AND same platform (two formats can share a platform, e.g. vgz and
        // miniqsf both -> Console, yet need different extension images).
        std::string logoKey =
            currentSongExt + "|" + slug + "|" + currentSongFormat;
        if (currentScreenshot == "" && logoKey == shownLogoKey &&
            !screenshots.empty())
            return;
        currentScreenshot = "";
        currentPlatformSlug = slug;
        shownLogoKey = logoKey;
        screenShotIcon.clear();
        screenshots.clear();
        appendLogoScreenshots();
        transitions.restart();
        return;
    }

    // Guards against re-loading the same URL.
    if (shot == currentScreenshot) {
        // Already loaded or loading — just advance to next frame
        transitions.next();
        return;
    }

    screenShotIcon.clear();
    screenshots.clear();
    currentScreenshot = shot;
    currentPlatformSlug = slug;

    auto parts = utils::split(shot, ";");
    // The callback fires once per requested part and MAY run on the web worker
    // thread (async download) or synchronously on the render thread (cache hit).
    // It must NOT touch any shared ChipMachine state -- in particular the
    // `screenshots` vector, which the render thread reads every frame (transition
    // source callbacks) and clears (loadScreenshot). Doing so from the worker
    // thread is a data race that corrupts the heap. So the callback only DECODES
    // the downloaded file(s) into a captured, self-locked accumulator; when the
    // last part settles it publishes the decoded bitmaps to pendingShotBms and
    // raises pendingShotReady. update() on the render thread then installs them
    // into `screenshots`, appends the logo fallback and restarts the transitions
    // -- so all shared state and all GL calls stay on the render thread.
    struct ShotAccum
    {
        std::mutex m;
        std::vector<NamedBitmap> bms;
        int remaining;
    };
    auto acc = std::make_shared<ShotAccum>();
    acc->remaining = (int)parts.size();
    auto cb = [this, acc, shot](utils::File f) {
        std::vector<NamedBitmap> decoded;
        if (f) {
            try {
                if (utils::toLower(utils::path_extension(
                        f.getName())) == "gif") {
                    for (auto& bm : image::load_gifs(f.getName())) {
                        for (auto& px : bm) {
                            if ((px & 0xffffff) == 0)
                                px &= 0xffffff;
                        }
                        decoded.emplace_back(f.getFileName(), bm);
                    }
                } else {
                    auto bm = image::load_image(f.getName());
                    for (auto& px : bm) {
                        if ((px & 0xffffff) == 0) px &= 0xffffff;
                    }
                    decoded.emplace_back(f.getFileName(), bm);
                }
            } catch (image::image_exception& e) {
                LOGD("Failed to load image");
            }
        }

        bool done;
        {
            std::lock_guard<std::mutex> lg(acc->m);
            for (auto& nb : decoded)
                acc->bms.push_back(std::move(nb));
            done = (--acc->remaining <= 0);
        }
        if (!done)
            return;

        // All parts settled. Hand the decoded screenshots to the render thread;
        // it validates the request is still current before installing (see the
        // pendingShotReady handler in update()).
        {
            std::lock_guard<std::mutex> lg(pendingShotLock);
            pendingShotBms = std::move(acc->bms);
            pendingShotKey = shot;
        }
        pendingShotReady.store(true, std::memory_order_release);
    };
    for (auto& p : parts)
        webutils::Web::getInstance().getFile(p, cb);
}

// Lowercased file extension of a song: prefer the DB ext, fall back to the path.
static std::string songExtension(const SongInfo& s)
{
    // Use the REAL inner format, never a container wrapper (.zip/.gz/.lha): a
    // compressed song's screenshot logo / ext key must match the wrapped module
    // (e.g. "mod"), not the archive.
    return MusicDatabase::resolveExtension(s);
}

// Appends the per-extension screenshot, or failing that the per-platform logo,
// for the current song. Returns true if one was appended. Does NOT append the
// generic ChipMachine icon -- that is a last resort reserved for the logo-only
// path (see appendLogoScreenshots).
// Arcade VGM rips all share the "vgz" extension, so the per-extension logo can't
// tell the boards apart. Map the (lowercased) format string to a per-board image
// in data/misc/extensionscreenshots/ (loaded into extensionShots by basename).
static const std::map<std::string, std::string>& arcadeSubLogos()
{
    static const std::map<std::string, std::string> m = {
        { "arcade", "vgz-arcade" },
        { "arcade (capcom)", "vgz-capcom" },
        { "arcade (konami)", "vgz-konami" },
        { "arcade (namco)", "vgz-namco" },
        { "arcade (sega)", "vgz-sega" },
        { "arcade (taito)", "vgz-taito" },
        { "neo geo", "vgz-neogeo" },
        // modland's .miniqsf CPS rips share the "Arcade (Capcom)" filter group
        // with the VGM Capcom rips above, so they take the same board logo.
        // (They used to map to the separate Capcom.png via consoleSubLogos,
        // which put two different Capcom logos inside one group.)
        { "capcom q-sound format", "vgz-capcom" },
    };
    return m;
}

// Extensions that name a codec or container rather than a format with a hardware
// identity. A tune is never "an OGG" the way it is "a SID" -- the Fujiology Atari
// Lynx tunes are .ZIPs of OGG, so ogg.png used to win over the Lynx logo and the
// platform read as its storage format. These lose to a *specific* platform logo
// in pickPlatformOrExtLogo; every other ext shot (mod/dtm/mdx/gtk/...) names a
// real format and still wins. Kept wider than the shots currently on disk so a
// later wav.png/flac.png can't silently reintroduce the bug.
static bool isGenericCodecExt(const std::string& ext)
{
    static const std::set<std::string> m = {
        "mp3", "mp2",  "ogg", "opus", "wav",  "aac", "ac3",
        "m4a", "mp4",  "aif", "aiff", "flac", "mpeg", "mpg",
    };
    return m.count(ext) > 0;
}

const image::bitmap* ChipMachine::pickPlatformOrExtLogo(
    const std::string& ext, const std::string& platformSlug,
    const std::string& format, std::string* label)
{
    // 0) Arcade boards: pick the per-board logo by format string (Capcom/Konami/
    //    Namco/Sega/Taito/Neo Geo/other), overriding the shared "vgz" ext logo.
    {
        auto a = arcadeSubLogos().find(format);
        if (a != arcadeSubLogos().end()) {
            auto it = extensionShots.find(a->second);
            if (it != extensionShots.end() && it->second.width() > 0) {
                if (label) *label = "ext:" + a->second;
                return &it->second;
            }
        }
    }
    // 1) Resolve the per-platform logo up front (it used to be step 2) so the
    //    extension step below can tell whether there is a specific platform logo
    //    for a generic codec ext to lose to.
    const image::bitmap* platformBm = nullptr;
    std::string logoSlug = platformSlug;
    {
        // An "Other Platforms" drill row is its own logo name, so "Oric.png"
        // gives the Oric row its logo with no table to edit. Wins over the byte
        // slug, which for these songs is only the generic "Other".
        auto sub = MusicDatabase::subPlatformName(format);
        if (findPlatformShot(sub) != nullptr) logoSlug = sub;
    }
    platformBm = findPlatformShot(logoSlug);
    // "Other" is a real logo but names no hardware, so it is not specific enough
    // to outrank the ext shot: an OTHER-byte .mp3 keeps mp3.png rather than
    // trading it for the generic Other.png.
    bool specificPlatform = platformBm != nullptr && logoSlug != "Other";

    // 2) Per-extension screenshot (e.g. mod.png, sid.png) otherwise still takes
    //    priority over the platform logo -- it is the more specific of the two.
    if (!ext.empty() && !(specificPlatform && isGenericCodecExt(ext))) {
        auto it = extensionShots.find(ext);
        if (it != extensionShots.end() && it->second.width() > 0) {
            if (label) *label = "ext:" + ext;
            return &it->second;
        }
    }
    // 3) Fall back to the platform logo resolved in step 1.
    if (platformBm != nullptr) {
        if (label) *label = "platform:" + logoSlug;
        return platformBm;
    }
    return nullptr;
}

bool ChipMachine::appendPlatformOrExtLogo()
{
    std::string label;
    const image::bitmap* bm = pickPlatformOrExtLogo(
        currentSongExt, currentPlatformSlug, currentSongFormat, &label);
    if (bm) {
        screenshots.emplace_back(label, *bm);
        LOGD("Screenshot logos: platform='%s' logo='%s' have=yes",
             currentPlatformSlug, label.c_str());
        return true;
    }
    LOGD("Screenshot logos: platform='%s' have=none", currentPlatformSlug);
    return false;
}

void ChipMachine::appendLogoScreenshots()
{
    // Prefer the ext/platform logo when the song has no real screenshot.
    if (appendPlatformOrExtLogo())
        return;
    // Only when there is no ext/platform logo, fall back to the ChipMachine icon
    // so the area isn't blank.
    if (defaultShot.width() == 0 || defaultShot.height() == 0) {
        try {
            auto ic = workDir / "data" / "misc" / "icon.png";
            defaultShot = image::load_image(ic.string());
        } catch (image::image_exception& e) {
            LOGD("Failed to load ChipMachine logo (icon.png)");
        }
    }
    if (defaultShot.width() > 0 && defaultShot.height() > 0)
        screenshots.emplace_back("chipmachine", defaultShot);
}

void ChipMachine::loadPlatformScreenshots()
{
    // Load every per-platform logo once at startup from
    // data/misc/platformscreenshots/<platform>.png (or .jpg). Missing files are
    // not fatal: collect them and emit a single warning so they can be added.
    auto dir = workDir / "data" / "misc" / "platformscreenshots";

    // Build the full decode work-list first (cheap filesystem probes only), then
    // decode every image in parallel (see decodeLogosParallel) and insert the
    // results. This keeps the exact keying and precedence of the old serial code
    // while moving the ~1.6s of PNG decoding off the critical path onto all cores.
    //
    // Precedence must match the original: a slug ("ZX Spectrum 128") wins over a
    // same-named directory file, so slugs are gathered first and their keys are
    // reserved before the directory scan fills in the rest.
    std::vector<std::string> keys;  // map key for result i
    std::vector<std::string> paths; // file to decode for result i
    std::set<std::string> reserved; // keys already claimed

    // Resolve a slug to an existing file, honouring the '/'->'-' safe variant
    // (a slug may contain '/', e.g. "ZX Spectrum 16/48", which no filename can).
    auto resolveSlug = [&](const std::string& name) -> std::string {
        std::vector<std::string> bases{ name };
        std::string safe = name;
        std::replace(safe.begin(), safe.end(), '/', '-');
        if (safe != name)
            bases.push_back(safe);
        for (auto& base : bases)
            for (auto ext : { ".png", ".jpg", ".jpeg" }) {
                auto p = dir / (base + ext);
                if (utils::File::exists(p.string()))
                    return p.string();
            }
        return "";
    };

    std::vector<std::string> missing;
    for (auto& name : MusicDatabase::platformScreenshotNames()) {
        auto p = resolveSlug(name);
        if (p.empty()) {
            missing.push_back(name);
        } else {
            keys.push_back(name);
            paths.push_back(p);
            reserved.insert(name);
        }
    }

    // Then every other image in the directory under its own basename. The
    // "Other Platforms" drill rows (Oric, Vectrex, VIC 20, ...) share the single
    // OTHER byte, so they have no slug of their own -- their logo is keyed by
    // the drill's group name instead (see pickPlatformOrExtLogo). Scanning means
    // dropping in "<group>.png" is all it takes to give one a logo: no table to
    // edit, and no dependency on the search index being built yet.
    if (utils::File::exists(dir.string())) {
        for (auto& f : utils::File(dir.string()).listFiles()) {
            auto fn = f.getFileName();
            auto e = utils::toLower(utils::path_extension(fn));
            if (e != "png" && e != "jpg" && e != "jpeg") continue;
            auto key = fn.substr(0, fn.size() - e.size() - 1);
            if (!reserved.insert(key).second) continue; // already claimed by slug
            keys.push_back(key);
            paths.push_back(f.getName());
        }
    }

    auto bitmaps = decodeLogosParallel(paths);
    for (size_t i = 0; i < keys.size(); i++)
        if (bitmaps[i].width() > 0)
            platformShots[keys[i]] = std::move(bitmaps[i]);

    // Generic platform slugs that have no dedicated artwork reuse a specific
    // variant's logo. E.g. VGM/VGZ rips of Spectrum games carry the bare
    // "ZX Spectrum" platform, which has no logo of its own -> use the 128K one.
    static const std::pair<std::string, std::string> aliases[] = {
        { "ZX Spectrum", "ZX Spectrum 128" },
        // The "Easter Egg!" Other-drill row (see subPlatformName) can't name its
        // logo file "Easter Egg!.png"; the user ships it as EasterEgg.png, so
        // point the row's display name at that basename.
        { "Easter Egg!", "EasterEgg" },
    };
    for (auto& [alias, src] : aliases)
        if (!platformShots.count(alias) && platformShots.count(src))
            platformShots[alias] = platformShots[src];
    // Any slug now satisfied via alias is no longer "missing".
    missing.erase(std::remove_if(missing.begin(), missing.end(),
                                 [&](const std::string& m) {
                                     return platformShots.count(m) > 0;
                                 }),
                  missing.end());

    LOGD("Loaded %d platform logos from %s", (int)platformShots.size(),
         dir.string());
    if (!missing.empty()) {
        std::string list;
        for (auto& m : missing)
            list += (list.empty() ? "" : ", ") + m;
        LOGW("Missing %d platform logo(s) in %s (add <name>.png or .jpg): %s",
             (int)missing.size(), dir.string(), list);
    }

    // Explicit house logos on top-level filterOptions rows (Atari.png,
    // Nintendo.png, ..., plus the platformless MP3-OGG/Radio/Podcasts/Other
    // rows -- see FilterOption::logo). Groups fall back to borrowing their
    // first child's logo when missing; the platformless leaf rows have no
    // such fallback and simply show nothing on the TAB filter screen. Both
    // are still reported, or a row nobody drew art for would silently stay
    // blank (or keep wearing one machine's face).
    std::vector<std::string> missingGroup;
    for (auto const& opt : filterOptions)
        if (!opt.logo.empty() && !findPlatformShot(opt.logo))
            missingGroup.push_back(opt.logo);
    if (!missingGroup.empty()) {
        std::string list;
        for (auto& m : missingGroup)
            list += (list.empty() ? "" : ", ") + m;
        LOGW("Missing %d filter-row logo(s) in %s (group rows fall back to "
             "their first sub-platform's logo; the platformless rows do not; "
             "add <name>.png or .jpg): %s",
             (int)missingGroup.size(), dir.string(), list);
    }

    // The remaining "Other Platforms" drill reports each drive a full-table SQL
    // GROUP BY over the ~380k-row song table (subPlatformNames /
    // subPlatformNamesNonHardware -> otherDrillNames), which cost ~1.2s at every
    // launch purely to warn a developer about missing art. They are pure
    // diagnostics, so run them only under -d (debugMode). Normal launches skip
    // the queries entirely and reach the splash ~1.2s sooner.
    if (debugMode) {
        // "Other Platforms" drill rows that name real hardware but have no logo
        // yet. Separate because they have no format byte (and so no slug) -- the
        // file is named after the drill row itself.
        std::vector<std::string> missingSub;
        auto reportRows = MusicDatabase::subPlatformNames();
        // Family parent rows (Virtual Platforms, ...) want a logo too; report
        // them alongside the real hardware rows.
        for (auto& fam : MusicDatabase::subPlatformFamilyNames())
            reportRows.push_back(fam);
        for (auto& name : reportRows)
            if (!findPlatformShot(name)) {
                // Report the FILENAME to create, not the drill name: a '/' in
                // the row ("TRS-80/CoCo/Dragon") can't be a filename, so the
                // logo is named with '-' ("TRS-80-CoCo-Dragon.png"), which
                // findPlatformShot resolves back. Show that safe form so the
                // name is copy-pasteable.
                std::string safe = name;
                std::replace(safe.begin(), safe.end(), '/', '-');
                missingSub.push_back(safe);
            }
        if (!missingSub.empty()) {
            std::string list;
            for (auto& m : missingSub)
                list += (list.empty() ? "" : ", ") + m;
            LOGW("Missing %d Other-platform logo(s) in %s (add <name>.png or "
                 ".jpg): %s",
                 (int)missingSub.size(), dir.string(), list);
        }

        // Non-hardware / meta Other rows (Java, JavaScript, Flash, VGM, ...) name
        // no machine, so a logo could never be "correct" and they are not gaps.
        // But they still render logo-less, easy to mistake for a missing file --
        // list the art-less ones as info, distinct from the warning above.
        std::vector<std::string> artlessMeta;
        for (auto& name : MusicDatabase::subPlatformNamesNonHardware())
            if (!findPlatformShot(name))
                artlessMeta.push_back(name);
        if (!artlessMeta.empty()) {
            std::string list;
            for (auto& m : artlessMeta)
                list += (list.empty() ? "" : ", ") + m;
            LOGD("%d Other row(s) render without a logo by design "
                 "(non-hardware/meta tags -- drop a <name>.png in %s only if you "
                 "want art): %s",
                 (int)artlessMeta.size(), dir.string(), list);
        }
    }
}

// Platform logo by name, case- and slash-insensitively. Two spellings must
// resolve to the same file: collections disagree on capitalisation ("ColecoVision"
// vs "Colecovision"), and a drill row can carry a '/' that no filename can
// ("TRS-80/CoCo/Dragon"), so its logo is named with '-' instead -- the same
// '/'->'-' rule platformSlugForByte uses for byte slugs ("Atari ST/STE" ->
// "Atari ST-STE.png"). Normalise both sides to compare. The exact match is the
// fast path; the scan is over ~50 entries and only runs on a song/selection change.
const image::bitmap* ChipMachine::findPlatformShot(const std::string& name)
{
    if (name.empty()) return nullptr;
    auto it = platformShots.find(name);
    if (it != platformShots.end())
        return it->second.width() > 0 ? &it->second : nullptr;
    auto norm = [](std::string s) {
        for (auto& c : s) {
            c = (char)std::tolower((unsigned char)c);
            if (c == '/') c = '-';
        }
        return s;
    };
    auto want = norm(name);
    for (auto& [k, bm] : platformShots)
        if (norm(k) == want)
            return bm.width() > 0 ? &bm : nullptr;
    return nullptr;
}

void ChipMachine::loadExtensionScreenshots()
{
    // Load whatever per-extension images are present (keyed by lowercased
    // basename, e.g. "mod.png" -> "mod"). The black-key matches the platform
    // logos so black-background images go transparent too.
    auto dir = workDir / "data" / "misc" / "extensionscreenshots";
    // Gather the work-list, then decode in parallel (see decodeLogosParallel) --
    // same black-keying as before, just off the critical path onto all cores.
    std::vector<std::string> keys;
    std::vector<std::string> paths;
    if (utils::File::exists(dir.string())) {
        for (auto& f : utils::File(dir.string()).listFiles()) {
            auto fn = f.getFileName();
            // path_extension() returns the extension WITHOUT the dot.
            auto e = utils::toLower(utils::path_extension(fn));
            if (e != "png" && e != "jpg" && e != "jpeg")
                continue;
            // Strip ".<ext>" (extension length + the dot) to get the key.
            keys.push_back(utils::toLower(fn.substr(0, fn.size() - e.size() - 1)));
            paths.push_back(f.getName());
        }
    }
    auto bitmaps = decodeLogosParallel(paths);
    for (size_t i = 0; i < keys.size(); i++)
        if (bitmaps[i].width() > 0)
            extensionShots[keys[i]] = std::move(bitmaps[i]);
    LOGD("Loaded %d extension screenshots from %s",
         (int)extensionShots.size(), dir.string());

    // Report extensions that have NO extension screenshot and whose platform
    // also has NO platform logo -- i.e. the ones the platform fallback can't
    // cover, so they would land on the app icon. Extensions covered by a
    // platform logo are intentionally omitted.
    //
    // Only extensions we can actually DECODE are worth reporting. A handful of
    // collections store a junk ext: scene.org rows are keyed on an archive
    // member that is a README/cover image, or on a bare-named module whose
    // "extension" is really the song title ("olut", "braxen", "1992"). Those are
    // an indexer bug, not a missing logo -- no <title>.png could ever cover them
    // -- and they buried the real misses under ~76 lines of noise. Judge the ext
    // with the plugin-derived set the archive picker uses (song + audio), i.e.
    // exactly what the app plays as a loose file, so a new plugin's format shows
    // up here automatically instead of needing a second hand-kept list.
    // This uncovered-extension report drives a full-table SQL query
    // (extensionPlatforms(), a GROUP BY over the ~380k-row song table), ~0.9s at
    // every launch purely to warn a developer about a missing <ext>.png. It is a
    // pure diagnostic, so run it only under -d (debugMode); normal launches skip
    // the query and reach the splash sooner.
    if (debugMode) {
        auto const& [songExt, audioExt] = MusicPlayerList::archiveExtensions();
        std::vector<std::string> uncovered;
        size_t undecodable = 0;
        for (auto& [ext, plats] : musicDatabase.extensionPlatforms()) {
            if (extensionShots.count(ext))
                continue; // already has its own screenshot
            if (!songExt.count(ext) && !audioExt.count(ext)) {
                undecodable++;
                continue; // no decoder -> a bogus/undecodable ext, not a logo gap
            }
            bool coveredByPlatform = false;
            for (auto& p : plats)
                if (!p.empty() && platformShots.count(p)) {
                    coveredByPlatform = true;
                    break;
                }
            if (!coveredByPlatform)
                uncovered.push_back(ext);
        }
        if (undecodable > 0)
            LOGD("%d uncovered extension(s) have no decoder; not reported as logo "
                 "gaps (bogus ext in the index)", (int)undecodable);
        if (!uncovered.empty()) {
            std::string list;
            for (auto& e : uncovered)
                list += (list.empty() ? "" : ", ") + e;
            LOGW("%d extension(s) have no screenshot and no platform logo "
                 "(add data/misc/extensionscreenshots/<ext>.png): %s",
                 (int)uncovered.size(), list);
        }
    }
}

void ChipMachine::updateNextField()
{
    auto psz = player.listSize();
    if (psz > 0) {
        auto info = player.getInfo(1);
        if (info.path != currentNextPath) {
            if (psz == 1)
                nextField.setText("Next");
            else
                nextField.setText(utils::format("Next (%d)", psz));
            info.format = MusicDatabase::describeFormat(info);
            nextInfoField.setInfo(info);
            currentNextPath = info.path;
        }
    } else if (nextField.getText() != "") {
        nextInfoField.setInfo(SongInfo());
        nextField.setText("");
    }
}

// Format an integer with thousands separators, e.g. 345000 -> "345,000".
std::string ChipMachine::withCommas(int n)
{
    std::string s = std::to_string(n);
    for (int pos = (int)s.size() - 3; pos > 0; pos -= 3)
        s.insert((size_t)pos, ",");
    return s;
}

void ChipMachine::setFilterLevel(const std::vector<FilterOption>* opts, int sel)
{
    activeFilterOptions = opts;
    int n = (int)currentFilterOptions().size();
    advancedList.setTotal(n);
    advancedList.setVisible(n);
    if (sel < 0) sel = 0;
    if (sel >= n) sel = n - 1;
    advancedList.select(sel);
    updateFilterLogo();
}

void ChipMachine::computeFilterCounts()
{
    if (musicDatabase.busy()) return; // not indexed yet -- retry later
    auto counts = musicDatabase.getFormatByteCounts();
    int total = 0;
    for (int c : counts)
        total += c;
    if (total == 0) return;
    totalSongs = total; // drives the splash welcome banner
    podcastShowCount = musicDatabase.getPodcastShowCount();
    otherPlatformCount = musicDatabase.getOtherPlatformCount();
    arcadePlatformCount = musicDatabase.getArcadePlatformCount();
    filterByteCounts = counts;
    filterTotalCount = total;
    filterCounts.assign(filterOptions.size(), 0);
    for (size_t i = 0; i < filterOptions.size(); i++)
        filterCounts[i] = filterOptionCount(filterOptions[i]);
}

// Tune count for a filter option: the grand total for the "[Show All]" entry
// (no formats, no children), the sum of its children's counts for a group, or
// the sum of its matched format bytes otherwise. Requires filterByteCounts.
int ChipMachine::filterOptionCount(FilterOption const& opt) const
{
    if (opt.matchedFormats.empty() && opt.children.empty())
        return filterTotalCount; // "[Show All]"
    int s = 0;
    for (auto const& ch : opt.children)
        s += filterOptionCount(ch);
    for (uint8_t b : opt.matchedFormats)
        if (b < filterByteCounts.size()) s += filterByteCounts[b];
    return s;
}

void ChipMachine::update()
{
#ifdef __APPLE__
    // Collect any Finder "Open With" / double-click paths delivered since the
    // last frame. Done before the indexing gate below so live events are never
    // missed; they accumulate in filesToOpen and are played once the DB is
    // ready (see after the gate).
    for (auto& p : drainPendingOpenFiles())
        filesToOpen.push_back(p);
#endif

    // Files dragged and dropped onto the window (native OS drag & drop, all
    // platforms via GLFW). Same downstream path as "Open With" above.
    for (auto& p : grappix::screen.get_dropped_files())
        filesToOpen.push_back(p);

    if (indexingDatabase) {
        if (!musicDatabase.busy()) {
            indexingDatabase = false;
            removeToast();
            computeFilterCounts();
            // Precompute the Format- and Database-filter browse lists now that
            // the index is ready, so the first TAB to those screens is instant
            // like the Platform screen. The Extension list needs a full song-table
            // scan (~360ms); precomputeBrowseListsAsync() runs it on a worker
            // thread (its own DB connection) so the render loop -- the scrolling
            // starfield on the splash -- never stutters.
            musicDatabase.precomputeBrowseListsAsync();
        } else {
            // Only show the "Indexing database" toast/bar for a real reindex.
            // A cached load is also briefly busy(); on slower machines that
            // window outlasts the delay and used to flash an empty progress bar
            // even though nothing is being indexed. isReindexing() stays false
            // for cached loads, so we wait silently instead.
            if (musicDatabase.isReindexing()) {
                static int delay = 30;
                if (delay-- == 0) toast("Indexing database", STICKY);
            }
            return;
        }
    }

    // Play files handed to us by Finder ("Open With" / double-click). The DB is
    // ready by here (the indexing gate above has passed). playSongs() clears the
    // queue, plays the first file, queues the rest, and shows the main screen.
    if (!filesToOpen.empty()) {
        std::vector<SongInfo> songs;
        songs.reserve(filesToOpen.size());
        for (auto const& p : filesToOpen)
            songs.emplace_back(p);
        filesToOpen.clear();
        playSongs(songs);
    }

    if (namedToPlay != "") {
        std::vector<SongInfo> target;
        SongInfo info;
        bool random = true;
        if (namedToPlay == "favorites") {
            target = musicDatabase.getPlaylist("Favorites");
        } else if (namedToPlay == "all") {
            musicDatabase.getSongs(target, info, 500, random);
        } else {
            info.path = namedToPlay + "::x";
            musicDatabase.getSongs(target, info, 500, random);
        }
        namedToPlay = "";
        for (const auto& s : target) {
            if (!utils::endsWith(s.path, ".plist")) player.addSong(s);
        }
        player.nextSong();
    }

    auto click = screen.get_click();

    if (currentDialog && currentDialog->getParent() == nullptr)
        currentDialog = nullptr;

    updateKeys();

    playerState = player.getState();

    // Fetch/buffer toast. A song that is not already on local disk needs one of
    // two waits, shown as one sticky toast:
    //   * whole-file download (a remote native module): the player sits in
    //     Loading while remoteLoader fetches the file -> "LOADING...".
    //   * progressive stream (ffmpeg/YouTube/radio): playCurrent() flips to
    //     Playstarted almost immediately and ffmpeg *then* prebuffers, so the
    //     track is nominally "playing" while no PCM has reached the DAC yet ->
    //     "BUFFERING...". (This is why keying the toast on the Loading state
    //     alone missed streams entirely: their Loading window is ~0 frames.)
    // willStream() is the same predicate playCurrent() routes on, so the message
    // always matches the path taken. Resolve once per song (recompute only when
    // the playing path changes) to avoid a stat()/lookup every frame.
    {
        auto info = player.getInfo();
        if (info.path != loadingToastPath) {
            loadingToastPath = info.path;
            loadingToastResolved = false;
            if (loadingToastShown) {
                removeToast();
                loadingToastShown = false;
            }
            loadingToastIsLocal = utils::exists(info.path);
            loadingToastStreamed =
                !loadingToastIsLocal && MusicPlayerList::willStream(info);
        }

        if (!loadingToastResolved) {
            if (loadingToastIsLocal) {
                loadingToastResolved = true; // on disk -> instant, no toast
            } else if (loadingToastStreamed) {
                // Prebuffering: hold "BUFFERING..." until the first samples reach
                // the audio callback, then clear it.
                if (player.hasAudioStarted()) {
                    if (loadingToastShown) {
                        removeToast();
                        loadingToastShown = false;
                    }
                    loadingToastResolved = true;
                } else if (!loadingToastShown) {
                    toast("BUFFERING...", STICKY);
                    loadingToastShown = true;
                    loadingToastStartMs = utils::getms();
                }
            } else if (playerState == MusicPlayerList::Loading) {
                // Whole-file download in flight -> "LOADING..." (skip if it is
                // already sitting in the local cache from a prior fetch).
                if (!loadingToastShown) {
                    bool cached = false;
                    try {
                        cached = remoteLoader.inCache(info.path);
                    } catch (...) {
                        cached = false; // unknown source -> assume a fetch
                    }
                    if (!cached) {
                        toast("LOADING...", STICKY);
                        loadingToastShown = true;
                        loadingToastStartMs = utils::getms();
                    } else {
                        loadingToastResolved = true;
                    }
                }
            } else {
                // Left Loading -> the download finished and playback began.
                if (loadingToastShown) {
                    removeToast();
                    loadingToastShown = false;
                }
                loadingToastResolved = true;
            }
        }
    }

    if (playerState == MusicPlayerList::Playstarted) {
        timeField.add = 0;
        // Restart stereo content detection for the new tune.
        stereoDiffAccum = 0;
        stereoSumAccum = 0;
        stereoDetectFrames = 0;
        currentInfo = player.getInfo();
        // Classify the platform from the raw format before describeFormat()
        // rewrites it into a display string ("Amiga - Soundtracker (MOD)"),
        // which would no longer classify.
        currentSongPlatform = MusicDatabase::platformScreenshotName(currentInfo);
        currentSongExt = songExtension(currentInfo);
        currentSongFormat = utils::toLower(currentInfo.format);
        currentInfo.format = MusicDatabase::describeFormat(currentInfo);
        dbInfo = player.getDBInfo();
        screen.setTitle(utils::format("%s / %s (" PROGRAM_NAME " " VERSION_STR ")",
                                      currentInfo.title, currentInfo.composer));
        bool isRadio = utils::startsWith(dbInfo.path, "radio::");
        // Detect podcasts from dbInfo: it carries the DB-sourced format
        // ("Podcast") and is never overwritten, whereas currentInfo.format is
        // replaced by the player's codec tag ("MP3") in updateInfo() before
        // Playstarted -- so classifying currentInfo missed the episode. Also
        // accept the already-described "Podcast (...)" string as a fallback.
        bool isPodcast =
            MusicDatabase::classifyFormat(dbInfo.format, dbInfo.path) ==
                PODCAST ||
            utils::startsWith(currentInfo.format, "Podcast");
        std::string m;
        if (isPodcast) {
            // Podcasts: scroll the episode title plus its description (the INFO
            // metadata, which parseRss falls back to the show description for
            // when an episode has none). Never append the module-format line --
            // "Podcast (MP3)" is meaningless for a talk/music show.
            m = currentInfo.title;
            auto desc = compressWhitespace(currentInfo.metadata[SongInfo::INFO]);
            // Some "standard" podcast collections (e.g. Demovibes) store a
            // screenshot URL in INFO rather than a text description -- don't
            // scroll a raw URL; the title alone is descriptive enough there.
            if (!desc.empty() && !utils::startsWith(desc, "http"))
                m += " ... " + desc;
        } else {
            if (currentInfo.metadata[SongInfo::INFO] != "") {
                m = compressWhitespace(currentInfo.metadata[SongInfo::INFO]);
            } else {
                m = compressWhitespace(player.getMeta("message"));
            }
            if (m == "" && isRadio) {
                m = currentInfo.title;
            }
            // Append the format info ("Platform - Name (EXT) ... <trackers> -
            // <description>") so the scroller cycles metadata -> format ->
            // back. When there is no embedded message/info the format line is
            // all there is to show. Leading/trailing dots give clean gaps
            // between sections. Radio streams have no meaningful module format,
            // so skip it there.
            if (!isRadio)
                m = appendFormatInfo(m, currentInfo);
        }
        if (scrollText != m) {
            scrollEffect.set("scrolltext", m);
            scrollText = m;
        }

        auto shot = currentInfo.metadata[SongInfo::SCREENSHOT];
        loadScreenshot(shot);

        currentTween.finish();
        // Reset title AND composer to the shared base x (the format field is
        // never scrolled, so its x is the base) -- either may have been mid-scroll
        // for the previous song.
        currentInfoField[0].pos.x = currentInfoField[2].pos.x;
        currentInfoField[1].pos.x = currentInfoField[2].pos.x;
        // Suspend the marquee during the intro slide-in (the tween below animates
        // the field positions); re-enabled in onComplete once they've settled.
        titleMarqueeActive = false;
        titleMarqueePhase[0] = titleMarqueePhase[1] = 0.0f;
        prevInfoField = currentInfoField;

        currentInfoField.setInfo(currentInfo);
        currentTune = player.getTune();

        if (currentInfo.numtunes > 0)
            songField.setText(utils::format("[%02d/%02d]", currentTune + 1,
                                            currentInfo.numtunes));
        else
            songField.setText("[01/01]");

        auto sub_title = player.getMeta("sub_title");

        auto f = [=]() {
            xinfoField.setText(sub_title);
            // Fields have settled at their base -- hand control to the per-frame
            // marquee, which scrolls the title/composer only if they don't fit.
            titleMarqueeActive = true;
            titleMarqueePhase[0] = titleMarqueePhase[1] = 0.0f;
        };

        updateFavorite();
        updateNextField();
        player.playlistUpdated();

        if (player.wasFromQueue()) {
            currentTween = Tween::make()
                               .from(prevInfoField, currentInfoField)
                               .from(currentInfoField, nextInfoField)
                               .from(nextInfoField, outsideInfoField)
                               .seconds(1.5)
                               .onComplete(f);
        } else {
            currentTween = Tween::make()
                               .from(prevInfoField, currentInfoField)
                               .from(currentInfoField, outsideInfoField)
                               .seconds(1.5)
                               .onComplete(f);
        }
        currentTween.start();
    }

    // Late-arrival screenshot poll: the async DB query in MusicPlayerList may
    // finish after Playstarted fires (typically 65-180ms later). When that
    // happens currentScreenshot is "" but player.getInfo() now has the URL.
    // Poll each frame while Playing with no screenshot loaded so we pick it up
    // without waiting for the next song change.
    if (playerState == MusicPlayerList::Playing && currentScreenshot == "") {
        auto shot = player.getInfo().metadata[SongInfo::SCREENSHOT];
        if (shot != "") {
            currentInfo.metadata[SongInfo::SCREENSHOT] = shot;
            loadScreenshot(shot);
        }
    }

    // A screenshot download settled (its callback may have run on the web worker
    // thread). Install the decoded bitmaps into `screenshots` HERE, on the render
    // thread, so the transition callbacks that read `screenshots` -- and the GL
    // work in transitions.restart() -- never race the downloader. acquire pairs
    // with the release-store in the download callback.
    if (pendingShotReady.exchange(false, std::memory_order_acquire)) {
        std::vector<NamedBitmap> bms;
        std::string key;
        {
            std::lock_guard<std::mutex> lg(pendingShotLock);
            bms = std::move(pendingShotBms);
            pendingShotBms.clear();
            key = pendingShotKey;
        }
        // Drop the result if the song (and thus the wanted screenshot) changed
        // while the download was in flight.
        if (key == currentScreenshot) {
            screenshots = std::move(bms);
            // Discard failed/empty entries, then sort for a stable rotation.
            screenshots.erase(
                std::remove(screenshots.begin(), screenshots.end(), ""),
                screenshots.end());
            std::sort(screenshots.begin(), screenshots.end());
            if (screenshots.empty()) {
                // Every download failed. For a YouTube song, walk the video's own
                // thumbnails before the (often mis-guessed) platform logo, so the
                // picture stays on-topic: sddefault -> hqdefault (present for
                // every video) -> logo. `key` tells us which rung just failed, so
                // we step down without looping.
                std::string base = youtubeThumbBase(currentInfo.path);
                bool retried = false;
                if (!base.empty()) {
                    std::string sd = base + "sddefault.jpg";
                    std::string hq = base + "hqdefault.jpg";
                    if (key != sd && key != hq) {
                        loadScreenshot(sd);
                        retried = true;
                    } else if (key == sd) {
                        loadScreenshot(hq);
                        retried = true;
                    }
                    // key == hq: even the guaranteed thumbnail failed -> logo.
                }
                if (!retried) {
                    appendLogoScreenshots();
                    transitions.restart();
                }
            } else {
                // Song has real screenshots -- still append the platform (or
                // extension) logo as the final frame so it always rotates in
                // last.
                appendPlatformOrExtLogo();
                transitions.restart();
            }
        }
    }

    // A podcast-artwork download finished (on the web worker); upload it to the
    // search-logo icon here on the render thread. Skip if the cursor has since
    // moved to a different row (searchLogoUrl no longer matches).
    if (pendingSearchLogo.exchange(false, std::memory_order_acquire)) {
        if (currentScreen == SEARCH_SCREEN &&
            pendingSearchLogoUrl == searchLogoUrl &&
            pendingSearchLogoBm.width() > 0) {
            searchLogoIcon.setBitmap(pendingSearchLogoBm);
            positionSearchLogo();
        }
    }

    // Same for a Databases-screen collection artwork download.
    if (pendingDatabaseLogo.exchange(false, std::memory_order_acquire)) {
        if (currentScreen == DATABASE_SCREEN &&
            pendingDatabaseLogoUrl == databaseLogoUrl &&
            pendingDatabaseLogoBm.width() > 0) {
            centerLogoIcon(databaseLogoIcon, pendingDatabaseLogoBm);
        }
    }

    if (playerState == MusicPlayerList::Error) {
        player.stop();
        currentTween.finish();
        currentInfoField[0].pos.x = currentInfoField[2].pos.x;
        currentInfoField[1].pos.x = currentInfoField[2].pos.x;
        titleMarqueeActive = false;
        titleMarqueePhase[0] = titleMarqueePhase[1] = 0.0f;

        SongInfo song = player.getInfo();
        song.format = MusicDatabase::describeFormat(song);
        prevInfoField.setInfo(song);
        currentTween = Tween::make()
                           .from(prevInfoField, nextInfoField)
                           .seconds(3.0)
                           .onComplete([=]() {
                               if (playerState == MusicPlayerList::Stopped)
                                   player.nextSong();
                           });
        currentTween.start();
    }

    if (playerState == MusicPlayerList::Playing ||
        playerState == MusicPlayerList::Stopped) {
        if (player.playlistUpdated()) {
            updateNextField();
        }
    }

    int tune = player.getTune();
    if (currentTune != tune) {
        songField.add = 0.0;
        Tween::make().sine().to(songField.add, 1.0).seconds(0.5);
        currentInfo = player.getInfo();
        currentSongPlatform = MusicDatabase::platformScreenshotName(currentInfo);
        currentSongExt = songExtension(currentInfo);
        currentSongFormat = utils::toLower(currentInfo.format);
        currentInfo.format = MusicDatabase::describeFormat(currentInfo);
        auto sub_title = player.getMeta("sub_title");
        xinfoField.setText(sub_title);
        currentInfoField.setInfo(currentInfo);
        currentTune = tune;
        songField.setText(utils::format("[%02d/%02d]", currentTune + 1,
                                        currentInfo.numtunes));
        auto m = compressWhitespace(player.getMeta("message"));
        bool isRadio = utils::startsWith(dbInfo.path, "radio::");
        if (m == "" && isRadio) {
            m = currentInfo.title;
        }
        // Same metadata -> format -> back cycle as on Playstarted, so subtune
        // changes keep the format info appended. Skip for radio streams.
        if (!isRadio)
            m = appendFormatInfo(m, currentInfo);
        if (m != "" && scrollText != m) {
            scrollEffect.set("scrolltext", m);
            scrollText = m;
        }
        updateFavorite();
    }

    if (player.isPlaying()) {
        auto br = player.getBitrate();
        if (br > 0) {
            songField.setText(utils::format("%d KBit", br));
        }

        auto p = player.getPosition();
        int length = player.getLength();
        timeField.setText(utils::format("%02d:%02d", p / 60, p % 60));
        if (length > 0)
            lengthField.setText(
                utils::format("(%02d:%02d)", length / 60, length % 60));
        else
            lengthField.setText("");

        auto sub_title = player.getMeta("sub_title");
        if (sub_title != xinfoField.getText()) xinfoField.setText(sub_title);
    }

    if (player.hasError()) {
        toast(player.getError(), ERROR);
    }

    if (!player.isPaused()) {
        auto decayEq = [](std::vector<uint8_t>& values) {
            for (auto& e : values) {
                if (e >= 4 * 4)
                    e -= 2 * 4;
                else
                    e = 2 * 4;
            }
        };
        decayEq(eq);
        decayEq(eqLeft);
        decayEq(eqRight);
        decayEq(eqMono);
    }

    if (player.isPlaying()) {
        auto delay = 1;
        if (fft.size() > delay) {
            while (fft.size() > delay + 4) {
                fft.popLevels();
            }
            spectrum = fft.getStereoLevels();
            fft.popLevels();
        }
        for (auto i : utils::count_to(fft.eq_slots)) {
            auto updateEq = [](uint16_t source, uint8_t& target) {
                if (source > 5) {
                    auto f = static_cast<unsigned>(logf(source) * 64);
                    if (f > 255) f = 255;
                    if (f > target) target = static_cast<uint8_t>(f);
                }
            };

            updateEq(spectrum.left[i], eqLeft[i]);
            updateEq(spectrum.right[i], eqRight[i]);
            updateEq((spectrum.left[i] + spectrum.right[i]) / 2, eqMono[i]);
            eq[i] = eqMono[i];

            // Accumulate per-channel difference vs. total energy. A mono source
            // (or a tune that simply duplicates one channel) yields left==right
            // across all slots, so the ratio stays at zero.
            int d = (int)spectrum.left[i] - (int)spectrum.right[i];
            stereoDiffAccum += (d < 0) ? -d : d;
            stereoSumAccum += (int)spectrum.left[i] + (int)spectrum.right[i];
        }

        if (autoStereoDetect && ++stereoDetectFrames >= 45) {
            // Hysteresis: need a clear difference to flip to stereo, and near
            // silence between the channels to fall back to mono.
            if (stereoSumAccum > 1) {
                double ratio = stereoDiffAccum / stereoSumAccum;
                if (!stereoSpectrum && ratio > 0.03) stereoSpectrum = true;
                else if (stereoSpectrum && ratio < 0.01) stereoSpectrum = false;
            }
            // Roll the window so the detector keeps tracking the live signal.
            stereoDiffAccum *= 0.5;
            stereoSumAccum *= 0.5;
            stereoDetectFrames = 0;
        }
    }
    bool busy = (playerState == MusicPlayerList::Loading || webutils::Web::inProgress() > 0);

    netIcon.visible(busy);

    if (transitions.idleFor(10000)) transitions.next();

    // Idle splash: show the platform/extension picture rotation whenever the app
    // sits on the main screen with nothing playing -- the "only the scroller is
    // visible" state at startup, and again whenever the user ESCs back to it.
    bool nowSplash = (currentScreen == MAIN_SCREEN) && !player.isPlaying() &&
                     currentInfo.title.empty() && !splashShots.empty();

    // The animated platform-logo transitions play behind the idle main-screen
    // splash AND (dimmed) behind the help/command screen. Drive the rotation
    // whenever either is showing.
    bool logoActive = !splashShots.empty() &&
                      (nowSplash || currentScreen == COMMAND_SCREEN);
    if (logoActive && !splashLogoActive) {
        // Just entered -- animate the first picture in. Safe here: update() runs
        // on the render thread, so the GL work in restart() is ok.
        splashTransitions.restart();
    } else if (!logoActive && splashLogoActive) {
        // Left -- drop the texture so no stale frame lingers.
        splashIcon.clear();
    }
    splashLogoActive = logoActive;
    if (logoActive && splashTransitions.idleFor(2000)) splashTransitions.next();

    splashActive = nowSplash;
    if (splashActive) {
        // No audio is playing, but we want the spectrum analyzer to look alive so
        // users see the feature at a glance. Randomly "hit" bars to fresh peaks
        // each frame; the decayEq() pass above brings them back down, producing
        // the same jumping-bars motion as a real tune. eqLeft/eqRight use
        // independent draws so the two stereo halves differ.
        static std::mt19937 srng{ std::random_device{}() };
        auto kick = [&](std::vector<uint8_t>& bars) {
            for (auto& b : bars) {
                if ((srng() & 0xff) < 70) { // ~27% chance to re-hit per frame
                    int peak = 90 + (int)(srng() % 166); // 90..255
                    if (peak > b) b = (uint8_t)peak;
                }
            }
        };
        kick(eqLeft);
        kick(eqRight);
        kick(eqMono);
        eq = eqMono;
    }
}

void ChipMachine::toast(std::string const& txt, ToastType type)
{
    // One entry per ToastType, in enum order.
    static std::vector<Color> colors = {
        0xffffff, // WHITE
        0xff8888, // ERROR        -- soft red
        0x55aa55, // NORMAL
        0xffffff, // STICKY
        0xff0000, // STICKY_ALERT -- saturated red
    };

    toastField.setText(txt);
    int tlen = toastField.getWidth();
    toastField.pos.x = topLeft.x + ((downRight.x - topLeft.x) - tlen) / 2;
    toastField.color = colors[static_cast<size_t>(type) % colors.size()];

    Tween::make()
        .to(toastField.color.alpha, 1.0)
        .seconds(0.25)
        .onComplete([=]() {
            // The STICKY kinds stay up until something replaces them; everything
            // else fades out on its own.
            if (type != STICKY && type != STICKY_ALERT)
                Tween::make()
                    .to(toastField.color.alpha, 0.0)
                    .delay(1.0)
                    .seconds(0.25);
        });
}

void ChipMachine::removeToast()
{
    toastField.setText("");
    toastField.color = 0;
}

void ChipMachine::drawProgressBar(float frac, std::string const& label)
{
    if (frac < 0.0f) frac = 0.0f;
    if (frac > 1.0f) frac = 1.0f;

    float gscale = screen.height() / 576.0f;

    float cx = topLeft.x + (downRight.x - topLeft.x) * 0.5f;
    float barW = (downRight.x - topLeft.x) * 0.5f;
    float barH = 44.0f * gscale;
    float barX = cx - barW * 0.5f;
    float barY = toastField.pos.y + toastField.getHeight() + 12.0f * gscale;
    float b = std::max(2.0f, 2.0f * gscale); // border thickness

    const uint32_t white = 0xffffffff;
    // White outline, black interior.
    screen.rectangle(barX, barY, barW, barH, white);
    screen.rectangle(barX + b, barY + b, barW - 2 * b, barH - 2 * b, 0xff000000);

    // Colour ramps red -> yellow -> green across the FULL bar width, revealed
    // left-to-right up to `frac`. So the fill starts red and its leading edge
    // shifts toward green as it approaches 100% (done).
    auto specColor = [](float t) -> uint32_t {
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
        float r, g;
        if (t < 0.5f) { r = 1.0f; g = t / 0.5f; }         // red -> yellow
        else { r = 1.0f - (t - 0.5f) / 0.5f; g = 1.0f; }  // yellow -> green
        return 0xff000000u | ((uint32_t)(r * 255.0f) << 16) |
               ((uint32_t)(g * 255.0f) << 8);
    };
    float innerX = barX + b;
    float innerY = barY + b;
    float innerW = barW - 2 * b;
    float innerH = barH - 2 * b;
    float filledW = innerW * frac;
    const int strips = 64;
    float stripW = innerW / strips;
    for (int i = 0; i < strips; i++) {
        float left = i * stripW;
        if (left >= filledW) break;
        float w = std::min(stripW, filledW - left);
        screen.rectangle(innerX + left, innerY, w, innerH,
                         specColor((i + 0.5f) / strips));
    }

    std::string pct = utils::format("%d%%", (int)(frac * 100.0f));
    float tScale = 0.6f * gscale;
    auto tsz = font.get_size(pct, tScale);
    // Centre the line box in the bar, then apply an explicit pixel nudge
    // (progressPctYNudge) because this font's glyphs sit high within their line
    // box. The nudge is in gscale pixels for a predictable 1:1 lever. Once the
    // fill passes the centred label (>= 50%), switch it to black so it stays
    // legible against the bright fill.
    uint32_t textColor = (frac >= 0.5f) ? 0xff000000 : white;
    screen.text(font, pct, cx - tsz.x * 0.5f,
                barY + (barH - tsz.y) * 0.5f + progressPctYNudge * gscale,
                textColor, tScale);

    // Source tag of whatever is being worked on (the collection id during
    // indexing), centred under the bar in the same smaller/dimmer style the
    // search results use next to the format line.
    if (!label.empty()) {
        float lScale = 1.5f * gscale;
        auto lsz = font.get_size(label, lScale);
        screen.text(font, label, cx - lsz.x * 0.5f,
                    barY + barH + 40.0f * gscale, 0xff30ff60, lScale);
    }
}

void ChipMachine::render(uint32_t delta)
{
    if (screen.size() != screenSize) {
        resizeDelay = 2;
        screenSize = screen.size();
    }

    if (resizeDelay) {
        resizeDelay--;
        if (resizeDelay == 0) {
            layoutScreen();
        }
    }

    screen.clear(0xff000000 | bgcolor);

    // Keep the simulated spectrum alive during the idle splash (update() fills
    // the eq arrays there); only blank the bars when genuinely stopped.
    if ((playerState == MusicPlayerList::Stopped ||
         playerState == MusicPlayerList::Error) &&
        !splashActive) {
        std::fill(eq.begin(), eq.end(), 0);
        std::fill(eqLeft.begin(), eqLeft.end(), 0);
        std::fill(eqRight.begin(), eqRight.end(), 0);
        std::fill(eqMono.begin(), eqMono.end(), 0);
    }

    int targetMusicBarsWidth = stereoSpectrum ? spectrumWidth : spectrumWidth * 2;
    if (musicBarsWidth != targetMusicBarsWidth) {
        musicBarsWidth = targetMusicBarsWidth;
        musicBars.setup(musicBarsWidth, spectrumHeight);
    }

    if (stereoSpectrum) {
        musicBars.render(spectrumPos, spectrumColor, eqLeft);
        utils::vec2i rightSpectrumPos = {
            spectrumPos.x + spectrumWidth * SpectrumAnalyzer::eq_slots + spectrumGap,
            spectrumPos.y
        };
        musicBars.render(rightSpectrumPos, spectrumColor, eqRight);
    } else {
        musicBars.render(spectrumPos, spectrumColor, eqMono);
    }

    if (starsOn) starEffect.render(delta);

    // Update the title bounce-scroll before the screen draws currentInfoField.
    updateTitleMarquee(delta);

    if (currentScreen == MAIN_SCREEN) {
        mainScreen.render(screenptr, delta);
        if (splashActive) {
            // Picture first, then the welcome banner on TOP of it (still below
            // the foreground scroller).
            splashIcon.alphaScale = 1.0f; // full brightness on the main splash
            splashIcon.render(screenptr, delta);
            updateSplashWelcome(delta);
            splashWelcomeField.render(screenptr, delta);
        }
    } else if (currentScreen == SEARCH_SCREEN) {
        searchScreen.render(screenptr, delta);
    } else if (currentScreen == ADVANCED_SCREEN) {
        advancedScreen.render(screenptr, delta);
    } else if (currentScreen == FORMAT_SCREEN) {
        formatScreen.render(screenptr, delta);
    } else if (currentScreen == DATABASE_SCREEN) {
        databaseScreen.render(screenptr, delta);
    } else if (currentScreen == PLUGIN_SCREEN) {
        pluginScreen.render(screenptr, delta);
    } else {
        // Help/command screen: the same animated platform-logo transitions as the
        // splash, but dimmed (same alpha as the platform-filter screen's logo) and
        // drawn behind the text -- in front of the stars, below the letters.
        if (splashLogoActive) {
            splashIcon.alphaScale = 0x50 / 255.0f;
            splashIcon.render(screenptr, delta);
        }
        commandScreen.render(screenptr, delta);
    }

    // Draw the scroller AFTER the screen content so it stays in the foreground.
    // It was previously drawn before the screens, so the album-art cover (and
    // other screen elements) painted over it whenever the sine wobble pushed the
    // text up into their area. Kept below the modal overlay so dialogs/help still
    // sit on top of the scroll.
    // Show a big centred app icon from launch until indexing finishes, so
    // there's something on screen the instant the app opens, before the
    // scroller/screenshots are ready. Drawn on top of everything else so far
    // (including the "Indexing database" toast) since the app is otherwise
    // effectively idle at this point.
    if (indexingDatabase)
        startupIcon.render(screenptr, delta);

    // Hold the scroller until indexing finishes, so it doesn't start scrolling
    // over the "Indexing database" progress screen.
    if (!indexingDatabase)
        scrollEffect.render(delta);

    // While paused (SPACE) on the main screen, show a big mute symbol in the
    // centre with a large white "SPACE" beside it, so the user knows what they
    // pressed and how to un-mute. Only drawn on MAIN_SCREEN -- that is where
    // SPACE toggles pause, so the overlay would be misleading on the help /
    // platform screens.
    // Blink once per second: visible for the first half of each second, hidden
    // for the second half.
    if (currentScreen == MAIN_SCREEN && player.isPlaying() && player.isPaused() &&
        pausedIcon.getTextureWidth() > 0 && (utils::getms() / 500) % 2 == 0) {
        // All sizes/offsets are authored against the 576px reference height and
        // multiplied by gscale so the overlay grows/shrinks with the window just
        // like the rest of the screen (gscale matches Scroller.h / the marquee).
        float gscale = screen.height() / 576.0f;
        float isz = 180.0f * gscale;
        float textScale = 2.25f * gscale;
        auto tsz = font.get_size("SPACE", textScale);
        float gap = 40.0f * gscale;
        float gx = 25.0f * gscale;
        float gy = ((float)screen.height() - isz) / 2.0f;
        screen.text(font, "SPACE", gx, gy + (isz - tsz.y) / 2.0f + 40 * gscale,
                    0xffffffff, textScale);
        pausedIcon.setArea({ gx + tsz.x + gap, gy + 60 * gscale, isz / 2, isz / 2 });
        pausedIcon.render(screenptr, 0);
    }

    overlay.render(screenptr, delta);

    // Startup indexing progress bar, drawn just below the "Indexing database"
    // toast. A plain white outline that fills left-to-right, with the percentage
    // centred inside. Progress is (rows processed / progressMaxSongs); since the
    // real DB size isn't known up-front we assume progressMaxSongs and clamp.
    if (indexingDatabase && toastField.getText() != "") {
        // Two-phase progress: the DB-creation ticks fill the first
        // progressDbPhase of the bar, the row indexing fills the rest.
        float dbFrac = (float)musicDatabase.getDbCreatedCount() /
                       (float)std::max(1, progressMaxDatabases);
        if (dbFrac > 1.0f) dbFrac = 1.0f;
        float rowFrac = (float)musicDatabase.getIndexedCount() /
                        (float)std::max(1, progressMaxSongs);
        if (rowFrac > 1.0f) rowFrac = 1.0f;
        float frac = progressDbPhase * dbFrac +
                     (1.0f - progressDbPhase) * rowFrac;
        drawProgressBar(frac, musicDatabase.getIndexingName());
    } else if (loadingToastShown && toastField.getText() != "" &&
               utils::getms() - loadingToastStartMs > 5000) {
        // Slow remote fetch/prebuffer (e.g. a large Zophar zip): once the wait
        // passes 5s and playback still hasn't started, show how much of the file
        // has downloaded below the LOADING/BUFFERING toast. Gated on a known
        // total size, so open-ended radio streams never draw a bar.
        int64_t dl = 0, total = 0;
        if (remoteLoader.downloadProgress(dl, total))
            drawProgressBar((float)dl / (float)total);
    }

    // Drawn last so the volume bar always sits on top of every screen element
    // (album art, lists, scroller) and is never partially obscured. Only the
    // filled portion of the icon is drawn; the rest stays transparent (no black
    // masking rectangle) so the starfield shows through.
    float vol = player.getVolume();
    if (vol <= 0.0f && mutedIcon.getTextureWidth() > 0) {
        // Fully silenced: keep the muted glyph on screen the WHOLE time the
        // volume is at zero (not just the transient volume-overlay window), so
        // the user always knows audio is muted. Centred and sized against the
        // 576px reference height like the other overlays.
        float gscale = screen.height() / 576.0f;
        float iw = mutedIcon.getTextureWidth() * gscale;
        float ih = mutedIcon.getTextureHeight() * gscale;
        mutedIcon.setArea({ ((float)screen.width() - iw) / 2.0f,
                            ((float)screen.height() - ih) / 2.0f, iw, ih });
        mutedIcon.render(screenptr, 0);
        if (showVolume) showVolume--;
    } else if (showVolume) {
        // Audible volume: bars flash briefly after a volume change.
        showVolume--;
        volumeIcon.renderFraction(screenptr, vol);
    }

    font.update_cache();
    listFont.update_cache();

    screen.flip();

    webutils::Web::pollAll();
}
} // namespace chipmachine
