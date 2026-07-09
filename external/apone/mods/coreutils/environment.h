#pragma once

#include <string>
#include <mutex>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef ERROR
#else
#include <unistd.h>
#endif

#include "utils.h"
#include "path.h"

#include "log.h"

// Forward declarations for Apple-specific platform bridging functions
#ifdef __APPLE__
namespace detail {
    std::string getMacCacheDir();
    std::string getMacConfigDir();
}
#endif

class Environment
{
public:
    /** $HOME dir */
    static utils::path const& getHomeDir()
    {
        std::lock_guard<std::mutex> lock(m);
        if (homeDir.empty()) {
#ifdef _WIN32
            homeDir = getenv("USERPROFILE");
#else
            homeDir = getenv("HOME");
#endif
        }
        return homeDir;
    }

    /** Directory of the running executable */
    static utils::path const& getExeDir()
    {
        std::lock_guard<std::mutex> lock(m);

        if (exeDir.empty()) {
            char buf[1024];
#if defined _WIN32
            GetModuleFileName(nullptr, buf, sizeof(buf) - 1);
            exeDir = utils::path(buf).parent_path();
#elif defined __APPLE__
            uint32_t size = sizeof(buf);
            if (_NSGetExecutablePath(buf, &size) == 0) {
                exeDir = utils::path(buf).parent_path();
            }
#else
            int rc = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
            if (rc >= 0) {
                buf[rc] = 0;
                exeDir = utils::path(buf).parent_path();
            }
#endif
        }
        return exeDir;
    }

    /** User specific writable cache dir.
     * macOS : ~/Library/Caches/<appname>         — sandbox-legal, no entitlement needed
     * Linux : ~/.cache/<appname>                 — XDG convention
     * Win   : same as exeDir                     — unchanged */
    static utils::path const& getCacheDir()
    {
        std::lock_guard<std::mutex> lock(m);
        if (cacheDir.empty()) {
#ifdef __APPLE__
            // Call isolated Apple implementation
            cacheDir = utils::path(detail::getMacCacheDir()) / appName;
#else
            const auto& homeDir = getHomeDir();
            cacheDir = homeDir / ".cache" / appName;
#endif
            if (!utils::exists(cacheDir)) utils::makedirs(cacheDir);
        }
        return cacheDir;
    }

    /** User specific config dir.
     * macOS : ~/Library/Application Support/<appname> — sandbox-legal, Time Machine backed
     * Linux : ~/.config/<appname>                     — XDG convention
     * Win   : same as exeDir                          — unchanged */
    static utils::path const& getConfigDir()
    {
        std::lock_guard<std::mutex> lock(m);
        if (configDir.empty()) {
#ifdef __APPLE__
            // Call isolated Apple implementation
            configDir = utils::path(detail::getMacConfigDir()) / appName;
#else
            const auto& homeDir = getHomeDir();
            configDir = homeDir / ".config" / appName;
#endif
            if (!utils::exists(configDir)) utils::makedirs(configDir);
        }
        return configDir;
    }

    /** The application data directory.
     * macOS : Contents/Resources/  (exe is in Contents/MacOS/, so exe/../Resources)
     * Linux : /usr/share/<appname> or exeDir if appName is unset
     * Win   : exeDir */
    static utils::path getAppDir()
    {
        const auto& exeDir = getExeDir();
        std::lock_guard<std::mutex> lock(m);
        if (appDir.empty()) {
#ifdef __APPLE__
            appDir = exeDir / ".." / "Resources";
#elif (defined _WIN32)
            appDir = exeDir;
#else
            if (appName.empty())
                appDir = exeDir;
            else
                appDir = "/usr/share/" + appName;
#endif
            LOGD("APPDIR %s", appDir);
            if (!utils::exists(appDir))
                appDir = exeDir;
        }
        return appDir;
    }

    static std::string const& getAppName() { return appName; }
    static void setAppName(std::string const& aname) { appName = aname; }

private:
    inline static utils::path homeDir;
    inline static utils::path exeDir;
    inline static utils::path configDir;
    inline static utils::path cacheDir;
    inline static utils::path appDir;

    inline static std::string appName;
    inline static std::mutex m;
};
