#ifndef MUSIC_DATABASE_H
#define MUSIC_DATABASE_H

#include "SearchIndex.h"
#include "SongInfo.h"

#include <coreutils/environment.h>
#include <coreutils/file.h>
#include <coreutils/utils.h>
#include <sqlite3/database.h>

#include <coreutils/thread.h>
#include <future>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class RemoteLoader;

namespace chipmachine {

class not_found_exception : public std::exception
{
public:
    [[nodiscard]] char const* what() const noexcept override { return "Not found exception"; }
};

// console -- sid -- tracker -- amiga
enum Formats
{

    NOT_SET,

    UNKNOWN_FORMAT,
    NO_FORMAT,
    PLAYLIST,

    CONSOLE,

    HES,

    NINTENDO,

    GAMEBOY,
    NES,
    SNES,
    NINTENDO64,
    GBA,
    NDS,

    SEGA,

    SEGAMS,
    MEGADRIVE,
    DREAMCAST,
    SATURN,     // Sega Saturn (.ssf)
    WONDERSWAN, // Bandai WonderSwan handheld

    SONY,

    PLAYSTATION,
    PLAYSTATION2,

    COMPUTER,
    SID, // Commodore 64 SID tunes (formerly C64)
    STR, // Stereo Sidplayer (.str), C64 stereo SID
    PRG, // Commodore TED (16/116/+4), .prg tunes

    SPECTRUM,    // generic / unclassified ZX Spectrum
    ZXBEEPER,    // ZX Spectrum 16/48 1-bit beeper (Beepola, Picatune2, ...)
    ZXAY,        // ZX Spectrum 128 AY/YM (Pro Tracker, Vortex, AY Emul, ...)
    MSX,         // MSX (Z80 + AY/SCC/OPLL/FM): MGSDRV, KSS, MoonBlaster, ...
    AMSTRAD,     // Amstrad CPC (AY): Starkos, ArkosTracker
    ACORN,       // Acorn Archimedes: Digital Symphony, Coconizer, ...
    SAMCOUPE,    // MGT Sam Coupe (SAA1099): COP / SAA tunes (zxart)

    APPLE,

    ATARI, // Atari ST/STE (YM2149): sndh, YM, SC68, ST UADE formats
    POKEY, // Atari XL/XE 8-bit (POKEY): .sap

    MP3,

    M3U,
    PLS,

    OGG,

    RADIO, // Live streaming radio stations (the "radio" collection)

    YOUTUBE,

    PODCAST, // Podcast episodes (RSS feeds / archive.org rips; "podcast" type)

    PC,
    JPFM, // Japanese FM computers: NEC PC-98, Sharp X68000, Fujitsu FM Towns

    ADPLUG,
    TRACKER = 0x30,
    SCREAMTRACKER,  // IBM PC: Scream Tracker (S3M/STM)
    IMPULSETRACKER, // IBM PC: Impulse Tracker (IT)
    FASTTRACKER,    // IBM PC: FastTracker II (XM)
    PCTRACKER,      // other IBM PC/DOS trackers (MTM, 669, MDL, GDM, ...)

    AMIGA,
    PROTRACKER,
    SOUNDTRACKER,

    UADE,

    PRODUCT = 0x40

};

struct Product
{
    std::string title;
    std::string creator;
    std::string type;
    std::string screenshots;
    std::vector<std::string> songs;
};

class MusicDatabase : public SearchProvider
{
public:
    using Variables = std::map<std::string, std::string>;

    explicit MusicDatabase(RemoteLoader& rl)
        : remoteLoader(rl),
          db((Environment::getCacheDir() / "music.db").string()),
          reindexNeeded(false)
    {
        createTables();
    }

    bool initFromLua(utils::path const& workDir);
    void initFromLuaAsync(utils::path const& workDir);

    void forceRebuild() { rebuildForced = true; }

    int search(std::string const& query, std::vector<int>& result,
               unsigned int searchLimit) override;
    // Lookup internal string for index
    std::string getString(int index) const override
    {
        // std::lock_guard lock{dbMutex};
        return utils::format("%s %s", getTitle(index), getComposer(index));
    }

    std::string getFullString(int index) const override
    {
        // std::lock_guard lock{dbMutex};
        int f;
        if (index >= PODCAST_SHOW_INDEX)
            f = PODCAST;
        else if (index >= PLAYLIST_INDEX)
            f = PLAYLIST;
        else
            f = formats[index];
        return utils::format("%s\t%s\t%d\t%d", getTitle(index),
                             getComposer(index), index, f);
    }
    // Get full data, may require SQL query
    SongInfo getSongInfo(int index) const;

    // True while a platform filter (F9) is active.
    bool hasFormatFilter() const { return formatFilterActive; }
    // Position of a song's sub-format among the distinct formats present in the
    // active filter, in [0,1) -- used to spread hues evenly so few-format
    // platforms separate as much as many-format ones. Returns -1 when there is
    // nothing to distinguish (no filter, <2 formats, or not a filtered song).
    float formatSpread(int index) const
    {
        if (filterHueCount < 2 || index < 0 ||
            index >= (int)formatHue.size())
            return -1.0f;
        auto it = filterHueRank.find(formatHue[index]);
        if (it == filterHueRank.end()) return -1.0f;
        return (it->second + 0.5f) / (float)filterHueCount;
    }

    // Unified one-line description of a song's format for the now-playing
    // screen: "Platform - Format name (EXT)", e.g. "Amiga - Soundtracker (MOD)".
    static std::string describeFormat(SongInfo const& s);

    // Trackers + prose description for a file extension, read from
    // data/misc/formats_descriptions.txt (lazily loaded/cached). Returns
    // "<trackers> - <description>", or "" when the extension isn't listed.
    // Used by the scroller fallback when a tune carries no embedded text.
    std::string describeExtension(std::string const& ext);

    // Map a modland/format string (+ path) to its format byte (platform/type).
    static uint8_t classifyFormat(std::string const& fmt,
                                  std::string const& path);

    // Human-readable platform name (the F9-filter label, e.g. "Amiga",
    // "Commodore 64", "MSX") for a bare file extension. Returns "" when the
    // extension maps to no hardware platform. Used by the cmtest
    // extension_to_platform_map report to verify every playable extension is
    // classifiable.
    static std::string platformForExtension(std::string const& ext);

    // Filesystem-safe platform name for a song, used to pick a per-platform
    // logo at data/misc/platformscreenshots/<name>.png|jpg. Returns "" for
    // songs without a real hardware platform (MP3, OGG, Radio, YouTube,
    // Podcast, Playlist, unknown). '/' in display names is replaced with '-'.
    static std::string platformScreenshotName(SongInfo const& s);

    // The full set of distinct platform names (slugs) that can carry a
    // per-platform logo. Used at startup to warn about missing images.
    static std::vector<std::string> platformScreenshotNames();

    // Distinct file extension (lowercased) -> the set of platform slugs its
    // songs classify to, read from the song DB. Used at startup to report which
    // extensions need a dedicated screenshot because no platform logo covers
    // them. Returns empty when the DB doesn't exist yet.
    std::map<std::string, std::set<std::string>> extensionPlatforms();

    // Number of indexed songs (excluding products) per format byte (0..255).
    // Used to show per-platform tune counts on the F9 filter screen.
    std::vector<int> getFormatByteCounts() const;

    // Number of distinct podcast shows (collections containing PODCAST-format
    // episodes). Used to label the F9 Podcasts filter ("9 Podcasts [...]").
    int getPodcastShowCount() const;

    std::string getTitle(int index) const
    {
        std::lock_guard lock{ dbMutex };
        if (index >= PODCAST_SHOW_INDEX) {
            int rowid = index - PODCAST_SHOW_INDEX;
            for (auto const& s : podcastShowList)
                if (s.first == rowid) return s.second;
            return "";
        }
        if (index >= PLAYLIST_INDEX)
            return playLists[index - PLAYLIST_INDEX].name;
        return titleIndex.getString(index);
    }

    std::string getComposer(int index) const
    {
        std::lock_guard lock{ dbMutex };
        if (index >= PLAYLIST_INDEX) return "";
        return composerIndex.getString(titleToComposer[index]);
    }

    std::shared_ptr<IncrementalQuery> createQuery()
    {
        std::lock_guard lock{ dbMutex };
        return std::make_shared<IncrementalQuery>(this);
    }

    int getSongs(std::vector<SongInfo>& target, SongInfo const& match,
                 int limit, bool random);

    bool busy()
    {
        std::lock_guard lock{ chkMutex };
        if (initFuture.valid()) {
            if (initFuture.wait_for(std::chrono::milliseconds(1)) ==
                std::future_status::ready) {
                initFuture.get();
                return false;
            }
            return true;
        }

        if (dbMutex.try_lock()) {
            dbMutex.unlock();
            return false;
        }
        return true;
    }

    SongInfo& lookup(SongInfo& song);

    std::vector<SongInfo> getProductSongs(uint32_t id);

private:
    std::string getProductScreenshots(uint32_t id);
    std::string getScreenshotURL(std::string const& collection);

public:
    std::string getSongScreenshots(SongInfo& s);

    struct Playlist
    {
        std::string name;
        std::string fileName;
        std::vector<SongInfo> songs;

        explicit Playlist(const utils::path& f) : fileName(f.string())
        {
            if (utils::exists(f)) {
                for (auto const& l : apone::File{ f }.lines()) {
                    if (!l.empty()) songs.emplace_back(l);
                }
            }
            name = f.filename().string();
        }

        void save()
        {
            apone::File f{ fileName, apone::File::Write };
            LOGD("Writing to %s", fileName);
            for (auto const& s : songs) {
                if (s.starttune >= 0)
                    f.writeln(utils::format("%s;%d", s.path, s.starttune));
                else
                    f.writeln(s.path);
            }
        }
    };

    void addToPlaylist(std::string const& plist, SongInfo const& song);
    void removeFromPlaylist(std::string const& plist, SongInfo const& toRemove);
    std::vector<SongInfo>& getPlaylist(std::string const& plist);

    void setFilter(std::string const& filter, int type = 0);
    void setFormatFilter(std::vector<uint8_t> const& allowedFormats);

private:
    void initDatabase(utils::path const& workDir, Variables& vars);
    void generateIndex();

    // --- Podcast live-feed refresh (Q4) ---------------------------------
    // A podcast whose episode list can be augmented from a live RSS feed.
    struct PodcastFeed
    {
        std::string id;         // collection id (also the cache file stem)
        std::string songList;   // shipped back-catalogue file (data/<id>.xml)
        std::string remoteList; // live feed URL (https)
    };
    std::vector<PodcastFeed> podcastFeeds;

    // Resolve a podcast's index source: the writable augmented copy in the
    // cache (back catalogue + merged live episodes) if present, else the
    // shipped file. Seeds the cache copy from the shipped file on first use.
    std::string podcastSource(utils::path const& workDir,
                              std::string const& id,
                              std::string const& songList) const;

    // Seed cache copies, detect whether a previous background refresh left new
    // episodes (returns true -> caller forces a reindex), and kick off a
    // throttled background fetch+merge for any feed not checked in ~24h.
    // Never blocks launch on the network.
    bool preparePodcasts(utils::path const& workDir);

    // Background worker: fetch remoteList, merge any new <item>s into the cache
    // copy (union by enclosure URL), and drop a .dirty marker when it changed.
    static void refreshPodcastFeed(utils::path cacheDir, std::string id,
                                   std::string remoteList);

    // Append episodes present in a podcast's cache XML but not yet in the song
    // table (without dropping/re-parsing other collections). Called instead of
    // a full reindex when a background refresh added episodes; the caller then
    // rebuilds just the search index from the table.
    void syncPodcastSongs();

    struct Collection
    {
        int id;
        std::string name;
        std::string url;
        utils::path local_dir;

        explicit Collection(int id = -1, std::string const& name = "",
                            std::string const& url = "",
                            utils::path const& local_dir = utils::path(""))
            : id(id), name(name), url(url), local_dir(local_dir)
        {}
    };

    template <typename T> using Callback = std::function<void(T const&)>;

    typedef bool (MusicDatabase::*ParseSongFun)(Variables&, std::string const&,
                                                Callback<SongInfo> const&);
    typedef bool (MusicDatabase::*ParseProdFun)(Variables&, std::string const&,
                                                Callback<Product> const&);

    bool parseCsdb(Variables& vars, std::string const& listFile,
                   Callback<Product> const& callback);
    bool parseBitworld(Variables& vars, std::string const& listFile,
                       Callback<Product> const& callback);
    bool parseGamebase(Variables& vars, std::string const& listFile,
                       Callback<Product> const& callback);
    bool parsePouet(Variables& vars, std::string const& listFile,
                    Callback<SongInfo> const& callback);
    bool parseRss(Variables& vars, std::string const& listFile,
                  Callback<SongInfo> const& callback);
    bool parseModland(Variables& vars, std::string const& listFile,
                      Callback<SongInfo> const& callback);
    bool parseAmp(Variables& vars, std::string const& listFile,
                  Callback<SongInfo> const& callback);
    bool parseStandard(Variables& vars, std::string const& listFile,
                       Callback<SongInfo> const& callback);

    void writeIndex(apone::File&& f);
    void readIndex(apone::File&& f);

    void createTables();

    static constexpr int PLAYLIST_INDEX = 0x10000000;

public:
    // Synthetic result indices for podcast SHOW rows (one per podcast
    // collection) shown when the Podcasts filter is active with an empty query.
    // index = PODCAST_SHOW_INDEX + collection ROWID. Kept above PLAYLIST_INDEX
    // and checked first wherever indices are dispatched.
    static constexpr int PODCAST_SHOW_INDEX = 0x18000000;

    // Podcast browse: list of (collection ROWID, name) for each podcast show,
    // sorted by name; populated when the Podcasts format filter activates.
    std::vector<std::pair<int, std::string>> const& podcastShows() const
    {
        return podcastShowList;
    }
    // Drill into one show (its ROWID) so an empty query lists that show's
    // episodes; pass -1 to go back to the show list.
    void setPodcastShow(int rowid) { podcastShowFilter = rowid; }
    int podcastShow() const { return podcastShowFilter; }
    bool podcastFilterActive_() const { return podcastFilterActive; }

private:
    RemoteLoader& remoteLoader;
    utils::path workDir;

    // Per-collection song-path -> screenshot URL maps, lazily loaded from
    // data/<id>_screenshots.txt (full Wayback URLs). Used by collections whose
    // art is matched offline (hvtc, sndh). Guarded by screenshotMutex.
    std::map<std::string, std::map<std::string, std::string>> fileShots;
    std::map<std::string, std::string> const& getFileShots(
        std::string const& collection);

    // ext -> "<trackers> - <description>", lazily loaded from
    // data/misc/formats_descriptions.txt by describeExtension().
    std::map<std::string, std::string> formatDescriptions;
    bool formatDescriptionsLoaded = false;

    SearchIndex composerIndex;
    SearchIndex titleIndex;

    std::vector<uint32_t> titleToComposer;
    std::vector<uint32_t> composerToTitle;
    std::vector<uint32_t> composerTitleStart;
    std::vector<uint16_t> formats;
    // Platform format-byte per product (indexed by product ordinal, i.e.
    // titleIndex position - productStartIndex). Products only carry the PRODUCT
    // byte in `formats`, so this side-channel lets the platform filter (F9)
    // include/exclude collections by platform. 0 = unknown (filtered out).
    std::vector<uint8_t> productPlatform;
    // Real SQL product.ROWID per indexed product (indexed by product ordinal).
    // The product index query skips single-song products (HAVING count>1), so
    // the ordinal is NOT the ROWID; getSongInfo() must map ordinal -> ROWID via
    // this table to fetch the correct product.
    std::vector<int> productRowid;
    // Per-entry sub-format key (16-bit hash of the format string), aligned with
    // `formats`. Distinct formats within a platform are ranked from these and
    // spread evenly across the hue range (see setFormatFilter / formatSpread).
    // 16-bit so the few formats in a platform don't collide to the same color.
    // 0 = neutral (products).
    std::vector<uint16_t> formatHue;

    // When a platform filter (F9) is active, the set of titleIndex indices that
    // pass it, precomputed in setFormatFilter(). Lets short queries (< 3 chars)
    // scan the (typically small) filtered set directly instead of the sparse
    // 1-2 letter substring buckets, so filtered search responds from the first
    // keystroke. Empty / false when no platform filter is active.
    std::vector<int> filteredCandidates;
    bool formatFilterActive = false;
    // Podcast browse state (see PODCAST_SHOW_INDEX / podcastShows()).
    bool podcastFilterActive = false;                     // PODCAST filter on
    int podcastShowFilter = -1;                           // drilled-in ROWID
    std::vector<std::pair<int, std::string>> podcastShowList; // (ROWID,name)
    // Rank (0..N-1) of each distinct sub-format hue present in the active
    // filter, and the count N. Built in setFormatFilter() so renderSong can
    // spread hues evenly across however many formats the platform actually has.
    std::map<uint16_t, int> filterHueRank;
    int filterHueCount = 0;

    mutable std::mutex chkMutex;
    mutable std::mutex dbMutex;
    sqlite3db::Database db;

    // Dedicated connection for getSongScreenshots() — called from a detached
    // thread, so it cannot share the main db connection.
    mutable std::mutex screenshotMutex;
    mutable std::unique_ptr<sqlite3db::Database> screenshotDb;

    bool reindexNeeded;
    bool rebuildForced = false;
    uint32_t totalSongs = 0;

    uint16_t dbVersion{};
    uint16_t indexVersion{};

    int collectionFilter = -1;

    std::future<void> initFuture;
    std::atomic<bool> indexing{};

    std::vector<Playlist> playLists;
    std::unordered_map<uint64_t, uint32_t> pathMap;
    uint32_t productStartIndex{};
    std::vector<uint8_t> dontIndex;
};
} // namespace chipmachine

#endif // MUSIC_DATABASE_H
