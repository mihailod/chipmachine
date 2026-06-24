#include "MusicDatabase.h"
#include "RemoteLoader.h"
#include "SongFileIdentifier.h"
#include "modutils.h"

#include <archive/archive.h>
#include <coreutils/environment.h>
#include <coreutils/searchpath.h>
#include <coreutils/utils.h>
#include <crypto/md5.h>
#include <webutils/web.h>
#include <xml/xml.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <map>
#include <set>
#include <thread>

#include <sol.hpp>

#include "csv.h"

using namespace utils;

// From Rosetta stone
// Compute Levenshtein Distance
// Martin Ettl, 2012-10-05
size_t levenshteinDistance(std::string const& s1, std::string const& s2)
{
    auto m = s1.length();
    auto n = s2.length();

    if (m == 0) return n;
    if (n == 0) return m;

    std::vector<size_t> costs(n + 1);

    size_t i = 0;
    for (auto it1 = s1.begin(); it1 != s1.end(); ++it1, ++i) {
        costs[0] = i + 1;
        size_t corner = i;

        size_t j = 0;
        for (auto it2 = s2.begin(); it2 != s2.end(); ++it2, ++j) {
            size_t upper = costs[j + 1];
            if (*it1 == *it2) {
                costs[j + 1] = corner;
            } else {
                size_t t(upper < corner ? upper : corner);
                costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
            }

            corner = upper;
        }
    }

    size_t result = costs[n];

    return result;
}

namespace chipmachine {

void MusicDatabase::createTables()
{
    db.exec("CREATE TABLE IF NOT EXISTS collection (name STRING, url STRING, "
            "localdir STRING, "
            "description STRING, id UNIQUE, version INTEGER, artwork STRING)");
    db.exec("CREATE TABLE IF NOT EXISTS song (title STRING, game STRING, "
            "composer STRING, "
            "format STRING, path STRING, collection INTEGER, metadata STRING, "
            "ext STRING, artwork STRING)");
    db.exec("CREATE TABLE IF NOT EXISTS product (title STRING, creator STRING, "
            "type STRING, "
            "screenshots STRING, collection INTEGER, metadata STRING)");
    db.exec("CREATE TABLE IF NOT EXISTS prod2song (songid INTEGER, prodid "
            "INTEGER)");
}

bool MusicDatabase::parseBitworld(
    Variables& vars, std::string const& listFile,
    std::function<void(Product const&)> const& callback)
{
    for (auto const& s : apone::File{ listFile }.lines()) {
        auto parts = split(s, "\t");
        Product prod;
        // LOGD("ID: %s", parts[0]);
        prod.title = parts[1];
        prod.creator = parts[2];
        prod.type = std::string("Amiga ") + parts[3];
        prod.screenshots = parts[5];
        for (const char* s : split(parts[4], ";")) {
            if (endsWith(s, ".smpl")) continue;
            if (s[0] == 'M')
                prod.songs.push_back(utils::urldecode(&s[2], ""));
            else
                prod.songs.push_back(&s[2]);
        }
        callback(prod);
    }
    return true;
}

bool MusicDatabase::parseGamebase(
    Variables& vars, std::string const& listFile,
    std::function<void(Product const&)> const& callback)
{

    using namespace io;
    CSVReader<3, trim_chars<' '>, double_quote_escape<',', '\"'>> in(listFile);
    in.read_header(io::ignore_extra_column, "Name", "ScrnshotFilename",
                   "SidFilename");
    std::string name, screenshot, sid;
    while (in.read_row(name, screenshot, sid)) {
        // do stuff with the data
        replace(screenshot.begin(), screenshot.end(), '\\', '/');
        replace(sid.begin(), sid.end(), '\\', '/');
        if (sid != "") {
            Product prod;
            prod.title = name;
            prod.type = "C64 Game";
            prod.screenshots = screenshot;
            prod.songs.push_back(sid);
            callback(prod);
        }
    }
    return true;
}

bool MusicDatabase::parseCsdb(
    Variables& vars, std::string const& listFile,
    std::function<void(Product const&)> const& callback)
{

    auto doc = xmldoc::fromFile(listFile);
    for (auto const& i : doc["ReleasesWithHVSC"].all("Release")) {
        Product prod;
        prod.title = htmldecode(utf8_encode(i["Name"].text()));
        prod.type = i["ReleaseType"].text();
        auto rating = i["CSDbRating"];

        //float rt = rating.valid() ? stod(rating.text()) : 0.0;

	float rt = 0.0;
	if (rating.valid()) {
    	    auto rtext = rating.text();
    	    if (!rtext.empty()) rt = stod(rtext);
	}

        // LOGD("Found %s (%s %d)", name, type, rt);
        std::string group;
        auto rb = i["ReleasedBy"];
        if (rb.valid()) {
            for (auto const& g : rb.all("Group")) {
                auto gn = utf8_encode(g["Group"].text());
                if (group != "") group += "+";
                group += gn;
            }
        }
        auto shot = i["Screenshot"];
        if (shot.valid()) {
            prod.screenshots = shot.text();
            // LOGD("Screenshot %s", prod.screenshots);
        }
        prod.creator = group;
        if ((endsWith(prod.type, "Music Collection") ||
             endsWith(prod.type, "Diskmag") || endsWith(prod.type, "Demo")) &&
            rt >= 0) {

            /*for (auto const& s : i["Sids"].all("HVSCPath")) {
                prod.songs.push_back(s.text().substr(1));
            }*/

	    auto sids = i["Sids"];
	    if (sids.valid()) {
    	    for (auto const& s : sids.all("HVSCPath")) {
        	prod.songs.push_back(s.text().substr(1));
    	    	}
	    }	

            callback(prod);
        }
    }
    return true;
}

bool MusicDatabase::parsePouet(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{
    auto doc = xmldoc::fromFile(listFile);
    for (auto const& i : doc["feed"].all("prod")) {
        auto title = i["name"].text();
        auto g = i["group1"];
        auto group = g.valid() ? g.text() : "";
        auto youtube = i["youtube"].text();
        callback(SongInfo(youtube, "", title, group, "Youtube"));
    }
    return true;
}

bool MusicDatabase::parseAmp(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{
    File f{ listFile };

    for (auto const& s : f.getLines()) {
        SongInfo song;
        auto path = urldecode(s, "");
        auto parts = split(path, "/");
        if (parts.size() < 3) {
            LOGD("%s (%s) broken", s, path);
            continue;
        }
        int l = parts.size();
        std::vector<std::string> titleParts = split(parts[l - 1], ".");
        if (titleParts.size() < 2) {
            LOGD("%s broken", s);
            continue;
        }
        titleParts[1][0] = toupper(titleParts[1][0]);
        song.path = s;
        song.composer = parts[l - 2];
        song.title = titleParts[1];
        song.format = titleParts[0] == "STK" ? "Soundtracker" : "Protracker";
        callback(song);
    }
    return true;
}

bool MusicDatabase::parseRss(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{

    xmldoc doc;

    try {
        doc = xmldoc::fromFile(listFile);
    } catch (xml_exception e) {
        return false;
    }
    auto rssNode = doc["rss"];
    if (!rssNode.valid()) {
        LOGE("Could not find rss node in xml");
        return false;
    }
    auto channelNode = rssNode["channel"];

    // Channel-level description, used as the per-episode fallback so the
    // scroller always has something show-relevant to display when an episode
    // carries no <description> of its own (common -- e.g. most C64 Take-away
    // items). Without this the scroller would fall through to the module-format
    // line, which is meaningless for a podcast.
    std::string showDescription = htmldecode(channelNode["description"].text());

    for (auto const& i : channelNode.all("item")) {
        auto title = i["title"].text();
        auto e = i["enclosure"];
        if (!e.valid()) continue;
        auto enclosure = e.attr("url");
        // LOGD("Title %s", title);
        std::string description;
        auto summary = i["itunes:summary"];
        auto sub_title = i["itunes:subtitle"];
        auto desc = i["description"];
        if (summary.valid())
            description = summary.text();
        else if (sub_title.valid())
            description = sub_title.text();
        else
            description = desc.text();

        description = htmldecode(description);
        if (description.empty()) description = showDescription;

        std::string composer;

        auto c = i["dc:creator"];
        if (c.valid()) composer = c.text();
        /*if(composer == "") {
            auto dash = title.rfind(" - ");
            if(dash != std::string::npos) {
                composer = title.substr(dash + 2);
                title = title.substr(0, dash);
            }
        }*/

        auto pos = enclosure.find("file=");
        if (pos != std::string::npos) enclosure = enclosure.substr(pos + 5);

        SongInfo song(enclosure, "", title, composer, "Podcast", description);
        // Per-episode artwork, when the feed provides it (<itunes:image>).
        // Episodes without one fall back to the show's representative image
        // (collection.artwork) in getSongScreenshots.
        auto img = i["itunes:image"];
        if (img.valid()) song.metadata[SongInfo::SCREENSHOT] = img.attr("href");
        callback(song);
    }
    LOGD("Done");
    return true;
}

// --- Podcast live-feed refresh (Q4) -------------------------------------
//
// Podcasts are dynamic: shows publish new episodes after release. The shipped
// data/<id>.xml is a frozen back catalogue, so we augment it from each show's
// live RSS feed (db.lua `remote_list`). The augmented copy lives in the
// writable cache (the .app bundle is read-only and code-signed -- never write
// data/ back), and the merge runs in a detached background thread so launch is
// never blocked. New episodes a refresh discovers become visible on the next
// launch, when the changed cache copy forces a reindex. Offline / failed
// fetches simply leave the cached copy untouched.
namespace {

utils::path podcastCacheDir()
{
    return Environment::getCacheDir() / "_podcasts";
}

// Normalise an enclosure URL into a dedup key. Feeds often serve the same
// episode under a different scheme (the C64 Take-away back catalogue ships as
// https:// while the live feed is http://) or with a trailing query string, so
// strip the scheme and any ?query to compare on the stable middle.
std::string podcastUrlKey(std::string url)
{
    if (startsWith(url, "https://"))
        url = url.substr(8);
    else if (startsWith(url, "http://"))
        url = url.substr(7);
    auto q = url.find('?');
    if (q != std::string::npos) url = url.substr(0, q);
    return url;
}

// Extract a dedup key for every <item>'s enclosure URL from an RSS document.
std::set<std::string> rssEnclosureUrls(std::string const& xmlText)
{
    std::set<std::string> urls;
    try {
        auto doc = xmldoc::fromText(xmlText);
        auto channel = doc["rss"]["channel"];
        for (auto const& i : channel.all("item")) {
            auto e = i["enclosure"];
            if (e.valid()) urls.insert(podcastUrlKey(e.attr("url")));
        }
    } catch (xml_exception const&) {}
    return urls;
}

} // namespace

std::string MusicDatabase::podcastSource(utils::path const& workDir,
                                         std::string const& id,
                                         std::string const& songList) const
{
    auto shipped = (workDir / songList).string();
    auto cached = (podcastCacheDir() / (id + ".xml")).string();
    std::error_code ec;
    // Seed the cache copy from the shipped back catalogue on first use.
    if (!utils::exists(cached) && utils::exists(shipped)) {
        std::filesystem::create_directories(podcastCacheDir().string(), ec);
        std::filesystem::copy_file(shipped, cached, ec);
    }
    return utils::exists(cached) ? cached : shipped;
}

bool MusicDatabase::preparePodcasts(utils::path const& workDir)
{
    using namespace std::chrono;
    bool changed = false;
    auto dir = podcastCacheDir();
    std::error_code ec;
    std::filesystem::create_directories(dir.string(), ec);

    auto now = duration_cast<seconds>(system_clock::now().time_since_epoch())
                   .count();
    const long long refreshInterval = 24 * 60 * 60; // 24h throttle

    for (auto const& feed : podcastFeeds) {
        // Make sure a cache copy exists (seeds from the shipped file).
        podcastSource(workDir, feed.id, feed.songList);

        // A previous background refresh that found new episodes left a .dirty
        // marker -> the cache copy now has more than the last index saw, so
        // force a reindex and clear the marker (this index run consumes it).
        auto dirty = (dir / (feed.id + ".dirty")).string();
        if (utils::exists(dirty)) {
            changed = true;
            std::filesystem::remove(dirty, ec);
        }

        if (feed.remoteList.empty()) continue;

        // Throttle: only hit the network if we have not checked in ~24h.
        auto stamp = (dir / (feed.id + ".stamp")).string();
        long long last = 0;
        if (utils::exists(stamp)) {
            try {
                last = std::stoll(utils::File{ stamp }.read());
            } catch (...) {}
        }
        if (now - last < refreshInterval) continue;

        // Record the attempt now so a hang/crash doesn't retry every launch.
        { utils::File s{ stamp }; s.write(std::to_string(now)); s.close(); }

        std::thread(&MusicDatabase::refreshPodcastFeed, dir, feed.id,
                    feed.remoteList)
            .detach();
    }
    return changed;
}

void MusicDatabase::refreshPodcastFeed(utils::path cacheDir, std::string id,
                                       std::string remoteList)
{
    std::string body;
    try {
        body = webutils::Web::getBlocking(remoteList);
    } catch (...) { return; }
    if (body.empty()) return;

    auto liveUrls = rssEnclosureUrls(body);
    if (liveUrls.empty()) return; // not a feed we understand / fetch failed

    auto cached = (cacheDir / (id + ".xml")).string();
    std::string current;
    try {
        current = utils::File{ cached }.read();
    } catch (...) { return; }
    auto have = rssEnclosureUrls(current);

    // Collect <item> blocks from the live feed whose enclosure we don't have.
    std::string additions;
    int added = 0;
    try {
        auto doc = xmldoc::fromText(body);
        auto channel = doc["rss"]["channel"];
        for (auto const& i : channel.all("item")) {
            auto e = i["enclosure"];
            if (!e.valid()) continue;
            auto url = e.attr("url");
            if (have.count(podcastUrlKey(url))) continue;
            auto esc = [](std::string s) {
                std::string o;
                for (char c : s) {
                    if (c == '&') o += "&amp;";
                    else if (c == '<') o += "&lt;";
                    else if (c == '>') o += "&gt;";
                    else o += c;
                }
                return o;
            };
            std::string title = i["title"].valid() ? i["title"].text() : "";
            std::string creator =
                i["dc:creator"].valid() ? i["dc:creator"].text() : "";
            std::string desc =
                i["description"].valid() ? i["description"].text() : "";
            auto img = i["itunes:image"];
            additions += "<item>\n<title>" + esc(title) + "</title>\n";
            if (!creator.empty())
                additions += "<dc:creator>" + esc(creator) + "</dc:creator>\n";
            if (img.valid())
                additions += "<itunes:image href=\"" + esc(img.attr("href")) +
                             "\"></itunes:image>\n";
            additions += "<description>" + esc(desc) +
                         "</description>\n<enclosure url=\"" + esc(url) +
                         "\" type=\"audio/mpeg\" length=\"0\"></enclosure>\n"
                         "</item>\n";
            added++;
        }
    } catch (...) { return; }

    if (added == 0) return; // nothing new

    // Insert before </channel> and write atomically (temp + rename) so a
    // concurrent index read never sees a half-written file.
    auto pos = current.rfind("</channel>");
    if (pos == std::string::npos) return;
    std::string merged =
        current.substr(0, pos) + additions + current.substr(pos);

    auto tmp = cached + ".tmp";
    { utils::File t{ tmp }; t.write(merged); t.close(); }
    std::error_code ec;
    std::filesystem::rename(tmp, cached, ec);
    if (ec) return;

    // Mark dirty so the next launch reindexes with the new episodes.
    auto dirty = (cacheDir / (id + ".dirty")).string();
    utils::File d{ dirty };
    d.write(std::to_string(added));
    d.close();
    LOGD("Podcast %s: merged %d new episode(s)", id.c_str(), added);
}

void MusicDatabase::syncPodcastSongs()
{
    auto insert = db.query("INSERT INTO song (title, game, composer, format, "
                           "path, collection, metadata, ext, artwork) "
                           "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    for (auto const& feed : podcastFeeds) {
        auto cq = db.query<int>("SELECT ROWID FROM collection WHERE id = ?",
                                feed.id);
        if (!cq.step()) continue; // not indexed yet -> full reindex handles it
        int collection_id = cq.get();

        // Episodes already in the table for this collection.
        std::set<std::string> have;
        auto pq = db.query<std::string>(
            "SELECT path FROM song WHERE collection = ?", collection_id);
        while (pq.step())
            have.insert(pq.get());

        auto src = podcastSource(workDir, feed.id, feed.songList);
        Variables vars;
        int added = 0;
        parseRss(vars, src, [&](SongInfo const& song) {
            if (have.count(song.path)) return; // append-only: keep ROWIDs stable
            insert
                .bind(song.title, song.game, song.composer, song.format,
                      song.path, collection_id,
                      song.metadata[SongInfo::INFO] != ""
                          ? song.metadata[SongInfo::INFO].c_str()
                          : nullptr,
                      song.ext != "" ? song.ext.c_str() : nullptr,
                      song.metadata[SongInfo::SCREENSHOT] != ""
                          ? song.metadata[SongInfo::SCREENSHOT].c_str()
                          : nullptr)
                .step();
            added++;
            totalSongs++;
        });
        if (added > 0)
            print_fmt("Podcast '%s': appended %d new episode(s)\n", feed.id,
                      added);
    }
}

bool MusicDatabase::parseModland(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{

    static const std::set<std::string> secondary = { "smpl", "sam", "ins",
                                                     "smp",  "pdx", "nt",
                                                     "as",
                                                     // Euphony instrument banks
                                                     // (fetched via getSecondaryFiles)
                                                     "fmb", "pmb", "pvi",
                                                     // MoonBlaster ADPCM sample
                                                     // banks (.mbk) -- companions
                                                     // of .mbm, not standalone
                                                     "mbk",
                                                     // SoundSmith wavebanks (.W)
                                                     // -- the 64KB DOC sound RAM
                                                     // companion of the bare-named
                                                     // song, fetched via
                                                     // getSecondaryFiles, never a
                                                     // standalone tune
                                                     "w" };
    static const std::set<std::string> secondary_pref = { "smpl", "smp" };
    static const std::set<std::string> hasSubFormats = { "Spectrum", "Ad Lib",
                                                         "Video Game Music" };

    auto parts = split(vars["exclude_formats"], ";");
    std::set<std::string> exclude(parts.begin(), parts.end());

    SongInfo lastSong;

    File f{ listFile };

    for (auto const& s : f.getLines()) {
        auto parts = split(s, "\t");
        if (parts.size() >= 2) {

            SongInfo song(parts[1]);

            /* std::string base = path_basename(song.path); */
            /* std::string ext = path_extension(song.path); */

            /* if(base == "mdat" || base == "jpn") { */
            /* std::swap(base, ext); */
            /* } */
            // Modland ships a human-readable ".info" metadata sibling next to
            // many modules (e.g. every PokeyNoise "pn.<song>.info"); it is never
            // a playable song, so never index it as one.
            if (endsWith(toLower(song.path), ".info")) { continue; }

            // KrisHatlelid (.kh) songs pair with a fixed-name "songplay" driver
            // file in the same game dir; it is a companion (fetched via
            // UADEPlugin::getSecondaryFiles), never a standalone tune. It has no
            // extension and sorts alphabetically before the ".kh", so without
            // this it becomes the primary of the game's MULTI: group and the
            // game "plays" the silent driver instead of the song.
            if (endsWith(toLower(song.path), "/songplay")) { continue; }

            // IFF-SMUS (and similar) carry per-instrument companion files in an
            // "instruments/" subdir (*.instr descriptors, raw *.ss samples).
            // Modland lists every file recursively, so each one would be indexed
            // as a bogus standalone song that downloads (FTP CODE 226) but has no
            // ext to route on and no plugin to decode it. They are fetched via
            // UADEPlugin::getSecondaryFiles at play time, never standalone tunes.
            if (toLower(song.path).find("/instruments/") != std::string::npos) {
                continue;
            }

            auto [ext, base] = getTypeAndBase(song.path);

            // Match the secondary-extension list case-insensitively: Modland
            // stores some collections UPPERCASE (e.g. FMP's .PVI/.OVI), so a
            // case-sensitive check let bank files slip in as bogus standalone
            // songs.
            std::string extLower = toLower(ext);
            if ((secondary.count(extLower) > 0) ||
                (secondary_pref.count(base) > 0) || endsWith(extLower, "sflib")) {
                continue;
            }

            std::vector<std::string> parts = split(song.path, "/");
            int l = parts.size();
            if (l < 3) {
                LOGD("%s", song.path);
                continue;
            }

            int i = 0;
            song.format = parts[i++];
            if (hasSubFormats.count(song.format) > 0) song.format = parts[i++];

            // Stereo Sidplayer tunes are a ".mus"/".str" pair. Index only the
            // ".str" (stereo) file; the ".mus" companion is fetched as a
            // secondary file at play time. (The mono "Sidplayer" collection,
            // which has standalone ".mus" files, is unaffected.)
            if (song.format == "Stereo Sidplayer" && ext == "mus") continue;

            song.composer = parts[i++];

            if (song.format == "MDX") {
                i--;
                song.composer = "?";
            }

            if (song.composer == "- unknown") song.composer = "?";

            if (parts[i].substr(0, 5) == "coop-")
                song.composer = song.composer + "+" + parts[i++].substr(5);

            // std::string game;
            if (l - i >= 2) song.game = parts[i++];

            if (i == l) {
                LOGD("Bad file %s", song.path);
                continue;
            }

            if (endsWith(parts[i], ".rar"))
                parts[i] = parts[i].substr(0, parts[i].length() - 4);

            song.title = base;
            if (exclude.count(song.format) > 0) continue;
            if (song.game != "" && song.game == lastSong.game &&
                song.composer == lastSong.composer) {
                // Keep adding songs of the same game to lastSong
                if (!startsWith(lastSong.path, "MULTI:")) {
                    lastSong.path = std::string("MULTI:") + lastSong.path;
                    lastSong.title = "";
                }
                lastSong.path = lastSong.path + "\t" + song.path;
                continue;
            } else {
                // song is not the same as lastSong, commit lastSong
                if (lastSong.path != "") callback(lastSong);
                lastSong = song;
            }
        }
    }
    if (lastSong.path != "") callback(lastSong);
    return true;
}

// Returns true if the filename uses the "songname.<audio-extension>" convention
// (the CD32 Ogg rips) rather than the Amiga "prefix.songname" convention.
static bool hasAudioExtension(std::string const& fname, size_t lastDot)
{
    if (lastDot == std::string::npos) return false;
    std::string ext = toLower(fname.substr(lastDot + 1));
    return ext == "ogg" || ext == "mp3" || ext == "wav" || ext == "flac" ||
           ext == "aiff" || ext == "aif";
}

// Pull the song name out of an UnExoticA filename. Amiga modules are named
// "<format-prefix>.<songname>" (mod.load, smus.Intro); CD32 rips are
// "<songname>.<audio-ext>" (03_ingame.ogg). The first token only counts as a
// format prefix if it was seen across several different game directories
// (passed in via prefixes) - that distinguishes real prefixes like "mod" from
// songname words that merely happen to contain a dot.
static std::string unexoticaSongName(std::string const& fname,
                                     std::set<std::string> const& prefixes)
{
    auto lastDot = fname.find_last_of('.');
    if (hasAudioExtension(fname, lastDot)) return fname.substr(0, lastDot);

    auto dot = fname.find('.');
    if (dot != std::string::npos &&
        prefixes.count(toLower(fname.substr(0, dot))) > 0)
        return fname.substr(dot + 1);

    // Unknown layout: keep the whole filename. It is unique within its
    // directory, so the dedup key can never wrongly fold distinct songs.
    return fname;
}

bool MusicDatabase::parseStandard(
    Variables& vars, std::string const& listFile,
    std::function<void(SongInfo const&)> const& callback)
{

    int pathIndex = 4, gameIndex = 1, titleIndex = 0, composerIndex = 2,
        formatIndex = 3, metaIndex = 5, extIndex = -1;
    auto templ = vars["song_template"];
    // if(temp == "")
    //  templ = "title game composer format path meta";
    auto format = vars["format"];
    auto composer = vars["composer"];
    int columns = 2;
    if (templ != "") {
        formatIndex = gameIndex = composerIndex = extIndex = -1;
        int i = 0;
        std::vector<std::string> parts = split(templ, " ");
        for (auto const& p : parts) {
            if (p == "title")
                titleIndex = i;
            else if (p == "composer")
                composerIndex = i;
            else if (p == "path")
                pathIndex = i;
            else if (p == "format")
                formatIndex = i;
            else if (p == "game")
                gameIndex = i;
            else if (p == "ext")
                extIndex = i;
            i++;
        }
        columns = i;
    }

    bool isUtf8 = (vars["utf8"] != "no");
    bool htmlDec = (vars["html_decode"] != "no");
    auto source = vars["source"];

    // UnExoticA's "title" column is a verbose "Game/Song/filename" path and the
    // "game" column repeats the game name. Rather than show one verbose row per
    // tune, collapse every tune of a game into a single "MULTI:" entry titled
    // with the game name (see the grouping loop below): the search results then
    // list one row per game that the user steps through with left/right, like a
    // multi-subsong file. A single-tune game stays a normal row titled
    // "<game>-<songname>" (songname = filename with its format prefix stripped).
    bool unexotica = (vars["id"] == "unexotica");

    File f{ listFile };

    // Pre-pass: determine which leading filename tokens are genuine format
    // prefixes. A token qualifies only if it heads files in at least two
    // different game directories - real prefixes (mod, mdat, cust...) span many
    // games, whereas a songname that happens to contain a dot does not.
    std::set<std::string> prefixes;
    if (unexotica) {
        std::map<std::string, std::set<std::string>> tokenDirs;
        for (auto const& s : f.getLines()) {
            std::vector<std::string> parts = split(s, "\t");
            if ((int)parts.size() <= pathIndex) continue;
            std::string const& path = parts[pathIndex];
            auto slash = path.find_last_of('/');
            std::string dir =
                (slash != std::string::npos) ? path.substr(0, slash) : "";
            std::string fname =
                (slash != std::string::npos) ? path.substr(slash + 1) : path;
            if (hasAudioExtension(fname, fname.find_last_of('.'))) continue;
            auto dot = fname.find('.');
            if (dot == std::string::npos) continue;
            tokenDirs[toLower(fname.substr(0, dot))].insert(dir);
        }
        for (auto const& [tok, dirs] : tokenDirs)
            if (dirs.size() >= 2) prefixes.insert(tok);
    }

    // UnExoticA grouping state: every tune of a game is collapsed into a single
    // "MULTI:" entry (titled with the game name) so the search results show one
    // row per game that the user steps through with left/right, exactly like a
    // multi-subsong file. `cur` buffers the entry being built; consecutive rows
    // with the same game+composer are appended to it.
    SongInfo cur;
    bool curValid = false;
    std::string groupGame, groupComposer;
    std::set<std::string> groupSongs; // songnames already in the group (dedup)

    auto flush = [&]() {
        if (curValid) callback(cur);
        curValid = false;
        groupSongs.clear();
    };

    for (auto const& s : f.getLines()) {
        std::vector<std::string> parts =
            isUtf8 ? split(s, "\t") : split(utf8_encode(s), "\t");
        if (parts.size() >= columns) {

            if (htmlDec) {
                for (auto& p : parts)
                    p = htmldecode(p);
            }

            SongInfo song;
            std::string metadata;

            // Strip sorce from path if necessary
            if (source != "" && parts[pathIndex].find(source) == 0)
                parts[pathIndex] = parts[pathIndex].substr(source.length());

            if (parts.size() > metaIndex) metadata = parts[metaIndex];

            std::string gameField = gameIndex >= 0 ? parts[gameIndex] : "";
            std::string titleField = parts[titleIndex];
            std::string composerField =
                composerIndex >= 0 ? parts[composerIndex] : composer;
            std::string formatField =
                formatIndex <= 0 ? format : parts[formatIndex];
            // `podcast = "yes"` marks a standard (.txt) collection as a podcast
            // show (Demovibes, AmigaVibes, Syntax Error): force the PODCAST
            // format byte regardless of the per-song/collection codec tag, so
            // these land in the Podcast F9 category and use the podcast
            // playback/scroll paths -- without the RSS `type = "podcast"` parser.
            if (vars["podcast"] == "yes") formatField = "Podcast";

            if (!unexotica) {
                song = SongInfo(parts[pathIndex], gameField, titleField,
                                composerField, formatField, metadata,
                                extIndex >= 0 ? parts[extIndex] : "");
                callback(song);
                continue;
            }

            // --- UnExoticA path: derive a clean song name, then group ---
            std::string fname = parts[pathIndex];
            auto slash = fname.find_last_of('/');
            if (slash != std::string::npos) fname = fname.substr(slash + 1);
            std::string songName = unexoticaSongName(fname, prefixes);

            std::string singleTitle =
                (gameField != "" && songName != "")
                    ? gameField + "-" + songName
                    : (songName != "" ? songName : gameField);

            bool sameGroup = curValid && gameField != "" &&
                             gameField == groupGame &&
                             composerField == groupComposer;

            if (sameGroup) {
                // Fold version-duplicates (AGA/ECS/OCS rips share the songname).
                std::string key = toLower(songName);
                if (!key.empty() && groupSongs.count(key) > 0) continue;
                groupSongs.insert(key);

                // Promote the buffered single entry to a MULTI entry the first
                // time a second tune shows up; its title becomes the game name.
                if (!startsWith(cur.path, "MULTI:")) {
                    cur.path = std::string("MULTI:") + cur.path;
                    cur.title = groupGame;
                }
                cur.path += "\t" + parts[pathIndex];
            } else {
                flush();
                cur = SongInfo(parts[pathIndex], "", singleTitle, composerField,
                               formatField, metadata,
                               extIndex >= 0 ? parts[extIndex] : "");
                curValid = true;
                groupGame = gameField;
                groupComposer = composerField;
                if (!songName.empty()) groupSongs.insert(toLower(songName));
            }
        }
    }
    flush();
    return true;
}

void MusicDatabase::initDatabase(utils::path const& workDir, Variables& vars)
{

    auto id = vars["id"];
    auto type = vars["type"];
    if (type == "") type = id;
    auto name = vars["name"];
    auto source = vars["source"];
    auto screen_source = vars["screen_source"];
    utils::path local_dir = vars["local_dir"];
    auto song_list = vars["song_list"];
    auto prod_list = vars["prod_list"];
    auto remote_list = vars["remote_list"];
    auto description = vars["description"];

    LOGD("Checking %s", name);

    // Return if this collection has already been indexed in this version
    auto cq =
        db.query<uint64_t>("SELECT ROWID FROM collection WHERE id = ?", id);
    if (cq.step()) {
        return;
    }
    cq.finalize();

    reindexNeeded = true;

    if (!local_dir.empty()) {
        if (!local_dir.is_absolute()) local_dir = workDir / local_dir;
    }

    uint32_t localCount = 0;

    if (source == "") source = screen_source;

    db.exec("BEGIN TRANSACTION");
    // Store the raw relative local_dir from vars so the DB is portable across
    // install locations (dev tree, /Applications, etc.). generateIndex()
    // resolves it against the current workDir at runtime.
    db.exec("INSERT INTO collection (name, id, url, localdir, description, "
            "artwork) VALUES (?, ?, ?, ?, ?, ?)",
            name, id, source, vars["local_dir"], description, vars["artwork"]);
    auto collection_id = db.last_rowid();
    dontIndex.resize(collection_id + 1);
    dontIndex[collection_id] = 0;

    if (vars["index"] == "no") {
        LOGD("Not indexing %s/%d", id, collection_id);
        dontIndex[collection_id] = 1;
    }

    LOGD("Workdir:%s", workDir);
    File listFile;
    bool writeListFile = false;
    webutils::Web web{ (Environment::getCacheDir() / "_webfiles").string() };

    bool prodCollection = false;

    if (prod_list != "") {
        song_list = prod_list;
        prodCollection = true;
    }

    if (song_list == "") song_list = remote_list;

    if (startsWith(song_list, "http://") || startsWith(song_list, "https://")) {
        listFile = web.getFileBlocking(song_list);
    } else if (type == "podcast" && song_list != "") {
        // Podcasts index the writable cache copy (back catalogue + any episodes
        // a background refresh has merged), seeded from the shipped file.
        listFile = File(podcastSource(workDir, id, song_list));
    } else if (song_list != "") {
        listFile = File(workDir.string(), song_list);
        writeListFile = listFile.exists();
    }

    if (prodCollection) {

        auto query = db.query("INSERT INTO product (title, creator, type, "
                              "screenshots, collection) "
                              "VALUES (?, ?, ?, ?, ?)");

        auto query2 = db.query("INSERT INTO prod2song (prodid, songid) "
                               "VALUES (?, ?)");

        std::map<std::string, ParseProdFun> parsers = {
            { "csdb", &MusicDatabase::parseCsdb },
            { "gb64", &MusicDatabase::parseGamebase },
            { "bitworld", &MusicDatabase::parseBitworld },
        };

        auto parser = parsers[type];
        // if(!parser)
        // parser = &MusicDatabase::parseStandard;
        LOGD("Parsing %s from %s", type, listFile.getName());

        (this->*parser)(vars, listFile.getName(), [&](Product const& prod) {
            query
                .bind(prod.title, prod.creator, prod.type, prod.screenshots,
                      collection_id)
                .step();
            localCount++;
            totalSongs++;
            auto prodrow = db.last_rowid();
            for (std::string path : prod.songs) {
                // TODO: Move to CORRECTIONS.LUA or something
                auto pos = path.find("Zombie (FI)");
                if (pos != std::string::npos)
                    path = path.substr(0, pos) + "Naksahtaja" +
                           path.substr(pos + 11);
                uint64_t hash = MD5::hash(toLower(path));
                auto it = pathMap.find(hash);
                if (it == pathMap.end()) {
                    LOGV("PATH '%s' not found", path);
                } else {
                    auto songrow = it->second;
                    query2.bind(prodrow, songrow).step();
                }
            }
        });
    } else {
        auto query = db.query("INSERT INTO song (title, game, composer, "
                              "format, path, collection, metadata, ext, "
                              "artwork) "
                              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");

        if (utils::exists(listFile.getName())) {

            std::map<std::string, ParseSongFun> parsers = {
                { "pouet", &MusicDatabase::parseStandard },
                { "amp", &MusicDatabase::parseAmp },
                { "modland", &MusicDatabase::parseModland },
                { "podcast", &MusicDatabase::parseRss },
                { "standard", &MusicDatabase::parseStandard },
            };

            auto parser = parsers[type];
            if (!parser) parser = &MusicDatabase::parseStandard;

            (this->*parser)(vars, listFile, [&](SongInfo const& song) {
                query
                    .bind(song.title, song.game, song.composer, song.format,
                          song.path, collection_id,
                          song.metadata[SongInfo::INFO] != ""
                              ? song.metadata[SongInfo::INFO].c_str()
                              : nullptr,
                          song.ext != "" ? song.ext.c_str() : nullptr,
                          song.metadata[SongInfo::SCREENSHOT] != ""
                              ? song.metadata[SongInfo::SCREENSHOT].c_str()
                              : nullptr)
                    .step();
                localCount++;
                totalSongs++;
                auto last = db.last_rowid();
                if (collection_id == 6) LOGV("Inserting '%s'", song.path);
                auto hash = MD5::hash(utils::toLower(song.path));
                pathMap[hash] = last;
            });

        } else if (utils::exists(local_dir)) {

            File root{ local_dir };
            LOGD("Checking local dir '%s'", root.getName());
            for (auto& rf : root.listRecursive()) {
                auto name = rf.getName();
                SongInfo songInfo(name);
                if (identify_song(songInfo)) {

                    auto pos = name.find(local_dir.string());
                    if (pos != std::string::npos) {
                        name = name.substr(pos + local_dir.string().length());
                    }

                    query
                        .bind(songInfo.title, songInfo.game, songInfo.composer,
                              songInfo.format, name, collection_id,
                              (char*)nullptr)
                        .step();
                    localCount++;
                    totalSongs++;

                    if (writeListFile)
                        listFile.writeln(join("\t", songInfo.title,
                                              songInfo.game, songInfo.composer,
                                              songInfo.format, name));
                }
            }
        }
    }

    listFile.close();
    db.exec("COMMIT");

    std::string usedFile = song_list;
    if (usedFile.empty()) usedFile = vars["local_dir"];

    print_fmt("Creating '%s' DB, source: %s, songs count: %d\n", name,
              usedFile, localCount);
}

void MusicDatabase::setFilter(std::string const& collection, int type)
{

    if (collection == "") {
        titleIndex.setFilter();
        collectionFilter = -1;
    } else {
        LOGD("FILTER: '%s'", collection);
        auto cq = db.query<int>("SELECT ROWID FROM collection WHERE id = ?",
                                collection);
        if (cq.step()) {
            collectionFilter = cq.get();
            LOGD("ID %d from %s", collectionFilter, collection);
            // collectionFilter = 2;
            titleIndex.setFilter([=](int index) {
                auto f = formats[index];
                if (type == 1 && (f & 0xff) == PRODUCT) return false;
                return ((formats[index] >> 8) != collectionFilter);
            });
        }
    }
}

// Map a product's free-text `type` (e.g. "C64 Game", "Amiga Demo", csdb
// "Music Collection") to a platform format byte, so collections obey the F9
// platform filter. Returns 0 (unknown) when no platform is recognised, which
// causes the product to be hidden whenever any platform filter is active.
static uint8_t productTypeToPlatform(std::string const& type)
{
    std::string t = toLower(type);
    if (startsWith(t, "amiga")) return AMIGA;
    // gamebase ("C64 Game") and csdb releases (Music Collection / Diskmag /
    // Demo, sourced from HVSC) are all Commodore 64 SID.
    if (t.find("c64") != std::string::npos ||
        t.find("commodore 64") != std::string::npos ||
        endsWith(t, "music collection") || endsWith(t, "diskmag") ||
        endsWith(t, "demo"))
        return SID;
    return 0;
}

void MusicDatabase::setFormatFilter(std::vector<uint8_t> const& allowedFormats)
{
    filterHueRank.clear();
    filterHueCount = 0;
    // Reset podcast-browse state on every filter change; rebuilt below when the
    // podcast filter is the one being activated.
    podcastFilterActive = (allowedFormats.size() == 1 &&
                           allowedFormats[0] == PODCAST);
    podcastShowFilter = -1;
    podcastShowList.clear();
    if (allowedFormats.empty()) {
        titleIndex.setFilter();
        formatFilterActive = false;
        filteredCandidates.clear();
        filteredCandidates.shrink_to_fit();
    } else {
        titleIndex.setFilter([=](int index) {
            auto f = formats[index];
            uint8_t fmtByte = f & 0xff;
            if (fmtByte == PRODUCT) {
                // Products (collections) carry their platform separately.
                int ord = index - static_cast<int>(productStartIndex);
                uint8_t plat = (ord >= 0 && ord < (int)productPlatform.size())
                                   ? productPlatform[ord]
                                   : 0;
                for (auto const& allowed : allowedFormats)
                    if (plat == allowed) return false; // keep
                return true;                            // exclude
            }
            for (auto const& allowed : allowedFormats) {
                if (fmtByte == allowed) {
                    return false;
                }
            }
            return true;
        });

        // Precompute the indices that pass the filter so short queries can scan
        // them directly (see MusicDatabase::search). Cheap: one pass over the
        // title index per F9 selection.
        formatFilterActive = true;
        filteredCandidates.clear();
        uint32_t n = titleIndex.size();
        std::set<uint16_t> hues;
        for (uint32_t i = 0; i < n; i++) {
            if (titleIndex.isFiltered(i)) continue;
            filteredCandidates.push_back(i);
            // Collect the distinct song sub-format keys present (skip products,
            // which carry the neutral 0) to rank them for an even hue spread.
            if (i < productStartIndex) hues.insert(formatHue[i]);
        }
        int rank = 0;
        for (uint16_t h : hues) filterHueRank[h] = rank++;
        filterHueCount = (int)hues.size();

        // Build the podcast show list (distinct collections among the podcast
        // episodes), names from the collection table, sorted alphabetically.
        if (podcastFilterActive) {
            std::set<int> shows;
            for (int idx : filteredCandidates)
                shows.insert(formats[idx] >> 8);
            for (int rowid : shows) {
                std::string name;
                auto q = db.query<std::string>(
                    "SELECT name FROM collection WHERE ROWID = ?", rowid);
                if (q.step()) name = q.get();
                podcastShowList.emplace_back(rowid, name);
            }
            std::sort(podcastShowList.begin(), podcastShowList.end(),
                      [](auto const& a, auto const& b) {
                          return toLower(a.second) < toLower(b.second);
                      });
        }
    }
}

int MusicDatabase::search(std::string const& query, std::vector<int>& result,
                          unsigned int searchLimit)
{

    std::lock_guard lock{ dbMutex };

    result.resize(0);
    std::set<std::string> seen;

    auto add_unique = [&](int index) {
        if (result.size() >= searchLimit) return false;

        std::string identity;
        if (index >= PODCAST_SHOW_INDEX) {
            identity = "SHOW:" + std::to_string(index - PODCAST_SHOW_INDEX);
        } else if (index >= PLAYLIST_INDEX) {
            identity = "PL:" + playLists[index - PLAYLIST_INDEX].name;
        } else {
            std::string title = titleIndex.getString(index);
            std::string composer =
                composerIndex.getString(titleToComposer[index]);
            uint8_t fmt = formats[index] & 0xff;
            identity = title + "\t" + composer + "\t" + std::to_string(fmt);
        }

        if (seen.find(identity) == seen.end()) {
            result.push_back(index);
            seen.insert(identity);
            return true;
        }
        return false;
    };

    std::string title_query = query;
    std::string composer_query = query;

    auto p = split(query, "/");
    if (p.size() > 1) {
        title_query = p[0];
        composer_query = p[1];
    }

    // Empty query: normally blank (the user types to search). But when a *small*
    // platform filter is active (fewer than kFilterShowAllLimit songs), a tiny
    // format isn't worth typing for -- so list ALL of its songs up front, sorted
    // alphabetically by title. Large filters / no filter stay blank.
    static constexpr size_t kFilterShowAllLimit = 1000;
    if (query == "") {
        // Podcasts browse: with the Podcasts filter active and no drilled-in
        // show, list the shows themselves (one synthetic row each). Drilled into
        // a show, list that show's episodes in feed order (each show is small,
        // so this ignores the show-all size limit).
        if (podcastFilterActive && podcastShowFilter < 0) {
            for (auto const& s : podcastShowList)
                if (!add_unique(PODCAST_SHOW_INDEX + s.first) &&
                    result.size() >= searchLimit)
                    break;
            return result.size();
        }
        if (podcastFilterActive && podcastShowFilter >= 0) {
            for (int idx : filteredCandidates)
                if ((formats[idx] >> 8) == podcastShowFilter)
                    if (!add_unique(idx) && result.size() >= searchLimit) break;
            return result.size();
        }
        if (formatFilterActive && !filteredCandidates.empty() &&
            filteredCandidates.size() < kFilterShowAllLimit) {
            std::vector<int> sorted(filteredCandidates.begin(),
                                    filteredCandidates.end());
            std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
                return toLower(titleIndex.getString(a)) <
                       toLower(titleIndex.getString(b));
            });
            for (int idx : sorted) {
                if (!add_unique(idx) && result.size() >= searchLimit) break;
            }
        }
        return result.size();
    }

    // Push back all matching playlists (unless a platform filter is active)
    if (!formatFilterActive) {
        for (int i = 0; i < (int)playLists.size(); i++) {
            if (toLower(playLists[i].name).find(query) != std::string::npos)
                add_unique(PLAYLIST_INDEX + i);
        }
    }

    // Short queries (< 3 chars) can't use the 3-letter substring buckets: for
    // 1-2 char queries those buckets only hold strings with a standalone short
    // word, so under a restrictive platform filter almost nothing matches until
    // the 3rd letter. When a filter is active, scan the precomputed (small)
    // filtered candidate set directly instead, matching title or composer, so
    // results appear from the first keystroke. Early-exits at searchLimit, so it
    // stays fast even for large filters.
    if (formatFilterActive && !title_query.empty() && title_query.size() < 3) {
        std::string tq = title_query;
        SearchIndex::simplify(tq);
        std::string cq = composer_query;
        SearchIndex::simplify(cq);
        for (int index : filteredCandidates) {
            std::string title = titleIndex.getString(index);
            SearchIndex::simplify(title);
            bool match = title.find(tq) != std::string::npos;
            if (!match) {
                std::string comp =
                    composerIndex.getString(titleToComposer[index]);
                SearchIndex::simplify(comp);
                match = comp.find(cq) != std::string::npos;
            }
            if (match && !add_unique(index) && result.size() >= searchLimit)
                break;
        }
        return result.size();
    }

    std::vector<int> tresult;
    titleIndex.search(title_query, tresult, searchLimit);
    for (int index : tresult) {
        if (!add_unique(index))
            if (result.size() >= searchLimit) break;
    }

    if (result.size() >= searchLimit) return result.size();

    std::vector<int> cresult;
    composerIndex.search(composer_query, cresult, searchLimit);
    for (int index : cresult) {
        int offset = composerTitleStart[index];
        while (composerToTitle[offset] != -1) {
            int songindex = composerToTitle[offset++];

            //if (seen.find(songindex) != seen.end()) continue;

            if (collectionFilter == -1 ||
                (formats[songindex] >> 8) == collectionFilter) {
                if (!titleIndex.isFiltered(songindex)) {
                    if (!add_unique(songindex))
                        if (result.size() >= searchLimit) break;
                }
            }
        }
        if (result.size() >= searchLimit) break;
    }

    return result.size();
}

// Lookup the given path in the database
SongInfo& MusicDatabase::lookup(SongInfo& song)
{

    std::lock_guard lock{ dbMutex };
    auto path = song.path;

    std::vector<std::string> parts = split(path, "::");
    if (parts.size() > 1) {
        path = parts[1];
        if (parts[0] == "index") {
            int index = stol(path);
            SongInfo song = getSongInfo(index);
            path = song.path;
            parts = split(path, "::");
            if (parts.size() > 1) {
                path = parts[1];
            }
        }
        LOGV("INDEX %s %s", parts[0], path);
    }

    auto q = db.query<std::string, std::string, std::string, std::string,
                      std::string, std::string, std::string, std::string,
                      std::string>(
        "SELECT path, title, game, composer, format, collection.id, metadata, "
        "ext, song.artwork "
        "FROM song, collection "
        "WHERE song.collection = collection.ROWID AND song.path = ?",
        path);

    if (q.step()) {
        std::string coll;
        tie(song.path, song.title, song.game, song.composer, song.format, coll,
            song.metadata[SongInfo::INFO], song.ext,
            song.metadata[SongInfo::SCREENSHOT]) = q.get_tuple();
        song.path = coll + "::" + song.path;
        //LOGD("LOOKUP '%s' became '%s'", path, song.path);
    } else {
        //LOGD("TODO: Check products");
    }

    return song;
}

std::string MusicDatabase::getScreenshotURL(std::string const& collection)
{
    std::string prefix;
    auto q = db.query<std::string>("SELECT url FROM collection WHERE id = ?",
                                   collection);
    if (q.step()) prefix = q.get();
    return prefix;
}

// Get SongInfo from the search result
SongInfo MusicDatabase::getSongInfo(int index) const
{

    if (index >= PODCAST_SHOW_INDEX) {
        // A podcast SHOW row: title = show name, path carries the collection
        // ROWID so the UI can drill into its episodes (it is not playable).
        int rowid = index - PODCAST_SHOW_INDEX;
        std::string name;
        {
            std::lock_guard lock{ dbMutex };
            for (auto const& s : podcastShowList)
                if (s.first == rowid) name = s.second;
        }
        return SongInfo("podcastshow::" + std::to_string(rowid), "", name, "",
                        "Podcast");
    }

    if (index >= PLAYLIST_INDEX) {
        std::string p = playLists[index - PLAYLIST_INDEX].name;
        auto path = Environment::getConfigDir() / "playlists" / p;
        return SongInfo("playlist::" + path.string(), "", p, "",
                        "Local playlist");
    }

    index++;
    // LOGD("ID %d vs PROD %d", index, productStartIndex);
    if (index >= productStartIndex) {
        // index is now ordinal+1 (the ++ above); map the ordinal to the real
        // product ROWID -- it is NOT the ordinal because single-song products
        // are skipped during indexing.
        int ord = index - productStartIndex - 1;
        int rowid = (ord >= 0 && ord < (int)productRowid.size())
                        ? productRowid[ord]
                        : index - productStartIndex;
        auto q = db.query<std::string, std::string, std::string, std::string,
                          std::string>(
            "SELECT title, creator, type, collection.id, metadata "
            "FROM  product, collection "
            "WHERE product.ROWID = ? AND product.collection = collection.ROWID",
            rowid);
        if (q.step()) {
            SongInfo song;
            std::string collection;
            tie(song.title, song.composer, song.format, collection,
                song.metadata[SongInfo::INFO]) = q.get_tuple();
            song.path = "product::" + std::to_string(rowid);
            return song;
        }

    } else {

        auto q = db.query<std::string, std::string, std::string, std::string,
                          std::string, std::string, std::string, std::string>(
            "SELECT title, game, composer, format, song.path, "
            "collection.id, metadata, ext "
            "FROM song, collection "
            "WHERE song.ROWID = ? AND song.collection = collection.ROWID",
            index);
        if (q.step()) {
            SongInfo song;
            std::string collection;
            tie(song.title, song.game, song.composer, song.format, song.path,
                collection, song.metadata[SongInfo::INFO],
                song.ext) = q.get_tuple();
            song.path = collection + "::" + song.path;
            return song;
        }
    }
    throw not_found_exception();
}
// Lazily load data/<collection>_screenshots.txt ("<song-path><TAB>url") on first
// use, caching per collection. Caller must hold screenshotMutex
// (getSongScreenshots does). Used by collections whose art is matched offline
// against an external DB (hvtc -> Plus/4 World, sndh -> Atari Mania), both served
// via the Wayback mirror.
std::map<std::string, std::string> const& MusicDatabase::getFileShots(
    std::string const& collection)
{
    auto it = fileShots.find(collection);
    if (it != fileShots.end()) return it->second;

    auto& m = fileShots[collection];   // inserts empty map (the "loaded" marker)
    File f{ workDir.string(), "data/" + collection + "_screenshots.txt" };
    if (f.exists()) {
        for (auto const& line : f.getLines()) {
            auto tab = line.find('\t');
            if (tab != std::string::npos)
                m[line.substr(0, tab)] = line.substr(tab + 1);
        }
        LOGD("Loaded %d %s screenshots", (int)m.size(), collection.c_str());
    }
    return m;
}

std::string MusicDatabase::getSongScreenshots(SongInfo& s)
{
    // Called from a detached thread in MusicPlayerList. lookup() has already
    // been called on the worker thread (safe, main db) before dispatch, so we
    // skip it here. All db access here uses screenshotDb — a dedicated
    // read-only connection — under screenshotMutex to avoid races with the
    // main db connection.
    std::lock_guard<std::mutex> lock(screenshotMutex);
    if (!screenshotDb) {
        screenshotDb = std::make_unique<sqlite3db::Database>(
            (Environment::getCacheDir() / "music.db").string());
    }
    auto& sdb = *screenshotDb;

    auto parts = split(s.path, "::");
    if (parts.size() < 2) return "";
    std::string collection = parts[0];
    std::string shot;
    std::string title;
    std::string baseName = path_basename(parts[1]);
    LOGV("Get screenhots / Path %s Collection '%s'", parts[1], parts[0]);
    if (s.metadata[SongInfo::SCREENSHOT] != "") {
        shot = s.metadata[SongInfo::SCREENSHOT];
    } else if (collection == "rsn") {
        auto base = path_basename(parts[1]);
        shot = std::string("http://snesmusic.org/v2/images/screenshots/") +
               base + ".png";
        s.metadata[SongInfo::SCREENSHOT] = shot;
        LOGV("Got rsn shot %s", shot);
    } else if (collection == "pouet" || collection == "radio" ||
               collection == "demovibes") {
        shot = s.metadata[SongInfo::INFO];
        s.metadata[SongInfo::SCREENSHOT] = shot;
        s.metadata[SongInfo::INFO] = "";
        LOGV("Got pouet shot %s", shot);
    } else if (collection == "hvtc" || collection == "sndh" ||
               collection == "unexotica" || collection == "modland" ||
               collection == "zxart") {
        // Game screenshots matched offline against an external database and
        // served via the Wayback mirror: hvtc -> Plus/4 World (keyed by
        // "games/<name>.prg"), sndh -> Atari Mania (keyed by
        // "<composer>/<game>.sndh"), unexotica -> Hall of Light (keyed by the
        // per-game "/Game/<composer>/<game>.lha" identifier), modland -> the ZX
        // Spectrum subset matched against ZXDB / World of Spectrum (keyed by the
        // full song path), zxart -> zxart.ee ZX game tunes matched against ZXDB
        // by game title (keyed by the full zxart.ee song URL). Full URLs in
        // data/<file>_screenshots.txt; no match -> blank (most songs aren't ZX
        // games, so they get nothing).
        std::string key = parts[1];
        // The ZX screenshots only cover the modland ZX-AY subset, kept in its
        // own data file rather than a giant modland_screenshots.txt.
        std::string file = (collection == "modland") ? "zxspectrum" : collection;
        if (collection == "unexotica") {
            // The song path is one (or a MULTI: list of) module path(s) inside
            // the game's .lha; reduce it to the shared "/Game/.../<game>.lha"
            // identifier that the screenshot map is keyed on.
            auto game = key.find("/Game/");
            auto lha = key.find(".lha");
            if (game != std::string::npos && lha != std::string::npos)
                key = key.substr(game, lha + 4 - game);
        }
        auto const& shots = getFileShots(file);
        auto it = shots.find(key);
        if (it != shots.end()) {
            shot = it->second;
            s.metadata[SongInfo::SCREENSHOT] = shot;
            LOGV("Got %s shot %s", collection, shot);
        }
    } else if (classifyFormat(s.format, s.path) == PODCAST) {
        // Podcast episode artwork. Prefer per-episode art where the enclosure
        // URL makes it derivable; otherwise fall back to the show's
        // representative image (collection.artwork, set in db.lua).
        std::string const& url = parts[1];
        auto dot = url.rfind('.');
        if (url.find("archive.org/") != std::string::npos &&
            (endsWith(url, ".m4a") || endsWith(url, ".mp3")) &&
            dot != std::string::npos) {
            // archive.org rips ship a sibling PNG (same basename, .png ext).
            shot = url.substr(0, dot) + ".png";
        } else if (url.find("youtube.com/") != std::string::npos ||
                   url.find("youtu.be/") != std::string::npos) {
            // YouTube thumbnail, keyed by the 11-char video id.
            std::string id;
            auto v = url.find("v=");
            if (v != std::string::npos)
                id = url.substr(v + 2, 11);
            else {
                auto sl = url.rfind('/');
                if (sl != std::string::npos) id = url.substr(sl + 1, 11);
            }
            if (id.size() == 11)
                shot = "https://img.youtube.com/vi/" + id + "/hqdefault.jpg";
        }
        if (shot.empty()) {
            // No per-episode art -> the show's representative artwork.
            auto q = sdb.query<std::string>(
                "SELECT artwork FROM collection WHERE id = ?", collection);
            if (q.step()) shot = q.get();
        }
        s.metadata[SongInfo::SCREENSHOT] = shot;
        LOGV("Got podcast shot %s", shot);
    } else {
        auto q = sdb.query<std::string, std::string, std::string, std::string>(
            "SELECT product.title, product.screenshots, product.type, "
            "collection.id "
            "FROM product, prod2song, song, collection "
            "WHERE product.rowid = prod2song.prodid AND prod2song.songid = "
            "song.ROWID AND "
            "product.collection = collection.ROWID AND song.path = ?",
            parts[1]);
        std::string format;
        int lowestDist = 999999;
        collection = "";
        while (q.step()) {
            std::string s, c;
            tie(title, s, format, c) = q.get_tuple();
            LOGV("%s Collection %s Format %s", title, c, format);
            auto ld = levenshteinDistance(title, baseName);
            if (collection == "gb64" && c == "csdb") ld += 7;
            LOGV("%s <=> %s : %d", title, baseName, ld);
            if (ld < lowestDist) {
                shot = s;
                collection = c;
                lowestDist = ld;
            }
        }
        if (lowestDist > static_cast<int>(baseName.length())) {
            shot = "";
            LOGV("Screenshot match too weak (%d), skipping", lowestDist);
        }
    }
    if (shot != "") {
        std::string prefix;
        if (!startsWith(shot, "http")) {
            // getScreenshotURL uses sdb (safe — same dedicated connection)
            auto q = sdb.query<std::string>(
                "SELECT url FROM collection WHERE id = ?", collection);
            if (q.step()) prefix = q.get();
        }
        std::vector<std::string> parts = split(shot, ";");
        if (collection == "gb64")
            parts.insert(parts.begin(), path_directory(parts[0]) + "/" +
                                            path_basename(parts[0]) + "_1." +
                                            path_extension(parts[0]));
        for (auto& p : parts) {
            if (p != "") p.insert(0, prefix);
        }
        shot = join(parts.begin(), parts.end(), ";");
    }
    return shot;
}

std::string MusicDatabase::getProductScreenshots(uint32_t id)
{
    std::vector<std::string> shots;
    auto q = db.query<std::string, std::string>(
        "SELECT collection.id,screenshots "
        "FROM product, collection "
        "WHERE product.rowid = ? AND collection.ROWID = product.collection",
        id);

    std::string screenshot;
    std::string collection;

    if (q.step()) {
        tie(collection, screenshot) = q.get_tuple();
        auto prefix = getScreenshotURL(collection);
        std::vector<std::string> parts = split(screenshot, ";");
        if (collection == "gb64")
            parts.push_back(path_basename(parts[0]) + "_1." +
                            path_extension(parts[0]));
        for (auto& p : parts) {
            p.insert(0, prefix);
        }
        return join(parts.begin(), parts.end(), ";");
    }
    return "";
}

std::vector<SongInfo> MusicDatabase::getProductSongs(uint32_t id)
{
    std::vector<SongInfo> songs;
    auto screenshot = getProductScreenshots(id);
    auto q = db.query<std::string, std::string, std::string, std::string,
                      std::string, std::string, std::string, std::string>(
        "SELECT title, game, composer, format, song.path, collection.id, "
        "metadata, ext "
        "FROM song, prod2song, collection "
        "WHERE prodid = ? AND songid = song.ROWID AND song.collection = "
        "collection.ROWID",
        id);

    while (q.step()) {
        SongInfo song;
        std::string collection;
        tie(song.title, song.game, song.composer, song.format, song.path,
            collection, song.metadata[SongInfo::INFO],
            song.ext) = q.get_tuple();
        song.path = collection + "::" + song.path;
        song.metadata[SongInfo::SCREENSHOT] = screenshot;
        songs.push_back(song);
    }
    return songs;
}

#include "formats.h"

static std::map<std::string, uint8_t> format_map;

void initFormats()
{
    for (char const* f : uade_formats) {
        format_map[f] = UADE;
    }
    for (char const* f : adlib_formats) {
        format_map[f] = ADPLUG;
    }

    format_map["commodore 64"] = SID;
    format_map["cyber tracker"] = SID;
    // Stereo Sidplayer is a C64 stereo SID format (played via UADE). Override
    // the uade_formats default so it groups/colours as Commodore 64 rather than
    // Amiga, and is reachable from the "Commodore 64" platform filter.
    format_map["stereo sidplayer"] = STR;
    // Commodore TED (16/116/+4) .prg tunes -- identify_song() tags these "TED".
    format_map["ted"] = PRG;

    // --- ZX Spectrum 16/48 beeper (1-bit) ---
    // beepola/picatune2 are listed in uade_formats (default UADE/Amiga); these
    // explicit entries override that so they group as ZX Spectrum, not Amiga.
    format_map["beepola"] = ZXBEEPER;
    format_map["picatune2"] = ZXBEEPER;

    // --- ZX Spectrum 128 AY/YM ---
    // All of these modland format labels live exclusively under "Spectrum/", so
    // mapping by string can't pull in Amstrad CPC (Starkos/ArkosTracker), MSX or
    // Atari-ST YM tunes -- those keep their own platforms and stay out of the ZX
    // AY filter. Names like "Pro Tracker 3"/"Sound Tracker Pro 2" are the ZX AY
    // formats and are distinct strings from the Amiga "Protracker"/"Soundtracker
    // Pro II" labels.
    format_map["ay emul"] = ZXAY;
    format_map["ay amadeus"] = ZXAY;
    format_map["ay strc"] = ZXAY;
    format_map["fuxoft ay language"] = ZXAY;
    format_map["zxs"] = ZXAY;
    format_map["pro tracker 1"] = ZXAY;
    format_map["pro tracker 2"] = ZXAY;
    format_map["pro tracker 3"] = ZXAY;
    format_map["st song compiler"] = ZXAY;
    format_map["asc sound master"] = ZXAY;
    format_map["sq tracker"] = ZXAY;
    format_map["vortex"] = ZXAY;
    format_map["vortex tracker ii"] = ZXAY;
    format_map["sound tracker pro"] = ZXAY;
    format_map["sound tracker pro 2"] = ZXAY;
    format_map["sound tracker 1.1"] = ZXAY;
    format_map["sound tracker 1.3"] = ZXAY;
    format_map["pro sound creator"] = ZXAY;
    format_map["pro sound maker"] = ZXAY;
    format_map["pro sound maker 1.0"] = ZXAY;
    format_map["flash tracker"] = ZXAY;
    format_map["fast tracker"] = ZXAY; // ZX AY .ftc, NOT Amiga/PC FastTracker
    format_map["global tracker"] = ZXAY;
    format_map["chip tracker"] = ZXAY;

    // --- MSX (Z80 + AY/SCC/OPLL/FM) ---
    // Several of these (kss, face the music, mvs tracker, the musical
    // enlightenment) are listed in uade_formats; these explicit entries override
    // that Amiga default so they group as MSX.
    format_map["mgsdrv"] = MSX;
    format_map["kss"] = MSX;
    format_map["musica"] = MSX;
    format_map["moonblaster"] = MSX;
    format_map["moonblaster (edit mode)"] = MSX;
    format_map["oplldrv"] = MSX;
    format_map["mpk"] = MSX;
    format_map["scc-musixx"] = MSX;
    format_map["face the music"] = MSX;
    format_map["fac soundtracker"] = MSX;
    format_map["mvs tracker"] = MSX;
    format_map["the musical enlightenment"] = MSX;
    format_map["editeur musical sequentiel"] = MSX;
    format_map["msx1"] = MSX;
    format_map["msx2"] = MSX;

    // --- Amstrad CPC (AY) --- (starkos/arkostracker are in uade_formats)
    format_map["starkos"] = AMSTRAD;
    format_map["arkostracker"] = AMSTRAD;
    // cpc-power.com YM audiotheque: CPC game .ym rips (AY-3-8912). Distinct from
    // the "ym" string (Atari ST) -- these are tagged "Amstrad CPC" by the builder.
    format_map["amstrad cpc"] = AMSTRAD;

    // --- Acorn Archimedes ---
    format_map["digital symphony"] = ACORN;
    format_map["archimedes tracker"] = ACORN;
    format_map["coconizer"] = ACORN;

    // --- zxart.ee music collection: chip-family format strings emitted by
    // tools/build_zxart.py (see that script's classify()). The collection routes
    // by ZX chip type so AY tunes, 1-bit beeper tunes and Sam Coupe SAA tunes
    // land in their own platform filters; unplayable originals fall back to ogg.
    format_map["spectrum ay"] = ZXAY;
    format_map["spectrum beeper"] = ZXBEEPER;
    format_map["sam coupe"] = SAMCOUPE;
    // The existing modland "Sam Coupe COP" (191) / "Sam Coupe SNG" (15) tunes are
    // listed in uade_formats, so they default to UADE/Amiga; override them onto
    // the dedicated Sam Coupe (SAA1099) platform alongside the zxart COP/SAA rips.
    format_map["sam coupe cop"] = SAMCOUPE;
    format_map["sam coupe sng"] = SAMCOUPE;

    format_map["super nintendo"] = SNES;
    format_map["hes"] = HES;
    format_map["mp3"] = MP3;
    format_map["podcast"] = PODCAST; // podcast episodes (RSS / archive.org)

    // --- Atari ST/STE (YM2149) ---
    format_map["sc68"] = ATARI;
    format_map["atari st"] = ATARI; // sndh collection
    format_map["ym"] = ATARI;
    format_map["ymst"] = ATARI;
    // UADE-played Atari ST formats -- override their uade_formats (Amiga)
    // default; the "ST" suffix distinguishes them from the Amiga namesakes
    // ("Special FX", "Quartet", "Jochen Hippel").
    format_map["quartet st"] = ATARI;
    format_map["special fx st"] = ATARI;
    format_map["tcb tracker"] = ATARI;
    format_map["hippel st"] = ATARI;
    // --- Atari XL/XE 8-bit (POKEY), distinct chip from the ST line ---
    // startsWith("atari") would otherwise lump "Atari 8Bit" (.sap) in with the
    // ST tunes; this explicit entry keeps POKEY separate.
    format_map["atari 8bit"] = POKEY;
    format_map["pokeynoise"] = POKEY; // Atari 8-bit POKEY (.pn), not Amiga
    format_map["soundsmith"] = APPLE; // Apple IIgs SoundSmith
    format_map["playerpro"] = APPLE;  // Macintosh PlayerPRO tracker (.mad), overrides uade_formats default
    format_map["jaytrax"] = TRACKER;  // JayTrax (.jxs), cross-platform synth tracker -- not UADE/Amiga
    format_map["ultra64 sound format"] = NINTENDO64;
    format_map["nintendo ds sound format"] = NDS;
    format_map["nintendo sound format"] = NES;
    // Sega 8-bit (SN76489 PSG): Master System, Game Gear, SG-1000, SC-3000
    format_map["sega master system"] = SEGAMS;
    format_map["sega game gear"] = SEGAMS;
    format_map["sega sg-1000"] = SEGAMS;
    format_map["sega sc-3000"] = SEGAMS;
    // Sega 16-bit (Mega Drive/Genesis, YM2612 + SN76489) and its add-ons
    format_map["sega megadrive"] = MEGADRIVE;
    format_map["sega genesis"] = MEGADRIVE; // Zophar Genesis VGM gamerips
    format_map["megadrive gym"] = MEGADRIVE;
    format_map["megadrive cym"] = MEGADRIVE;
    format_map["sega 32x"] = MEGADRIVE;
    format_map["sega mega cd"] = MEGADRIVE;
    format_map["playstation sound format"] = PLAYSTATION;
    format_map["dreamcast sound format"] = DREAMCAST;
    format_map["playlist"] = PLAYLIST;
    format_map["c64 demo"] = PLAYLIST;
    format_map["c64 event"] = PLAYLIST;
    format_map["pls"] = PLS;
    format_map["m3u"] = M3U;

    // -----------------------------------------------------------------------
    // Catch-all classification pass: many songs (especially from ModArchive)
    // carry bare format codes or names not covered by the loops/fallbacks above
    // and would otherwise be UNKNOWN (invisible to every platform filter). Map
    // them to the closest platform so every song is classifiable.
    // -----------------------------------------------------------------------
    // Classic module trackers. MOD is Amiga; XM/IT/S3M are the IBM PC/DOS
    // trackers (FastTracker II / Impulse Tracker / Scream Tracker), which live
    // in the dedicated "IBM PC Trackers" filter (see ChipMachine::filterOptions).
    format_map["mod"] = PROTRACKER;        // Amiga ProTracker module
    format_map["xm"] = FASTTRACKER;        // FastTracker II
    format_map["it"] = IMPULSETRACKER;     // Impulse Tracker
    format_map["s3m"] = SCREAMTRACKER;     // Scream Tracker 3
    format_map["stm"] = SCREAMTRACKER;     // Scream Tracker 2
    // Other IBM PC / DOS trackers -> PCTRACKER (also in "IBM PC Trackers").
    for (char const* f :
         { "mtm", "mo3", "669", "unis 669", "mdl", "gdm", "dmf", "amf", "ptm",
           "ult", "far", "ams", "dsm", "umx", "mptm", "openmpt mptm", "mt2",
           "dsmi compact", "composer 670 (cdfm)", "composer 667", "c67",
           "multitracker", "x-tracker", "polytracker", "ultratracker",
           "digitrakker" })
        format_map[f] = PCTRACKER;
    // Ambiguous/cross-platform trackers -> generic TRACKER (stays under Amiga).
    format_map["dtm"] = TRACKER; // Digital Tracker (Atari Falcon / Amiga)
    format_map["plm"] = TRACKER;
    // Amiga (native trackers, custom players, packers, composer rips).
    for (char const* f :
         { "med",        "octamed mmdc", "mmdc",        "iff-smus",
           "iff-8svx",   "custom",       "his master's noise",
           "hippel coso", "hippel / hippel-coso", "infogrames",
           "noise tracker gs 1", "startrekker", "startrekker flt8",
           "maximum effect", "dbm", "digi booster", "digi", "soundmon 2.0",
           "soundmon 2.2", "bp soundmon 1", "the player 4.x", "the player 5.x",
           "the player 6.x", "sonix music driver", "benn daglish",
           "mikmod unitrk", "tfmx pro", "tfmx 7v", "okt", "martin walker",
           "hvl", "prorunner 2.0", "prorunner 1.0", "buzzic 2", "buzzic",
           "noisepacker 3.x", "noisepacker 2.x", "amos music bank",
           "pixel painters fmf", "pixel painters ftf", "arpeggiator",
           "astroidea xmf", "easytrax", "m.o.n new", "m.o.n old",
           "trackerpacker 3", "musicmaker v8 old", "ac1d-dc1a packer",
           "ashley hogg", "mugician", "mugician ii", "pha packer",
           "propacker 2.1", "synth pack", "alcatraz packer", "digital illusions",
           "digital sound creations", "rob hubbard old", "fred editor",
           "peter verswyvelen packer", "promizer", "sfx", "sidmon ii", "sidmon",
           "actionamics sound tool", "andrew parton", "art & magic",
           "channel players", "cinemaware", "deltamusic 2.0",
           "digital sonix & chrome", "fwmp", "heatseeker mc1.0",
           "kefrens sound machine", "musicline", "steve turner",
           "midi-loriciel" })
        format_map[f] = AMIGA;
    // AdLib / OPL (PC).
    for (char const* f : { "raw opl capture", "edlib packed", "edlib d00",
                           "edlib d01", "herad music system", "imf",
                           "a.m.composer 1.2" })
        format_map[f] = ADPLUG;
    // Japanese FM home computers (NEC PC-98, Sharp X68000, Fujitsu FM Towns).
    for (char const* f :
         { "fm sound driver (fmp)", "pmd", "s98", "mdx", "euphony" })
        format_map[f] = JPFM;
    // PC / DAW / synth.
    for (char const* f : { "deflemask", "v2", "piston collage",
                           "piston collage protected", "renoise", "renoise old",
                           "piece music driver", "sierra agi" })
        format_map[f] = PC;
    // Remaining single-platform mappings.
    format_map["nsfe"] = NES;
    format_map["hvsc"] = SID;
    format_map["benn daglish sid"] = SID;
    format_map["playstation 2 sound format"] = PLAYSTATION2;
    format_map["sgc"] = SEGAMS;             // SG-1000 / Coleco / SMS rips
    format_map["saturn sound format"] = SATURN;
    format_map["wonderswan"] = WONDERSWAN;
    format_map["ogg vorbis"] = OGG;
    format_map["ogg"] = OGG; // zxart ogg-fallback tunes (format string "OGG")
    format_map["iso-mpeg audio layer-2"] = MP3;
    format_map["iso-mpeg audio layer-3"] = MP3;
    format_map["hippel st coso"] = ATARI;   // Atari ST Hippel
    format_map["bbc micro"] = ACORN;        // Acorn 8-bit (close enough)
    // Misc small consoles/arcade -> generic CONSOLE ("Other Consoles" filter).
    for (char const* f : { "vectrex", "colecovision", "capcom q-sound format" })
        format_map[f] = CONSOLE;

    // Correct cross-platform formats that the generic fallbacks (endsWith
    // "tracker" -> TRACKER, uade_formats -> UADE) would otherwise mis-file under
    // Amiga. These are native to other platforms.
    format_map["sidplayer"] = SID;          // Commodore 64 (not Amiga)
    format_map["goattracker"] = SID;
    format_map["goattracker 2"] = SID;
    format_map["cybertracker"] = SID;
    format_map["cybertracker c64"] = SID;
    format_map["famitracker"] = NES;
    format_map["nerdtracker 2"] = NES;
    format_map["psycle"] = PC;              // Windows tracker/DAW
    format_map["sunvox"] = PC;
    format_map["buzz"] = PC;
    format_map["organya"] = PC;             // Cave Story (.org)
    format_map["organya 2"] = PC;
    format_map["epic megagames masi"] = PCTRACKER;               // PSM, DOS
    format_map["digital sound and music interface"] = PCTRACKER; // DSMI, DOS
    format_map["digital sound interface kit"] = PCTRACKER;       // DSIK, DOS
    format_map["digital sound interface kit riff"] = PCTRACKER;

    // -----------------------------------------------------------------------
    // Formats that sit in uade_formats (so they would default to UADE/Amiga)
    // but are NOT Amiga -- they are played by their own dedicated plugins, and
    // this only fixes the now-playing platform label / F9 filter / screenshot
    // logo. Platform attributions verified against data/misc/formats_descriptions.txt
    // and the per-format notes. (2026-06-24 audit.)
    // -----------------------------------------------------------------------
    // Commodore 64 (SID).
    format_map["ben daglish sid"] = SID; // bds: "based on his C64 version"
                                         // (the old "benn daglish sid" was a typo)
    format_map["goattracker stereo"] = SID; // GoatTracker is C64, like goattracker/2
    // ZX Spectrum.
    format_map["picatune"] = ZXBEEPER;       // Shiru 1-bit beeper, sibling of picatune2
    format_map["tfm music maker"] = ZXAY;    // tfe: ZX Spectrum TurboSound FM
    // Atari ST/STE.
    format_map["quartet psg"] = ATARI;       // Quartet (Atari ST), like quartet st
    format_map["tfmx st"] = ATARI;           // Atari ST TFMX variant
    format_map["rob hubbard st"] = ATARI;    // Atari ST Rob Hubbard
    format_map["graoumf tracker"] = ATARI;   // Graoumf Tracker (Atari ST/Falcon)
    format_map["graoumf tracker 2"] = ATARI;
    format_map["megatracker"] = ATARI;       // mgt: "Atari ST sample tracker by Cream"
    // IBM PC trackers (DOS/Windows, sample-based).
    format_map["imago orpheus"] = PCTRACKER; // DOS tracker (.imf Imago Orpheus)
    format_map["sbstudio"] = PCTRACKER;      // pac: "general-purpose...MS-DOS tracker"
    format_map["mad tracker 2"] = PCTRACKER; // MadTracker 2 (Windows)
    format_map["velvet studio"] = PCTRACKER; // Velvet Studio (Windows, .ams)
    format_map["skale tracker"] = PCTRACKER; // Skale Tracker (Windows/DOS)
    format_map["liquid tracker"] = PCTRACKER;// Liquid Tracker (DOS, .liq)
    format_map["funktracker"] = PCTRACKER;   // FunkTracker (DOS, .funk)
    format_map["real tracker"] = PCTRACKER;  // RealTracker (DOS/Windows, .rtm)
    // PC softsynth / chiptune trackers / DAWs (group with psycle/sunvox/buzz).
    format_map["monotone"] = PC;             // PC-speaker 1-bit beeper tracker, NOT Amiga
    format_map["noisetrekker"] = PC;         // NoiseTrekker (Windows)
    format_map["noisetrekker 2"] = PC;
    format_map["protrekkr"] = PC;            // ProTrekkr (cross-platform softsynth)
    format_map["protrekkr 2.0"] = PC;
    format_map["klystrack"] = PC;            // klystrack (cross-platform chiptune tracker)
    format_map["darkwave studio"] = PC;      // DarkWave Studio (Windows DAW)
    format_map["dreamstation"] = PC;         // DreamStation (Windows)
    format_map["ixalance"] = PC;             // Ixalance softsynth (Windows)
}

// A stable 16-bit per-format key from the format string, so every distinct
// sub-format within a platform gets its own value (16-bit avoids the collisions
// an 8-bit hash would have for a platform's handful of formats). 0 is reserved
// as the neutral value used for products.
static uint16_t hueSeed(std::string const& fmt)
{
    uint32_t h = 2166136261u; // FNV-1a
    for (char c : fmt) {
        h ^= (uint8_t)tolower((unsigned char)c);
        h *= 16777619u;
    }
    uint16_t v = (uint16_t)(h & 0xffff);
    return v == 0 ? 1 : v; // reserve 0 for products
}

static uint8_t formatToByte(std::string const& fmt, std::string const& path,
                            int coll)
{

    static bool init = false;
    if (!init) {
        initFormats();
        init = true;
    }

    std::string f = toLower(fmt);
    uint8_t l = format_map[f];
    if (l == 0) {

        l = UNKNOWN_FORMAT;

        if ((path.find("youtube.com/") != std::string::npos) ||
            (path.find("youtu.be/") != std::string::npos)) {
            return YOUTUBE;
        }

        if (endsWith(f, "tracker")) l = TRACKER;
        if (startsWith(f, "soundtracker"))
            l = SOUNDTRACKER;
        else if (startsWith(f, "protracker"))
            l = PROTRACKER;
        else if (startsWith(f, "fasttracker"))
            l = FASTTRACKER;
        else if (startsWith(f, "impulsetracker"))
            l = IMPULSETRACKER;
        else if (startsWith(f, "screamtracker"))
            l = SCREAMTRACKER;
        else if (startsWith(f, "atari"))
            l = ATARI;
        else if (startsWith(f, "ay ") || startsWith(f, "spectrum "))
            l = SPECTRUM;
        else if (startsWith(f, "gameboy"))
            l = GAMEBOY;
        if (f.find("megadrive") != std::string::npos) l = MEGADRIVE;
        if (l != UNKNOWN_FORMAT) format_map[f] = l;
        // fprintf(stderr, "%s\n", f.c_str());
    }
    return l;
}

// Human-readable platform name for a format byte, used by describeFormat().
// Mirrors the F9 platform filters (see ChipMachine::filterOptions).
static std::string platformName(uint8_t b)
{
    switch (b) {
    case AMIGA:
    case PROTRACKER:
    case SOUNDTRACKER:
    case UADE:
    case TRACKER: return "Amiga";
    case SCREAMTRACKER:
    case IMPULSETRACKER:
    case FASTTRACKER:
    case PCTRACKER: return "PC";
    case JPFM: return "PC-98 / X68000";
    case SID:
    case STR: return "Commodore 64";
    case PRG: return "Commodore 16/+4";
    case ATARI: return "Atari ST/STE";
    case POKEY: return "Atari XL/XE";
    case ZXBEEPER: return "ZX Spectrum 16/48";
    case ZXAY: return "ZX Spectrum 128";
    case SAMCOUPE: return "Sam Coupe";
    case MSX: return "MSX";
    case AMSTRAD: return "Amstrad CPC";
    case ACORN: return "Acorn Archimedes";
    case APPLE: return "Apple IIGS";
    case NES: return "Nintendo NES";
    case SNES: return "Nintendo SNES";
    case GAMEBOY:
    case GBA: return "Nintendo Game Boy";
    case NINTENDO64: return "Nintendo 64";
    case NDS: return "Nintendo DS";
    case SEGA:
    case MEGADRIVE: return "Sega Mega Drive";
    case SEGAMS: return "Sega 8-bit";
    case DREAMCAST: return "Sega Dreamcast";
    case SATURN: return "Sega Saturn";
    case WONDERSWAN: return "WonderSwan";
    case PLAYSTATION:
    case PLAYSTATION2: return "PlayStation";
    case HES: return "PC Engine";
    case CONSOLE: return "Console";
    case ADPLUG: return "PC AdLib";
    case PC: return "PC";
    case MP3: return "MP3";
    case OGG: return "OGG";
    case M3U:
    case PLS: return "Playlist";
    case RADIO: return "Radio";
    case YOUTUBE: return "YouTube";
    case PODCAST: return "Podcast";
    default: return "";
    }
}

uint8_t MusicDatabase::classifyFormat(std::string const& fmt,
                                      std::string const& path)
{
    return formatToByte(fmt, path, 0);
}

// Map a format byte to a filesystem-safe platform slug for the per-platform
// logo lookup, or "" for "platforms" that are really streaming/meta sources
// (no hardware screenshot makes sense for them).
static std::string platformSlugForByte(uint8_t b)
{
    std::string name = platformName(b);
    static const std::set<std::string> nonHardware = {
        "", "MP3", "OGG", "Playlist", "Radio", "YouTube", "Podcast"
    };
    if (nonHardware.count(name)) return "";
    // '/' is not a legal filename character; keep everything else (spaces are
    // fine on disk and keep the names readable).
    for (auto& c : name)
        if (c == '/') c = '-';
    return name;
}

std::string MusicDatabase::platformScreenshotName(SongInfo const& s)
{
    return platformSlugForByte(formatToByte(s.format, s.path, 0));
}

std::vector<std::string> MusicDatabase::platformScreenshotNames()
{
    std::vector<std::string> out;
    std::set<std::string> seen;
    for (int b = 0; b < 256; b++) {
        auto slug = platformSlugForByte((uint8_t)b);
        if (slug.empty() || seen.count(slug)) continue;
        seen.insert(slug);
        out.push_back(slug);
    }
    return out;
}

std::vector<int> MusicDatabase::getFormatByteCounts() const
{
    std::lock_guard lock{ dbMutex };
    std::vector<int> counts(256, 0);
    // Songs occupy [0, productStartIndex); the rest are products -- skip them.
    uint32_t n = (productStartIndex > 0 &&
                  productStartIndex <= (uint32_t)formats.size())
                     ? productStartIndex
                     : (uint32_t)formats.size();
    for (uint32_t i = 0; i < n; i++) counts[formats[i] & 0xff]++;
    return counts;
}

int MusicDatabase::getPodcastShowCount() const
{
    std::lock_guard lock{ dbMutex };
    std::set<int> shows;
    uint32_t n = (productStartIndex > 0 &&
                  productStartIndex <= (uint32_t)formats.size())
                     ? productStartIndex
                     : (uint32_t)formats.size();
    // formats[i] packs the collection id in the high bits; collect the distinct
    // collections that carry PODCAST episodes.
    for (uint32_t i = 0; i < n; i++)
        if ((formats[i] & 0xff) == PODCAST) shows.insert(formats[i] >> 8);
    return (int)shows.size();
}

std::string MusicDatabase::describeFormat(SongInfo const& s)
{
    uint8_t b = formatToByte(s.format, s.path, 0);

    // YouTube videos have no module format/extension. The format string carries
    // the source platform(s) in parentheses (e.g. "Youtube (ZX Spectrum)");
    // surface that as "YouTube - ZX Spectrum".
    if (b == YOUTUBE) {
        auto open = s.format.find('(');
        auto close = s.format.rfind(')');
        if (open != std::string::npos && close != std::string::npos &&
            close > open + 1)
            return "YouTube - " + s.format.substr(open + 1, close - open - 1);
        return "YouTube";
    }

    // Podcasts are streamed audio; the enclosure "extension" is derived from the
    // URL and often carries a query string (".mp3?p=f", ".mp3?dest-id=..."), so
    // skip the "(EXT)" suffix entirely and just label them "Podcast".
    if (b == PODCAST) return "Podcast";

    // Extension (uppercase, no dot); prefer the stored ext, fall back to path.
    std::string ext = s.ext;
    if (ext.empty()) ext = utils::path_extension(s.path);
    for (auto& c : ext) c = (char)toupper((unsigned char)c);

    // Many songs (e.g. from ModArchive) carry a bare format code as their format
    // string (MOD, XM, IT, ...) which would otherwise render as "MOD (MOD)".
    // Expand those to a real platform + tracker name. (Audio codes like MP3 are
    // already classified via format_map, so they don't need an entry here -- the
    // redundancy collapse below turns "MP3 - MP3 (MP3)" into just "MP3".)
    static const std::map<std::string, std::pair<std::string, std::string>>
        codeNames = {
            { "mod", { "Amiga", "ProTracker" } },
            { "ahx", { "Amiga", "AHX" } },
            { "thx", { "Amiga", "AHX" } },
            { "med", { "Amiga", "OctaMED" } },
            { "okt", { "Amiga", "Oktalyzer" } },
            { "dbm", { "Amiga", "DigiBooster Pro" } },
            { "digi", { "Amiga", "DigiBooster" } },
            { "xm", { "PC", "FastTracker II" } },
            { "it", { "PC", "Impulse Tracker" } },
            { "s3m", { "PC", "Scream Tracker 3" } },
            { "stm", { "PC", "Scream Tracker 2" } },
            { "mtm", { "PC", "MultiTracker" } },
            { "mdl", { "PC", "DigiTrakker" } },
            { "mptm", { "OpenMPT", "OpenMPT" } },
            { "mo3", { "PC", "MO3" } },
            { "669", { "PC", "Composer 669" } },
            { "far", { "PC", "Farandole Composer" } },
            { "ptm", { "PC", "PolyTracker" } },
            { "ult", { "PC", "UltraTracker" } },
        };

    std::string plat, name;
    std::string key = toLower(s.format.empty() ? ext : s.format);
    auto cit = codeNames.find(key);
    if (cit != codeNames.end()) {
        plat = cit->second.first;
        name = cit->second.second;
    } else {
        plat = platformName(b);
        name = s.format;
    }
    if (name.empty()) name = ext.empty() ? "Unknown" : ext;

    // Build "Platform - Name (EXT)", dropping any piece that just repeats
    // another (e.g. "MP3 - MP3 (MP3)" -> "MP3", "YM (YM)" -> ...).
    auto ieq = [](std::string const& a, std::string const& b2) {
        if (a.size() != b2.size()) return false;
        for (size_t i = 0; i < a.size(); i++)
            if (tolower((unsigned char)a[i]) != tolower((unsigned char)b2[i]))
                return false;
        return true;
    };
    std::string out = name;
    if (!plat.empty() && !ieq(plat, name)) out = plat + " - " + name;
    // Skip the "(EXT)" suffix when it would be redundant: when it matches the
    // name/platform, or the name already ends in a parenthetical that conveys
    // the format detail (e.g. "FM sound driver (FMP)").
    bool nameHasParen = !name.empty() && name.back() == ')';
    if (!ext.empty() && !ieq(ext, name) && !ieq(ext, plat) && !nameHasParen)
        out += " (" + ext + ")";
    return out;
}

std::string MusicDatabase::describeExtension(std::string const& ext)
{
    if (!formatDescriptionsLoaded) {
        formatDescriptionsLoaded = true;
        // Each entry spans two lines: "<ext>\t<trackers>" followed by a prose
        // description, then a blank separator line.
        File f{ workDir.string(), "data/misc/formats_descriptions.txt" };
        if (f.exists()) {
            auto lines = f.getLines();
            for (size_t i = 0; i < lines.size(); i++) {
                auto tab = lines[i].find('\t');
                if (tab == std::string::npos) continue; // desc / blank line
                std::string key = toLower(lines[i].substr(0, tab));
                std::string combined = lines[i].substr(tab + 1);
                if (i + 1 < lines.size() && !lines[i + 1].empty())
                    combined += " - " + lines[i + 1];
                formatDescriptions[key] = combined;
            }
            LOGD("Loaded %d format descriptions",
                 (int)formatDescriptions.size());
        }
    }
    auto it = formatDescriptions.find(toLower(ext));
    return it == formatDescriptions.end() ? "" : it->second;
}

template <typename T> static void readVector(std::vector<T>& v, apone::File& f)
{
    auto sz = f.read<uint32_t>();
    v.resize(sz);
    for (uint32_t i = 0; i < sz; i++) {
        if constexpr (std::is_enum_v<T>) {
            v[i] = static_cast<T>(f.read<uint32_t>());
        } else {
            v[i] = f.read<T>();
        }
    }
}

template <typename T> static void writeVector(std::vector<T>& v, apone::File& f)
{
    auto sz = static_cast<uint32_t>(v.size());
    f.write<uint32_t>(sz);
    for (uint32_t i = 0; i < sz; i++) {
        if constexpr (std::is_enum_v<T>) {
            f.write<uint32_t>(static_cast<uint32_t>(v[i]));
        } else {
            f.write<T>(v[i]);
        }
    }
}

void MusicDatabase::readIndex(apone::File&& f)
{

    indexVersion = 0;
    auto marker = f.read<uint16_t>();
    if (marker == 0xFEDC)
        indexVersion = f.read<uint16_t>();
    else
        f.seek(0);
    productStartIndex = f.read<uint32_t>();
    readVector(titleToComposer, f);
    readVector(composerToTitle, f);
    readVector(composerTitleStart, f);
    readVector(formats, f);
    readVector(productPlatform, f);
    readVector(productRowid, f);
    readVector(formatHue, f);

    titleIndex.load(f);
    composerIndex.load(f);
}

void MusicDatabase::writeIndex(apone::File&& f)
{
    f.write<uint16_t>(0xFEDC);
    f.write<uint16_t>(dbVersion);
    f.write<uint32_t>(productStartIndex);
    writeVector(titleToComposer, f);
    writeVector(composerToTitle, f);
    writeVector(composerTitleStart, f);
    writeVector(formats, f);
    writeVector(productPlatform, f);
    writeVector(productRowid, f);
    writeVector(formatHue, f);

    titleIndex.dump(f);
    composerIndex.dump(f);
    f.close();
}

void MusicDatabase::generateIndex()
{

    // std::lock_guard lock{dbMutex};

    RemoteLoader& loader = remoteLoader;
    auto q = db.query<int, std::string, std::string, std::string>(
        "SELECT ROWID,id,url,localdir FROM collection");
    while (q.step()) {
        auto c = q.get<Collection>();
        // Resolve relative local_dir against the current resource root so the
        // app works correctly regardless of where it was indexed (dev tree vs
        // /Applications bundle).
        if (!c.local_dir.empty() && !c.local_dir.is_absolute())
            c.local_dir = workDir / c.local_dir;
        // NOTE c.name is really c.id
        // hvtc songs live on plus4world.powweb.com, a flaky shared host (~20s
        // per .prg, intermittent connection failures). Serve them from the fast
        // Wayback mirror first, falling back to the live host for the ~34% of
        // tunes Wayback never archived. Derived from c.url so no DB/db.lua change.
        if (c.name == "hvtc") {
            std::string live = c.url;
            std::string wayback = "https://web.archive.org/web/2id_/" + live;
            loader.registerSource(c.name, wayback, c.local_dir.string(), live);
        } else {
            loader.registerSource(c.name, c.url, c.local_dir.string());
        }
    }
    auto indexPath = Environment::getCacheDir() / "index.dat";

    if (!reindexNeeded && utils::exists(indexPath)) {
        readIndex(apone::File{ indexPath });
        return;
    }

    print_fmt("Creating Search Index...\n");

    std::string oldComposer;
    auto query = db.query<std::string, std::string, std::string, std::string,
                          std::string, int>(
        "SELECT title, game, format, composer, path, collection FROM song");

    // The "radio" collection holds live streaming stations whose format tags
    // (M3U/MP3) are shared with regular content; tag them by collection ROWID so
    // they get a dedicated RADIO format byte (and their own F9 filter).
    int radioColl = -1;
    try {
        auto cq =
            db.query<int>("SELECT ROWID FROM collection WHERE id='radio'");
        if (cq.step()) radioColl = cq.get();
    } catch (...) {}

    int count = 0;
    // int maxTotal = 3;
    int cindex = 0;

    titleToComposer.reserve(438000);
    composerToTitle.reserve(37000);
    titleIndex.reserve(438000);
    composerIndex.reserve(37000);
    formats.reserve(438000);

    int step = 438000 / 20;

    std::unordered_map<std::string, std::vector<uint32_t>> composers;

    std::string title, game, fmt, composer, path;
    int collection;

    while (count < 1000000) {
        count++;
        if (!query.step()) break;

        if (count % step == 0) {
            LOGD("%d songs indexed", count);
        }

        tie(title, game, fmt, composer, path, collection) = query.get_tuple();

        uint8_t b = formatToByte(fmt, path, collection);
        if (collection == radioColl) b = RADIO;
        formats.push_back(b | (collection << 8));
        // Hue key: normally per-format, but podcasts all share the format
        // "Podcast" -- key them by their show (collection) so episodes of one
        // podcast share a hue and differ from another's.
        formatHue.push_back(fmt == "Podcast"
                                ? hueSeed("podcast#" + std::to_string(collection))
                                : hueSeed(fmt));

        if (game != "") {
            if (title != "")
                title = format("%s [%s]", game, title);
            else
                title = game;
        }

        if (dontIndex[collection]) {
            title = "";
            composer = "";
        }

        // The title index maps one-to-one with the database
        int tindex = titleIndex.add(title);

        auto& v = composers[composer];
        if (v.empty()) {
            cindex = composerIndex.add(composer);
            composers[composer].push_back(cindex);
        } else
            cindex = composers[composer][0];

        composers[composer].push_back(tindex);

        // We also need to find the composer for a give title
        titleToComposer.push_back(cindex);
    }

    productStartIndex = titleIndex.size();

    auto prodQuery = db.query<int, std::string, std::string, std::string, int>(
        "SELECT product.ROWID, product.title, type, creator, collection FROM "
        "product, prod2song WHERE prodid = product.ROWID GROUP BY prodid HAVING "
        "count(*) > 1");
    int prodRowid;
    while (count < 1000000) {
        count++;
        if (!prodQuery.step()) break;

        if (count % step == 0) {
            LOGD("%d songs indexed", count);
        }

        tie(prodRowid, title, fmt, composer, collection) =
            prodQuery.get_tuple();

        uint8_t b = PRODUCT;
        formats.push_back(b | (collection << 8));
        formatHue.push_back(0); // products: neutral (no hue shift)
        // Tag the product with a platform byte (from its `type`) so the F9
        // filter can include/exclude collections by platform. Aligned with
        // productStartIndex (this is the (formats.size()-productStartIndex)'th
        // product).
        productPlatform.push_back(productTypeToPlatform(fmt));
        // Remember the real ROWID -- the ordinal here is not the ROWID because
        // single-song products are skipped above (see getSongInfo).
        productRowid.push_back(prodRowid);

        if (dontIndex[collection]) {
            title = "";
            composer = "";
        }

        // The title index maps one-to-one with the database
        int tindex = titleIndex.add(title);

        auto& v = composers[composer];
        if (v.empty()) {
            cindex = composerIndex.add(composer);
            composers[composer].push_back(cindex);
        } else
            cindex = composers[composer][0];

        composers[composer].push_back(tindex);

        // We also need to find the composer for a give title
        titleToComposer.push_back(cindex);
    }

    // composers[name] -> std::vector of titleindexes for each composer.

    LOGD("Found %d composers and %d titles", composers.size(),
         titleToComposer.size());

    composerTitleStart.resize(composers.size());
    for (auto const& p : composers) {
        // p,first == composer, p.second == std::vector
        auto cindex = p.second[0];
        composerTitleStart[cindex] = composerToTitle.size();
        for (int i = 1; i < (int)p.second.size(); i++)
            composerToTitle.push_back(p.second[i]);
        composerToTitle.push_back(-1);
    }

    writeIndex(apone::File{ indexPath, apone::File::Write });

    reindexNeeded = false;
}

void MusicDatabase::initFromLuaAsync(utils::path const& workDir)
{
    this->workDir = workDir;
    indexing = true;
    initFuture = std::async(std::launch::async, [=]() {
        std::lock_guard lock{ dbMutex };
        if (!initFromLua(workDir)) {
        }
        std::lock_guard lock2{ chkMutex };
        indexing = false;
    });
}

bool MusicDatabase::initFromLua(utils::path const& workDir)
{
    this->workDir = workDir;
    auto playlistPath = Environment::getConfigDir() / "playlists";
    utils::create_directory(playlistPath);
    bool favFound = false;
    for (auto const& f : utils::File{ playlistPath }.listRecursive()) {
        // for (auto const& f : fs::directory_iterator(playlistPath)) {
        playLists.emplace_back(f.getName());
        if (playLists.back().name == "Favorites") favFound = true;
    }
    if (!favFound) {
        playLists.emplace_back(playlistPath / "Favorites");
        playLists.back().save();
    }

    reindexNeeded = false;
    auto indexDir = Environment::getCacheDir() / "index.dat";

    indexVersion = 0;
    if (utils::exists(indexDir)) {
        apone::File fi{ indexDir };
        auto marker = fi.read<uint16_t>();
        if (marker == 0xFEDC) indexVersion = fi.read<uint16_t>();
    }

    if (rebuildForced) {
        indexVersion = -1;
    }

    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::package);

    std::map<std::string, std::string> dbmap;
    lua["create_db"] = [&] {
        std::string db_name = dbmap["name"];
        try {
            initDatabase(workDir, dbmap);
        } catch (std::exception& e) {
            LOGE("Error creating database '%s': %s", db_name, e.what());
        } catch (...) {
            LOGE("Unknown error creating database '%s'", db_name);
        }
        dbmap.clear();
    };

    lua["set_db_var"] = [&](std::string const& name, sol::object val) {
        if (val.is<std::string>())
            dbmap[name] = val.as<std::string>();
        else if (val.is<uint32_t>())
            dbmap[name] = std::to_string(val.as<uint32_t>());
        else if (val.is<bool>())
            dbmap[name] = val.as<bool>() ? "yes" : "no";
        else
            dbmap[name] = "";
    };

    // Collect podcast feeds (id + shipped list + live feed URL) during a
    // pre-pass so we can run the throttled background refresh and decide
    // whether new episodes warrant a reindex *before* the version gate below.
    podcastFeeds.clear();
    lua["register_podcast"] = [&](std::string const& id,
                                  std::string const& songList,
                                  std::string const& remoteList) {
        podcastFeeds.push_back({ id, songList, remoteList });
    };

    if (auto f = findFile(workDir.string(), "lua/db.lua")) {
        auto res = lua.safe_script_file(f->string(), sol::script_pass_on_error);
        if (!res.valid()) {
            sol::error err = res;
            LOGE("Lua error in db.lua: %s", err.what());
            return false;
        }
    }

    lua.safe_script(R"(
        for _, b in pairs(DB) do
            if type(b) == 'table' and b.type == 'podcast' then
                register_podcast(b.id or '', b.song_list or '',
                                 b.remote_list or '')
            end
        end
    )", sol::script_pass_on_error);
    bool podcastsChanged = preparePodcasts(workDir);

    totalSongs = 0;
    dbVersion = lua["VERSION"];

    int sqliteVersion = 0;
    try {
        auto q = db.query<int>("PRAGMA user_version");
        if (q.step()) {
            sqliteVersion = q.get();
        }
    } catch (...) {}

    LOGD("DBVERSION %d INDEXVERSION %d SQLITEVERSION %d", dbVersion,
         indexVersion, sqliteVersion);
    bool fullReindex = dbVersion != indexVersion || dbVersion != sqliteVersion;
    if (fullReindex) {
        utils::print_fmt("Clearing Web Cache (DB update detected)...\n");
        auto cacheDir = Environment::getCacheDir();
        auto webFilesDir = cacheDir / "_webfiles";
        std::error_code ec;
        std::filesystem::remove_all(webFilesDir.string(), ec);

        db.exec("DROP TABLE IF EXISTS collection");
        db.exec("DROP TABLE IF EXISTS song");
        db.exec("DROP TABLE IF EXISTS product");
        db.exec("DROP TABLE IF EXISTS prod2song");
        createTables();
        db.exec(utils::format("PRAGMA user_version = %d", dbVersion));
        reindexNeeded = true;
    } else if (podcastsChanged) {
        // A background feed refresh found new episodes (full catalogue already
        // in the cache XML). Don't drop/re-parse the big collections -- just
        // append the new episodes to the song table and rebuild the search
        // index from the table. Append-only keeps song ROWIDs contiguous, which
        // getSongInfo relies on (search position i <-> song.ROWID i+1).
        syncPodcastSongs();
        reindexNeeded = true;
    }

    try {
        auto res = lua.safe_script(R"(
            for a,b in pairs(DB) do
                if type(b) == 'table' then
                    for a1,b1 in pairs(b) do
                        set_db_var(a1, b1)
                    end
                    create_db()
                end
            end
        )", sol::script_pass_on_error);

        if (!res.valid()) {
            sol::error err = res;
            LOGE("Lua error during DB creation: %s", err.what());
        }
    } catch (std::exception& e) {
        LOGE("C++ exception during DB creation: %s", e.what());
    } catch (...) {
        LOGE("Unknown exception during DB creation");
    }

    if (totalSongs > 0) {
        print_fmt("Total songs count: %d\n", totalSongs);
    }

    generateIndex();
    return true;
}

int MusicDatabase::getSongs(std::vector<SongInfo>& target,
                            SongInfo const& match, int limit, bool random)
{

    std::lock_guard lock{ dbMutex };
    std::string txt =
        "SELECT path, game, title, composer, format, collection.id "
        "FROM song, collection "
        "WHERE song.collection = collection.ROWID";

    std::string collection;
    if (match.path != "") {
        auto parts = split(match.path, "::");
        if (parts.size() >= 2) collection = parts[0];
    }

    if (match.format != "") txt += " AND format=?";
    if (match.composer != "") txt += " AND composer=?";
    if (collection != "") txt += " AND collection.id=?";
    if (random) txt += " ORDER BY RANDOM()";
    if (limit > 0) txt += format(" LIMIT %d", limit);

    LOGD("SQL:%s", txt);

    auto q = db.query<std::string, std::string, std::string, std::string,
                      std::string, std::string>(txt);
    int index = 1;
    if (match.format != "") q.bind(index++, match.format);
    if (match.composer != "") q.bind(index++, match.composer);
    if (collection != "") q.bind(index++, collection);

    while (q.step()) {
        std::string collection;
        SongInfo song;
        tie(song.path, song.game, song.title, song.composer, song.format,
            collection) = q.get_tuple();
        song.path = collection + "::" + song.path;
        if (song.game != "")
            song.title = utils::format("%s [%s]", song.game, song.title);
        target.push_back(song);
    }
    return 0;
}

void MusicDatabase::addToPlaylist(std::string const& plist,
                                  SongInfo const& song)
{
    for (auto& pl : playLists) {
        if (pl.name == plist) {
            pl.songs.push_back(song);
            pl.save();
            break;
        }
    }
}

void MusicDatabase::removeFromPlaylist(std::string const& plist,
                                       SongInfo const& toRemove)
{
    for (auto& pl : playLists) {
        if (pl.name == plist) {
            pl.songs.erase(std::remove_if(pl.songs.begin(), pl.songs.end(),
                                          [&](SongInfo const& song) -> bool {
                                              return song.path ==
                                                         toRemove.path &&
                                                     (song.starttune == -1 ||
                                                      song.starttune ==
                                                          toRemove.starttune);
                                          }),
                           pl.songs.end());
            pl.save();
            break;
        }
    }
}

std::vector<SongInfo>& MusicDatabase::getPlaylist(std::string const& plist)
{
    static std::vector<SongInfo> empty;
    for (auto& pl : playLists) {
        if (pl.name == plist) return pl.songs;
    }
    return empty;
}
} // namespace chipmachine

