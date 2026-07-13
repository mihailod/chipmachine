#ifndef REMOTE_LOADER_H
#define REMOTE_LOADER_H

#include <coreutils/file.h>
#include <webutils/web.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <iostream>

class RemoteLoader
{
public:
    RemoteLoader();

    // `fallback_url`, when set, is tried if a fetch from `url` fails (non-200).
    // Used for collections whose fast/reliable mirror has partial coverage
    // (e.g. hvtc: Wayback primary, flaky live host as authoritative fallback).
    void registerSource(const std::string& name, std::string url,
                        std::string local_dir, std::string fallback_url = "");

    bool load(const std::string& path,
              std::function<void(utils::File)> done_cb);

    // HTTP status of the most recent load() fetch, so a caller that received an
    // empty File can tell a genuine 404 (file not on the host) apart from a
    // connection/other failure and report it precisely. -1 means a local-cache
    // hit (success), 0 means no response was received.
    [[nodiscard]] long lastHttpCode() const { return last_http_code; }

    // Lists the files in a remote directory (e.g. an IFF-SMUS "Instruments/"
    // folder whose member names are unpredictable). The callback receives the
    // bare basenames. If the directory is already present in a local mirror, the
    // callback receives an empty list -- the player reads those files in place,
    // so nothing needs fetching.
    void listDirectory(const std::string& path,
                       std::function<void(std::vector<std::string>)> done_cb);

    std::shared_ptr<webutils::WebJob> stream(
        const std::string& path,
        std::function<bool(int what, const uint8_t* data, int size)> data_cb);

    // Resolves a "prefix::relpath" song path to the full URL that stream()/load()
    // would fetch (i.e. source.url + relpath). Used to hand radio/mp3 streams to
    // ffmpeg, which does its own HTTP.
    std::string resolveUrl(const std::string& path);

    void preCache(const std::string& path);

    bool inCache(const std::string& path) const;

    bool isOffline(const std::string& p);

    // True when this song is served straight from a local_dir mirror on disk --
    // the SAME condition load()/inCache short-circuit on, so a local file is by
    // construction never fetched into the web cache. The GUI marks these with a
    // "+" (vs "*" for cached remote files). Preferred over the prefix-based
    // isLocalAsset() below because it tracks the actual on-disk reality, so the
    // "+" mark can never drift from the never-cached behaviour.
    [[nodiscard]] bool isLocalFile(const std::string& p) const;

    void cancel()
    {
        if (lastSession) lastSession->stop();
        lastSession = nullptr;
    }

    void update() { webgetter.poll(); }

    // Progress of the current whole-file download (load()) or stream, for the
    // GUI's LOADING/BUFFERING progress bar. Fills `downloaded`/`total` (bytes)
    // and returns true only when a transfer is in flight AND its total size is
    // known; returns false otherwise (no active fetch, or an open-ended stream
    // whose size the server never reported).
    [[nodiscard]] bool downloadProgress(int64_t& downloaded,
                                        int64_t& total) const
    {
        // Ignore a finished job left in lastSession by a previous song (it would
        // otherwise report a stale 100%): only an in-flight transfer counts.
        if (!lastSession || lastSession->done()) return false;
        total = lastSession->totalBytes();
        if (total <= 0) return false;
        downloaded = lastSession->downloadedBytes();
        return true;
    }

    static constexpr int DATA = 0;
    static constexpr int PARAMETER = 1;
    static constexpr int END = 2;

    [[nodiscard]] static bool isLocalAsset(const std::string& path) {
        // std::cout << path << std::endl; 
        // return path.find("http://") == std::string::npos &&
        //       path.find("https://") == std::string::npos &&
        //       path.find("ftp://") == std::string::npos;

        // todo implement better
        // the path prefix is actually db name prefix
        // eg modland::Soundtracker/SLL/sll1.mod
        // so need a map of dbs which are local
        // The locally-shipped collections bundle their files inside the .app
        // (a music/<dir> local_dir), so a song from one is always served from
        // disk, never the network. NB: only APP-SHIPPED collections belong here
        // -- collections with a /opt/Music local_dir (modland, asma, rko, ...)
        // are the user's own mirrors and are NOT present on most machines.
        //   nsfe      -> music/Console     (nsfe::31_orange_painting.nsfe)
        //   hvtc      -> music/hvtc  (TED)  (hvtc::demos/crazy_scroll_89.prg)
        //   projectay -> music/projectay   (projectay::ironfist/arkanoid.ay)
        return path.find("nsfe::") == 0 || path.find("hvtc::") == 0 ||
               path.find("projectay::") == 0;
    }

private:
    struct Source
    {
        Source() = default;
        Source(const std::string& url, const std::string& ld,
               const std::string& fb = "")
            : url(url), local_dir(ld), fallback_url(fb)
        {}
        std::string url;
        std::string local_dir;
        std::string fallback_url;
    };

    std::unordered_map<std::string, Source> sources;

    webutils::Web webgetter;
    std::shared_ptr<webutils::WebJob> lastSession;
    long last_http_code = 0;
};

#endif // REMOTE_LOADER_H
