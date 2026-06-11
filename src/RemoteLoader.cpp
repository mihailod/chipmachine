#include "RemoteLoader.h"

#include <coreutils/log.h>
#include <coreutils/split.h>

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
                                  const std::string local_dir)
{
    Source s(url, local_dir);
    if (s.local_dir != "" && !endsWith(s.local_dir, "/")) s.local_dir += "/";
    sources[name] = s;
}

bool RemoteLoader::inCache(const std::string& p) const
{
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

    lastSession = webgetter.getFile(url, [=](webutils::WebJob job) {
        LOGD("CODE %d", job.code());
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
    });
    return true;
}

// void RemoteLoader::preCache(const std::string &path) {}

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
