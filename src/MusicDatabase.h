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

    OTHER,

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
    JPFM, // NEC PC-98 (FMP/PMD/S98 drivers). Was "Japanese FM computers"; the
          // X68000 and FM Towns members split into JPX68000/JPFMTOWNS below.

    ADPLUG,

    ARCADE, // Arcade boards (MAME-style rips): generic + Capcom/Konami/Namco/
            // Sega/Taito. Like OTHER, one byte fronting several sub-platforms
            // recovered from the format string (see buildSubPlatforms).

    TRACKER = 0x30,
    SCREAMTRACKER,  // IBM PC: Scream Tracker (S3M/STM)
    IMPULSETRACKER, // IBM PC: Impulse Tracker (IT)
    FASTTRACKER,    // IBM PC: FastTracker II (XM)
    PCTRACKER,      // other IBM PC/DOS trackers (MTM, 669, MDL, GDM, ...)

    AMIGA,
    PROTRACKER,
    SOUNDTRACKER,

    UADE,

    // Zophar streamed-tier consoles (recorded rips played via vgmstream/ffmpeg).
    // Deliberately placed here in the free 57-63 range (after UADE, before the
    // fixed PRODUCT=0x40 anchor): the console region above ends at ARCADE=46 with
    // only one slot before the fixed TRACKER=0x30 anchor, so inserting them there
    // would push ARCADE past 0x30 and collide with the tracker bytes.
    N3DS,       // Nintendo 3DS
    GAMECUBE,   // Nintendo GameCube
    WII,        // Nintendo Wii
    PS3,        // Sony PlayStation 3
    PSP,        // Sony PlayStation Portable
    XBOX,       // Microsoft Xbox
    XBOX360,    // Microsoft Xbox 360

    PRODUCT = 0x40,

    // Apple sub-platforms. APPLE (0x50 region below is full) historically fronted
    // all four; it now means Apple IIGS specifically, and these three split off
    // the Macintosh / Mac OS / iOS tunes so the TAB "Apple" group can drill into
    // all four. Placed after the PRODUCT anchor (0x41-0x43); nothing treats
    // PRODUCT as the max byte, and getFormatByteCounts()/formatColor cover 0..255.
    APPLEMAC, // Original Apple Macintosh (classic Mac OS trackers, PlayerPRO .mad)
    MACOS,    // Mac OS / macOS (PPC + Intel)
    IOS,      // Apple iOS

    // Nintendo Virtual Boy (VSU). Placed here for the same reason as the Apple
    // block: the console region above is full up to the fixed TRACKER=0x30
    // anchor. Sources are the VGMRips VSU logs (which only libvgm decodes -- see
    // vgm_opl_detect.h) plus pouet's "Youtube (Virtual Boy)" captures.
    VIRTUALBOY,

    // The Atari machines that used to be folded into ATARI (Falcon, Jaguar) or
    // POKEY (VCS/TIA) or OTHER (7800, Lynx), split out so the TAB "Atari" group
    // can drill into all seven. Placed here for the same reason as the Apple and
    // Virtual Boy blocks above -- the console region is full up to the fixed
    // TRACKER=0x30 anchor -- and nothing caps at PRODUCT=0x40.
    //
    // NOTE for any future "Atari <machine>" format string: formatToByte has a
    // startsWith(f, "atari") -> ATARI fallback, so a new machine MUST get an
    // explicit format_map entry or it silently lands in the ST/STE/TT filter.
    ATARIVCS,    // Atari 2600 / VCS (TIA)
    ATARI7800,   // Atari 7800 (TIA + POKEY cart)
    ATARIFALCON, // Atari Falcon 030 (DSP sample trackers: .gtk/.dtm/.mix)
    ATARILYNX,   // Atari Lynx (handheld)
    ATARIJAGUAR, // Atari Jaguar (mostly rips, no native chip format)

    // The Japanese FM computers, split so the TAB "Japanese Computers" group can
    // drill into all three (was one JPFM byte behind the "PC-98/X68000/FM Towns"
    // hack row). JPFM is repurposed to mean NEC PC-98 specifically (the archetypal
    // Japanese FM machine: FMP/PMD/S98 drivers), the same way APPLE narrowed to
    // Apple IIGS when the Apple group was split. These two are the other members.
    JPX68000,  // Sharp X68000 (MDX; Sharp X1 folds in)
    JPFMTOWNS, // Fujitsu FM Towns (Euphony .eup; Fujitsu FM-7 folds in)

    // Commodore VIC-20 (MOS 6560/6561 VIC-I sound). VIC-TRACKER (.vt) tunes,
    // played by victrackerplugin. Placed after the PRODUCT anchor for the same
    // reason as the Apple/Virtual Boy/Atari blocks above -- the console region is
    // full up to the fixed TRACKER=0x30 anchor -- and nothing caps at PRODUCT.
    VIC20,

    // SNK Neo Geo Pocket / Color (T6W28 sound, a SN76489 sibling). Promoted from
    // an "Other Platforms" sub-platform (OTHER byte) to its own top-level TAB
    // filter row. Placed after the PRODUCT anchor for the same reason as above.
    NEOGEOPOCKET

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
        self = this;
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
        if (index >= OTHER_PLATFORM_INDEX)
            f = subPlatformByte; // OTHER or ARCADE group row
        else if (index >= PODCAST_SHOW_INDEX)
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

    // True while a platform filter (TAB) is active.
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

    // The song's REAL (inner) file extension, lowercased and without a leading
    // dot. Compressed/archived songs (.zip/.gz/.lha/...) carry the container
    // extension in SongInfo::ext, but the actual format lives in the wrapped
    // file -- this strips container wrappers and (for ".lha/<member>" paths and
    // modland prefix-form names) resolves the inner type, so callers always get
    // the playable format (e.g. "mod" for "rubbergoat.zip"), never "zip". Falls
    // back to "" when the inner format can't be determined (e.g. an unextracted
    // ".zip" whose member name isn't yet known). Used by describeFormat() and by
    // the now-playing screen for the scroller/format line and screenshot logos.
    static std::string resolveExtension(SongInfo const& s);

    // Trackers + prose description for a file extension, read from
    // data/misc/formats_descriptions.txt (lazily loaded/cached). Returns
    // "<trackers> - <description>", or "" when the extension isn't listed.
    // Used by the scroller fallback when a tune carries no embedded text.
    std::string describeExtension(std::string const& ext);

    // Just the short NAME field (line 1 of a formats_descriptions.txt entry, no
    // prose), e.g. "FastTracker 2.0" for "xm". Shares describeExtension()'s lazy
    // load. Returns "" when the extension isn't listed. Used to label rows on the
    // CTRL+TAB Formats screen.
    std::string extensionName(std::string const& ext);

    // Map a modland/format string (+ path) to its format byte (platform/type).
    static uint8_t classifyFormat(std::string const& fmt,
                                  std::string const& path);

    // Human-readable platform name (the TAB-filter label, e.g. "Amiga",
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

    // Filesystem-safe platform logo slug for a raw format byte, or "" for
    // non-hardware bytes (MP3, OGG, Radio, YouTube, Podcast, Playlist, unknown).
    // Lets the TAB filter map a highlighted platform/category to its logo.
    static std::string platformScreenshotSlug(uint8_t formatByte);

    // Canonical sub-platform group name for a raw DB format string -- the row a
    // song lands on in the Other/Arcade drill, and the base name of its logo.
    // Shared by buildSubPlatforms() and the logo lookup so the two can never
    // disagree about what a group is called. See the definition for the rules.
    static std::string subPlatformName(std::string const& fmt);

    // The "Other Platforms" drill rows that name real hardware, and so should
    // carry a <name>.png logo. Read straight from the song DB so it works at
    // startup before the search index exists. Used to report missing logos.
    // Returns empty when the DB doesn't exist yet.
    static std::vector<std::string> subPlatformNames();

    // The complementary set: Other drill rows that name no machine (non-hardware
    // pouet tags like Java/JavaScript/Flash and meta buckets like VGM/Browser).
    // A logo could never be "correct" for these, so they are NOT flagged as gaps
    // by subPlatformNames() -- this lists them separately so the startup report
    // can still show which rows are art-less, distinct from a real missing logo.
    static std::vector<std::string> subPlatformNamesNonHardware();

    // Distinct byte-less "family" parent rows in the Other drill (e.g. "Virtual
    // Platforms", which nests TIC-80/PICO-8/MicroW8). Each wants a <name>.png of
    // its own, so the startup missing-logo report checks them like a real row.
    static std::vector<std::string> subPlatformFamilyNames();

    // Distinct file extension (lowercased) -> the set of platform slugs its
    // songs classify to, read from the song DB. Used at startup to report which
    // extensions need a dedicated screenshot because no platform logo covers
    // them. Returns empty when the DB doesn't exist yet.
    std::map<std::string, std::set<std::string>> extensionPlatforms();

    // Number of indexed songs (excluding products) per format byte (0..255).
    // Used to show per-platform tune counts on the TAB filter screen.
    std::vector<int> getFormatByteCounts() const;

    // Number of distinct podcast shows (collections containing PODCAST-format
    // episodes). Used to label the TAB Podcasts filter ("9 Podcasts [...]").
    int getPodcastShowCount() const;

    // Number of distinct sub-platforms among "Other Platforms" (OTHER-format)
    // songs. Used to label the TAB Other Platforms filter ("N Other Platforms").
    int getOtherPlatformCount();

    // Number of distinct sub-platforms among "Arcade" (ARCADE-format) songs.
    // Used to label the TAB Arcade filter ("N Arcade").
    int getArcadePlatformCount();

    std::string getTitle(int index) const
    {
        std::lock_guard lock{ dbMutex };
        if (index >= OTHER_PLATFORM_INDEX) {
            int gid = index - OTHER_PLATFORM_INDEX;
            for (size_t i = 0; i < otherPlatformList.size(); i++)
                if (otherPlatformList[i].first == gid)
                    return utils::format("%s  [%d tunes]",
                                         otherPlatformList[i].second,
                                         otherGroupCount[i]);
            return "";
        }
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

    // Number of song rows the indexer has processed so far (see indexedCount).
    // Drives the startup indexing progress bar.
    int getIndexedCount() const
    {
        return indexedCount.load(std::memory_order_relaxed);
    }

    // Number of per-collection databases created so far (see dbCreatedCount).
    int getDbCreatedCount() const
    {
        return dbCreatedCount.load(std::memory_order_relaxed);
    }

    // Whether the in-flight init is performing a real reindex (vs a plain cached
    // load). The startup progress bar is shown only when this is true.
    bool isReindexing() const
    {
        return reindexingNow.load(std::memory_order_relaxed);
    }

    // Collection id ("hvsc", "mirsoft", ...) the indexer is currently working
    // on, published for the startup progress screen. Same tag the search results
    // show next to the format line. Empty when nothing is in flight.
    std::string getIndexingName() const
    {
        std::lock_guard lock{ indexNameMutex };
        return indexName;
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
    // Drop every song from `plist` and persist the (now empty) list to disk.
    void clearPlaylist(std::string const& plist);
    std::vector<SongInfo>& getPlaylist(std::string const& plist);

    // Create a new on-disk playlist named `name` seeded with `songs`, and add it
    // to the in-memory list so it shows up immediately. The caller is expected to
    // have validated `name` (non-empty, no collision) via playlistNames() first.
    void createPlaylist(std::string const& name,
                        std::vector<SongInfo> const& songs);
    // Delete the playlist named `name`: remove its file and drop it from the
    // in-memory list. No-op if it does not exist.
    void deletePlaylist(std::string const& name);
    // Names of all known playlists, for collision checks before createPlaylist().
    std::vector<std::string> playlistNames() const;

    void setFilter(std::string const& filter, int type = 0);
    void setFormatFilter(std::vector<uint8_t> const& allowedFormats);

    // --- "Formats" screen (per-extension filter) -----------------------------
    // One browsable row per resolved file extension, sorted by song count. A row
    // is admitted if the extension is described in formats_descriptions.txt, or
    // has >= kExtGroupMinSongs songs and a clean token (so song-name fragments
    // that resolveExtension() mistakes for extensions never become rows).
    struct ExtGroup
    {
        std::string ext;    // resolved extension, lowercased, no dot (e.g. "mod")
        std::string name;   // description name, else modal DB format string, else ""
        int count;          // songs in this group
        uint8_t platform;   // format byte of the modal DB format (for row colour)
    };
    // The extension list (built lazily on first call, then cached), highest count
    // first. The vector index equals the group id used by setExtensionFilter().
    std::vector<ExtGroup> const& extensionGroups();
    // Restrict search to one extension group (its index in extensionGroups());
    // pass -1 to clear. Reuses the same title-index predicate slot and the
    // formatFilterActive machinery the platform (TAB) filter uses -- so only one
    // of the two can be active at a time, and every downstream search/prompt/ESC
    // path works unchanged.
    void setExtensionFilter(int gid);
    int extensionFilter() const { return extensionFilterGid; }

    // --- "Databases" screen (per-collection filter) --------------------------
    // One browsable row per source collection (HVSC, Modland, ...), sorted by
    // song count. Same stack as the extension filter -- setDatabaseFilter reuses
    // the formatFilterActive machinery, so browse/prompt/ESC all work unchanged.
    struct DatabaseGroup
    {
        int rowid;         // collection.ROWID (packed in formats[i] >> 8)
        std::string id;    // short id ("hvsc")
        std::string name;  // display name ("HVSC"); falls back to id when empty
        int count;         // songs in this collection
        uint8_t platform;  // modal format byte of its songs (for row colour)
    };
    // The database list (built lazily, then cached), highest count first. The
    // vector index is the row's position; setDatabaseFilter takes the ROWID.
    std::vector<DatabaseGroup> const& databaseGroups();
    // Restrict search to one collection ROWID; pass -1 to clear. Mutually
    // exclusive with the platform and extension filters (shares the slot).
    void setDatabaseFilter(int rowid);
    int databaseFilter() const { return databaseFilterRowid; }

    // --- "Plugins" screen (per-plugin filter) ---------------------------------
    // One browsable row per registered ChipPlugin that claims at least one song,
    // sorted by song count. A song's plugin is resolved by EXTENSION only (its
    // resolveExtension()'d extension looked up against each plugin's
    // getSupportedExtensions(), first -- i.e. highest-priority -- claimer wins,
    // same order MusicPlayer::fromFile() tries plugins in) -- not canHandle(),
    // since some plugins disambiguate by magic bytes and would need to open
    // every (mostly remote/uncached) catalog file to answer.
    struct PluginGroup
    {
        std::string name;  // plugin name()
        int index;         // index of the plugin in getPlugins()
        int count;         // songs whose extension this plugin claims
        uint8_t platform;  // modal format byte of its songs (for row colour)
    };
    std::vector<PluginGroup> const& pluginGroups();
    void setPluginFilter(int gid);
    int pluginFilter() const { return pluginFilterGid; }

    // Precompute the Format and Database browse lists so the first TAB to those
    // screens is instant. Call once, after indexing has finished. The Database
    // list is cheap (in-memory) and built inline; the Extension list needs a
    // full song-table scan (~360ms), so it runs on a worker thread with its own
    // DB connection -- keeping the render loop (star scroll) smooth. Safe to call
    // more than once; later calls are no-ops once built.
    void precomputeBrowseListsAsync();

private:
    void initDatabase(utils::path const& workDir, Variables& vars);
    void generateIndex();

    // Extensions the indexer must NOT add as playable songs, loaded from
    // data/misc/not_supported_extensions.txt at startup. Active (uncommented)
    // ".ext" lines land here (lowercased, no leading dot); commented-out lines
    // ("# .gtk") are ignored, so those extensions stay indexable.
    std::set<std::string> unsupportedExts;
    void loadUnsupportedExtensions(utils::path const& workDir);

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
    // The show's representative artwork URL (collection.artwork), keyed by the
    // collection ROWID podcastShows() carries. Empty when the show has no image.
    // Used to preview a podcast's logo while browsing the show list.
    std::string getPodcastShowArtwork(int rowid) const;
    // Drill into one show (its ROWID) so an empty query lists that show's
    // episodes; pass -1 to go back to the show list.
    void setPodcastShow(int rowid) { podcastShowFilter = rowid; }
    int podcastShow() const { return podcastShowFilter; }
    bool podcastFilterActive_() const { return podcastFilterActive; }

    // Synthetic result indices for "Other Platforms" GROUP rows (one per
    // distinct sub-platform among OTHER-format songs) shown when the Other
    // Platforms filter is active with an empty query and not drilled in.
    // index = OTHER_PLATFORM_INDEX + groupId. Kept above PODCAST_SHOW_INDEX and
    // checked first wherever indices are dispatched.
    static constexpr int OTHER_PLATFORM_INDEX = 0x1C000000;

    // Other-platforms browse: list of (groupId, name) for each sub-platform,
    // sorted by name; populated when the Other Platforms format filter
    // activates (see buildSubPlatforms).
    std::vector<std::pair<int, std::string>> const& otherPlatforms() const
    {
        return otherPlatformList;
    }
    // Extensions we ship a plugin for but deliberately cannot play
    // (data/misc/not_supported_extensions.txt). Exposed because the archive
    // track picker (MusicPlayerList) derives its member allowlist from the
    // registered plugins and must subtract these, exactly as the indexer does.
    std::set<std::string> const& unsupportedExtensions() const
    {
        return unsupportedExts;
    }
    // The live instance (set in the ctor). Null before construction.
    static MusicDatabase* instance() { return self; }

    // Song count for one sub-platform group id (0 if unknown). Same lookup
    // getTitle() uses for its "[N tunes]" suffix.
    int otherPlatformSongCount(int gid) const
    {
        for (size_t i = 0; i < otherPlatformList.size(); i++)
            if (otherPlatformList[i].first == gid)
                return otherGroupCount[i];
        return 0;
    }
    // True if a groupId is a family PARENT row (a byte-less 2nd-level grouping
    // like "Virtual Platforms") rather than a real sub-platform. Its song count
    // is the sum of its children's, so callers tallying totals must skip it.
    bool isOtherFamilyRow(int gid) const { return otherFamilyGids.count(gid) > 0; }
    // Drill into one sub-platform (its groupId) so an empty query lists that
    // platform's songs; pass -1 to go back to the platform list.
    void setOtherPlatform(int gid) { otherPlatformFilter = gid; }
    int otherPlatform() const { return otherPlatformFilter; }
    bool otherFilterActive_() const { return otherFilterActive; }
    // A second drill level WITHIN the Other list: a handful of sub-platforms are
    // grouped under a byte-less "family" parent row (e.g. TIC-80/PICO-8/MicroW8
    // under "Virtual Platforms"). Entering a family (its parent gid) makes the
    // empty-query menu list that family's children instead of the top rows; -1
    // returns to the top. Orthogonal to setOtherPlatform, which drills a single
    // group down to its songs (ESC pops songs -> family -> top, one step each).
    void setOtherParent(int familyGid) { otherParentFilter = familyGid; }
    int otherParent() const { return otherParentFilter; }

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
    // ext -> just the line-1 name field (no prose), loaded alongside the above.
    std::map<std::string, std::string> formatNames;
    bool formatDescriptionsLoaded = false;

    // The (singleton) instance, so the static resolveExtension()/describeFormat()
    // helpers can consult the instance-loaded formats_descriptions table to tell
    // a real format prefix ("cust.ingame" -> "cust") from a song-name token.
    static MusicDatabase* self;

    SearchIndex composerIndex;
    SearchIndex titleIndex;

    std::vector<uint32_t> titleToComposer;
    std::vector<uint32_t> composerToTitle;
    std::vector<uint32_t> composerTitleStart;
    std::vector<uint16_t> formats;
    // Platform format-byte per product (indexed by product ordinal, i.e.
    // titleIndex position - productStartIndex). Products only carry the PRODUCT
    // byte in `formats`, so this side-channel lets the platform filter (TAB)
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

    // Per-entry REAL-format token (interned id of the true module extension),
    // aligned with `formats`. Used only by search()'s add_unique dedup so it
    // folds two rows only when their ACTUAL format matches -- not the coarse
    // platform byte. This distinguishes e.g. a mirsoft ".mod" remix filed under
    // Commodore 64 from the real HVSC ".sid", which share {title,composer,byte}
    // and would otherwise shadow each other. 0 = unknown format (never folds).
    std::vector<uint32_t> formatKey;

    // When a platform filter (TAB) is active, the set of titleIndex indices that
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

    // Sub-platform browse state (see OTHER_PLATFORM_INDEX / otherPlatforms()).
    // Some format bytes (OTHER, ARCADE) collapse many real platforms into one
    // filter, so a song's sub-platform survives only as its DB format string.
    // buildSubPlatforms() recovers it with one scan and groups by name. Only one
    // such filter is active at a time, so a single working set is reused; the
    // byte it was built for is tracked so switching filters rebuilds it.
    bool otherFilterActive = false;                     // OTHER/ARCADE filter on
    uint8_t subPlatformByte = OTHER;                    // active drill byte
    int otherPlatformFilter = -1;                       // drilled-in groupId
    int otherParentFilter = -1;                         // drilled-in family gid (-1=top)
    int builtSubPlatformByte = -1;                      // byte the set was built for
    std::vector<std::pair<int, std::string>> otherPlatformList; // (groupId,name)
    std::vector<int> otherGroupCount;                   // songs per group (by pos)
    std::unordered_map<int, int> otherIndexToGroup;     // song index -> groupId
    // Family (2nd-level) grouping over otherPlatformList. A family parent is a
    // synthetic group appended after the real ones: it holds no songs of its own,
    // its otherGroupCount is the sum of its children's, and getSongInfo gives it
    // an "othergroup::" path so a click enters its submenu instead of playing.
    std::set<int> otherFamilyGids;                      // gids that are family parents
    std::vector<int> otherTopRows;                      // top-menu synthetic indices (sorted)
    std::map<int, std::vector<int>> otherFamilyChildRows; // parent gid -> child synthetic indices
    // Scan the song table (ROWID == search index + 1) once, classify songs whose
    // format byte == subPlatformByte by their format string, and populate the
    // browse state above. Rebuilds when subPlatformByte changes.
    void buildSubPlatforms();

    // Extension-filter browse state (see extensionGroups() / setExtensionFilter).
    static constexpr int kExtGroupMinSongs = 10; // admit undescribed exts at/above
    std::vector<ExtGroup> extensionGroupList;    // by count desc; index == gid
    std::vector<int16_t> extGroupOf;             // song index -> gid, or -1
    // Atomic so extensionGroups()'s fast path can read it while the worker
    // (precomputeBrowseListsAsync) sets it. The build critical section itself is
    // serialized by extGroupsMutex so the worker and a racing lazy GUI call can
    // never build (or publish) at the same time.
    std::atomic<bool> extensionGroupsBuilt{ false };
    std::mutex extGroupsMutex;                    // guards buildExtensionGroups()
    std::future<void> extGroupsFuture;            // worker handle (joins on destroy)
    int extensionFilterGid = -1;                 // active gid, -1 = none
    // One scan of the song table (on its OWN db connection, so it is worker-safe):
    // resolveExtension() per song, group, count, sort by count, admit per the rule
    // above. Fills extensionGroupList / extGroupOf, then publishes under the mutex.
    void buildExtensionGroups();

    // Database-filter browse state (see databaseGroups() / setDatabaseFilter).
    std::vector<DatabaseGroup> databaseGroupList; // by count desc; index == row
    bool databaseGroupsBuilt = false;
    int databaseFilterRowid = -1;                 // active collection ROWID, -1 = none
    // ROWID of the "Playlists" collection (db.lua id "pl"), resolved lazily and
    // cached. -2 = not looked up yet, -1 = no such collection. When this is the
    // active database filter, search() also lists the user's config-dir playlists
    // (playLists) so runtime-created lists show alongside the indexed ones.
    int playlistsCollRowid = -2;
    int playlistsCollectionRowid();
    // Count songs per collection (from formats[] >> 8), fetch id/name and the
    // modal platform byte, sort by count. Fills databaseGroupList.
    void buildDatabaseGroups();

    // Plugin-filter browse state
    std::vector<PluginGroup> pluginGroupList;
    std::vector<int16_t> pluginGroupOf;             // song index -> gid, or -1
    std::atomic<bool> pluginGroupsBuilt{ false };
    std::mutex pluginGroupsMutex;
    std::future<void> pluginGroupsFuture;
    int pluginFilterGid = -1;
    void buildPluginGroups();

    // Order a candidate-index list alphabetically by title. Uses the precomputed
    // titleRank (an int compare per element, no strings) when it's ready, so even
    // a 250k-song filter sorts in a few ms; falls back to decorate-sort-undecorate
    // only if a filter is somehow activated before the rank is built. Called once
    // when a filter is activated so the per-keystroke search paths never re-sort.
    void sortCandidatesByTitle(std::vector<int>& cands);

    // Alphabetical rank (0..N-1) of each titleIndex entry, by lowercased title.
    // Built once at startup on the indexing thread (buildTitleRank) so every
    // subsequent filter activation is a plain integer sort. In-memory only (not
    // persisted -- no index-format change); rebuilt each launch and after a
    // reindex. Empty / shorter than titleIndex until built or after a live append.
    std::vector<uint32_t> titleRank;
    // Populate titleRank from the (loaded or freshly built) titleIndex. Runs on
    // the background indexing thread, before the indexing flag clears.
    void buildTitleRank();
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
    std::string checkingNames;

    uint16_t dbVersion{};
    uint16_t indexVersion{};

    int collectionFilter = -1;

    std::future<void> initFuture;
    std::atomic<bool> indexing{};
    // Live count of rows processed by the indexer, published for the startup
    // progress bar. Written from the async index thread, read from the render
    // thread; relaxed access is fine (a slightly stale count just moves the bar).
    std::atomic<int> indexedCount{ 0 };
    // Live count of per-collection databases created (one tick per "Creating
    // '...' DB" step). Drives the first phase of the startup progress bar.
    std::atomic<int> dbCreatedCount{ 0 };
    // True only while a run is doing a *real* reindex (version bump, new
    // collection, podcast refresh, or missing index.dat) -- i.e. reindexNeeded
    // fired. A plain cached load leaves this false, so the UI can suppress the
    // "Indexing database" toast/bar entirely instead of briefly flashing it on
    // slow machines where the cached-load window outlasts the toast delay.
    std::atomic<bool> reindexingNow{ false };
    // Collection id currently being created/indexed (see getIndexingName).
    // Written from the async index thread, read from the render thread.
    mutable std::mutex indexNameMutex;
    std::string indexName;
    void setIndexingName(std::string const& n)
    {
        std::lock_guard lock{ indexNameMutex };
        indexName = n;
    }

    std::vector<Playlist> playLists;
    std::unordered_map<uint64_t, uint32_t> pathMap;
    uint32_t productStartIndex{};
    std::vector<uint8_t> dontIndex;

    // Search precedence per collection ROWID (from the collection table's
    // `priority` column, set in db.lua). Higher = surfaces first and wins the
    // dedup when two rows would otherwise fold. Loaded on every launch (cached
    // or rebuilt) in generateIndex; search() stable-sorts title matches by it.
    std::vector<int> collPriority;
};
} // namespace chipmachine

#endif // MUSIC_DATABASE_H
