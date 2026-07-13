#include "MusicDatabase.h"
#include "RemoteLoader.h"
#include "SongFileIdentifier.h"
#include "modutils.h"

#include <musicplayer/src/chipplugin.h>

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
            "description STRING, id UNIQUE, version INTEGER, artwork STRING, "
            "priority INTEGER)");
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
    //LOGD("Done");
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
                                                     "w",
                                                     // FAC SoundTracker drumkit
                                                     // sample banks (.sm1/.sm2) --
                                                     // the percussion companions a
                                                     // .mus song loads (fetched via
                                                     // KSSPlugin::getSecondaryFiles),
                                                     // not standalone tunes. All 666
                                                     // on Modland are FAC drumkits.
                                                     "sm1", "sm2" };
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

            // ".txt" files scattered through Modland are documentation/lyrics/
            // format notes (readme.txt, "...CHI format.en.txt", song lyrics,
            // game index.txt), never playable songs -- no song in the whole
            // catalogue uses a .txt extension. The shallow ones were logged as
            // "bad format"/"bad file"; the deeper ones (e.g. "S98/.../index.txt",
            // "Video Game Music/MSX1/.../Galaga.txt") were being indexed as bogus
            // tracks that download but can't play. Drop them all uniformly.
            if (endsWith(toLower(song.path), ".txt")) { continue; }

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

            // Modland "Ad Lib/MUS" is a single orphan file (Nick Jones/first
            // samurai.mus) in an unidentified AdLib format that no open replayer
            // decodes: AdPlug's own MUS loader rejects it (it needs version 1.0,
            // the file is 0x67.0x03 -- an offset-table layout, not AdLib-MIDI),
            // and OpenMPT/UADE/Vice all fail too. Not worth a decoder for one
            // file; skip it so it doesn't show in the GUI as a broken entry that
            // downloads then can't play.
            if (startsWith(song.path, "Ad Lib/MUS/")) { continue; }

            // Modland "MVS Tracker" is 2 files (Kaneda/arktest.mus, entertn.mus),
            // a sample tracker (magic "MVSM1") that no open replayer in our stack
            // decodes (OpenMPT/UADE/Vice/libxmp/ZXTune all reject it). Parked --
            // skip so they don't show as broken GUI entries.
            if (startsWith(song.path, "MVS Tracker/")) { continue; }

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

            // A few real formats live flat on Modland: the format directory
            // holds the "<prefix>.<song>" files directly, with no composer
            // subdirectory (only 2 path segments), so the generic
            // Format/Composer/title parse below rejects them. "Ashley Hogg" is
            // one -- a genuine UADE custom format (Codemasters Amiga game music,
            // eagleplayer prefixes=ash). Index it directly: the composer is the
            // format's namesake, the title is the prefix-stripped base, and the
            // real 2-segment path is preserved verbatim so the FTP download URL
            // (source.url + path) still resolves. The "smp.<song>" sample
            // companions are already dropped as secondary above and fetched by
            // UADEPlugin::getSecondaryFiles at play time.
            static const std::set<std::string> flatFormats = { "Ashley Hogg" };
            bool flat = (l == 2 && flatFormats.count(parts[0]) > 0);

            if (l < 3 && !flat) {
                LOGD("SKIPPED (bad format): %s", song.path);
                continue;
            }

            if (flat) {
                song.format = parts[0];
                song.composer = parts[0];
                song.title = base;
            } else {

            int i = 0;
            song.format = parts[i++];
            if (hasSubFormats.count(song.format) > 0) song.format = parts[i++];

            // Stereo Sidplayer tunes are a ".mus"/".str" pair. Index only the
            // ".str" (stereo) file; the ".mus" companion is fetched as a
            // secondary file at play time. (The mono "Sidplayer" collection,
            // which has standalone ".mus" files, is unaffected.)
            if (song.format == "Stereo Sidplayer" && ext == "mus") continue;

            if (i == l - 1) {
                // Orphan song sitting directly in the format directory with no
                // composer subdir -- only the filename token is left. Happens
                // under the sub-formats (which eat two tokens), e.g.
                // "Spectrum/ASC Sound Master/soldfut0.asc" or
                // "Spectrum/Pro Tracker 2/cappella.pt2". Index it with an unknown
                // composer instead of consuming the filename as the composer and
                // then failing the "bad file" guard below. (Also avoids a stray
                // out-of-bounds parts[i] read in the composer logic.)
                song.composer = "?";
            } else {

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

            } // end composer/game parse (skipped for composer-less orphans)

            if (i == l) {
                LOGD("SKIPPED (bad file): %s", song.path);
                continue;
            }

            if (endsWith(parts[i], ".rar"))
                parts[i] = parts[i].substr(0, parts[i].length() - 4);

            song.title = base;

            } // end non-flat parse
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
        formatIndex = 3, metaIndex = 5, extIndex = -1, screenshotIndex = -1;
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
            else if (p == "screenshot")
                screenshotIndex = i;
            else if (p == "info")
                metaIndex = i;
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

    // modarchive's title column is the upstream marchive-open-db composite
    // "<filename>//<realtitle>" (see data/misc/ModArchive.md). The filename half
    // is dead weight -- playback routes on the `ext` column plus the server's
    // Content-Disposition name, nothing reads the embedded filename -- and it
    // both uglifies the GUI and poisons any {title,composer,format} dedup key.
    // Show/dedup on the real title only (transform applied per-row below).
    bool modarchive = (vars["id"] == "modarchive");

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
            if (modarchive) {
                // Keep everything after the FIRST "//" (a filename never
                // contains "//", so joke titles that themselves hold slashes --
                // e.g. "//// VAMPIRE \\\\" -- survive intact).
                auto sep = titleField.find("//");
                if (sep != std::string::npos)
                    titleField = titleField.substr(sep + 2);
                // ~5% of rows carry no real title, so the filename was reused
                // verbatim ("1394.it//1394.it"). Drop a trailing ".<ext>" that
                // matches this row's ext column so those read "1394", not
                // "1394.it". Real titles practically never end in a module ext.
                if (extIndex >= 0) {
                    auto dotExt = "." + toLower(parts[extIndex]);
                    if (titleField.size() > dotExt.size() &&
                        toLower(titleField.substr(titleField.size() -
                                                  dotExt.size())) == dotExt)
                        titleField =
                            titleField.substr(0, titleField.size() - dotExt.size());
                }
            }
            std::string composerField =
                composerIndex >= 0 ? parts[composerIndex] : composer;
            std::string formatField =
                formatIndex <= 0 ? format : parts[formatIndex];
            // `podcast = "yes"` marks a standard (.txt) collection as a podcast
            // show (Demovibes, AmigaVibes, Syntax Error): force the PODCAST
            // format byte regardless of the per-song/collection codec tag, so
            // these land in the Podcast TAB category and use the podcast
            // playback/scroll paths -- without the RSS `type = "podcast"` parser.
            if (vars["podcast"] == "yes") formatField = "Podcast";

            if (!unexotica) {
                song = SongInfo(parts[pathIndex], gameField, titleField,
                                composerField, formatField, metadata,
                                extIndex >= 0 ? parts[extIndex] : "");
                // `screenshot` template column -> per-song artwork stored verbatim
                // (a full URL, e.g. an img.youtube.com thumbnail). Flows through
                // the song.artwork DB column and is used directly by
                // getSongScreenshots (its SCREENSHOT-already-set fast path).
                if (screenshotIndex >= 0 && (int)parts.size() > screenshotIndex)
                    song.metadata[SongInfo::SCREENSHOT] = parts[screenshotIndex];
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

// The extension a song path would route on, lowercased and without the leading
// dot, for matching against the not_supported_extensions list. Handles MULTI:
// groups (the first member decides) and URL query strings (".ay?x" -> "ay").
static std::string routingExtension(std::string p)
{
    if (startsWith(p, "MULTI:")) {
        p = p.substr(6);
        auto tab = p.find('\t');
        if (tab != std::string::npos) p = p.substr(0, tab);
    }
    auto ext = toLower(utils::path_extension(p));
    auto q = ext.find('?');
    if (q != std::string::npos) ext = ext.substr(0, q);
    return ext;
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

    if (!checkingNames.empty()) checkingNames += ", ";
    checkingNames += name;

    // Return if this collection has already been indexed in this version
    auto cq =
        db.query<uint64_t>("SELECT ROWID FROM collection WHERE id = ?", id);
    if (cq.step()) {
        return;
    }
    cq.finalize();

    reindexNeeded = true;
    reindexingNow.store(true, std::memory_order_relaxed);

    if (!local_dir.empty()) {
        if (!local_dir.is_absolute()) local_dir = workDir / local_dir;
    }

    uint32_t localCount = 0;

    if (source == "") source = screen_source;

    db.exec("BEGIN TRANSACTION");
    // Store the raw relative local_dir from vars so the DB is portable across
    // install locations (dev tree, /Applications, etc.). generateIndex()
    // resolves it against the current workDir at runtime.
    // priority: search precedence & dedup winner (higher surfaces first). Default
    // 0; set per-collection in db.lua (e.g. hvsc high, remixes negative).
    int priority = 0;
    try { priority = std::stoi(vars["priority"]); } catch (...) {}
    db.exec("INSERT INTO collection (name, id, url, localdir, description, "
            "artwork, priority) VALUES (?, ?, ?, ?, ?, ?, ?)",
            name, id, source, vars["local_dir"], description, vars["artwork"],
            priority);
    auto collection_id = db.last_rowid();
    dontIndex.resize(collection_id + 1);
    dontIndex[collection_id] = 0;

    if (vars["index"] == "no") {
        LOGD("Not indexing %s/%d", id, collection_id);
        dontIndex[collection_id] = 1;
    }

    //LOGD("Workdir:%s", workDir);
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
        // Log the short source path (e.g. "data/Games.csv") to match the
        // "Creating '...' DB, source: ..." line the other databases print,
        // rather than the fully-resolved absolute path.
        //LOGD("Parsing %s from %s", type, song_list);

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
                { "modland", &MusicDatabase::parseModland },
                { "podcast", &MusicDatabase::parseRss },
                { "standard", &MusicDatabase::parseStandard },
            };

            auto parser = parsers[type];
            if (!parser) parser = &MusicDatabase::parseStandard;

            (this->*parser)(vars, listFile, [&](SongInfo const& song) {
                // Drop songs whose extension is listed (uncommented) in
                // data/misc/not_supported_extensions.txt: we have no decoder for
                // them, so indexing them only yields broken GUI entries that
                // download then can't play.
                if (!unsupportedExts.empty() &&
                    unsupportedExts.count(routingExtension(song.path)) > 0) {
                    return;
                }
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
    // One progress tick per created collection DB (first phase of the bar).
    dbCreatedCount.fetch_add(1, std::memory_order_relaxed);
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
// "Music Collection") to a platform format byte, so collections obey the TAB
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
    // Same reset for the sub-platform browse (OTHER / ARCADE); the grouping
    // itself is built (and cached) on demand by buildSubPlatforms() below.
    otherFilterActive = (allowedFormats.size() == 1 &&
                         (allowedFormats[0] == OTHER ||
                          allowedFormats[0] == ARCADE));
    if (otherFilterActive) subPlatformByte = allowedFormats[0];
    otherPlatformFilter = -1;
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
        // title index per TAB selection.
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

        // Build the sub-platform grouping for the active byte (OTHER / ARCADE).
        if (otherFilterActive) buildSubPlatforms();
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
        if (index >= OTHER_PLATFORM_INDEX) {
            identity = "OTHERP:" + std::to_string(index - OTHER_PLATFORM_INDEX);
        } else if (index >= PODCAST_SHOW_INDEX) {
            identity = "SHOW:" + std::to_string(index - PODCAST_SHOW_INDEX);
        } else if (index >= PLAYLIST_INDEX) {
            identity = "PL:" + playLists[index - PLAYLIST_INDEX].name;
        } else {
            std::string title = titleIndex.getString(index);
            std::string composer =
                composerIndex.getString(titleToComposer[index]);
            // Fold two songs only when title, composer AND the REAL format all
            // match. If the author or the format is unknown, keep both -- a
            // false negative (a visible duplicate) is always preferable to a
            // false positive that shadows a distinct song. Keying on the real
            // format token (not the coarse platform byte) stops e.g. a mirsoft
            // ".mod" remix filed under Commodore 64 from hiding the real HVSC
            // ".sid" of the same title+composer.
            uint32_t fk =
                (index < (int)formatKey.size()) ? formatKey[index] : 0;
            if (composer.empty() || fk == 0) {
                result.push_back(index);
                return true;
            }
            identity = title + "\t" + composer + "\t" + std::to_string(fk);
        }

        if (seen.find(identity) == seen.end()) {
            result.push_back(index);
            seen.insert(identity);
            return true;
        }
        return false;
    };

    // When drilled into one Other-platform, restrict typed searches to that
    // sub-platform's songs (mirrors the empty-query listing above).
    auto passesOtherDrill = [&](int index) {
        if (!(otherFilterActive && otherPlatformFilter >= 0)) return true;
        auto it = otherIndexToGroup.find(index);
        return it != otherIndexToGroup.end() &&
               it->second == otherPlatformFilter;
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
        // Other-platforms browse: with no drilled-in platform, list the
        // sub-platform GROUP rows; drilled in, list that platform's songs
        // sorted alphabetically by title.
        if (otherFilterActive && otherPlatformFilter < 0) {
            for (auto const& g : otherPlatformList)
                if (!add_unique(OTHER_PLATFORM_INDEX + g.first) &&
                    result.size() >= searchLimit)
                    break;
            return result.size();
        }
        if (otherFilterActive && otherPlatformFilter >= 0) {
            std::vector<int> songs;
            for (int idx : filteredCandidates) {
                auto it = otherIndexToGroup.find(idx);
                if (it != otherIndexToGroup.end() &&
                    it->second == otherPlatformFilter)
                    songs.push_back(idx);
            }
            std::sort(songs.begin(), songs.end(), [&](int a, int b) {
                return toLower(titleIndex.getString(a)) <
                       toLower(titleIndex.getString(b));
            });
            for (int idx : songs)
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
            if (match && passesOtherDrill(index) && !add_unique(index) &&
                result.size() >= searchLimit)
                break;
        }
        return result.size();
    }

    std::vector<int> tresult;
    titleIndex.search(title_query, tresult, searchLimit);
    // Order title matches by collection priority (higher first) so preferred
    // sources -- e.g. HVSC over remix collections -- surface first AND win the
    // dedup (add_unique keeps the first-seen of any fold). Stable, so rows of
    // equal priority keep their existing (index) order. Priority defaults to 0.
    if (!collPriority.empty()) {
        std::stable_sort(tresult.begin(), tresult.end(), [&](int a, int b) {
            auto prio = [&](int idx) {
                int coll = formats[idx] >> 8;
                return coll < (int)collPriority.size() ? collPriority[coll] : 0;
            };
            return prio(a) > prio(b);
        });
    }
    for (int index : tresult) {
        if (!passesOtherDrill(index)) continue;
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
                if (!titleIndex.isFiltered(songindex) &&
                    passesOtherDrill(songindex)) {
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

    // A song path may carry a "<collection.id>::<path>" prefix. Keep the
    // collection so we can disambiguate below: different collections can store
    // the SAME bare path (e.g. AMP and modarchive both use a bare module id, so
    // "173770" exists in both) -- without filtering on the collection the
    // WHERE song.path = ? query returns whichever row indexed first and plays
    // the wrong tune from the wrong source.
    std::string wantColl;
    std::vector<std::string> parts = split(path, "::");
    if (parts.size() > 1) {
        path = parts[1];
        if (parts[0] == "index") {
            int index = stol(path);
            SongInfo song = getSongInfo(index);
            path = song.path;
            parts = split(path, "::");
            if (parts.size() > 1) {
                wantColl = parts[0];
                path = parts[1];
            }
        } else {
            wantColl = parts[0];
        }
        LOGV("INDEX %s %s", parts[0], path);
    }

    bool found = false;
    std::string coll;
    if (!wantColl.empty()) {
        auto q = db.query<std::string, std::string, std::string, std::string,
                          std::string, std::string, std::string, std::string,
                          std::string>(
            "SELECT path, title, game, composer, format, collection.id, "
            "metadata, ext, song.artwork "
            "FROM song, collection "
            "WHERE song.collection = collection.ROWID AND song.path = ? "
            "AND collection.id = ?",
            path, wantColl);
        if (q.step()) {
            tie(song.path, song.title, song.game, song.composer, song.format,
                coll, song.metadata[SongInfo::INFO], song.ext,
                song.metadata[SongInfo::SCREENSHOT]) = q.get_tuple();
            found = true;
        }
    }

    // Fall back to a collection-agnostic match (no prefix, or the prefixed row
    // is gone) -- old behaviour for every non-colliding collection.
    if (!found) {
        auto q = db.query<std::string, std::string, std::string, std::string,
                          std::string, std::string, std::string, std::string,
                          std::string>(
            "SELECT path, title, game, composer, format, collection.id, "
            "metadata, ext, song.artwork "
            "FROM song, collection "
            "WHERE song.collection = collection.ROWID AND song.path = ?",
            path);
        if (q.step()) {
            tie(song.path, song.title, song.game, song.composer, song.format,
                coll, song.metadata[SongInfo::INFO], song.ext,
                song.metadata[SongInfo::SCREENSHOT]) = q.get_tuple();
            found = true;
        }
    }

    if (found) {
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

    if (index >= OTHER_PLATFORM_INDEX) {
        // An Other-platforms GROUP row: title = sub-platform name, path carries
        // the groupId so the UI can drill in (it is not playable).
        int gid = index - OTHER_PLATFORM_INDEX;
        std::string name;
        {
            std::lock_guard lock{ dbMutex };
            for (auto const& g : otherPlatformList)
                if (g.first == gid) name = g.second;
        }
        return SongInfo("otherplatform::" + std::to_string(gid), "", name, "",
                        subPlatformByte == ARCADE ? "Arcade" : "Other");
    }

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
    // Songs occupy 0-based search positions [0, productStartIndex); after the
    // ++ the last song's index equals productStartIndex, so this must be a
    // strict ">" -- ">=" misroutes the very last song into the product branch
    // (product ROWID 0 -> no row -> not_found_exception thrown below). This bit
    // whichever song was indexed last (previously the final Pouet entry).
    if (index > productStartIndex) {
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
               collection == "hvsc" || collection == "asma" ||
               collection == "zxart" || collection == "zxtunes" ||
               collection == "demozoo" ||
               collection == "sceneorg" || collection == "zophar" ||
               collection == "cpcpower" || collection == "vampi" ||
               collection == "rko" || collection == "amigaremix" ||
               collection == "amp" || collection == "modarchive" ||
               collection == "projectay" || collection == "vgmrips" ||
               collection == "smspower" || collection == "mirsoft" ||
               collection == "botb" || collection == "ocremix") {
        // Game/production screenshots matched offline against an external
        // database, keyed by the song path/URL in data/<file>_screenshots.txt:
        // hvtc -> Plus/4 World ("games/<name>.prg"), sndh -> Atari Mania
        // ("<composer>/<game>.sndh"), unexotica -> Hall of Light (the per-game
        // "/Game/<composer>/<game>.lha" id), zxart -> zxart.ee ZX game tunes vs
        // ZXDB (full zxart.ee URL), zxtunes -> the ZX game's World of Spectrum
        // loading screen (ZXDB match, keyed by the full zxtunes.com
        // downloads.php?id= URL; built by scripts/update_zxtunes_screenshots.py),
        // demozoo -> the production's own
        // media.demozoo.org screens (full song URL), zophar -> the game's
        // soundcover image on Zophar's Domain (full song URL, keyed by the
        // fi.zophar.net .zip URL; built by build_zophar.py --screenshots),
        // cpcpower -> the game's screenshot on CPC-Power (full song URL, keyed
        // by the cpc-power.com /YM/ .ym URL; built by
        // build_cpcpower.py --screenshots), vampi -> the game's Wikipedia
        // infobox cover/flyer (full song URL, keyed by the mdx.vampi.tech .MDX
        // URL; built by build_vampi.py --screenshots), rko -> the source C64
        // game's gb64 screenshot (else a demo shot), keyed by the remix id (the
        // rko song path); the remix's original HVSC SID (rko.txt col2) links to
        // gb64 Games.csv SidFilename; built by build_rko.py --screenshots.
        // amigaremix -> the source Amiga game's Wikipedia box-art, keyed by the
        // "<id>/<file>.mp3" song path (source-stripped); built by
        // build_amiremix.py --screenshots. projectay -> the ZX game's World of
        // Spectrum loading screen (ZXDB match), keyed by the local song path
        // ("ironfist/<game>.ay"); ZX rips only, CPC demo rips get no shot; built
        // by scripts/update_projectay_screenshots.py. smspower -> the game's
        // title-screen .png on SMS Power (full song URL, keyed by the live
        // /uploads/Music/<game>.zip pack URL; built by
        // build_smspower.py --screenshots). mirsoft -> best-effort game shot from
        // the sources we already ship offline (gb64 for C64, Hall of Light/abime
        // for Amiga, zophar/vgmrips/smspower/cpcpower reuse), matched by game name
        // and keyed by the mirsoft "<Game>.zip" song path; built by
        // build_mirsoft.py --screenshots. mirsoft hosts no shots of its own.
        // ocremix -> the source game's title-screen/box image on ocremix.org
        // (the remix's og:image; a genuine game shot), keyed by the full mirror
        // .mp3 song URL in data/ocremix_screenshots.txt; built by
        // build_ocremix.py --build.
        // botb -> the entry's battle cover art (themed compos have a distinctive
        // artwork.png/entry image), keyed by the full disk/battle/<id>/<file>
        // song URL; built by build_botb.py --build (~14.8k distinctive covers;
        // the generic One-Hour-Battle /disk/debris/ banners are dropped). Cover
        // art, not a game screenshot -- BotB entries are original community
        // compos, not rips.
        // modland/hvsc/sndh/asma/
        // sceneorg are additionally augmented from Demozoo: a tune is matched to
        // the demos that use it as soundtrack and borrows that production's
        // screenshot (built by chipmachine/scripts/build_demozoo.py --augment, keyed by the
        // full song path; sceneorg matched by its archive.scene.org URL). No
        // match -> blank.
        std::string key = parts[1];
        if (collection == "unexotica") {
            // The song path is one (or a MULTI: list of) module path(s) inside
            // the game's .lha; reduce it to the shared "/Game/.../<game>.lha"
            // identifier that the screenshot map is keyed on.
            auto game = key.find("/Game/");
            auto lha = key.find(".lha");
            if (game != std::string::npos && lha != std::string::npos)
                key = key.substr(game, lha + 4 - game);
        }
        // Files to consult, in priority order. modland has two: the curated ZX
        // subset (zxspectrum, matched via ZXDB / World of Spectrum -- a
        // title-accurate match, so it wins) plus the larger demozoo-derived set.
        std::vector<std::string> files;
        if (collection == "modland")
            files = { "zxspectrum", "modland" };
        else
            files = { collection };
        for (auto const& file : files) {
            auto const& shots = getFileShots(file);
            auto it = shots.find(key);
            if (it != shots.end()) {
                shot = it->second;
                s.metadata[SongInfo::SCREENSHOT] = shot;
                LOGV("Got %s shot %s", collection, shot);
                break;
            }
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
    // Several of these (kss, mvs tracker, the musical enlightenment) are listed
    // in uade_formats; these explicit entries override that Amiga default so
    // they group as MSX. (Note: "face the music" was previously in this list by
    // mistake -- it is an Amiga format; see the correction below.)
    format_map["mgsdrv"] = MSX;
    format_map["kss"] = MSX;
    format_map["musica"] = MSX;
    format_map["moonblaster"] = MSX;
    format_map["moonblaster (edit mode)"] = MSX;
    format_map["oplldrv"] = MSX;
    format_map["mpk"] = MSX;
    format_map["scc-musixx"] = MSX;
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
    // dsym: DSym module format, native to Acorn Archimedes / RISC OS (Oregan
    // Developments). modland tags these "Digital Symphony" (above), but key the
    // bare extension too so the extension-fallback / platformForExtension path
    // classifies a .dsym correctly instead of returning UNKNOWN.
    format_map["dsym"] = ACORN;

    // --- zxart.ee music collection: chip-family format strings emitted by
    // chipmachine/scripts/build_zxart.py (see that script's classify()). The collection routes
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

    // ZX Spectrum AY-3-8910/12 module formats keyed by EXTENSION. These are the
    // last-resort ext fallback (only consulted when the format string didn't
    // resolve), so they don't disturb zxart -- which already tags its tunes
    // "spectrum ay"/"spectrum beeper" above -- and instead rescue demozoo tunes
    // carrying the bare "ZX Spectrum" platform tag (which resolves to nothing),
    // routing them into the "ZX Spectrum 128K (AY)" filter. All AY per
    // data/misc/formats_descriptions.txt. NOTE: .psm is left out (the extension
    // also means PC Epic MASI); .fls is left out (beeper/AY ambiguous, parked,
    // no replayer). .pt2 is nominally shared with the parked Picatune2 beeper
    // XML, but every real .pt2 here is Pro Tracker 2 (AY), so AY is correct.
    for (char const* e :
         { "pt1", "pt2", "pt3", "asc", "stc", "stp", "stp2", "st11", "st13",
           "sqt", "psc", "vtx", "vt2", "ay", "psg", "ftc", "fxm", "chi", "gtr" })
        format_map[e] = ZXAY;

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
    // mix: Atari ST/STE/Falcon digital sample tracker (Digital Tracker /
    // Digi-Mix). The modland format string "Atari Digi-Mix" is listed in
    // uade_formats, so the loop above pre-seeds it to UADE (->"Amiga"); that
    // non-zero entry short-circuits the startsWith("atari") fallback, so it must
    // be overridden explicitly here. Also key the bare extension so demozoo .mix
    // tunes (tagged "Amiga"/"Demoscene", which don't resolve) classify as Atari.
    format_map["atari digi-mix"] = ATARI; // override uade_formats UADE default
    format_map["mix"] = ATARI;
    // --- Atari XL/XE 8-bit (POKEY), distinct chip from the ST line ---
    // startsWith("atari") would otherwise lump "Atari 8Bit" (.sap) in with the
    // ST tunes; this explicit entry keeps POKEY separate.
    format_map["atari 8bit"] = POKEY;
    format_map["pokeynoise"] = POKEY; // Atari 8-bit POKEY (.pn), not Amiga
    // Atari 2600/VCS (TIA chip) demoscene tunes from demozoo/Fujiology carry the
    // platform string "Atari 2600 Video Computer System (VCS)". That starts with
    // "atari", so the generic startsWith("atari") fallback below would lump them
    // in with the Atari ST/STE line -- wrong machine. The TIA is its own chip
    // (not POKEY either), but there's no dedicated 2600 filter yet, so bucket
    // them under Atari 8Bit for now via this explicit override (checked before
    // the fallback). Revisit if a proper VCS/TIA category is added.
    format_map["atari 2600 video computer system (vcs)"] = POKEY;
    // Atari Jaguar demozoo/Fujiology entries: game-soundtrack rips (mostly MP3
    // recordings, a few .xm/.mod modules), NOT ST chiptunes -- but "atari jaguar"
    // would hit the startsWith("atari") fallback and pollute the Atari ST/STE
    // filter. The Jaguar has no native chip music format and there's no
    // dedicated filter, so bucket it under OTHER ("Other Platforms"). (The few
    // .mod among these are genuine Amiga ProTracker modules and get pulled to
    // Amiga by the .mod correction near the end of formatToByte.)
    format_map["atari jaguar"] = OTHER;
    format_map["soundsmith"] = APPLE; // Apple IIgs SoundSmith
    format_map["playerpro"] = APPLE;  // Macintosh PlayerPRO tracker (.mad), overrides uade_formats default
    format_map["jaytrax"] = TRACKER;  // JayTrax (.jxs), cross-platform synth tracker -- not UADE/Amiga
    format_map["ultra64 sound format"] = NINTENDO64;
    format_map["nintendo ds sound format"] = NDS;
    format_map["nintendo sound format"] = NES;
    // demozoo console platform tags (verbose names, incl. the .zip game-rips the
    // host extracts). Route to the matching console byte instead of UNKNOWN.
    format_map["nintendo entertainment system (nes)"] = NES;
    format_map["nintendo game boy (gb)"] = GAMEBOY;
    format_map["nintendo game boy color (gbc)"] = GAMEBOY;
    // Zophar GBA gamerips (.gsf) carry the label "Gameboy Advance". Without this
    // the generic startsWith("gameboy") fallback would file them under GAMEBOY,
    // not GBA (both share the "Nintendo GameBoy/GBA" TAB filter, but keep the
    // byte correct for colour/label).
    format_map["gameboy advance"] = GBA;
    // Zophar 3DS / Xbox 360 gamerips onboarded ONLY for their ffmpeg-playable
    // ogg/wav members (the zips are streamed-audio grab-bags); no dedicated TAB
    // filter for either console -> "Other Platforms", like Wii/GameCube/Xbox.
    format_map["nintendo 3ds"] = OTHER;
    format_map["xbox 360"] = OTHER;
    format_map["nintendo snes/super famicom"] = SNES;
    format_map["nec pc engine"] = HES;
    // Sega 8-bit (SN76489 PSG): Master System, Game Gear, SG-1000, SC-3000
    format_map["sega master system"] = SEGAMS;
    format_map["sega game gear"] = SEGAMS;
    format_map["sega sg-1000"] = SEGAMS;
    format_map["sega sc-3000"] = SEGAMS;
    // ("colecovision" also uses SN76489 but is filed under OTHER below, with the
    // other misc small consoles -- shared with the VGMRips/SMS Power labels.)
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
    // dtm: predominantly Digital Tracker, native to the Atari Falcon/ST/STE
    // (modland "Digital Tracker DTM"); the DeFy DTM (AdLib) and DigiTrekker
    // outliers carry non-resolving format strings and fall through to this
    // extension key. -> Atari ST/STE/Falcon.
    format_map["dtm"] = ATARI;
    format_map["digital tracker dtm"] = ATARI; // explicit (robust for empty-path calls)
    // plm: Disorder Tracker 2, an MS-DOS chiptune tracker (Statix/Psychic Link,
    // 1995), close cousin of Scream Tracker 3 -> IBM PC, not Amiga.
    format_map["plm"] = PCTRACKER;
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
           "propacker 2.1", "propacker 3.0", "synth pack", "alcatraz packer",
           "digital illusions",
           "digital sound creations", "rob hubbard old", "fred editor",
           "peter verswyvelen packer", "promizer", "sfx", "sidmon ii", "sidmon",
           "actionamics sound tool", "andrew parton", "art & magic",
           "channel players", "cinemaware", "deltamusic 2.0",
           "digital sonix & chrome", "fwmp", "heatseeker mc1.0",
           "kefrens sound machine", "musicline", "steve turner",
           "midi-loriciel" })
        format_map[f] = AMIGA;
    // .aon = Art Of Noise (Amiga, UADE-played). Extension key rescues demozoo
    // "Amiga"-tagged .aon tunes (that tag doesn't resolve) from UNKNOWN.
    format_map["aon"] = AMIGA;
    // Face The Music: an 8-voice AMIGA tracker (magic "FTMN", played by
    // openmptplugin's Load_ftm.cpp, which sets SONG_ISAMIGA / MOD_TYPE_MOD).
    // It is NOT MSX (where it was mis-grouped) and NOT Atari (as
    // formats_descriptions.txt claimed); verified 2026-06-30 against the loader
    // and Exotica/Aminet (mods/8voic). The other .ftm format, FamiTracker
    // (magic "FamiTracker Module"), is NES and routes to famitrackerplugin.
    format_map["face the music"] = AMIGA;
    // --- AMP (amp.dascene.net) short format codes ---------------------------
    // AMP stores a short uppercase format code as the `format` string, and its
    // path is a bare "downmod.php?index=" module id with NO file extension, so
    // classifyFormat's extension fallback can't help. Map the AMP-specific
    // codes that aren't already keyed by a shared ext above. Nearly all are
    // Amiga native trackers/players/composer rips; the PC/Atari tracker codes
    // (xm/it/s3m/stm/mtm/dmf/ptm/ult/far/ams/dsm/mptm/mt2/669/mdl/amf/plm/dtm)
    // are already handled by their shared keys.
    for (char const* f :
         { "stk", "fst", "oss", "bp", "bp3", "gmc", "ml", "thx", "dss", "mm8",
           "dmu", "dmu2", "emod", "jam", "sa", "fc13", "ast", "abk", "oct",
           "prt", "sfx2", "ok", "hvl", "ftm", "st26", "stp", "stp2", "sid2",
           "mom", "mod3" })
        format_map[f] = AMIGA;
    format_map["tcb"] = ATARI; // TCB Tracker (Atari ST); .tcb, played by openmpt
    // AdLib / OPL (PC).
    for (char const* f : { "raw opl capture", "edlib packed", "edlib d00",
                           "edlib d01", "herad music system", "imf",
                           "a.m.composer 1.2",
                           "opl archive" }) // OPL2/OPL3 VGM logs (opl.wafflenet.com)
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
    format_map["gc"] = SEGAMS;              // some rips carry ext "gc" for .sgc
    format_map["saturn sound format"] = SATURN;
    format_map["wonderswan"] = WONDERSWAN;
    format_map["ogg vorbis"] = OGG;
    format_map["ogg"] = OGG; // zxart ogg-fallback tunes (format string "OGG")
    format_map["iso-mpeg audio layer-2"] = MP3;
    format_map["iso-mpeg audio layer-3"] = MP3;
    format_map["hippel st coso"] = ATARI;   // Atari ST Hippel
    format_map["bbc micro"] = ACORN;        // Acorn 8-bit (close enough)
    // Battle of the Bits (build_botb.py) format labels not covered above.
    format_map["mptm"] = PCTRACKER;         // OpenMPT native module
    format_map["adlib"] = ADPLUG;           // AdPlug OPL (rad/amd/a2m/dfm/snd)
    format_map["pxtone"] = PC;              // PxTone Collage (Studio Pixel)
    format_map["vgm"] = OTHER;              // generic multi-chip VGM log
    format_map["playstation 2"] = PLAYSTATION2; // OCReMix PS2 game remixes
    // Misc small consoles/arcade -> generic OTHER ("Other Platforms" filter).
    for (char const* f : { "vectrex", "colecovision", "capcom q-sound format" })
        format_map[f] = OTHER;

    // --- VGMRips (vgmrips.net) game rips -------------------------------------
    // The VGMRips collection stores the display platform as the `format` string
    // (build_vgmrips.py) and its path is an archive.org ".../<game>.zip" URL, so
    // the extension fallback can't help -- classify each label explicitly.
    format_map["sega mega drive"] = MEGADRIVE; // YM2612 + SN76489
    format_map["sega pico"] = MEGADRIVE;       // Genesis-based (YM2612)
    format_map["nes"] = NES;                   // 2A03
    format_map["game boy"] = GAMEBOY;          // DMG
    format_map["pc engine"] = HES;             // TurboGrafx (HuC6280)
    format_map["msx"] = MSX;
    // Japanese FM home computers (OPN/OPNA family) -> JPFM, like .mdx/.s98/pmd.
    for (char const* f : { "nec pc-98", "nec pc-88", "nec pc-80", "nec pc-88/98",
                           "sharp x68000", "sharp x1", "fm towns", "fujitsu fm-7" })
        format_map[f] = JPFM;
    format_map["ibm pc"] = PC;
    format_map["zx spectrum"] = SPECTRUM;
    format_map["commodore 64"] = SID;
    format_map["apple ii"] = APPLE;
    format_map["apple iigs"] = APPLE;
    // Arcade boards get their own top-level filter ("Arcade"), which drills into
    // these sub-platforms (recovered from the format string, see buildSubPlatforms).
    // Neo Geo (arcade/AES) joins them, shown as "Arcade (Neo Geo)".
    for (char const* f : { "arcade", "arcade (capcom)", "arcade (konami)",
                           "arcade (namco)", "arcade (sega)", "arcade (taito)",
                           "neo geo" })
        format_map[f] = ARCADE;
    // Atari Jaguar folds into the Atari ST/E/Falcon/Jaguar filter.
    format_map["atari jaguar"] = ATARI;
    // Neo Geo Pocket / pinball have no dedicated TAB filter yet -> "Other Platforms".
    for (char const* f : { "neo geo pocket", "pinball", "other" })
        format_map[f] = OTHER;

    // mirsoft.info "World of Game MODs": as of db.lua v94 its `format` column is
    // the ACTUAL module format (mod-family -> "Amiga", xm/it/s3m -> "PC", a few
    // "Atari ST"), NOT the game's platform. Classifying by game platform (v86)
    // gave a C64 game's .mod the SID byte and made it shadow the real HVSC SID in
    // search dedup, so it was reverted. "amiga"/"pc"/"atari st" are the labels it
    // now emits; the console labels below are retained only for OTHER collections.
    format_map["amiga"] = AMIGA;
    format_map["pc"] = PC;                   // PC DOS/Windows game tracker mods
    format_map["macintosh"] = APPLE;
    format_map["playstation"] = PLAYSTATION;
    format_map["atari falcon"] = ATARI;
    format_map["nintendo 64"] = NINTENDO64;
    format_map["sega saturn"] = SATURN;
    format_map["dreamcast"] = DREAMCAST;
    // chipmusic.org rendered-MP3 tracks that carry no platform-bearing tag land
    // in the generic "Chipmusic" bucket -> the existing Unclassified MP3/OGG
    // filter (the classifiable majority route to Game Boy/C64/NES/Atari ST/Amiga/
    // ZX Spectrum/PC via the labels already mapped above).
    format_map["chipmusic"] = MP3;

    // Correct cross-platform formats that the generic fallbacks (endsWith
    // "tracker" -> TRACKER, uade_formats -> UADE) would otherwise mis-file under
    // Amiga. These are native to other platforms.
    format_map["sidplayer"] = SID;          // Commodore 64 (not Amiga)
    format_map["goattracker"] = SID;
    format_map["goattracker 2"] = SID;
    // Bare .sng extension fallback -> GoatTracker (C64 SID), the highest-priority
    // plugin claiming .sng, so a row whose format string is missing/non-canonical
    // still lands on Commodore 64 instead of UNKNOWN. The other .sng formats reach
    // the classifier with resolving format strings ("sam coupe sng" -> SAMCOUPE,
    // Richard Joseph/ZoundMonitor -> UADE) and never fall through to this key.
    format_map["sng"] = SID;
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
    // this only fixes the now-playing platform label / TAB filter / screenshot
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
    // Graoumf Tracker 2 (.gt2) is the Windows successor -> IBM PC, not Atari.
    // (Most .gt2 tunes reach the classifier with the demozoo "Atari ST" category
    // string, handled by the extension override at the top of formatToByte.)
    format_map["graoumf tracker 2"] = PCTRACKER;
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
    // Key the bare extensions too: NoiseTrekker/ProTrekkr modules onboarded from
    // sources that don't carry a canonical format string (e.g. Fujiology .NTK
    // URLs) reach the classifier via the extension fallback -- without these they
    // land in UNKNOWN and show no platform logo. ptkplugin plays both.
    format_map["ntk"] = PC;                  // NoiseTrekker / ProTrekkr .ntk
    format_map["ptk"] = PC;                  // ProTrekkr .ptk
    // .mon is the Maniacs Of Noise format (a C64/SID driver by origin) but is
    // played through UADE, so its modland tunes carry the "Maniacs Of Noise"
    // format string and already resolve to UADE/Amiga. This bare-extension key
    // is the safety net for .mon files onboarded without that canonical string
    // (e.g. a scene.org "Demoscene"-tagged internal.mon) so they group with the
    // rest of the extension under Amiga instead of falling through to no logo.
    format_map["mon"] = UADE;                // Maniacs Of Noise .mon (UADE-played)
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

// Resolve a human platform name to its format byte. Used to rescue the
// "unclassified" audio buckets (YouTube videos, MP3/OGG rips) whose format
// string names a platform even though the URL/extension would otherwise bucket
// them as unclassified. Handles the pouet "Youtube (<platform>)" wrapper, bare
// manualDatabasePatch names ("ZX Spectrum Beeper", "Amiga AGA", "Other"), and
// multi-platform combos ("Atari STe,Atari ST" -> first recognised). Returns 0
// when the tag isn't a hardware platform we file (Wild, Animation/Video,
// JavaScript, ...), so the caller keeps its unclassified fallback.
static uint8_t platformNameToByte(std::string s)
{
    s = toLower(s);
    // Strip a "youtube (...)" wrapper, if present (pouet).
    if (startsWith(s, "youtube")) {
        auto op = s.find('(');
        auto cp = s.rfind(')');
        if (op != std::string::npos && cp != std::string::npos && cp > op)
            s = s.substr(op + 1, cp - op - 1);
    }
    auto trim = [](std::string x) {
        size_t a = x.find_first_not_of(" \t");
        if (a == std::string::npos) return std::string();
        return x.substr(a, x.find_last_not_of(" \t") - a + 1);
    };
    static const std::map<std::string, uint8_t> m = {
        { "amiga", AMIGA },          { "amiga ocs/ecs", AMIGA },
        { "amiga aga", AMIGA },      { "amiga ppc/rtg", AMIGA },
        { "commodore 64", SID },     { "c64", SID },
        { "c64 dtv", SID },          { "commodore 128", SID },
        { "c64dx/c65/mega65", SID },
        { "c16/116/plus4", PRG },    { "commodore plus/4", PRG },
        { "commodore 16", PRG },
        { "atari st", ATARI },       { "atari ste", ATARI },
        { "atari falcon 030", ATARI },{ "atari falcon", ATARI },
        { "atari tt 030", ATARI },
        { "atari xl/xe", POKEY },    { "atari vcs", POKEY },
        { "zx spectrum", SPECTRUM }, { "zx enhanced", SPECTRUM },
        { "zx-81", SPECTRUM },       { "spectrum", SPECTRUM },
        { "zx spectrum beeper", ZXBEEPER },
        { "amstrad cpc", AMSTRAD },  { "amstrad plus", AMSTRAD },
        { "apple ii", APPLE },       { "apple ii gs", APPLE },
        { "macos", APPLE },          { "macosx intel", APPLE },
        { "macosx ppc", APPLE },     { "ios", APPLE },
        { "acorn", ACORN },          { "bbc micro", ACORN },
        { "msx", MSX },              { "msx 2", MSX },
        { "msx turbo-r", MSX },
        { "sam coupe", SAMCOUPE },   { "sam coupé", SAMCOUPE },
        { "sega genesis/mega drive", MEGADRIVE },
        { "sega megadrive/genesis", MEGADRIVE },
        { "sega master system", SEGAMS },
        { "sega game gear", SEGAMS },{ "sega sg-1000", SEGAMS },
        { "dreamcast", DREAMCAST },
        { "nes/famicom", NES },      { "snes/super famicom", SNES },
        { "nintendo 64", NINTENDO64 },{ "nintendo ds", NDS },
        { "gameboy", GAMEBOY },      { "gameboy color", GAMEBOY },
        { "gameboy advance", GBA },
        { "nec turbografx/pc engine", HES }, { "nec pc engine", HES },
        { "playstation", PLAYSTATION }, { "playstation 2", PLAYSTATION },
        { "playstation 3", PLAYSTATION }, { "playstation portable", PLAYSTATION },
        { "windows", PC },           { "ms-dos", PC },
        { "ms-dos/gus", PC },        { "linux", PC },
        { "audiosurf", PC },
        // Real hardware, but no dedicated TAB filter -> "Other Platforms".
        { "other", OTHER },          { "atari jaguar", OTHER },
        { "atari lynx", OTHER },     { "atari 7800", OTHER },
        { "vectrex", OTHER },        { "intellivision", OTHER },
        { "vic 20", OTHER },         { "commodore pet", OTHER },
        { "oric", OTHER },           { "thomson", OTHER },
        { "enterprise", OTHER },     { "sinclair ql", OTHER },
        { "vector-06c", OTHER },     { "bk-0010/11m", OTHER },
        { "sharp mz", OTHER },       { "kc-85", OTHER },
        { "trs-80/coco/dragon", OTHER }, { "spectravideo 3x8", OTHER },
        { "wonderswan", OTHER },     { "neogeo pocket", OTHER },
        { "virtual boy", OTHER },    { "pokemon mini", OTHER },
        { "nintendo wii", OTHER },   { "gamecube", OTHER },
        { "xbox", OTHER },           { "xbox 360", OTHER },
        // No hardware chip identity of their own (fantasy consoles, web/VM,
        // mobile, calculators, compo buckets) -> "Other Platforms".
        { "wild", OTHER },           { "javascript", OTHER },
        { "java", OTHER },           { "flash", OTHER },
        { "android", OTHER },        { "pocketpc", OTHER },
        { "mobile phone", OTHER },   { "raspberry pi", OTHER },
        { "tic-80", OTHER },         { "pico-8", OTHER },
        { "microw8", OTHER },        { "ti-8x (z80)", OTHER },
        { "ti-8x (68k)", OTHER },    { "gamepark gp32", OTHER },
        { "gamepark gp2x", OTHER },
    };
    std::string whole = trim(s);
    auto it = m.find(whole);
    if (it != m.end()) return it->second;
    // Combo string ("A,B,C"): a specific chip/computer platform wins over the
    // generic OTHER tags (Wild, JavaScript, fantasy consoles...), so e.g.
    // "Wild,Amiga AGA" resolves to Amiga rather than Other.
    uint8_t generic = 0;
    for (auto const& tok : split(whole, ",")) {
        auto j = m.find(trim(tok));
        if (j != m.end()) {
            if (j->second != OTHER) return j->second;
            if (generic == 0) generic = OTHER;
        }
    }
    return generic;
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

    // Extension-authoritative overrides: a few formats are reliably identified
    // by their file extension even when the source collection mis-tags the
    // platform. e.g. demozoo/Fujiology tag Graoumf Tracker 2 (.gt2) tunes
    // "Atari ST" (which would resolve to ATARI below), but GT2 is the Windows
    // successor to the Atari Graoumf Tracker -> IBM PC. Here the extension wins
    // over the format string; the bulk format-string map handles everything else.
    if (!path.empty()) {
        std::string ext = toLower(utils::path_extension(path));
        if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
        if (ext == "gt2") return PCTRACKER; // Graoumf Tracker 2 (Windows)
    }

    uint8_t l = format_map[f];
    if (l == 0) {

        l = UNKNOWN_FORMAT;

        if ((path.find("youtube.com/") != std::string::npos) ||
            (path.find("youtu.be/") != std::string::npos)) {
            // The format string names the source platform (pouet's
            // "Youtube (<platform>)" or a hand-curated manualDatabasePatch name
            // like "ZX Spectrum Beeper"). File the video under that platform;
            // only genuinely non-hardware tags (Wild, Animation/Video, web...)
            // fall through to the unclassified YouTube bucket.
            uint8_t p = platformNameToByte(fmt);
            return p ? p : YOUTUBE;
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

        // Cache only format-string-derived results: the key is the format
        // string f, so caching an extension-derived byte here would mis-route
        // later songs that share f but have a different extension.
        if (l != UNKNOWN_FORMAT) format_map[f] = l;

        // Last resort: classify by the file extension. format_map keys many
        // extensions directly (mdx, s98, sgc, hes, ...), so a song whose stored
        // format string is missing or non-canonical still lands on the right
        // platform. This guarantees e.g. every .mdx is JPFM (PC-98/X68000/FM
        // Towns) regardless of how its source spelled the format. Not cached
        // under f (see above).
        if (l == UNKNOWN_FORMAT && !path.empty()) {
            std::string ext = toLower(utils::path_extension(path));
            if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
            if (!ext.empty()) {
                auto it = format_map.find(ext);
                if (it != format_map.end() && it->second != 0)
                    l = it->second;
            }
        }
        // fprintf(stderr, "%s\n", f.c_str());
    }

    // Refine the generic "ZX Spectrum" platform (SPECTRUM) to 128K AY (ZXAY)
    // when we can prove the tune is a 128K AY one. Demozoo/VGMRips tag these with
    // the bare "zx spectrum" platform string -- which resolves to SPECTRUM before
    // the ext fallback can run -- even though there is a definitive signal:
    //   1) the extension is an AY-chip module format (.pt3, .asc, .stc, ...), or
    //   2) the file name carries the model token "ZX Spectrum 128" (VGMRips names
    //      its .vgz rips "<Game> (ZX Spectrum 128[K])", and the 128K is the model
    //      that actually has the AY chip a .vgz logs).
    // Anything without such a signal (archives, .mp3, mis-tagged non-ZX rips)
    // stays on the generic slug.
    if (l == SPECTRUM && !path.empty()) {
        std::string ext = toLower(utils::path_extension(path));
        if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
        auto it = ext.empty() ? format_map.end() : format_map.find(ext);
        std::string lpath = toLower(path);
        if ((it != format_map.end() && it->second == ZXAY) ||
            lpath.find("spectrum 128") != std::string::npos ||
            lpath.find("spectrum_128") != std::string::npos)
            l = ZXAY;
    }

    // Some demoscene sources (demozoo / scene.org) tag a module with the
    // *release* platform of the production it appeared in ("Atari 8Bit",
    // "Atari Jaguar", "MS-Dos", ...), which misfiles it under a foreign chip
    // that can't even play it. For module formats whose platform is fixed by the
    // format itself, the extension is authoritative:
    //   .mod -> Amiga ProTracker -- but only if it landed off-Amiga; Amiga-family
    //           bytes (AMIGA/PROTRACKER/SOUNDTRACKER/UADE/TRACKER) are left alone
    //           to preserve the modland Protracker-vs-Soundtracker distinction.
    //   .xm  -> FastTracker II (IBM PC), always -- .xm has no platform variants.
    if (!path.empty()) {
        std::string ext = toLower(utils::path_extension(path));
        if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
        if (ext == "mod" && l != AMIGA && l != PROTRACKER && l != SOUNDTRACKER &&
            l != UADE && l != TRACKER)
            l = PROTRACKER;
        else if (ext == "xm")
            l = FASTTRACKER;
    }

    // Demoscene MP3/OGG rips (mainly demozoo) whose format string is the source
    // platform ("ZX Spectrum", "Amiga", "Windows", ...) rather than a codec:
    // file them under that platform instead of the unclassified MP3/OGG bucket.
    // Gated on the extension having classified them as MP3/OGG, so real chip
    // tunes and the module entries (which carry the same category strings) are
    // untouched. Generic tags with no hardware ("Demoscene", "Mobile"->Other)
    // are handled per the table; unlisted ones stay MP3/OGG.
    if (l == MP3 || l == OGG) {
        static const std::map<std::string, uint8_t> cat = {
            { "zx spectrum", SPECTRUM },
            { "amiga", AMIGA },      { "amiga ppc/rtg", AMIGA },
            { "windows", PC },       { "msx", MSX },
            { "nintendo game boy advance (gba)", GBA },
            { "nintendo ds (nds)", NDS },
            { "mobile", OTHER },     { "neo geo", OTHER },
            { "sony playstation portable (psp)", OTHER },
            { "ms-dos", OTHER },     { "linux", OTHER },
            { "gamepark gp2x", OTHER }, { "custom hardware", OTHER },
        };
        auto it = cat.find(f);
        if (it != cat.end()) l = it->second;
    }
    return l;
}

// Human-readable platform name for a format byte, used by describeFormat().
// Mirrors the TAB platform filters (see ChipMachine::filterOptions).
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
    case SPECTRUM: return "ZX Spectrum";
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
    case OTHER: return "Other";
    case ARCADE: return "Arcade";
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

// The platform a bare file of extension `ext` belongs to: the platform of the
// highest-priority plugin that plays it -- the same plugin the GUI would pick
// (mirrors priority_map). This is the app-owned source of truth for the
// extension->platform question, used by tooling/tests. It is DISTINCT from
// classifyFormat(): a real DB row always carries a format string, which is the
// richer signal (it alone disambiguates shared extensions like .sng = GoatTracker
// vs Sam Coupe vs Richard Joseph), so classifyFormat() stays format-string-driven
// and remains the runtime classifier. Here there is no format string -- only the
// extension and the plugins claiming it -- so the plugin decides. Returns "" when
// the extension maps to no known platform (which the test asserts never happens).
//
// Requires the plugins to be loaded (ChipPlugin::createPlugins); returns "" if not.
std::string MusicDatabase::platformForExtension(std::string const& rawExt)
{
    using std::string;
    auto ext = toLower(rawExt);

    // Multi-platform plugins (GME, HTPlugin, Audio Overload, ffmpeg) carry
    // unrelated systems under one plugin, so the *extension* picks the platform.
    static const std::map<string, string> extPlatform = {
        // Game Music Engine
        { "nsf", "Nintendo NES" }, { "nsfe", "Nintendo NES" },
        { "spc", "Nintendo SNES" }, { "gbs", "Nintendo Game Boy" },
        { "gbr", "Nintendo Game Boy" }, { "gym", "Sega Mega Drive" },
        { "vgm", "Sega Mega Drive" }, { "vgz", "Sega Mega Drive" },
        { "sgc", "Sega 8-bit" }, { "hes", "PC Engine" }, { "kss", "MSX" },
        { "ay", "ZX Spectrum 128" }, { "emul", "ZX Spectrum 128" },
        { "sap", "Atari XL/XE" },
        // HTPlugin / Audio Overload
        { "dsf", "Sega Dreamcast" }, { "minidsf", "Sega Dreamcast" },
        { "ssf", "Sega Saturn" }, { "minissf", "Sega Saturn" },
        { "qsf", "Sega Saturn" }, { "miniqsf", "Sega Saturn" },
        { "spu", "PlayStation" },
        // ffmpeg
        { "mp3", "MP3" }, { "ogg", "OGG" }, { "8svx", "Amiga" },
        { "aac", "PC" }, { "m4a", "PC" }, { "mp4", "PC" },
        // .cop is SAA1099 Sam Coupe, not the ZX Spectrum its plugin otherwise serves
        { "cop", "Sam Coupe" },
    };
    if (auto it = extPlatform.find(ext); it != extPlatform.end())
        return it->second;

    // Highest-priority plugin claiming this extension (what plays in the app).
    string primary;
    int best = 0;
    bool found = false;
    for (auto const& plugin : musix::ChipPlugin::getPlugins()) {
        for (auto const& raw : plugin->getSupportedExtensions()) {
            if (toLower(raw) != ext) continue;
            if (!found || plugin->priority() > best) {
                primary = plugin->name();
                best = plugin->priority();
                found = true;
            }
        }
    }
    if (!found) return "";

    // OpenMPT tracker formats split Amiga (mod/med/okt/...) vs PC (xm/s3m/it/...);
    // the format-string classifier knows which is which.
    if (primary == "OpenMPT") {
        auto p = platformName(formatToByte(ext, "dummy." + ext, 0));
        return p.empty() ? string("PC") : p;
    }

    // Most plugins target a single hardware platform.
    static const std::map<string, string> pluginPlatform = {
        { "UADE", "Amiga" },
        { "AdPlug", "PC AdLib" },
        { "Ayfly ZX", "ZX Spectrum 128" },
        { "ZX Spectrum (ZXTune)", "ZX Spectrum 128" },
        { "RSNPlugin", "Nintendo SNES" },
        { "MSX (libkss)", "MSX" },
        { "HEPlugin", "PlayStation" },
        { "libvice", "Commodore 64" },
        { "SC68", "Atari ST/STE" },
        { "FMPPlugin", "PC-98 / X68000" },
        { "V2Plugin", "PC" },
        { "USFPlugin", "Nintendo 64" },
        { "StSound", "Atari ST/STE" },
        { "STarKos", "Amstrad CPC" },
        { "Quartet", "Atari ST/STE" },
        { "PxTone Collage Player", "PC" },
        { "NDSPlugin", "Nintendo DS" },
        { "HivelyPlugin", "Amiga" },
        { "Gameboy Advance", "Nintendo Game Boy" },
        { "WonderSwan (in_wsr)", "WonderSwan" },
        { "Tedplay", "Commodore 16/+4" },
        { "SunVox Player", "PC" },
        { "SoundSmith", "Apple IIGS" },
        { "SBStudio", "PC" },
        { "S98", "PC-98 / X68000" },
        { "PokeyNoise", "Atari XL/XE" },
        { "Organya Player", "PC" },
        { "NerdTracker2", "Nintendo NES" },
        { "Monotone", "PC" },
        { "MikMod", "Amiga" },
        { "Megatracker", "Atari ST/STE" },
        { "MED", "Amiga" },
        { "MaxTrax", "Amiga" },
        { "MDX", "PC-98 / X68000" },
        { "JayTrax", "PC" },
        { "IXS", "PC" },
        { "Euphony", "PC-98 / X68000" },
        { "Coconizer", "Acorn Archimedes" },
        { "BBSong", "ZX Spectrum 16/48" },
        { "Archimedes Tracker", "Acorn Archimedes" },
        { "SCC-Musixx", "MSX" },
        { "GoatTracker", "Commodore 64" },
        { "DMF", "PC" },
    };
    if (auto it = pluginPlatform.find(primary); it != pluginPlatform.end())
        return it->second;
    return "";
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
    std::string slug = platformSlugForByte(formatToByte(s.format, s.path, 0));
    if (!slug.empty())
        return slug;
    // The format string can be empty/unknown for streamed console rips whose DB
    // row carries no format name (e.g. .sgc served over HTTP). Fall back to the
    // file extension, which format_map also keys (sgc, gc, hes, mdx, ...).
    std::string ext = toLower(s.ext);
    if (ext.empty()) {
        ext = toLower(utils::path_extension(s.path));
        if (!ext.empty() && ext[0] == '.')
            ext = ext.substr(1);
    }
    if (!ext.empty())
        slug = platformSlugForByte(formatToByte(ext, s.path, 0));
    return slug;
}

std::string MusicDatabase::platformScreenshotSlug(uint8_t formatByte)
{
    return platformSlugForByte(formatByte);
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

std::map<std::string, std::set<std::string>> MusicDatabase::extensionPlatforms()
{
    std::map<std::string, std::set<std::string>> out;
    try {
        sqlite3db::Database db{
            (Environment::getCacheDir() / "music.db").string()
        };
        auto q = db.query<std::string, std::string, std::string>(
            "SELECT DISTINCT ext, format, path FROM song");
        std::string ext, fmt, path;
        // A token is a "known format" if formats_descriptions describes it.
        // (Container exts like gz/zip/lha aren't described, so they never match.)
        auto described = [this](std::string e) {
            return !e.empty() && !describeExtension(e).empty();
        };
        while (q.step()) {
            std::tie(ext, fmt, path) = q.get_tuple();
            ext = toLower(ext);
            if (ext.empty()) continue;
            // Disqualify bogus prefix-form suffixes. Modland/scene names come as
            // "<prefix>.<songname>" (e.g. "mod.brax", "cust.henk", "pn.jetset")
            // where the real format is the LEADING token; the indexer stored the
            // TRAILING token ("brax") as ext, which is just the song name -- not a
            // format. When the prefix is a recognized format and the stored ext is
            // not, remap to the prefix so the "missing screenshot" report lists
            // the real format (usually already covered) instead of a phantom
            // extension. Uses both recognizers the playback path relies on:
            // describeExtension() (formats_descriptions) and getTypeAndBase()'s
            // modland prefix list (pn/ash/jpn/smpl/sng/...).
            std::string leaf = utils::path_filename(path);
            if (auto qp = leaf.find('?'); qp != std::string::npos)
                leaf = leaf.substr(0, qp);
            auto fd = leaf.find_first_of('.');
            std::string prefix =
                fd != std::string::npos ? toLower(leaf.substr(0, fd)) : "";
            bool knownPrefix =
                described(prefix) || toLower(getTypeFromName(leaf)) == prefix;
            if (!prefix.empty() && knownPrefix && !described(ext)) ext = prefix;
            // Pass the real path (not "") so formatToByte's extension fallback
            // and per-extension corrections fire exactly as they do at play time.
            // Without it, formats identified only by their extension (e.g. .mon
            // tunes tagged with a generic "Demoscene" platform string) resolve to
            // no platform here and get mis-reported as needing a screenshot, even
            // though playback classifies them fine.
            out[ext].insert(platformSlugForByte(formatToByte(fmt, path, 0)));
        }
    } catch (...) {
        // No cached DB yet (first run) or it is mid-reindex -- skip the report.
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

std::string MusicDatabase::getPodcastShowArtwork(int rowid) const
{
    std::lock_guard lock{ dbMutex };
    try {
        // 1) The curated show-level artwork (collection.artwork) -- the real logo
        //    for most shows. EXCEPT the archive.org "services/img/<id>" endpoint,
        //    which returns a generic auto-generated placeholder (a grayscale
        //    waveform or the "no image" icon) when the item has no cover, e.g. the
        //    Completely Unnecessary Podcast. Skip that so step 2 can do better.
        {
            auto q = db.query<std::string>(
                "SELECT artwork FROM collection WHERE ROWID = ?", rowid);
            if (q.step()) {
                std::string art = q.get();
                if (!art.empty() &&
                    art.find("archive.org/services/img/") == std::string::npos)
                    return art;
            }
        }
        // 2) No usable show logo: fall back to the MOST COMMON episode artwork.
        //    The show's own logo recurs across many episodes (so it wins the
        //    count), while one-off per-episode covers appear once -- this reliably
        //    picks the branding rather than an arbitrary episode. ROWID order is
        //    NOT chronological (RSS feeds index newest-first, .txt oldest-first),
        //    so "first/last episode" cannot mean "newest"; frequency is the robust
        //    representative.
        {
            auto q = db.query<std::string>(
                "SELECT artwork FROM song WHERE collection = ? AND artwork LIKE "
                "'http%' GROUP BY artwork ORDER BY COUNT(*) DESC, MIN(ROWID) "
                "LIMIT 1",
                rowid);
            if (q.step()) {
                std::string art = q.get();
                if (!art.empty()) return art;
            }
        }
        // 3) Some shows (Demovibes) store the per-episode image URL in the
        //    metadata column instead. Others store a text description there, which
        //    the 'http%' filter plus the image-extension check below reject. Use
        //    the most common one here too, for the same reason as step 2.
        {
            auto q = db.query<std::string>(
                "SELECT metadata FROM song WHERE collection = ? AND metadata LIKE "
                "'http%' GROUP BY metadata ORDER BY COUNT(*) DESC, MIN(ROWID) "
                "LIMIT 1",
                rowid);
            if (q.step()) {
                std::string m = q.get();
                std::string lower = toLower(m);
                auto qpos = lower.find('?'); // strip any query string
                std::string probe =
                    qpos == std::string::npos ? lower : lower.substr(0, qpos);
                for (auto ext : { ".jpg", ".jpeg", ".png", ".gif" })
                    if (endsWith(probe, ext)) return m;
            }
        }
    } catch (std::exception const& e) {
        LOGD("getPodcastShowArtwork failed: %s", e.what());
    }
    return "";
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

void MusicDatabase::buildSubPlatforms()
{
    if (builtSubPlatformByte == subPlatformByte) return;

    uint32_t n = (productStartIndex > 0 &&
                  productStartIndex <= (uint32_t)formats.size())
                     ? productStartIndex
                     : (uint32_t)formats.size();
    // Not indexed yet: don't cache an empty result -- retry on the next call.
    if (n == 0) return;

    builtSubPlatformByte = subPlatformByte;
    otherPlatformList.clear();
    otherGroupCount.clear();
    otherIndexToGroup.clear();

    auto trim = [](std::string x) {
        size_t a = x.find_first_not_of(" \t");
        if (a == std::string::npos) return std::string();
        return x.substr(a, x.find_last_not_of(" \t") - a + 1);
    };

    // The active format byte (OTHER / ARCADE) collapses many real platforms into
    // one filter, so a song's sub-platform survives only as its DB format string.
    // Recover it with one scan of the song table -- search position i maps to
    // song.ROWID i+1 (contiguous; see getSongInfo / syncPodcastSongs) -- and
    // group by name.
    std::map<std::string, std::vector<int>> byName;
    auto q = db.query<int, std::string>("SELECT ROWID, format FROM song");
    while (q.step()) {
        int rowid;
        std::string fmt;
        tie(rowid, fmt) = q.get_tuple();
        int idx = rowid - 1;
        if (idx < 0 || idx >= (int)n) continue;
        if ((formats[idx] & 0xff) != subPlatformByte) continue;
        std::string name = trim(fmt);
        if (name.empty()) name = "Unknown";
        // The bare "Arcade" group sits alongside the vendor-specific ones
        // (Arcade (Capcom), ...), so disambiguate it as "Arcade (Other)"; and
        // fold Neo Geo in as another vendor-style "Arcade (Neo Geo)" group.
        if (subPlatformByte == ARCADE) {
            if (toLower(name) == "arcade") name = "Arcade (Other)";
            else if (toLower(name) == "neo geo") name = "Arcade (Neo Geo)";
        }
        byName[name].push_back(idx);
    }

    // Assign group ids in alphabetical (case-insensitive) name order, so the
    // groupId equals the position in otherPlatformList / otherGroupCount.
    std::vector<std::string> names;
    names.reserve(byName.size());
    for (auto const& kv : byName) names.push_back(kv.first);
    // Alphabetical (case-insensitive), except "Arcade (Other)" is forced last so
    // the catch-all bucket sits below the named vendor groups.
    std::sort(names.begin(), names.end(), [](auto const& a, auto const& b) {
        bool ao = (a == "Arcade (Other)"), bo = (b == "Arcade (Other)");
        if (ao != bo) return bo; // a before b iff b is the catch-all
        return toLower(a) < toLower(b);
    });
    for (int gid = 0; gid < (int)names.size(); gid++) {
        auto const& idxs = byName[names[gid]];
        otherPlatformList.emplace_back(gid, names[gid]);
        otherGroupCount.push_back((int)idxs.size());
        for (int i : idxs) otherIndexToGroup[i] = gid;
    }
}

int MusicDatabase::getOtherPlatformCount()
{
    std::lock_guard lock{ dbMutex };
    subPlatformByte = OTHER;
    buildSubPlatforms();
    return (int)otherPlatformList.size();
}

int MusicDatabase::getArcadePlatformCount()
{
    std::lock_guard lock{ dbMutex };
    subPlatformByte = ARCADE;
    buildSubPlatforms();
    return (int)otherPlatformList.size();
}

// Compression/archive extensions that wrap a real module. They must never be
// surfaced as a song's format -- the playable format lives in the inner file.
static bool isContainerExt(std::string e)
{
    if (!e.empty() && e[0] == '.') e = e.substr(1);
    e = toLower(e);
    static const std::set<std::string> k = { "lha", "gz",  "zip", "rar",
                                             "lzh", "lzx", "z",   "7z" };
    return k.count(e) > 0;
}

MusicDatabase* MusicDatabase::self = nullptr;

std::string MusicDatabase::resolveExtension(SongInfo const& s)
{
    // Prefer the stored ext, but never a container wrapper.
    std::string ext = toLower(s.ext);
    if (!ext.empty() && ext[0] == '.') ext = ext.substr(1);
    if (!ext.empty() && !isContainerExt(ext)) return ext;

    // Derive from the path. For an ".lha/<member>" path use the member's FILE
    // NAME: members can live in a subdir (e.g. "Custom_Version/cust.ingame"),
    // and the subdir must not pollute the prefix/suffix tokens below.
    std::string path = s.path;
    auto lpos = toLower(path).find(".lha/");
    std::string member = (lpos != std::string::npos) ? path.substr(lpos + 5) : path;
    std::string leaf = utils::path_filename(member);

    // Strip a URL query string ("song.mod?id=1" -> "song.mod").
    auto q = leaf.find('?');
    if (q != std::string::npos) leaf = leaf.substr(0, q);

    // Strip trailing container wrappers ("x.sid.gz" -> "x.sid").
    for (bool stripped = true; stripped;) {
        stripped = false;
        auto d = leaf.find_last_of('.');
        if (d == std::string::npos) break;
        if (isContainerExt(leaf.substr(d + 1))) {
            leaf = leaf.substr(0, d);
            stripped = true;
        }
    }

    // Modland/UnExoticA names come in suffix-form ("song.mod" -> "mod") and
    // prefix-form ("cust.ingame", "mod.song" -> "cust"/"mod"). Prefer whichever
    // token the formats_descriptions table actually recognizes, so prefix-form
    // custom players resolve to their real format ("cust") rather than the
    // meaningless song-name suffix ("ingame"). This applies to ALL songs, not
    // just compressed ones.
    auto firstDot = leaf.find_first_of('.');
    auto lastDot = leaf.find_last_of('.');
    std::string suffix =
        lastDot != std::string::npos ? toLower(leaf.substr(lastDot + 1)) : "";
    std::string prefix =
        firstDot != std::string::npos ? toLower(leaf.substr(0, firstDot)) : "";
    auto described = [](std::string const& e) {
        return self != nullptr && !e.empty() && !isContainerExt(e) &&
               !self->describeExtension(e).empty();
    };
    if (described(suffix)) return suffix;
    if (described(prefix)) return prefix;

    // Neither token is a described format. getTypeAndBase still resolves the
    // core modland prefixes ("mdat", "smp", ...); else fall back to the suffix.
    std::string type = toLower(getTypeFromName(leaf));
    if (!type.empty() && !isContainerExt(type)) return type;
    if (!suffix.empty() && !isContainerExt(suffix)) return suffix;

    // Last resort: some formats have NO extension on disk (the song is a
    // bare-named file, e.g. Apple IIgs SoundSmith on Modland), so neither the
    // path nor the stored ext yields a key. Map the DB format NAME to its
    // canonical described extension so the scroller still finds a description.
    std::string fmt = toLower(s.format);
    if (!fmt.empty()) {
        static const std::map<std::string, std::string> nameToExt = {
            { "soundsmith", "w" },
        };
        auto it = nameToExt.find(fmt);
        if (it != nameToExt.end()) return it->second;
    }
    return "";
}

std::string MusicDatabase::describeFormat(SongInfo const& s)
{
    uint8_t b = formatToByte(s.format, s.path, 0);

    // YouTube videos have no module format/extension. The format string carries
    // the source platform(s) in parentheses (e.g. "Youtube (ZX Spectrum)") or a
    // bare platform name (manualDatabasePatch: "Amiga AGA", "ZX Spectrum
    // Beeper"); surface it as "YouTube - <platform>". Keyed on the URL, not the
    // format byte, because these now classify to their real platform (SID,
    // AMIGA, ...) so they group under that TAB filter instead of "unclassified".
    if (s.path.find("youtube.com/") != std::string::npos ||
        s.path.find("youtu.be/") != std::string::npos) {
        // Friendlier playback labels for a few tags (the raw pouet tag is terse
        // or ambiguous): shown as "YouTube - <designator>".
        static const std::map<std::string, std::string> designator = {
            { "wild", "Wild Compo" },
            { "tic-80", "TIC-80 Fantasy Console" },
            { "pico-8", "PICO-8 Fantasy Console" },
            { "microw8", "MicroW8 Fantasy Console" },
            { "mobile phone", "Misc Mobile Phone" },
            { "ti-8x (z80)", "TI Calculator" },
            { "ti-8x (68k)", "TI Calculator" },
            { "gamepark gp32", "GamePark GP32" },
            { "gamepark gp2x", "GamePark GP32" },
        };
        std::string inner;
        auto open = s.format.find('(');
        auto close = s.format.rfind(')');
        if (open != std::string::npos && close != std::string::npos &&
            close > open + 1)
            inner = s.format.substr(open + 1, close - open - 1);
        else if (!s.format.empty() &&
                 toLower(s.format).find("youtube") == std::string::npos)
            inner = s.format; // manualDatabasePatch bare platform name
        if (inner.empty()) return "YouTube";
        auto d = designator.find(toLower(inner));
        return "YouTube - " + (d != designator.end() ? d->second : inner);
    }

    // Podcasts are streamed audio; the enclosure "extension" is derived from the
    // URL and often carries a query string (".mp3?p=f", ".mp3?dest-id=..."), so
    // skip the "(EXT)" suffix entirely and just label them "Podcast".
    if (b == PODCAST) return "Podcast";

    // Atari 2600 (VCS / TIA) demoscene rips carry the verbose platform string
    // "Atari 2600 Video Computer System (VCS)" and are bucketed under POKEY
    // (Atari 8Bit) for the TAB filter -- but that string is a machine descriptor,
    // not a tracker name, and the files are zip/gz/mp3 rips with no meaningful
    // inner format. Surface the concise machine name (like YouTube/Podcast
    // above), so it reads "Atari 2600" instead of the misleading
    // "Atari XL/XE - Atari 2600 Video Computer System (VCS)".
    if (toLower(s.format) == "atari 2600 video computer system (vcs)")
        return "Atari 2600";

    // Extension (uppercase, no dot). resolveExtension() gives the REAL inner
    // format, so a compressed song shows "(MOD)" not "(ZIP)" -- and never the
    // container wrapper.
    std::string ext = resolveExtension(s);
    for (auto& c : ext) c = (char)toupper((unsigned char)c);

    // When the format string didn't classify (empty/unknown), reclassify by the
    // file extension so streamed rips that carry no format name still get a real
    // platform (e.g. ".sgc"/"gc" -> Sega 8-bit) instead of a bare "GC".
    if (platformName(b).empty() && !ext.empty()) {
        std::string le = toLower(ext);
        if (le[0] == '.') le = le.substr(1);
        b = formatToByte(le, s.path, 0);
    }

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
    // Atari Falcon sub-label. The YM2149 ST chiptune formats (sndh/ym/sc68/TCB/
    // Hippel ST/Quartet/Special FX...) keep "Atari ST/STE", but the Falcon-native
    // sample trackers -- Graoumf Tracker (.gtk), Digital Tracker (.dtm), Digi-Mix
    // (.mix) -- show "Atari Falcon" instead. They stay the ATARI platform byte
    // (same TAB "Atari ST/STE/Falcon" filter and classification); only the
    // displayed platform word changes. Keyed on ext under b==ATARI, so the
    // unrelated .dtm variants (DeFy AdLib / DigiTrekker) -- which classify as
    // PC/AdLib, not ATARI -- are never relabelled. The Falcon logo is already
    // handled separately by the per-extension screenshots (gtk/dtm/mix.png).
    if (b == ATARI && (ext == "GTK" || ext == "DTM" || ext == "MIX")) {
        plat = "Atari Falcon";
    }

    if (b == APPLE && ext == "MAD") {
        plat = "Macintosh";
    }

    // A .mod is an Amiga ProTracker module. Some demoscene rips carry a stale
    // release-platform tag as their format string (reclassified to Amiga in
    // formatToByte), which would otherwise render as the name -- e.g. the
    // contradictory "Amiga - Atari 8Bit (MOD)". Surface "ProTracker" instead.
    if (b == PROTRACKER && ext == "MOD") name = "ProTracker";
    if (b == FASTTRACKER && ext == "XM") name = "FastTracker II";


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
        // Each entry spans two lines: "<ext>\t<trackers>" followed by a prose
        // description, then a blank separator line.
        // NOTE: only mark the table as loaded once the file is actually found.
        // describeExtension() can be called (via resolveExtension) before
        // workDir is set, when File{"", ...} resolves to nothing -- caching an
        // empty table there would permanently blank every format description.
        File f{ workDir.string(), "data/misc/formats_descriptions.txt" };
        if (f.exists()) {
            formatDescriptionsLoaded = true;
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
    readVector(formatKey, f);

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
    writeVector(formatKey, f);

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
        } else if (c.name == "mirsoft") {
            // mirsoft.info game-mod zips: serve from the Internet Archive
            // snapshot first (mirsoftJuly2021snapshot/gamemods/<Game>.zip), with
            // the live mirsoft host (c.url) as fallback for anything the 2021
            // snapshot lacks -- so mirsoft's own server is never crawled. Same
            // primary+fallback shape as hvtc, just Archive-primary.
            std::string live = c.url;
            std::string archive =
                "https://archive.org/download/mirsoftJuly2021snapshot/gamemods/";
            loader.registerSource(c.name, archive, c.local_dir.string(), live);
        } else {
            loader.registerSource(c.name, c.url, c.local_dir.string());
        }
    }
    // Load per-collection search priority (ROWID-indexed). Done here -- before
    // the cached-index early-return -- so it is available every launch, since
    // search() runs whether or not the title index was rebuilt.
    collPriority.clear();
    try {
        auto pq = db.query<int, int>(
            "SELECT ROWID, IFNULL(priority, 0) FROM collection");
        while (pq.step()) {
            auto [rowid, prio] = pq.get_tuple();
            if (rowid >= (int)collPriority.size())
                collPriority.resize(rowid + 1, 0);
            collPriority[rowid] = prio;
        }
    } catch (...) {
        // Pre-priority-column DB (older schema that a version bump will rebuild):
        // fall back to no reordering rather than crashing.
        collPriority.clear();
    }

    auto indexPath = Environment::getCacheDir() / "index.dat";

    if (!reindexNeeded && utils::exists(indexPath)) {
        readIndex(apone::File{ indexPath });
        return;
    }

    // A real rebuild is about to run (version bump, new collection, podcast
    // refresh, or a missing index.dat) -- flag it so the UI shows the progress
    // bar. A plain cached load returns above and never reaches here.
    reindexingNow.store(true, std::memory_order_relaxed);

    print_fmt("Creating Search Index...\n");

    std::string oldComposer;
    auto query = db.query<std::string, std::string, std::string, std::string,
                          std::string, int, std::string>(
        "SELECT title, game, format, composer, path, collection, "
        "IFNULL(ext,'') FROM song");

    // The "radio" collection holds live streaming stations whose format tags
    // (M3U/MP3) are shared with regular content; tag them by collection ROWID so
    // they get a dedicated RADIO format byte (and their own TAB filter).
    int radioColl = -1;
    try {
        auto cq =
            db.query<int>("SELECT ROWID FROM collection WHERE id='radio'");
        if (cq.step()) radioColl = cq.get();
    } catch (...) {}

    // Collections whose MP3/OGG rips carry a fixed "MP3"/"OGG" format string but
    // a known platform affinity (they're renders/remixes of that platform's
    // music). Resolve their ROWIDs so the index loop can file them under that
    // platform instead of the unclassified MP3/OGG bucket.
    auto collId = [&](char const* id) -> int {
        try {
            auto cq = db.query<int>(
                "SELECT ROWID FROM collection WHERE id = ?", id);
            if (cq.step()) return cq.get();
        } catch (...) {}
        return -1;
    };
    int rkoColl = collId("rko");             // C64 SID remixes (Remix.Kwed.Org)
    int amiremixColl = collId("amigaremix"); // Amiga remixes
    int unexoticaColl = collId("unexotica"); // Amiga games music (mp3 rips)
    int zxartColl = collId("zxart");         // ZX tunes rendered to ogg

    int count = 0;
    // int maxTotal = 3;
    int cindex = 0;

    titleToComposer.reserve(50000);
    composerToTitle.reserve(50000);
    titleIndex.reserve(700000);
    composerIndex.reserve(60000);
    formats.reserve(700000);
    formatKey.reserve(700000);

    int step = 700000 / 5;

    std::unordered_map<std::string, std::vector<uint32_t>> composers;

    // Intern the REAL module extension of each song into a small stable id for
    // the search dedup key (formatKey). 0 is reserved for "unknown format".
    std::unordered_map<std::string, uint32_t> extIds;
    auto internExt = [&](std::string const& fmtStr,
                         std::string const& pathStr) -> uint32_t {
        // Prefer the stored `ext` column (the true format even when the path is
        // a container like "<Game>.zip"); fall back to the path's extension.
        std::string e = toLower(fmtStr);
        if (e.empty()) {
            e = toLower(utils::path_extension(pathStr));
            if (!e.empty() && e[0] == '.') e = e.substr(1);
        }
        if (e.empty()) return 0;
        auto it = extIds.find(e);
        if (it != extIds.end()) return it->second;
        uint32_t id = (uint32_t)extIds.size() + 1;
        extIds.emplace(e, id);
        return id;
    };

    std::string title, game, fmt, composer, path, ext;
    int collection;

    while (count < 1000000) {
        count++;
        if (!query.step()) break;

        if (count % step == 0) {
            LOGD("%d songs indexed", count);
        }
        // Publish progress for the startup bar every 2000 rows (cheap, smooth).
        if (count % 2000 == 0)
            indexedCount.store(count, std::memory_order_relaxed);

        tie(title, game, fmt, composer, path, collection, ext) =
            query.get_tuple();

        // Real-format token for the search dedup key (see add_unique).
        formatKey.push_back(internExt(ext, path));

        uint8_t b = formatToByte(fmt, path, collection);
        if (collection == radioColl)
            b = RADIO;
        else if (b == MP3 && collection == rkoColl)
            b = SID; // C64 SID remixes
        else if (b == MP3 &&
                 (collection == amiremixColl || collection == unexoticaColl))
            b = AMIGA; // Amiga remixes / rips
        else if (b == OGG && collection == zxartColl)
            b = SPECTRUM; // ZX ogg fallbacks -> ZX Spectrum (AY filter)
        formats.push_back(b | (collection << 8));
        // Hue key: the sub-format, so each distinct format gets its own hue
        // (spread evenly across the platform filter). Same format = same colour
        // (e.g. every .pt2 tune matches, and differs from .pt3 / .asc). Two
        // exceptions:
        //  - Podcasts all share the format "Podcast" -> key by show (collection)
        //    so episodes of one podcast share a hue and differ from another's.
        //  - zxart tags every tune with a coarse chip name ("Spectrum AY",
        //    "Spectrum Beeper", "Sam Coupe") and demozoo tags them "ZX Spectrum"
        //    -- one string for a dozen distinct sub-formats. For these, key by
        //    the file EXTENSION (the real sub-format: pt3/pt2/asc/vtx/stc/...)
        //    so each format gets its own hue instead of all sharing one.
        std::string hueKey;
        if (fmt == "Podcast") {
            hueKey = "podcast#" + std::to_string(collection);
        } else {
            std::string lf = toLower(fmt);
            if (lf == "spectrum ay" || lf == "spectrum beeper" ||
                lf == "sam coupe" || lf == "zx spectrum")
                hueKey = lf + "|" + toLower(utils::path_extension(path));
            else
                hueKey = fmt;
        }
        formatHue.push_back(hueSeed(hueKey));

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
        // Products share one stable format token, so they still dedup by
        // {title,composer} (as before), never on a real module extension.
        formatKey.push_back(internExt("product", ""));
        // Tag the product with a platform byte (from its `type`) so the TAB
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
    reindexingNow.store(false, std::memory_order_relaxed);
    indexedCount.store(0, std::memory_order_relaxed);
    dbCreatedCount.store(0, std::memory_order_relaxed);
    initFuture = std::async(std::launch::async, [=]() {
        std::lock_guard lock{ dbMutex };
        if (!initFromLua(workDir)) {
        }
        std::lock_guard lock2{ chkMutex };
        indexing = false;
    });
}

void MusicDatabase::loadUnsupportedExtensions(utils::path const& workDir)
{
    unsupportedExts.clear();
    auto f = findFile(workDir.string(), "data/misc/not_supported_extensions.txt");
    if (!f) {
        LOGW("not_supported_extensions.txt not found; indexing all extensions");
        return;
    }
    std::string shown;
    for (auto line : utils::File{ *f }.getLines()) {
        auto a = line.find_first_not_of(" \t\r\n");
        if (a == std::string::npos) continue; // blank
        auto b = line.find_last_not_of(" \t\r\n");
        line = line.substr(a, b - a + 1);
        if (line[0] == '#') continue;       // commented-out -> stays indexable
        if (line[0] == '.') line.erase(0, 1); // strip leading dot
        if (line.empty()) continue;
        auto ext = toLower(line);
        if (unsupportedExts.insert(ext).second) shown += " ." + ext;
    }
    LOGI("INDEX SKIPPED UNSUPPORTED EXTENSIONS:%s", shown.c_str());
}

bool MusicDatabase::initFromLua(utils::path const& workDir)
{
    this->workDir = workDir;
    loadUnsupportedExtensions(workDir);
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
            // A throw here aborts indexing of the WHOLE collection partway (e.g.
            // a stoi crash on one bad row left Demozoo at ~3k of ~42k songs), and
            // the only symptom is this one line at boot. Make it impossible to
            // miss: bright-red, banner-prefixed (ANSI \x1b[1;31m ... \x1b[0m).
            LOGE("\x1b[1;31m!!!!!!!!!!!!!!! Error creating database '%s': "
                 "%s\x1b[0m",
                 db_name, e.what());
        } catch (...) {
            LOGE("\x1b[1;31m!!!!!!!!!!!!!!! Unknown error creating database "
                 "'%s'\x1b[0m",
                 db_name);
        }
        dbmap.clear();
    };

    lua["set_db_var"] = [&](std::string const& name, sol::object val) {
        if (val.is<std::string>())
            dbmap[name] = val.as<std::string>();
        else if (val.is<bool>())
            dbmap[name] = val.as<bool>() ? "yes" : "no";
        else if (val.get_type() == sol::type::number)
            // Signed so negative priorities (sink a collection below the
            // default-0 mass) survive; positive numbers (color, priority) are
            // unaffected.
            dbmap[name] = std::to_string(val.as<int64_t>());
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
        reindexingNow.store(true, std::memory_order_relaxed);
    } else if (podcastsChanged) {
        // A background feed refresh found new episodes (full catalogue already
        // in the cache XML). Don't drop/re-parse the big collections -- just
        // append the new episodes to the song table and rebuild the search
        // index from the table. Append-only keeps song ROWIDs contiguous, which
        // getSongInfo relies on (search position i <-> song.ROWID i+1).
        syncPodcastSongs();
        reindexNeeded = true;
        reindexingNow.store(true, std::memory_order_relaxed);
    }

    checkingNames.clear();
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

    LOGD("Initialized DBs: %s", checkingNames);

    if (totalSongs > 0) {
        print_fmt("Total songs count: %d\n", totalSongs);
    }
    int dbCount = dbCreatedCount.load(std::memory_order_relaxed);
    if (dbCount > 0) {
        print_fmt("Total databases count: %d\n", dbCount);
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

