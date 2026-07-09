#include "ChipMachine.h"
#include "Icons.h"
#include "version.h"
#include <coreutils/environment.h>
#include <coreutils/format.h>
#include <coreutils/searchpath.h>
#include <grappix/window.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
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

const std::vector<FilterOption> ChipMachine::filterOptions = {
    { "[show all]", {} },
    { "Amiga", { AMIGA, PROTRACKER, SOUNDTRACKER, UADE, TRACKER } },
    { "Atari ST/STE/Falcon", { ATARI } },
    { "Atari 8bit (TIA/POKEY)", { POKEY } },
    { "Commodore 64 (SID)", { SID, STR } },
    { "Commodore 16/116/+4 (TED)", { PRG } },
    { "ZX Spectrum 16K/48K (Beeper)", { ZXBEEPER } },
    { "ZX Spectrum 128K (AY)", { ZXAY, SPECTRUM } },
    { "IBM PC (Trackers/DAWs)", { FASTTRACKER, IMPULSETRACKER, SCREAMTRACKER, PCTRACKER, PC } },
    { "IBM PC (AdLib/OPL)", { ADPLUG } },
    { "MSX", { MSX } },
    { "Amstrad CPC", { AMSTRAD } },
    { "Sam Coupe", { SAMCOUPE } },
    { "Acorn Archimedes", { ACORN } },
    { "Apple Mac/IIGS/MacOS/iOS", { APPLE } },
    { "Sony PlayStation 1/2", { PLAYSTATION, PLAYSTATION2 } },
    { "Nintendo NES", { NES } },
    { "Nintendo SNES", { SNES } },
    { "Nintendo GameBoy/GBA", { GAMEBOY, GBA } },
    { "Nintendo 64", { NINTENDO64 } },
    { "Nintendo DS", { NDS } },
    { "Sega 8bit", { SEGAMS } },
    { "Sega 16bit/32X/Saturn", { SEGA, MEGADRIVE, SATURN } },
    { "Sega Dreamcast", { DREAMCAST } },
    { "PC-98/X68000/FM Towns", { JPFM } },
    { "PC Engine/TurboGrafx-16", { HES } },
    { "WonderSwan", { WONDERSWAN } },
    { "Arcade", { ARCADE } },
    { "Other Platforms", { OTHER } },
    { "Unclassified MP3/OGG", { MP3, OGG } },
    { "Unclassified YouTube Audio", { YOUTUBE } },
    { "Podcasts", { PODCAST } },
    { "Radio Stations", { RADIO } }
};

// Base color for a format byte. Shared by the now-playing list (renderSong)
// and the F9 filter screen so platforms keep a consistent color everywhere.
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
        { SID, 0xffcc8844 },     { PRG, 0xffbb66cc },
        { ZXBEEPER, 0xffff88dd }, { ZXAY, 0xffbb88ff }, { SPECTRUM, 0xffbb88ff },
        { MSX, 0xff66ddaa },     { AMSTRAD, 0xff44aadd },
        { ACORN, 0xff88dd55 },   { SAMCOUPE, 0xffdd66aa },
        { ATARI, 0xffcccc33 },   { POKEY, 0xffee7711 },
        { MP3, 0xff88ff88 },
        { APPLE, 0xff66cccc },
        { M3U, 0xffaaddaa },     { RADIO, 0xffff7722 },
        { YOUTUBE, 0xffff0000 },
        { PODCAST, 0xff22bbff },
        { PC, 0xffcccccc },      { JPFM, 0xffff66cc },
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
    bool isShow = std::stol(parts[2]) >= MusicDatabase::PODCAST_SHOW_INDEX;

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
            text = utils::format("%s / %s", parts[0], parts[1]);
    }
    uint32_t base = formatColor(f);
    long fullIndex = std::stol(parts[2]);
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

    screenShotIcon = Icon(image::bitmap(8, 8), 100, 100);
    mainScreen.add(&screenShotIcon);
    // Give the transition driver access to the screenshot icon, the loaded
    // screenshot bitmaps, and the geometry recompute.
    transitions.configure(
        screenShotIcon, [this] { return (int)screenshots.size(); },
        [this](int i) -> const image::bitmap& { return screenshots[i].bm; },
        [this] { updateScreenshotArea(); });

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
    searchScreen.add(&searchField);
    searchField.visible(false);

    searchScreen.add(&topStatus);
    topStatus.visible(false);

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
                uint32_t c = 0xaa00cc00;
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
                int cmdPos = rec.w * 0.6;
                std::string displayName = cmd->name;
                for (char& ch : displayName) {
                    if (ch == '_') ch = ' ';
                }
                grappix::screen.text(listFont, displayName, rec.x, rec.y, c,
                                     resultFieldTemplate.scale);
                grappix::screen.text(listFont, cmd->shortcut, rec.x + cmdPos,
                                     rec.y, 0xffffffff,
                                     resultFieldTemplate.scale * 0.8);
            }
        });

    commandList.setTotal(commands.size());
    clearCommand();

    updateLists();

    commandScreen.add(&commandField);
    commandScreen.add(&commandList);

    mainFilterField.setFont(font);
    mainFilterField.visible(true);
    mainFilterField.setText("");
    mainScreen.add(&mainFilterField);

    advancedTitle.setFont(font);
    advancedTitle.color = 0xffffffaa;
    advancedTitle.scale = searchField.scale;
    advancedTitle.visible(true);
    advancedTitle.setText("FILTER SEARCH RESULTS BY PLATFORM / CATEGORY:");
    advancedScreen.add(&advancedTitle);

    // The filter screen lays its entries out in two columns (column-major: the
    // left column holds the first half, the right column the rest) so all
    // platforms fit without scrolling. Up/Down still walk the single selection
    // index (down the left column, then down the right). visibleItems is set to
    // the item count so the list renders every entry (and never scrolls).
    advancedList = VerticalList(
        listrec, (int)filterOptions.size(),
        [=](grappix::Rectangle& rec, int y, uint32_t index, bool hilight) {
            if (index >= filterOptions.size()) return;
            auto const& opt = filterOptions[index];
            // Inherit the platform's color (see formatColor / renderSong); the
            // "[No Filter]" entry has no single format, so render it white.
            uint32_t c = opt.matchedFormats.empty()
                             ? 0xffffffff
                             : formatColor(opt.matchedFormats[0]);
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
            std::string label = opt.name;
            uint8_t fmt0 =
                opt.matchedFormats.empty() ? 0 : opt.matchedFormats[0];
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
            if (index < filterCounts.size()) {
                if (fmt0 == RADIO) {
                    // Each radio entry IS one station, so just count-prefix the
                    // name ("10 Radio Stations") -- no "[N streams]" bracket.
                    label = utils::format("%s %s",
                                          withCommas(filterCounts[index]),
                                          opt.name);
                } else {
                    // Count unit by platform: "[No Filter]" spans everything
                    // (tunes + podcasts + radio) so it counts in "items";
                    // podcasts in episodes, everything else in tunes.
                    const char* unit = opt.matchedFormats.empty()
                                           ? "items"
                                           : (fmt0 == PODCAST ? "episodes"
                                                              : "tunes");
                    label += utils::format("  [%s %s]",
                                           withCommas(filterCounts[index]),
                                           unit);
                }
            }

            int rows = ((int)filterOptions.size() + 1) / 2;
            int col = (int)index / rows;
            int row = (int)index % rows;
            // Leave the top row of the right column empty so "[No Filter]"
            // (alone at the top of the left column) stands out: shift the right
            // column down by one row.
            row += col;
            float lineH = advancedArea.h / (float)numLines;
            float colW = advancedArea.w / 2.0f;
            float px = advancedArea.x + col * colW;
            float py = advancedArea.y + lineH * (row + 1);
            grappix::screen.text(listFont, label, px, py, c,
                                 resultFieldTemplate.scale * 0.9f);
        });
    advancedList.setTotal(filterOptions.size());
    advancedList.setVisible((int)filterOptions.size());
    advancedList.setArea(advancedArea); // match the layout area (scissor clip)
    advancedScreen.add(&advancedList);

    scrollText = "INITIAL_TEXT";
    scrollEffect.set("scrolltext",
        "Type to search . . UP+DOWN/ENTER navigate/play"
        " . . F9=formats"
        " . . TAB=help . . . "
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

bool ChipMachine::noImages = false;

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
    // One callback fires per requested part; count them down so we know when
    // every download has settled (success or failure) before finalizing.
    auto remaining = std::make_shared<int>((int)parts.size());
    auto cb = [=](utils::File f) {
        // Bail if the song changed (or went to the logo-only path) meanwhile.
        if (currentScreenshot != shot)
            return;
        if (f) {
            try {
                if (utils::toLower(utils::path_extension(
                        f.getName())) == "gif") {
                    for (auto& bm : image::load_gifs(f.getName())) {
                        for (auto& px : bm) {
                            if ((px & 0xffffff) == 0)
                                px &= 0xffffff;
                        }
                        screenshots.emplace_back(f.getFileName(), bm);
                    }
                } else {
                    auto bm = image::load_image(f.getName());
                    for (auto& px : bm) {
                        if ((px & 0xffffff) == 0) px &= 0xffffff;
                    }
                    screenshots.emplace_back(f.getFileName(), bm);
                }
            } catch (image::image_exception& e) {
                LOGD("Failed to load image");
            }
        }

        if (--(*remaining) <= 0) {
            // All downloads settled. Sort the real screenshots. Only fall back
            // to the platform + ChipMachine logos when no real screenshot loaded
            // (e.g. every download failed) -- if the song has art, show only it.
            screenshots.erase(std::remove(screenshots.begin(),
                                          screenshots.end(), ""),
                              screenshots.end());
            sort(screenshots.begin(), screenshots.end());
            if (screenshots.empty()) {
                // Every download failed. For a YouTube song, walk the video's own
                // thumbnails before the (often mis-guessed) platform logo, so the
                // picture stays on-topic: matched external art -> sddefault ->
                // hqdefault (present for every video) -> logo. Comparing against
                // `shot` tells us which rung just failed, so we step down without
                // looping.
                std::string base = youtubeThumbBase(currentInfo.path);
                if (!base.empty()) {
                    std::string sd = base + "sddefault.jpg";
                    std::string hq = base + "hqdefault.jpg";
                    if (shot != sd && shot != hq) {
                        loadScreenshot(sd);
                        return;
                    }
                    if (shot == sd) {
                        loadScreenshot(hq);
                        return;
                    }
                    // shot == hq: even the guaranteed thumbnail failed -> logo.
                }
                appendLogoScreenshots();
            } else {
                // Song has real screenshots -- still append the platform (or
                // extension) logo as the final frame so it always rotates in
                // last. No generic icon here: if there is no platform/ext logo,
                // just rotate the real screenshots as before.
                appendPlatformOrExtLogo();
            }
            // Do NOT call transitions.restart() here: this callback may run on
            // the web worker thread (async download, no GL context), and
            // restart()->next() touches OpenGL (releasing held textures such as
            // the cube-flip's cubeTexOld runs glDeleteTextures -> crash off the
            // render thread). It may ALSO run synchronously on the render thread
            // (cache hit, via getFile->call_handler), so we can't block/marshal
            // either (run_safely would self-deadlock). Just flag it; update() on
            // the render thread consumes the flag and does the GL-touching
            // restart. screenshots[] filled above is published by this
            // release-store; update() acquire-loads before reading it.
            pendingShotRestart.store(true, std::memory_order_release);
        }
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

// Songs that classify to the generic "Console" platform can optionally use a
// per-system logo (e.g. vectrex.png) instead of the generic Console.png. Maps
// the raw (lowercased) format string to the image base name in
// data/misc/platformscreenshots/.
static const std::map<std::string, std::string>& consoleSubLogos()
{
    static const std::map<std::string, std::string> m = {
        { "vectrex", "vectrex" },
        { "colecovision", "colecovision" },
        { "capcom q-sound format", "capcom" },
    };
    return m;
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
    };
    return m;
}

bool ChipMachine::appendPlatformOrExtLogo()
{
    // 0) Arcade boards: pick the per-board logo by format string (Capcom/Konami/
    //    Namco/Sega/Taito/Neo Geo/other), overriding the shared "vgz" ext logo.
    {
        auto a = arcadeSubLogos().find(currentSongFormat);
        if (a != arcadeSubLogos().end()) {
            auto it = extensionShots.find(a->second);
            if (it != extensionShots.end() && it->second.width() > 0) {
                screenshots.emplace_back("ext:" + a->second, it->second);
                LOGD("Screenshot logos: arcade='%s' logo='%s'",
                     currentSongFormat.c_str(), a->second.c_str());
                return true;
            }
        }
    }
    // 1) Per-extension screenshot (e.g. mod.png, sid.png) takes priority over
    //    the platform logo.
    if (!currentSongExt.empty()) {
        auto it = extensionShots.find(currentSongExt);
        if (it != extensionShots.end() && it->second.width() > 0) {
            screenshots.emplace_back("ext:" + currentSongExt, it->second);
            LOGD("Screenshot logos: ext='%s' logo=yes", currentSongExt);
            return true;
        }
    }
    // 2) Per-platform logo for the current song, when one is installed.
    //    For the generic "Console" platform, prefer a per-system logo
    //    (vectrex.png, colecovision.png, capcom.png) when one is present.
    std::string logoSlug = currentPlatformSlug;
    if (currentPlatformSlug == "Console") {
        auto sub = consoleSubLogos().find(currentSongFormat);
        if (sub != consoleSubLogos().end() && platformShots.count(sub->second))
            logoSlug = sub->second;
    }
    if (!logoSlug.empty()) {
        auto it = platformShots.find(logoSlug);
        if (it != platformShots.end() && it->second.width() > 0) {
            screenshots.emplace_back("platform:" + logoSlug, it->second);
            LOGD("Screenshot logos: platform='%s' logo='%s' have=yes",
                 currentPlatformSlug, logoSlug);
            return true;
        }
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
    // Probe <name>.png|jpg|jpeg, black-key it, store under `key`; return found.
    // A slug may contain '/' (e.g. "ZX Spectrum 16/48"), which can't be a
    // filename, so also try a filesystem-safe variant with '/' replaced by '-'.
    auto load = [&](const std::string& key, const std::string& name) {
        std::vector<std::string> bases{ name };
        std::string safe = name;
        std::replace(safe.begin(), safe.end(), '/', '-');
        if (safe != name)
            bases.push_back(safe);
        for (auto& base : bases)
        for (auto ext : { ".png", ".jpg", ".jpeg" }) {
            auto p = dir / (base + ext);
            if (!utils::File::exists(p.string()))
                continue;
            try {
                auto bm = image::load_image(p.string());
                // Match the downloaded-screenshot behaviour: key out pure black
                // so logos exported on a black background show the starfield
                // through. (Real RGBA alpha is preserved either way.)
                for (auto& px : bm)
                    if ((px & 0xffffff) == 0) px &= 0xffffff;
                platformShots[key] = bm;
                return true;
            } catch (image::image_exception& e) {
                LOGW("Could not decode platform logo %s", p.string());
            }
        }
        return false;
    };

    std::vector<std::string> missing;
    for (auto& name : MusicDatabase::platformScreenshotNames())
        if (!load(name, name))
            missing.push_back(name);
    // Optional per-system logos for the generic "Console" platform (not part of
    // the platform list, so absence is never reported as missing).
    for (auto& [fmt, base] : consoleSubLogos())
        load(base, base);

    // Generic platform slugs that have no dedicated artwork reuse a specific
    // variant's logo. E.g. VGM/VGZ rips of Spectrum games carry the bare
    // "ZX Spectrum" platform, which has no logo of its own -> use the 128K one.
    static const std::pair<std::string, std::string> aliases[] = {
        { "ZX Spectrum", "ZX Spectrum 128" },
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
}

void ChipMachine::loadExtensionScreenshots()
{
    // Load whatever per-extension images are present (keyed by lowercased
    // basename, e.g. "mod.png" -> "mod"). The black-key matches the platform
    // logos so black-background images go transparent too.
    auto dir = workDir / "data" / "misc" / "extensionscreenshots";
    if (utils::File::exists(dir.string())) {
        for (auto& f : utils::File(dir.string()).listFiles()) {
            auto fn = f.getFileName();
            // path_extension() returns the extension WITHOUT the dot.
            auto e = utils::toLower(utils::path_extension(fn));
            if (e != "png" && e != "jpg" && e != "jpeg")
                continue;
            // Strip ".<ext>" (extension length + the dot) to get the key.
            auto key = utils::toLower(fn.substr(0, fn.size() - e.size() - 1));
            try {
                auto bm = image::load_image(f.getName());
                for (auto& px : bm)
                    if ((px & 0xffffff) == 0) px &= 0xffffff;
                extensionShots[key] = bm;
            } catch (image::image_exception& ex) {
                LOGW("Could not decode extension screenshot %s", f.getName());
            }
        }
    }
    LOGD("Loaded %d extension screenshots from %s",
         (int)extensionShots.size(), dir.string());

    // Report extensions that have NO extension screenshot and whose platform
    // also has NO platform logo -- i.e. the ones the platform fallback can't
    // cover, so they would land on the app icon. Extensions covered by a
    // platform logo are intentionally omitted.
    std::vector<std::string> uncovered;
    for (auto& [ext, plats] : musicDatabase.extensionPlatforms()) {
        if (extensionShots.count(ext))
            continue; // already has its own screenshot
        bool coveredByPlatform = false;
        for (auto& p : plats)
            if (!p.empty() && platformShots.count(p)) {
                coveredByPlatform = true;
                break;
            }
        if (!coveredByPlatform)
            uncovered.push_back(ext);
    }
    if (!uncovered.empty()) {
        std::string list;
        for (auto& e : uncovered)
            list += (list.empty() ? "" : ", ") + e;
        LOGW("%d extension(s) have no screenshot and no platform logo "
             "(add data/misc/extensionscreenshots/<ext>.png): %s",
             (int)uncovered.size(), list);
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

void ChipMachine::computeFilterCounts()
{
    if (musicDatabase.busy()) return; // not indexed yet -- retry later
    auto counts = musicDatabase.getFormatByteCounts();
    int total = 0;
    for (int c : counts)
        total += c;
    if (total == 0) return;
    podcastShowCount = musicDatabase.getPodcastShowCount();
    otherPlatformCount = musicDatabase.getOtherPlatformCount();
    arcadePlatformCount = musicDatabase.getArcadePlatformCount();
    filterCounts.assign(filterOptions.size(), 0);
    for (size_t i = 0; i < filterOptions.size(); i++) {
        auto const& opt = filterOptions[i];
        if (opt.matchedFormats.empty()) {
            filterCounts[i] = total; // "[Show All]"
        } else {
            int s = 0;
            for (uint8_t b : opt.matchedFormats)
                s += counts[b];
            filterCounts[i] = s;
        }
    }
}

void ChipMachine::update()
{
    if (indexingDatabase) {
        static int delay = 30;
        if (delay-- == 0) toast("Indexing database", STICKY);

        if (!musicDatabase.busy()) {
            indexingDatabase = false;
            removeToast();
            computeFilterCounts();
        } else
            return;
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

    // Show a "LOADING..." toast while a song that is not already in the local
    // cache is being fetched, and clear it the moment playback starts (or the
    // load otherwise ends). Cached songs load instantly, so they get no toast.
    if (playerState == MusicPlayerList::Loading) {
        if (!loadingToastChecked) {
            loadingToastChecked = true;
            auto rawPath = player.getInfo().path;
            auto loadingPath = utils::toLower(rawPath);
            // A file already present on local disk plays instantly -> no toast.
            bool isLocal = utils::exists(rawPath);
            // Radio and the ffmpeg-decoded formats are streamed rather than
            // played from a finished local file, so they "buffer".
            bool streamed = utils::startsWith(loadingPath, "radio::") ||
                            utils::endsWith(loadingPath, ".mp3") ||
                            utils::endsWith(loadingPath, ".ogg") ||
                            utils::endsWith(loadingPath, ".aac") ||
                            utils::endsWith(loadingPath, ".m4a") ||
                            utils::endsWith(loadingPath, ".mp4") ||
                            utils::endsWith(loadingPath, ".8svx");
            if (isLocal) {
                // nothing to fetch
            } else if (streamed) {
                toast("BUFFERING...", STICKY);
                loadingToastShown = true;
            } else {
                bool cached = false;
                try {
                    cached = remoteLoader.inCache(rawPath);
                } catch (...) {
                    cached = false; // unknown source -> assume it needs fetching
                }
                if (!cached) {
                    toast("LOADING...", STICKY);
                    loadingToastShown = true;
                }
            }
        }
    } else {
        loadingToastChecked = false;
        if (loadingToastShown) {
            removeToast();
            loadingToastShown = false;
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

    // A screenshot download callback (possibly on the web worker thread) asked
    // to (re)start the transition. Do the GL-touching restart here, on the
    // render thread. acquire-load pairs with the release-store in loadScreenshot
    // so the screenshots[] it filled are visible before restart() reads them.
    if (pendingShotRestart.exchange(false, std::memory_order_acquire))
        transitions.restart();

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
}

void ChipMachine::toast(std::string const& txt, ToastType type)
{
    static std::vector<Color> colors = {
        0xffffff, 0xff8888, 0x55aa55
    };

    toastField.setText(txt);
    int tlen = toastField.getWidth();
    toastField.pos.x = topLeft.x + ((downRight.x - topLeft.x) - tlen) / 2;
    toastField.color = colors[(int)type % 3];

    Tween::make()
        .to(toastField.color.alpha, 1.0)
        .seconds(0.25)
        .onComplete([=]() {
            if ((int)type < 3)
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

    if (playerState == MusicPlayerList::Stopped || playerState == MusicPlayerList::Error) {
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
    } else if (currentScreen == SEARCH_SCREEN) {
        searchScreen.render(screenptr, delta);
    } else if (currentScreen == ADVANCED_SCREEN) {
        advancedScreen.render(screenptr, delta);
    } else {
        commandScreen.render(screenptr, delta);
    }

    // Draw the scroller AFTER the screen content so it stays in the foreground.
    // It was previously drawn before the screens, so the album-art cover (and
    // other screen elements) painted over it whenever the sine wobble pushed the
    // text up into their area. Kept below the modal overlay so dialogs/help still
    // sit on top of the scroll.
    scrollEffect.render(delta);

    // While paused (F5), show a big mute symbol in the centre with a large white
    // "F5" beside it, so the user knows what they pressed and how to un-mute.
    // Blink once per second: visible for the first half of each second, hidden
    // for the second half.
    if (player.isPlaying() && player.isPaused() &&
        pausedIcon.getTextureWidth() > 0 && (utils::getms() / 500) % 2 == 0) {
        // All sizes/offsets are authored against the 576px reference height and
        // multiplied by gscale so the overlay grows/shrinks with the window just
        // like the rest of the screen (gscale matches Scroller.h / the marquee).
        float gscale = screen.height() / 576.0f;
        float isz = 180.0f * gscale;
        float textScale = 2.25f * gscale;
        auto tsz = font.get_size("F5", textScale);
        float gap = 40.0f * gscale;
        float gx = 25.0f * gscale;
        float gy = ((float)screen.height() - isz) / 2.0f;
        screen.text(font, "F5", gx, gy + (isz - tsz.y) / 2.0f + 40 * gscale,
                    0xffffffff, textScale);
        pausedIcon.setArea({ gx + tsz.x + gap, gy + 60 * gscale, isz / 2, isz / 2 });
        pausedIcon.render(screenptr, 0);
    }

    overlay.render(screenptr, delta);

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
