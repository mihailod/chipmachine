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
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
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
        target->draw(*texture, rec.x, rec.y, rec.w, rec.h, nullptr, color);
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
};

class ChipMachine
{
public:
    static const std::vector<FilterOption> filterOptions;

    // When set (via the --donotloadimages CLI flag), loadScreenshot() never
    // fetches a screenshot. Useful when the screenshot host (e.g. the Wayback
    // mirror that gb64/hvtc/sndh/unexotica depend on) is down or unreachable.
    static bool noImages;

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

    enum ToastType
    {
        WHITE,
        ERROR,
        NORMAL,
        STICKY
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

    void setScrolltext(const std::string& txt);
    void shuffleSongs(int what, int limit);

    void shuffleFavorites();
    MusicPlayerList& musicPlayer() { return player; }
    void playSongs(std::vector<SongInfo> const& songs);
    void playNamed(const std::string& what) { namedToPlay = what; }

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
    void updateLists()
    {
        int y = resultFieldTemplate.pos.y + (15 * resultFieldTemplate.scale);

        songList.setArea(grappix::Rectangle(topLeft.x, y,
                                            grappix::screen.width() - topLeft.x,
                                            downRight.y - topLeft.y - y));
        commandList.setArea(grappix::Rectangle(
            topLeft.x, y, grappix::screen.width() - topLeft.x,
            downRight.y - topLeft.y - y));
        advancedArea = grappix::Rectangle(topLeft.x, y,
                                          grappix::screen.width() - topLeft.x,
                                          downRight.y - topLeft.y - y);
        advancedList.setArea(advancedArea);
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
        matchingCommands.resize(commands.size());
        int i = 0;
        for (auto& c : commands)
            matchingCommands[i++] = &c;
    }

    bool haveSelection()
    {
        return songList.selected() >= 0 &&
               (songList.selected() < songList.size());
    }

    void loadScreenshot(const std::string& shot);
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
    // Big "muted" overlay shown in the screen centre while paused (F5), so the
    // user sees what they pressed and which key un-mutes.
    Icon pausedIcon;

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
    grappix::VerticalList songList;

    TextField resultFieldTemplate;

    RenderSet commandScreen;
    LineEdit commandField;
    grappix::VerticalList commandList;

    RenderSet advancedScreen;
    grappix::VerticalList advancedList;
    grappix::Rectangle advancedArea; // area of the F9 list (for 2-column layout)
    TextField advancedTitle;
    TextField mainFilterField;
    std::string selectedFilterName;
    // Per-filterOptions tune counts, shown as "[N tunes]" on the F9 screen.
    // Populated once the database finishes indexing.
    std::vector<int> filterCounts;
    // Number of distinct podcast shows, used to prefix the F9 "Podcasts" label
    // ("9 Podcasts  [N episodes]"). Populated alongside filterCounts.
    int podcastShowCount = 0;
    // Number of distinct sub-platforms among OTHER songs, used to prefix the F9
    // "Other Platforms" label ("N Other Platforms"). Populated with filterCounts.
    int otherPlatformCount = 0;
    // Number of distinct sub-platforms among ARCADE songs, used to prefix the F9
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

    MusicBars musicBars;
    MusicPlayerList::State playerState;
    // "LOADING..." toast shown while a non-cached song is being fetched.
    bool loadingToastShown = false;
    bool loadingToastChecked = false;
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
    // Screenshot download callbacks can fire on the web worker thread (async
    // download) OR synchronously on the render thread (cache hit, via
    // getFile->call_handler). transitions.restart()/next() touch OpenGL, which
    // is only valid on the render thread. So the web-callback path just raises
    // this flag; update() (always the render thread) consumes it and does the
    // GL-touching restart. Must not block here -- run_safely would deadlock when
    // the callback already runs on the render thread.
    std::atomic<bool> pendingShotRestart{false};
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
