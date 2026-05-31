#ifndef REMOTE_LOADER_H
#define REMOTE_LOADER_H

#include <coreutils/file.h>
#include <webutils/web.h>

#include <functional>
#include <string>
#include <unordered_map>

#include <iostream>

class RemoteLoader
{
public:
    RemoteLoader();

    void registerSource(const std::string& name, std::string url,
                        std::string local_dir);

    bool load(const std::string& path,
              std::function<void(utils::File)> done_cb);

    std::shared_ptr<webutils::WebJob> stream(
        const std::string& path,
        std::function<bool(int what, const uint8_t* data, int size)> data_cb);

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
        // and for now only the nsfe one is...
        // eg: nsfe::31_orange_painting.nsfe
        return !(path.find("nsfe::") == std::string::npos);
    }

private:
    struct Source
    {
        Source() = default;
        Source(const std::string& url, const std::string& ld)
            : url(url), local_dir(ld)
        {}
        std::string url;
        std::string local_dir;
    };

    std::unordered_map<std::string, Source> sources;

    webutils::Web webgetter;
    std::shared_ptr<webutils::WebJob> lastSession;
};

#endif // REMOTE_LOADER_H
