#include "environment.h"
#import <Foundation/Foundation.h>

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------
static std::string mac_nsdir(NSSearchPathDirectory which, const std::string& sub)
{
    @autoreleasepool {
        NSArray<NSString*>* dirs =
            NSSearchPathForDirectoriesInDomains(which, NSUserDomainMask, YES);
        NSString* path = [dirs firstObject];
        if (!path) return "";
        std::string result = [path UTF8String];
        if (!sub.empty()) result += "/" + sub;
        return result;
    }
}

// ---------------------------------------------------------------------------
// detail:: API — called from environment.h and file.cpp
// ---------------------------------------------------------------------------
namespace detail {

    // ~/Library/Caches/<appName>
    std::string getMacCacheDir()  { return mac_nsdir(NSCachesDirectory, ""); }

    // ~/Library/Application Support/<appName>
    std::string getMacConfigDir() { return mac_nsdir(NSApplicationSupportDirectory, ""); }

} // namespace detail

// ---------------------------------------------------------------------------
// Free function API — called from utils.h (musicplayer's vendored apone).
// Must be a plain free function (no namespace) so the forward declaration
// in utils.h matches without pulling in any Obj-C headers.
// ~/Library/Caches/<subPath>
// ---------------------------------------------------------------------------
std::string _mac_get_cache_subdir(const std::string& subPath)
{
    return mac_nsdir(NSCachesDirectory, subPath);
}
