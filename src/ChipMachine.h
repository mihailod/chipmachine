#pragma once

#include "Dialog.h"
#include "LineEdit.h"
#include "MusicBars.h"
#include "MusicDatabase.h"
#include "MusicPlayerList.h"
#include "SongInfoField.h"
#include "TelnetInterface.h"
#include "TextField.h"
#include "state_machine.h"

#include "../demofx/Scroller.h"
#include "../demofx/StarField.h"
#include "../demofx/Transitions.h"
#include "../sol2/sol.hpp"

#include <coreutils/utils.h>
#include <fft/spectrum.h>
#include <grappix/grappix.h>
#include <grappix/gui/list.h>
#include <grappix/gui/renderset.h>
#include <tween/tween.h>

#include <cstdint>
#include <random>
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <atomic>

namespace chipmachine {

class Icon : public Renderable
{
public:
    Icon() = default;

    Icon(std::shared_ptr<grappix::Texture> tx, float x, float y, float w,
         float h)
        : texture(tx), rec(x, y, w, h)
    {}

    Icon(const image::bitmap& bm, int x = 0, int y = 0)
        : rec(x, y, bm.width(), bm.height())
    {
        setBitmap(bm);
    }

    Icon(const image::bitmap& bm, float x, float y, float w, float h)
        : rec(x, y, w, h)
    {
        setBitmap(bm);
    }

    void render(std::shared_ptr<grappix::RenderTarget> target,
                uint32_t delta) override
    {
        if (!texture || (color >> 24) == 0) return;
        // An effect (e.g. ScreenshotTransitions) can install a custom renderer
        // that fully takes over drawing while it runs; otherwise draw the plain
        // textured quad.
        if (customRender) {
            customRender(target, delta);
            return;
        }
        target->draw(*texture, rec.x, rec.y, rec.w, rec.h, nullptr,
                     scaledColor((uint32_t)color));
    }

    // Render only the left `frac` (0..1) of the texture, scaled into the left
    // `frac` of the icon's area. The unfilled part is simply not drawn, so it
    // stays transparent instead of being masked with an opaque rectangle (used
    // for the volume-level bar over the starfield).
    void renderFraction(std::shared_ptr<grappix::RenderTarget> target,
                        float frac)
    {
        if (!texture || (color >> 24) == 0) return;
        if (frac <= 0.0f) return;
        if (frac > 1.0f) frac = 1.0f;
        float uvs[8] = { 0, 0, frac, 0, 0, 1, frac, 1 };
        target->draw(*texture, rec.x, rec.y, rec.w * frac, rec.h, uvs, color);
    }

    // The current texture, for custom renderers that draw it themselves.
    grappix::Texture* getTexture() const { return texture.get(); }

    void setBitmap(const image::bitmap& bm, bool filter = false)
    {
        texture = std::make_shared<grappix::Texture>(bm);
        rec.w = bm.width();
        rec.h = bm.height();
        glBindTexture(GL_TEXTURE_2D, texture->id());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                        filter ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        filter ? GL_LINEAR : GL_NEAREST);
    }

    void clear() { texture = nullptr; }

    void setArea(const grappix::Rectangle& r) { rec = r; }

    int getTextureWidth() const { return texture ? texture->width() : 0; }
    int getTextureHeight() const { return texture ? texture->height() : 0; }

    grappix::Color color{ 0xffffffff };
    grappix::Rectangle rec;

    // Extra opacity multiplier (0..1) applied on top of `color`, so an animated
    // icon (whose colour the ScreenshotTransitions constantly drive/reset) can
    // still be shown dimmed -- e.g. the platform-logo transitions behind the
    // help screen. The transitions read this too (see Transitions.cpp).
    float alphaScale = 1.0f;
    // color with its alpha scaled by alphaScale.
    uint32_t scaledColor(uint32_t c) const
    {
        if (alphaScale >= 1.0f) return c;
        uint32_t a = (uint32_t)(((c >> 24) & 0xff) * alphaScale);
        return (a << 24) | (c & 0x00ffffff);
    }

    // Optional custom renderer installed by an effect (e.g. the screenshot
    // transitions). When set, it draws the icon instead of the default textured
    // quad. Cleared when the effect finishes.
    std::function<void(std::shared_ptr<grappix::RenderTarget>, uint32_t)>
        customRender;

private:
    std::shared_ptr<grappix::Texture> texture;
};

struct FilterOption {
    std::string name;
    std::vector<uint8_t> matchedFormats;
    // A grouping entry (e.g. "Nintendo", "Sony"): selecting it does not apply a
    // filter but drills into a 2nd-level list of these child options, which
    // themselves apply the filter. Empty for ordinary (leaf) entries.
    std::vector<FilterOption> children;
    // House logo for a GROUP row, as the basename of an image in
    // data/misc/platformscreenshots (e.g. "Atari" -> Atari.png). A group has no
    // format byte of its own, so by default it borrows its first child's logo --
    // which makes the row look like that one machine. Set this to show the family
    // mark instead: on the platform screen when the group row is highlighted, and
    // on the splash.
    // Also used by ordinary (leaf) entries whose format byte resolves to no
    // platform-shot slug at all (MP3/OGG/Radio/Podcasts/Other Platforms --
    // platformSlugForByte's "nonHardware" bytes plus the too-generic "Other"),
    // since those have neither a byte-slug fallback nor a child to borrow from.
    // Give such a row its own logo here or it shows nothing when highlighted on
    // the TAB filter screen (updateFilterLogo). Remember to also add the
    // basename to splashExcludePlatforms if it isn't real hardware.
    // The image is OPTIONAL -- a group falls back to the borrowed child logo
    // rather than failing -- but it IS reported at startup like the other logo
    // gaps, so it doesn't stay un-drawn just because nothing breaks.
    std::string logo;
};

class ChipMachine
{
public:
    static const std::vector<FilterOption> filterOptions;

    // When set (via the --donotloadimages CLI flag), loadScreenshot() never
    // fetches a screenshot. Useful when the screenshot host (e.g. the Wayback
    // mirror that gb64/hvtc/sndh/unexotica depend on) is down or unreachable.
    static bool noImages;

    // Set when the app is launched with -d. Surfaces developer-only affordances
    // in the command list (e.g. F4 "layout screen", which hot-reloads
    // lua/screen.lua); those keys still work without it, they're just hidden.
    static bool debugMode;

    using Color = grappix::Color;

    void renderSong(const grappix::Rectangle& rec, int y, uint32_t index,
                    bool hilight);
    void renderCommand(grappix::Rectangle& rec, int y, uint32_t index,
                       bool hilight);

    ChipMachine(utils::path const& workDir, RemoteLoader& rl,
                MusicPlayerList& mpl, MusicDatabase& mdb, sol::state& lua);
    ~ChipMachine();

    void initLua();
    void layoutScreen();
    void updateTitleMarquee(uint32_t delta);
    void play(const SongInfo& si);
    void update();
    void render(uint32_t delta);

    // Order matters: toast() indexes its colour table with these, and the STICKY
    // kinds (which stay up until replaced instead of fading) must come last.
    enum ToastType
    {
        WHITE,
        ERROR,
        NORMAL,
        STICKY,
        // Sticky AND saturated red: for a prompt that is asking before doing
        // something destructive, so it must neither fade nor look routine.
        STICKY_ALERT
    };

    enum Shuffle
    {
        All = 0,
        Format = 1,
        Composer = 2,
        Collection = 4,
    };

    void toast(const std::string& txt, ToastType type = NORMAL);
    void removeToast();

    // Draws a spectrum-coloured progress bar (fraction in [0,1]) centred just
    // below the current toast. Shared by the startup DB-indexing screen and the
    // LOADING/BUFFERING download bar. A non-empty `label` is drawn centred in a
    // smaller, dimmer font just below the bar.
    void drawProgressBar(float frac, std::string const& label = "");

    void setScrolltext(const std::string& txt);
    void shuffleSongs(int what, int limit);

    void shuffleFavorites();
    // Wipes the favorites list and syncs the heart icon. Only ever called once a
    // pendingFavoritesClear confirmation has been answered with Y.
    void clearFavorites();
    // Set while CLEAR FAVORITES LIST waits for its confirmation. The next key
    // press resolves it -- Y wipes the list, anything else cancels -- and is
    // swallowed either way, so it cannot also fire its own binding.
    bool pendingFavoritesClear = false;
    // Name of the playlist a DEL press (on the Database-filter Playlists screen)
    // is waiting to delete; empty when nothing is armed. The next key press
    // resolves it -- Y deletes the file, anything else cancels -- and is
    // swallowed either way, mirroring pendingFavoritesClear above.
    std::string pendingPlaylistDelete;
    // The song a shuffle seeds itself from: the highlighted row while browsing
    // search results, otherwise whatever is playing.
    SongInfo shuffleSeed();
    // True when `composer` is one of the database's markers for "nobody knows who
    // wrote this" rather than an actual name.
    static bool isUnknownComposer(std::string const& composer);
    MusicPlayerList& musicPlayer() { return player; }
    void playSongs(std::vector<SongInfo> const& songs);
    void playNamed(const std::string& what) { namedToPlay = what; }

    // The songs of the current shuffle in play order, or empty when the user is
    // not shuffling. Retained because the play queue is destructive -- a song is
    // popped off it as it starts -- so this list is the only record of what has
    // already played, and hence the only way CTRL+LEFT can step back into it.
    std::vector<SongInfo> shuffleList;
    // Paths delivered by macOS "Open With" / double-click (Apple Event), drained
    // from the platform handler at the top of update() and played once the
    // database is ready. Held as a member so files that arrive during cold-start
    // indexing survive the early-return frames instead of being dropped.
    std::vector<std::string> filesToOpen;
    // Index into shuffleList of the song playing now. Derived from what is left
    // in the queue rather than counted, so songs the player advances to on its
    // own (when a tune ends) need no bookkeeping. -1 when not shuffling.
    int currentShuffleIndex() const;
    // (Re)start the shuffle at `index`, queueing everything after it.
    void playShuffleFrom(int index);
    // The generator behind every shuffle (defined alongside them in
    // ChipMachine_keys.cpp), shared so each one is drawn from the same stream.
    static std::mt19937& shuffleRng();

private:
    // Append the now-playing format info ("Platform - Name (EXT) ... <trackers>
    // - <description>") to a scroll line so the scroller cycles
    // metadata -> format -> back. When `text` is empty the format info is all
    // there is to show.
    std::string appendFormatInfo(std::string const& text, SongInfo const& info);

    enum Screen
    {
        NO_SCREEN = -1,
        MAIN_SCREEN = 0,
        SEARCH_SCREEN = 1,
        COMMAND_SCREEN = 2,
        ADVANCED_SCREEN = 3,
        FORMAT_SCREEN = 4,
        DATABASE_SCREEN = 5,
        PLUGIN_SCREEN = 6,
    };

    static const uint32_t SHIFT = 0x10000;
    static const uint32_t CTRL = 0x20000;
    static const uint32_t ALT = 0x40000;

    void setVariable(const std::string& name, int index,
                     const std::string& val);

    // Load every .otf from `folder` (relative to workDir) into the scroller's
    // rotating font pool, in alphabetical order. Called at startup from the
    // config (Settings.scroll[4]); rebuilds/regenerates each font's distance-map
    // cache on the way, so a font newly dropped into the folder is picked up on
    // the next launch.
    void loadScrollFonts(const std::string& folder);

    void showScreen(Screen screen);
    SongInfo getSelectedSong();

    void setupRules();
    void setupCommands();
    void updateKeys();
    void updateFavorite();
    void updateNextField();
    void computeFilterCounts();
    static std::string withCommas(int n); // 345000 -> "345,000"
    void updateScreenshotArea();
    // Park the smaller-font key hint just after the platform-filter title on the
    // same baseline. Depends on advancedTitle's font/scale, so it must re-run
    // whenever those change (layoutScreen / updateLists).
    void positionAdvancedHint()
    {
        advancedHint.scale = advancedTitle.scale * 0.65f;
        advancedHint.pos = { advancedTitle.pos.x + advancedTitle.getWidth() +
                                 15 * advancedHint.scale,
                             advancedTitle.pos.y };
    }
    // Same treatment for the Formats screen's key hint.
    void positionFormatHint()
    {
        formatHint.scale = formatTitle.scale * 0.65f;
        formatHint.pos = { formatTitle.pos.x + formatTitle.getWidth() +
                               15 * formatHint.scale,
                           formatTitle.pos.y };
    }
    // ...and the Databases screen's.
    void positionDatabaseHint()
    {
        databaseHint.scale = databaseTitle.scale * 0.65f;
        databaseHint.pos = { databaseTitle.pos.x + databaseTitle.getWidth() +
                                 15 * databaseHint.scale,
                             databaseTitle.pos.y };
    }
    // ...and the Plugins screen's.
    void positionPluginHint()
    {
        pluginHint.scale = pluginTitle.scale * 0.65f;
        pluginHint.pos = { pluginTitle.pos.x + pluginTitle.getWidth() +
                               15 * pluginHint.scale,
                           pluginTitle.pos.y };
    }
    void updateLists()
    {
        positionAdvancedHint();
        positionFormatHint();
        positionDatabaseHint();
        positionPluginHint();
        int y = resultFieldTemplate.pos.y + (15 * resultFieldTemplate.scale);
        int w = grappix::screen.width() - topLeft.x;
        int h = downRight.y - topLeft.y - y;

        songList.setArea(grappix::Rectangle(topLeft.x, y, w, h));
        advancedArea = grappix::Rectangle(topLeft.x, y, w, h);
        advancedList.setArea(advancedArea);
        // The Formats and Databases screens are single scrolling columns over the
        // same area.
        formatArea = grappix::Rectangle(topLeft.x, y, w, h);
        formatList.setArea(formatArea);
        databaseArea = grappix::Rectangle(topLeft.x, y, w, h);
        databaseList.setArea(databaseArea);
        pluginArea = grappix::Rectangle(topLeft.x, y, w, h);
        pluginList.setArea(pluginArea);

        // The help/command screen is one non-scrolling screen, so give it a
        // dedicated, taller area: title nudged up and the list running all the
        // way to the bottom margin. fitCommandList() sizes the rows to fill it
        // and renderCommand scales the font up to match. The title baseline can
        // only move up a little -- its big-font glyphs sit above pos.y.
        commandTitle.pos.y = topLeft.y * 0.90f;
        int cmdTop = (int)(topLeft.y * 1.30f);
        int cmdH = downRight.y - cmdTop;
        commandList.setArea(grappix::Rectangle(topLeft.x, cmdTop, w, cmdH));

        // The help list holds the same rows in this taller area as it would in
        // the shared list area, so its font can grow by the height ratio. Clamp
        // so long labels never collide with the shortcut column.
        commandFontFactor = h > 0 ? (float)cmdH / (float)h : 1.0f;
        if (commandFontFactor < 1.0f) commandFontFactor = 1.0f;
        if (commandFontFactor > 1.4f) commandFontFactor = 1.4f;
    }

    static inline const std::vector<std::string> key_names = {
        "UP",  "DOWN",   "LEFT",     "RIGHT",  "ENTER", "ESCAPE", "BACKSPACE",
        "TAB", "PAGEUP", "PAGEDOWN", "DELETE", "INSERT", "HOME",  "END",    "F1",
        "F2",  "F3",     "F4",       "F5",     "F6",    "F7",     "F8",
        "F9",  "F10",    "F11",      "F12"
    };

    std::string open_file_dialog();

    void addKey(uint32_t key, statemachine::Condition const& cond,
                std::string const& cmd);

    void addKey(std::vector<uint32_t> const& events,
                statemachine::Condition const& cond, std::string const& cmd)
    {
        for (auto& e : events)
            addKey(e, cond, cmd);
    }

    void addKey(std::vector<uint32_t> const& events, std::string const& cmd)
    {
        addKey(events, statemachine::ALWAYS_TRUE, cmd);
    }

    void addKey(uint32_t key, std::string const& cmd)
    {
        addKey(key, statemachine::ALWAYS_TRUE, cmd);
    }

    void clearCommand()
    {
        // Command names that begin a new logical group in the help menu: a
        // half-row gap is drawn before each (see renderCommand / matchingGap).
        // Add the first command of a group here to visually separate the sets.
        static const std::set<std::string> groupBreaks = {
            "cycle_platform/format/db/plugin_filters",
            "local_file_playback",
            "play_song",
            "Spectrum_Analyzer_Mode",
            "favor/unfavor_playing_song",
            "shuffle_all_songs_randomly",
            // Splits the next/prev steppers off from the shuffle set itself.
            "next_/_prev_shuffle_song",
        };
        // Only surface commands that have a key binding -- one with an empty
        // shortcut has no way to be triggered, so there is no point listing it.
        // matchingGap[i] is the cumulative divider gap (in rows) before row i.
        matchingCommands.clear();
        matchingGap.clear();
        matchingGroup.clear();
        float gap = 0.0f;
        int group = 0;
        for (auto& c : commands) {
            if (c.shortcut.empty()) continue;
            if (groupBreaks.count(c.name)) {
                gap += 0.3f;
                group++;
            }
            matchingCommands.push_back(&c);
            matchingGap.push_back(gap);
            matchingGroup.push_back(group);
        }
        fitCommandList();
    }

    // Size the help list so every row plus its group-divider gaps fit without
    // scrolling. VerticalList only counts rows, so hand it enough "slots" that
    // N rows + total gap span the area (compressing the row pitch only when the
    // extra gaps would otherwise overflow; normal height when there's room).
    void fitCommandList()
    {
        if (matchingCommands.empty()) {
            commandList.setVisible(numLines);
            return;
        }
        float gaps = matchingGap.empty() ? 0.0f : matchingGap.back();
        float need = (float)matchingCommands.size() + gaps;
        int slots = (int)need;
        if ((float)slots < need) slots++; // ceil
        if (slots < 1) slots = 1;
        // Fill the dedicated help area exactly (all rows + gaps span it, no
        // scrolling and no empty tail rows).
        commandList.setVisible(slots);
    }

    bool haveSelection()
    {
        return songList.selected() >= 0 &&
               (songList.selected() < songList.size());
    }

    void loadScreenshot(const std::string& shot);
    // Recomputes the centred, half-screen rectangle of the splash icon.
    void updateSplashArea();
    // Recomputes the centred, half-screen rectangle of the startup icon.
    void updateStartupIconArea();
    // Builds (once totalSongs is known) and ping-pong-scrolls the splash welcome
    // banner across the top each frame, mirroring the song-title marquee.
    void updateSplashWelcome(uint32_t delta);
    // Collects the (deduplicated) platform + extension pictures shown by the
    // idle splash animation. Must run after loadPlatformScreenshots() and
    // loadExtensionScreenshots() so their bitmaps are already loaded.
    void loadSplashScreenshots();
    // Loads the per-platform logos at startup and warns about missing ones.
    void loadPlatformScreenshots();
    // Loads per-extension screenshots and reports extensions that have neither
    // an extension image nor a covering platform logo.
    void loadExtensionScreenshots();
    // Appends the current song's platform logo (if any) and the ChipMachine
    // logo to the screenshots rotation, so it is never empty.
    void appendLogoScreenshots();
    // Appends only the per-extension or per-platform logo (no generic icon) for
    // the current song. Returns true if one was appended. Used both by
    // appendLogoScreenshots and to tack the logo onto real screenshots.
    bool appendPlatformOrExtLogo();

    // Picks the per-extension / per-platform logo bitmap for the given song
    // attributes (mirrors appendPlatformOrExtLogo's tiered choice), returning
    // nullptr when none is installed. When `label` is non-null it receives the
    // screenshot name that appendPlatformOrExtLogo would use. Shared by the
    // playback screenshot fallback and the search-list logo preview.
    const image::bitmap* pickPlatformOrExtLogo(const std::string& ext,
                                               const std::string& platformSlug,
                                               const std::string& format,
                                               std::string* label = nullptr);

    // Platform logo by name, case-insensitively (collections disagree on the
    // capitalisation of the same platform). nullptr when none is installed.
    const image::bitmap* findPlatformShot(const std::string& name);

    // Refreshes searchLogoIcon with the highlighted song's platform/ext logo,
    // positioned in the screenshot slot. Clears it when no song is selected or
    // no logo exists. Podcast SHOW rows preview their remote artwork instead
    // (via loadSearchArtwork).
    void updateSearchLogo();

    // Fits searchLogoIcon's current texture into the screenshot slot.
    void positionSearchLogo();

    // Loads a remote artwork URL (podcast show image) into searchLogoIcon,
    // asynchronously. searchLogoUrl tracks the desired image so a late download
    // for a row the user has already scrolled past is discarded.
    void loadSearchArtwork(const std::string& url);

    // Refreshes filterLogoIcon with the highlighted TAB filter entry's platform
    // logo, centred behind the list. Clears it off the filter screen or when the
    // entry has no hardware platform.
    void updateFilterLogo();

    // Same idea for the Formats screen: the highlighted extension's per-extension
    // screenshot (else its platform logo), centred behind the list.
    void updateFormatLogo();

    // ...and the Databases screen: the highlighted collection's modal-platform
    // logo (or, for a platformless collection like a podcast, its remote artwork
    // loaded via loadDatabaseArtwork), centred behind the list.
    void updateDatabaseLogo();
    // Load a remote collection-artwork URL into databaseLogoIcon asynchronously
    // (same worker->render-thread handoff as loadSearchArtwork). databaseLogoUrl
    // tracks the desired image so a late download for a scrolled-past row is
    // discarded.
    void loadDatabaseArtwork(const std::string& url);

    // ...and the Plugins screen: the highlighted plugin's modal-platform logo,
    // centred behind the list.
    void updatePluginLogo();
    // Set a bitmap on `icon` and size it to a centred box (half the screen,
    // aspect-preserving) -- the shared fit used by the filter-screen logos.
    void centerLogoIcon(Icon& icon, const image::bitmap& bm);

    utils::path workDir;
    // Resolved folder the scroller fonts were last loaded from; used to skip a
    // redundant reload when the constructor seed and the config both point at the
    // same folder (avoids rebuilding all fonts twice at startup).
    std::string scrollFontDir;

    RemoteLoader& remoteLoader;
    MusicPlayerList& player;
    MusicDatabase& musicDatabase;

    Screen lastScreen = MAIN_SCREEN;
    Screen currentScreen = MAIN_SCREEN;

    std::unique_ptr<TelnetInterface> telnet;

    utils::vec2i topLeft = { 80, 54 };
    utils::vec2i downRight = { 636, 520 };

    grappix::Font font;
    grappix::Font listFont;

    int spectrumHeight = 20;
    int spectrumWidth = 24;
    utils::vec2i spectrumPos;
    std::vector<uint8_t> eq;
    std::vector<uint8_t> eqLeft;
    std::vector<uint8_t> eqRight;
    std::vector<uint8_t> eqMono;
    SpectrumAnalyzer fft;
    SpectrumAnalyzer::StereoLevels spectrum;
    bool stereoSpectrum = true;
    // When true, stereoSpectrum follows automatic L/R content detection.
    // CTRL+M cycles Auto -> Mono -> Stereo.
    bool autoStereoDetect = true;
    double stereoDiffAccum = 0;
    double stereoSumAccum = 0;
    int stereoDetectFrames = 0;
    int spectrumGap = 4;
    int musicBarsWidth = 0;

    uint32_t bgcolor = 0;
    bool starsOn = true;

    sol::state& lua;

    demofx::StarField starEffect;
    demofx::Scroller scrollEffect;

    RenderSet overlay;
    TextField toastField;

    Icon favIcon;
    Icon netIcon;
    Icon volumeIcon;
    Icon screenShotIcon;
    // Platform/extension logo shown in the screenshot slot while browsing the
    // search list, previewing which platform the highlighted song resolves to
    // (the same logo playback would show). Updated on selection change, drawn
    // only on the search screen. Textureless (cleared) when there is no logo.
    Icon searchLogoIcon;
    // Desired image for searchLogoIcon when it is a remote podcast-show artwork
    // (empty for local platform/ext logos). Both the render thread and the web
    // download callback read/write it to decide whether a finished download is
    // still wanted (same std::string-across-threads pattern as currentScreenshot).
    std::string searchLogoUrl;
    // A downloaded podcast artwork handed from the web worker to the render
    // thread: the callback fills the bitmap + url and sets the flag (release);
    // update() consumes it (acquire) and uploads the texture (GL, render-thread).
    std::atomic<bool> pendingSearchLogo{ false };
    image::bitmap pendingSearchLogoBm;
    std::string pendingSearchLogoUrl;
    // Same async pipeline for the Databases screen: a podcast (or other
    // platformless) collection has no hardware logo, so its own artwork is
    // fetched remotely and uploaded on the render thread.
    std::string databaseLogoUrl;
    std::atomic<bool> pendingDatabaseLogo{ false };
    image::bitmap pendingDatabaseLogoBm;
    std::string pendingDatabaseLogoUrl;
    // Logo of the highlighted platform/category on the TAB filter screen, drawn
    // dimmed and centred behind the two-column list. Textureless when the
    // highlighted entry has no hardware platform (e.g. "[No Filter]", MP3).
    Icon filterLogoIcon;
    // Big "muted" overlay shown in the screen centre while paused (SPACE), so the
    // user sees what they pressed and which key un-mutes.
    Icon pausedIcon;
    // Shown centred in place of the volume bars when the volume is turned all
    // the way down to silence.
    Icon mutedIcon;

    RenderSet mainScreen;

    SongInfoField currentInfoField;
    SongInfoField nextInfoField;
    SongInfoField prevInfoField;
    SongInfoField outsideInfoField;

    TextField timeField;
    TextField lengthField;
    TextField songField;
    TextField nextField;
    TextField xinfoField;

    RenderSet searchScreen;

    LineEdit searchField;
    TextField filterField;
    TextField topStatus;
    // Smaller-font source-collection tag shown right after topStatus's format
    // line (e.g. "hvsc", "mirsoft") so the user sees which DB a result is from.
    TextField sourceStatus;
    grappix::VerticalList songList;

    TextField resultFieldTemplate;

    RenderSet commandScreen;
    LineEdit commandField;
    grappix::VerticalList commandList;

    RenderSet advancedScreen;
    grappix::VerticalList advancedList;
    grappix::Rectangle advancedArea; // area of the TAB list (for 2-column layout)
    TextField advancedTitle;
    // Key hint drawn right after advancedTitle on the same line, in a smaller,
    // dimmer font (like sourceStatus next to topStatus). Positioned at runtime
    // from advancedTitle's width -- see positionAdvancedHint().
    TextField advancedHint;

    // The Formats screen: a single scrolling column, one row per file extension
    // ("EXT   count   name"), highest count first. Selecting one restricts search
    // to that extension (MusicDatabase::setExtensionFilter).
    RenderSet formatScreen;
    grappix::VerticalList formatList;
    grappix::Rectangle formatArea;
    TextField formatTitle;
    TextField formatHint;
    Icon formatLogoIcon; // per-extension logo, centred behind the list

    // Type-to-narrow state for the Formats screen. formatFilterText is the
    // (lowercased) query the user is typing; a match keeps any group whose
    // extension OR description name contains it. formatVisibleGroups maps each
    // shown list row (row 0 is [no filter]; group rows follow) to an index into
    // MusicDatabase::extensionGroups() -- everything that resolves a row back to
    // a group (render, logo, ENTER-to-apply) goes through this indirection.
    std::string formatFilterText;
    std::vector<int> formatVisibleGroups;
    // Rebuild formatVisibleGroups from formatFilterText, resize/clamp the list
    // and refresh the title. Call after any edit to the query or on entry.
    void rebuildFormatVisible();

    // The Databases screen: same single-column layout, one row per source
    // collection ("id   count   name"), highest count first. Selecting one
    // restricts search to that database (MusicDatabase::setDatabaseFilter).
    RenderSet databaseScreen;
    grappix::VerticalList databaseList;
    grappix::Rectangle databaseArea;
    TextField databaseTitle;
    TextField databaseHint;
    Icon databaseLogoIcon; // collection's modal-platform logo, behind the list

    // The Plugins screen: same single-column layout, one row per registered
    // ChipPlugin ("count   name"), highest count first. Selecting one restricts
    // search to the songs that plugin claims by extension (MusicDatabase's
    // setPluginFilter / pluginGroups; see buildPluginGroups for why this is
    // extension-based rather than canHandle()-based).
    RenderSet pluginScreen;
    grappix::VerticalList pluginList;
    grappix::Rectangle pluginArea;
    TextField pluginTitle;
    TextField pluginHint;
    Icon pluginLogoIcon; // plugin's modal-platform logo, behind the list

    // Type-to-narrow state for the Plugins screen, same shape as the Formats
    // screen's (see formatFilterText/formatVisibleGroups above): pluginFilterText
    // is the (lowercased) query; a match keeps any plugin whose name contains it.
    // pluginVisibleGroups maps each shown list row (row 0 is [no filter]; plugin
    // rows follow) to an index into MusicDatabase::pluginGroups().
    std::string pluginFilterText;
    std::vector<int> pluginVisibleGroups;
    // Rebuild pluginVisibleGroups from pluginFilterText, resize/clamp the list
    // and refresh the title. Call after any edit to the query or on entry.
    void rebuildPluginVisible();
    // Header on the command/help screen (e.g. "ChipMachineAS 1.9 HELP MENU"),
    // drawn above the first entry; hidden while a command filter is being typed.
    TextField commandTitle;
    TextField mainFilterField;
    std::string selectedFilterName;
    // Per-filterOptions tune counts, shown as "[N tunes]" on the TAB screen.
    // Populated once the database finishes indexing.
    std::vector<int> filterCounts;
    // Raw per-format-byte tune counts (index = format byte), plus the grand
    // total. Kept so a drilled-in submenu can count its children on the fly.
    std::vector<int> filterByteCounts;
    int filterTotalCount = 0;
    // TAB filter drill-down state. activeFilterOptions points at the list the
    // advancedList currently renders: the static top-level filterOptions, or
    // drillOptions (one group's children). drillReturnIndex is the top-level row
    // to re-select when popping back out with ESC.
    const std::vector<FilterOption>* activeFilterOptions = nullptr;
    std::vector<FilterOption> drillOptions;
    int drillReturnIndex = 0;

    // The filter list currently shown on the TAB screen (top-level or a drill).
    const std::vector<FilterOption>& currentFilterOptions() const
    {
        return activeFilterOptions ? *activeFilterOptions : filterOptions;
    }
    // Tune count for a filter option (sum of its formats, or of its children).
    int filterOptionCount(FilterOption const& opt) const;
    // Switch the TAB list between the top level (opts == nullptr) and a drilled
    // submenu (opts == &drillOptions), resizing the list and selecting `sel`.
    void setFilterLevel(const std::vector<FilterOption>* opts, int sel);
    // Number of distinct podcast shows, used to prefix the TAB "Podcasts" label
    // ("9 Podcasts  [N episodes]"). Populated alongside filterCounts.
    int podcastShowCount = 0;
    // Number of distinct sub-platforms among OTHER songs, used to prefix the TAB
    // "Other Platforms" label ("N Other Platforms"). Populated with filterCounts.
    int otherPlatformCount = 0;
    // Number of distinct sub-platforms among ARCADE songs, used to prefix the TAB
    // "Arcade" label ("N Arcade"). Populated with filterCounts.
    int arcadePlatformCount = 0;
    // Tune count of the currently selected platform filter (0 = no filter);
    // used for the "type to search N songs" prompt hint on large filters.
    int activeFilterCount = 0;

    std::string currentNextPath;
    SongInfo currentInfo;
    SongInfo dbInfo;
    int currentTune = 0;

    tween::Tween currentTween;
    bool isFavorite = false;

    grappix::Rectangle favPos;
    grappix::Rectangle volPos;

    int numLines = 20;

    tween::Tween markTween;

    // Per-frame marquee that bounce-scrolls a too-long song title/composer so all
    // of it becomes visible. Driven live in updateTitleMarquee() rather than by a
    // baked tween, so the scroll distance always tracks the CURRENT window size
    // (resize/maximize) instead of the value captured when the song started. Only
    // active after the intro slide-in completes; a field that fits doesn't scroll.
    // Index 0 = title (currentInfoField[0]), 1 = composer (currentInfoField[1]).
    bool titleMarqueeActive = false;
    float titleMarqueePhase[2] = { 0.0f, 0.0f };

    Color timeColor;
    Color spectrumColor = 0xffffffff;
    Color spectrumColorMain = 0xff00aaee;
    Color spectrumColorSearch = 0xff111155;
    Color markColor = 0xff00ff00;
    Color hilightColor = 0xffffffff;

    std::shared_ptr<IncrementalQuery> iquery;

    bool haveSearchChars = false;

    statemachine::StateMachine smac;

    std::string currentPlaylistName = "Favorites";

    bool commandMode = false;

    std::shared_ptr<Dialog> currentDialog;

    std::pair<float, float> screenSize;
    int resizeDelay = 0;
    int showVolume = 0;

    bool hasMoved = false;

    bool indexingDatabase = false;
    // Big centred app icon shown from app launch until indexing finishes and
    // the scroller takes over (see setup()/updateStartupIconArea()/render()).
    Icon startupIcon;
    // Fraction of the screen the startup icon fills (per axis, aspect kept).
    float startupIconSizeFraction = 1.0f;

    MusicBars musicBars;
    MusicPlayerList::State playerState;
    // Fetch/buffer toast state. "LOADING..." while a whole-file download is in
    // flight; "BUFFERING..." while a progressive stream prebuffers. Resolved once
    // per song: recomputed only when the playing path changes, then cleared the
    // moment real audio starts (stream) or the load ends (download).
    bool loadingToastShown = false;
    bool loadingToastResolved = false;
    bool loadingToastIsLocal = false;
    bool loadingToastStreamed = false;
    std::string loadingToastPath;
    // Timestamp (ms) when the LOADING/BUFFERING toast was shown. The download
    // progress bar only appears once the wait exceeds 5s, so a quick fetch never
    // flashes a bar.
    uint32_t loadingToastStartMs = 0;
    std::string scrollText;

    struct Command
    {
        Command(const std::string& name, std::function<void()> const& fn)
            : name(name), fn(fn)
        {}
        std::string name;
        std::function<void()> fn;
        std::string shortcut;
        bool operator==(const std::string& n) { return n == name; }
        bool operator==(const Command& c) { return c.name == name; }
    };

    std::vector<Command> commands;
    std::vector<Command*> matchingCommands;
    // Parallel to matchingCommands: cumulative group-divider gap (in rows) drawn
    // above each help-menu row. Rebuilt with matchingCommands in clearCommand().
    std::vector<float> matchingGap;
    // Parallel to matchingCommands: which logical group (0,1,2,...) each row is
    // in -- incremented at every divider. renderCommand alternates the row colour
    // by group parity so adjacent sets are visually distinct.
    std::vector<int> matchingGroup;
    // How much bigger the help font is than the result-list font: the ratio of
    // the (taller) dedicated help area to the shared list area, since both hold
    // the same command rows. Clamped; set in updateLists(), used by renderCommand.
    float commandFontFactor = 1.0f;
    std::mutex multiLoadLock;
    int lastKey = 0;
    bool searchUpdated = false;
    std::string filter;
    uint32_t favColor = 0x884444;

    std::string namedToPlay;
    // Drives the animated transitions between screenshots (fade, zoom, mosaic,
    // starfield). Configured in the constructor with callbacks into `screenshots`
    // and updateScreenshotArea().
    ScreenshotTransitions transitions;
    struct NamedBitmap
    {
        NamedBitmap() {}
        NamedBitmap(const std::string& name, const image::bitmap& bm)
            : name(name), bm(bm)
        {}
        std::string name;
        image::bitmap bm;
        bool operator==(const char* n) const
        {
            return strcmp(name.c_str(), n) == 0;
        }
        bool operator<(const NamedBitmap& other) const
        {
            return name < other.name;
        }
    };
    std::vector<NamedBitmap> screenshots;
    std::string currentScreenshot;
    // Screenshot download callbacks can fire on the web worker thread (async
    // download) OR synchronously on the render thread (cache hit, via
    // getFile->call_handler). Mutating `screenshots` from the worker thread while
    // the render thread reads it (the transition source callbacks) / clears it
    // (loadScreenshot) is an unsynchronized data race that corrupts the heap
    // (observed as libmalloc "free block" / autorelease-pool page stomps that
    // later crash in unrelated code such as the font glyph loader). So the
    // callback only DECODES into pendingShotBms under pendingShotLock and raises
    // pendingShotReady; update() (always the render thread) installs the bitmaps
    // into `screenshots`, appends the logo fallback and restarts the transitions.
    // pendingShotKey is the request URL the staged bitmaps belong to, so a result
    // that arrives after the song changed can be dropped.
    std::mutex pendingShotLock;
    std::vector<NamedBitmap> pendingShotBms;
    std::string pendingShotKey;
    std::atomic<bool> pendingShotReady{ false };
    // Splash animation shown while the app is idle on the main screen (nothing
    // playing, "only the scroller visible"). Rotates the platform + extension
    // pictures, centred and large, reusing the same transition effects as the
    // per-song screenshot area but driven by its own icon/bitmap set so the two
    // never interfere. See updateSplashArea()/loadSplashScreenshots().
    ScreenshotTransitions splashTransitions;
    // Default-constructed (no texture): the first bitmap is uploaded by
    // splashTransitions.restart(), which runs on the render thread.
    Icon splashIcon;
    // Deduplicated platform + extension pictures rotated by the splash. Some
    // extensions share the same picture, so identical bitmaps are collapsed to
    // a single entry (see loadSplashScreenshots()).
    std::vector<NamedBitmap> splashShots;
    // Fraction of the screen the splash picture fills (per axis, aspect kept).
    float splashSizeFraction = 0.5f;
    // Whether the splash was active last frame; render() reads this (set by
    // update()) and it drives the restart-on-enter transition.
    bool splashActive = false;
    // Whether the platform-logo transitions are running (idle main splash OR the
    // help/command screen). Broader than splashActive; drives restart/next.
    bool splashLogoActive = false;
    // Welcome banner ping-pong-scrolled across the top of the splash, in the
    // song-title font/size/colour. Rendered behind the splash pictures.
    TextField splashWelcomeField;
    float splashWelcomePhase = 0.0f;   // ping-pong scroll phase (seconds)
    int splashWelcomeSongs = -1;       // totalSongs baked into the current text
    // Total number of indexed songs, computed at startup in computeFilterCounts
    // (the same total the "[Show All]" TAB filter reports).
    int totalSongs = 0;
    // The startup indexing progress bar runs in two phases:
    //  - first progressDbPhase (75%): one tick per collection DB created, out of
    //    progressMaxDatabases.
    //  - remaining 25%: row indexing, scaled by progressMaxSongs (the final DB
    //    size can't be known up-front). Bump either as the library grows.
    int progressMaxSongs = 750000;
    int progressMaxDatabases = 40;
    float progressDbPhase = 0.85f;
    // Vertical nudge (in gscale pixels) of the % label inside the progress bar.
    // Positive = down. Tweak this to fine-tune the vertical centring.
    float progressPctYNudge = 4.0f;
    // Lazily-loaded ChipMachine logo (data/misc/icon.png), used as the final
    // fallback so the screenshot area is never blank.
    image::bitmap defaultShot;
    // Per-platform logos loaded once at startup from
    // data/misc/platformscreenshots/<platform>.png|jpg. Missing ones are simply
    // absent (warned about at startup, never fatal).
    std::map<std::string, image::bitmap> platformShots;
    // Per-extension screenshots loaded from data/misc/extensionscreenshots/
    // <ext>.png|jpg (keyed by lowercased extension, e.g. "mod","sid"). Tried
    // before the platform logo when a song has no real screenshot.
    std::map<std::string, image::bitmap> extensionShots;
    // File extension (lowercased) of the playing song, for the ext screenshot.
    std::string currentSongExt;
    // Raw (lowercased) format string of the playing song, captured before
    // describeFormat() rewrites it; used to pick a per-system Console logo.
    std::string currentSongFormat;
    // Identity of the fallback (logo-only) set currently displayed ("ext|plat"),
    // so consecutive songs that need a DIFFERENT fallback image rebuild instead
    // of being suppressed by the same-platform anti-flicker guard.
    std::string shownLogoKey;
    // Platform slug of the song currently shown; lets loadScreenshot avoid
    // rebuilding (and re-fading) the logo-only set between same-platform songs.
    std::string currentPlatformSlug;
    // Platform slug of the playing song, classified from its raw format BEFORE
    // describeFormat() rewrites currentInfo.format into a display string.
    std::string currentSongPlatform;

    // The defensive thread barrier for application destruction
    std::atomic<bool> isShuttingDown{false};
};
} // namespace chipmachine
