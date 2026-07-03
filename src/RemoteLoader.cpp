#include "RemoteLoader.h"

#include <coreutils/log.h>
#include <coreutils/split.h>

#include <algorithm>

using namespace std;
using namespace utils;

RemoteLoader::RemoteLoader()
    : webgetter((Environment::getCacheDir() / "_webfiles").string())
{
    // webgetter.setErrorCallback([](int code, const string &msg) {
    //	LOGD("Error %d %s", code, msg);
    //});
}

void RemoteLoader::registerSource(const std::string& name,
                                  const std::string url,
                                  const std::string local_dir,
                                  const std::string fallback_url)
{
    Source s(url, local_dir, fallback_url);
    if (s.local_dir != "" && !endsWith(s.local_dir, "/")) s.local_dir += "/";
    sources[name] = s;
}

bool RemoteLoader::inCache(const std::string& p) const
{
    // Collapsed-game entries (UnExoticA et al) store every subsong of a game as
    // a single tab-separated "MULTI:<sub0>\t<sub1>\t..." path. Such a game is
    // "locally present" if ANY of its subsongs is cached. Checking all of them
    // is essential: UnExoticA references stale per-version archives (e.g.
    // ".../Hybris/Custom_Version.lha") that 550 at play time, so the tune that
    // actually gets fetched/extracted lives in a DIFFERENT sibling archive
    // (".../Hybris.lha") named only by a later subsong.
    {
        const auto parts = split(p, "::", 2);
        std::string prefix, rel;
        if (parts.size() > 1) {
            prefix = parts[0];
            rel = parts[1];
        } else {
            rel = p;
        }
        if (startsWith(rel, "MULTI:")) {
            for (char const* sub : split(rel.substr(6), "\t")) {
                if (sub == nullptr || *sub == '\0') continue;
                std::string subPath = prefix.empty()
                                          ? std::string(sub)
                                          : (prefix + "::" + sub);
                if (inCache(subPath)) return true;
            }
            return false;
        }
    }

    // LHA-packed sources (UnExoticA et al): the played tune is not the member
    // URL in the web cache but an extracted member under
    // <cache>/_lha2/<safeName>/<member>. The web cache only ever holds the .lha
    // archive (keyed by its own URL), so the plain member-URL lookup below
    // always misses and these tunes never report as cached (no "*" hint, and a
    // spurious LOADING toast). Mirror MusicPlayerList::loadLhaSong's path scheme
    // and treat an extracted member as a cache hit.
    if (toLower(p).find(".lha/") != string::npos) {
        string prefix, rel;
        const auto parts = split(p, "::");
        if (parts.size() > 1) {
            prefix = parts[0];
            rel = parts[1];
        } else {
            rel = p;
        }
        auto lpos = toLower(rel).find(".lha/");
        if (lpos != string::npos) {
            string archiveRel = rel.substr(0, lpos + 4); // ".../X.lha"
            string member = rel.substr(lpos + 5);         // member after ".lha/"
            string safeName = prefix + archiveRel;
            std::replace(safeName.begin(), safeName.end(), '/', '_');
            string memberFile =
                (Environment::getCacheDir() / "_lha2" / safeName).string() +
                "/" + member;
            if (File::exists(memberFile)) return true;
        }
        // Not extracted yet -- fall through to the normal checks.
    }

    Source source;
    string path = p;
    const auto parts = split(path, "::");
    if (parts.size() > 1) {
        source = sources.at(parts[0]);
        path = parts[1];
    }

    string local_path = source.local_dir + path;
    if (File::exists(local_path)) return true;

    string url = source.url + path;

    if (url.find("snesmusic.org") != string::npos) {
        url = url.substr(0, url.length() - 4);
    }

    return webgetter.inCache(url);
}

bool RemoteLoader::isOffline(const std::string& p)
{

    Source source;
    string path = p;

    auto parts = split(path, "::");
    if (parts.size() > 1) {
        source = sources[parts[0]];
        path = parts[1];
    }

    string local_path = source.local_dir + path;
    return inCache(p) || File::exists(local_path);
}

bool RemoteLoader::load(const std::string& p, function<void(File f)> done_cb)
{

    Source source;
    string path = p;

    auto parts = split(path, "::");
    if (parts.size() > 1) {
        source = sources[parts[0]];
        path = parts[1];
    }

    string local_path = source.local_dir + path;
    LOGD("Local path: %s", local_path);
    if (File::exists(local_path)) {
        schedule_callback([=]() { done_cb(File(local_path)); });
        return true;
    }

    string url = source.url + path;

    if (url.find("snesmusic.org") != string::npos) {
        url = url.substr(0, url.length() - 4);
    }

    // If the primary source has a fallback (e.g. hvtc: fast Wayback mirror with
    // partial coverage, flaky live host as authoritative backup), retry there
    // when the primary fetch returns nothing (404 from Wayback for an unarchived
    // tune, or a connection failure). The web layer deletes the target on any
    // non-200, so a failed primary leaves no stale file to confuse the player.
    string fallback = source.fallback_url.empty() ? ""
                                                   : source.fallback_url + path;

    auto finish = [=](webutils::WebJob job) {
        LOGD("CODE %d", job.code());
        last_http_code = job.code();
        auto f = job.file();
        string fileName = f.getName();
        if (fileName.find("snesmusic.org") != string::npos) {
            auto newFile = fileName + ".rsn";
            rename(fileName.c_str(), newFile.c_str());
            f = File{ newFile };
        }
        // AdLib Visual Composer .rol tunes carry no instrument data of their own:
        // the AdPlug ROL player loads voices from a companion "standard.bnk" in
        // the same directory (see adplug/rol.cpp). When streaming from a remote
        // source only the .rol is fetched, so the bank is absent next to it in the
        // web cache and playback fails. Pull the sibling bank into the same cache
        // directory before handing the .rol to the player. A missing bank is
        // non-fatal (the callback still fires), matching how other companions are
        // treated -- the tune just renders bankless rather than aborting the load.
        if (endsWith(toLower(path), ".rol")) {
            auto slash = url.find_last_of('/');
            string bankUrl = url.substr(0, slash + 1) + "standard.bnk";
            webgetter.getFile(bankUrl,
                              [=](webutils::WebJob) { done_cb(f); });
            return;
        }
        done_cb(f);
    };

    lastSession = webgetter.getFile(url, [=](webutils::WebJob job) {
        // rc == -1 means the file was served from the local cache (no curl
        // handle) — that's a success, not a failure. Only 200 responses are
        // ever cached, so a cache hit is always valid. Fall back only on a real
        // network/HTTP failure (0 = no response, >=400 = error).
        long rc = job.code();
        if (rc != 200 && rc != 226 && rc != -1 && !fallback.empty()) {
            LOGD("Primary failed (%ld), retrying via fallback %s", rc, fallback);
            webgetter.getFile(fallback, finish);
            return;
        }
        finish(job);
    });
    return true;
}

void RemoteLoader::listDirectory(
    const std::string& p, function<void(vector<string>)> done_cb)
{
    Source source;
    string path = p;

    auto parts = split(path, "::");
    if (parts.size() > 1) {
        source = sources[parts[0]];
        path = parts[1];
    }

    // If a local mirror already holds this directory, the song was loaded from
    // it and the player reads the members in place -- nothing to fetch.
    string local_path = source.local_dir + path;
    if (File::exists(local_path)) {
        LOGD("Directory present in local mirror: %s", local_path);
        schedule_callback([=]() { done_cb({}); });
        return;
    }

    string url = source.url + path;
    if (!url.empty() && url.back() != '/') { url += "/"; }

    webgetter.listDir(url, [=](const std::string& listing) {
        vector<string> names;
        for (auto& line : split(listing, "\n")) {
            string l = line;
            while (!l.empty() && (l.back() == '\r' || l.back() == '\n')) {
                l.pop_back();
            }
            if (l.empty()) { continue; }
            // With CURLFTPMETHOD_NOCWD the server echoes full paths; keep only
            // the basename.
            auto slash = l.find_last_of('/');
            string base = (slash == string::npos) ? l : l.substr(slash + 1);
            if (base.empty() || base == "." || base == "..") { continue; }
            names.push_back(base);
        }
        done_cb(names);
    });
}

// void RemoteLoader::preCache(const std::string &path) {}

std::string RemoteLoader::resolveUrl(const std::string& p)
{
    Source source;
    string path = p;
    auto parts = split(path, "::");
    if (parts.size() > 1) {
        source = sources[parts[0]];
        path = parts[1];
    }
    return source.url + path;
}

std::shared_ptr<webutils::WebJob> RemoteLoader::stream(
    const std::string& p,
    std::function<bool(int what, const uint8_t* data, int size)> data_cb)
{

    Source source;
    string path = p;

    auto parts = split(path, "::");
    if (parts.size() > 1) {
        source = sources[parts[0]];
        path = parts[1];
    }

    string local_path = source.local_dir + path;
    // if(File::exists(local_path)) {
    //	done_cb(File(local_path));
    //	return true;
    //}

    string url = source.url + path;
    bool headers = false;
    lastSession = webgetter.streamData(
        url,
        [=](webutils::WebJob& job, uint8_t* data, int size) mutable -> bool {
            if (!headers) {
                string s = job.getHeader("icy-metaint");
                if (s != "") {
                    int mi = stol(s);
                    data_cb(PARAMETER, (uint8_t*)"icy-interval", mi);
                }
                LOGD("CONTENT LENGTH %d", job.contentLength());
                if (job.contentLength() > 0)
                    data_cb(PARAMETER, (uint8_t*)"size", job.contentLength());
                headers = true;
            }
            if (data == nullptr) return data_cb(END, nullptr, size);
            return data_cb(DATA, data, size);
        });
    return lastSession;
}
