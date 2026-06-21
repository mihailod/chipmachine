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

    APPLE,

    ATARI, // Atari ST/STE (YM2149): sndh, YM, SC68, ST UADE formats
    POKEY, // Atari XL/XE 8-bit (POKEY): .sap

    MP3,

    M3U,
    PLS,

    OGG,

    RADIO, // Live streaming radio stations (the "radio" collection)

    YOUTUBE,

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
        if (index >= PLAYLIST_INDEX)
            f = PLAYLIST;
        else
            f = formats[index];
        return utils::format("%s\t%s\t%d\t%d", getTitle(index),
                             getComposer(index), index, f);
    }
    // Get full data, may require SQL query
    SongInfo getSongInfo(int index) const;

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

    // Number of indexed songs (excluding products) per format byte (0..255).
    // Used to show per-platform tune counts on the F9 filter screen.
    std::vector<int> getFormatByteCounts() const;

    std::string getTitle(int index) const
    {
        std::lock_guard lock{ dbMutex };
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

    // When a platform filter (F9) is active, the set of titleIndex indices that
    // pass it, precomputed in setFormatFilter(). Lets short queries (< 3 chars)
    // scan the (typically small) filtered set directly instead of the sparse
    // 1-2 letter substring buckets, so filtered search responds from the first
    // keystroke. Empty / false when no platform filter is active.
    std::vector<int> filteredCandidates;
    bool formatFilterActive = false;

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
