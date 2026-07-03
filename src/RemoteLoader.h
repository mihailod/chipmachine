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

    void cancel()
    {
        if (lastSession) lastSession->stop();
        lastSession = nullptr;
    }

    void update() { webgetter.poll(); }

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
        // for now the locally-shipped collections are nsfe (music/Console)
        // and hvtc (music/hvtc) -- both ship their files in the .app bundle, so
        // a song from either is always served from disk, never the network.
        // eg: nsfe::31_orange_painting.nsfe / hvtc::demos/crazy_scroll_89.prg
        return path.find("nsfe::") == 0 || path.find("hvtc::") == 0;
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
