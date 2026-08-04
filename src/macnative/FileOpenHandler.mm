#include "FileOpenHandler.h"

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>
#import <objc/runtime.h>

#include <mutex>

namespace {

std::mutex g_mutex;
std::vector<std::string> g_pending;

void pushPath(const std::string& path)
{
    if (path.empty()) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    g_pending.push_back(path);
}

// -------------------------------------------------------------------------
// Delegate method injected into GLFW's NSApplication delegate class.
//
// Finder's "Open With" / double-click sends the app a kAEOpenDocuments Apple
// Event. AppKit installs its OWN handler for that event during
// -[NSApplication finishLaunching], and that handler routes the open request to
// the application delegate's -application:openURLs:. GLFW owns the delegate
// (GLFWApplicationDelegate) and does NOT implement that method, so AppKit's
// default machinery has nowhere to deliver the files and instead shows the
// "cannot open files in the <type> format" error.
//
// Crucially, GLFW calls [NSApp run] *inside glfwInit()* to finish launching, so
// the cold-launch open event is dispatched THERE -- before any code of ours
// that runs after the window is created. Trying to register our own
// NSAppleEventManager handler loses that race (and finishLaunching overwrites it
// anyway). So instead we add -application:openURLs: to GLFW's delegate CLASS at
// runtime, BEFORE glfwInit runs. The class is registered when libglfw loads (at
// process start), so objc_getClass finds it even before the delegate instance
// exists. AppKit then delivers every open request -- cold launch or while
// already running -- straight to us, timing-independently.
// -------------------------------------------------------------------------
void cm_openURLs(id /*self*/, SEL /*_cmd*/, NSApplication* /*app*/,
                 NSArray<NSURL*>* urls)
{
    for (NSURL* url in urls) {
        NSString* path = [url path];
        if (path != nil) pushPath(std::string([path UTF8String]));
    }
}

} // namespace

// Fallback only: an NSAppleEventManager target, used if GLFW's delegate class
// cannot be found (e.g. a future GLFW rename or a non-GLFW build). Registered
// late, so it reliably catches only the already-running case, but it is better
// than nothing. The delegate-method path above is the primary mechanism.
@interface CMFileOpenFallback : NSObject
- (void)handleOpenDocuments:(NSAppleEventDescriptor*)event
             withReplyEvent:(NSAppleEventDescriptor*)reply;
@end

@implementation CMFileOpenFallback
- (void)handleOpenDocuments:(NSAppleEventDescriptor*)event
             withReplyEvent:(NSAppleEventDescriptor*)reply
{
    NSAppleEventDescriptor* docs =
        [event paramDescriptorForKeyword:keyDirectObject];
    if (docs == nil) return;
    NSInteger count = [docs numberOfItems];
    for (NSInteger i = 1; i <= count; i++) { // AE lists are 1-based
        NSAppleEventDescriptor* item = [docs descriptorAtIndex:i];
        NSAppleEventDescriptor* urlDesc = [item coerceToDescriptorType:typeFileURL];
        if (urlDesc == nil) continue;
        NSData* urlData = [urlDesc data];
        if (urlData == nil) continue;
        NSString* urlStr = [[NSString alloc] initWithData:urlData
                                                 encoding:NSUTF8StringEncoding];
        NSURL* url = [NSURL URLWithString:urlStr];
        NSString* path = [url path];
        if (path != nil) pushPath(std::string([path UTF8String]));
    }
}
@end

namespace chipmachine {

void installFileOpenHandler()
{
    static bool installed = false;
    if (installed) return;
    installed = true;

    // Primary path: teach GLFW's app delegate to answer -application:openURLs:.
    // Must be done BEFORE glfwInit() (which runs [NSApp run] and dispatches the
    // cold-launch open event). The class is present as soon as libglfw is
    // loaded, so we can patch it before the delegate instance is created.
    Class glfwDelegate = objc_getClass("GLFWApplicationDelegate");
    if (glfwDelegate != nil) {
        SEL sel = @selector(application:openURLs:);
        if (!class_getInstanceMethod(glfwDelegate, sel)) {
            // Type encoding: void return; self, _cmd, NSApplication*, NSArray*.
            class_addMethod(glfwDelegate, sel, (IMP)cm_openURLs, "v@:@@");
        }
        return;
    }

    // Fallback: no GLFW delegate class (unexpected). Register a raw Apple Event
    // handler; catches the already-running case at least.
    [NSApplication sharedApplication];
    static CMFileOpenFallback* fallback = [[CMFileOpenFallback alloc] init];
    [[NSAppleEventManager sharedAppleEventManager]
        setEventHandler:fallback
            andSelector:@selector(handleOpenDocuments:withReplyEvent:)
          forEventClass:kCoreEventClass
             andEventID:kAEOpenDocuments];
}

std::vector<std::string> drainPendingOpenFiles()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<std::string> out;
    out.swap(g_pending);
    return out;
}

// -------------------------------------------------------------------------
// App Sandbox security-scoped bookmarks.
//
// This code is IDENTICAL in both variants -- there is no #ifdef CM_MAS here on
// purpose, so the mas and plus sources do not diverge. The behaviour difference
// is driven entirely by the entitlements .plist (which must differ anyway) plus
// a cheap runtime sandbox check below:
//   * Sandboxed (mas): creates/persists app-scoped bookmarks and re-acquires
//     access, as required to reach user-opened files across launches.
//   * Not sandboxed (plus/Developer ID): isSandboxed() is false, so both public
//     functions return immediately. (Even if they didn't, security-scoped
//     bookmark creation cleanly returns nil without the sandbox+entitlement, so
//     the code is a safe no-op regardless -- the runtime guard just skips the
//     pointless work and its benign OS error-log line.)
// -------------------------------------------------------------------------
namespace {

// Are we running inside the App Sandbox? The OS sets APP_SANDBOX_CONTAINER_ID
// in the environment of every sandboxed process; a Developer ID / dev build has
// no container and never sees it. Cheap, no entitlement API needed.
bool isSandboxed()
{
    return getenv("APP_SANDBOX_CONTAINER_ID") != nullptr;
}

// The bookmark store lives in the app's Application Support directory, which
// under the sandbox resolves to the app container (always writable, no extra
// entitlement). Keyed by absolute POSIX path -> app-scoped bookmark NSData.
NSURL* bookmarkStoreURL()
{
    NSFileManager* fm = [NSFileManager defaultManager];
    NSError* err = nil;
    NSURL* dir = [fm URLForDirectory:NSApplicationSupportDirectory
                            inDomain:NSUserDomainMask
                   appropriateForURL:nil
                              create:YES
                               error:&err];
    if (dir == nil) return nil;
    return [dir URLByAppendingPathComponent:@"security-bookmarks.plist"];
}

NSMutableDictionary<NSString*, NSData*>* loadBookmarkStore()
{
    NSURL* url = bookmarkStoreURL();
    if (url == nil) return [NSMutableDictionary dictionary];
    NSDictionary* d = [NSDictionary dictionaryWithContentsOfURL:url];
    return d ? [d mutableCopy] : [NSMutableDictionary dictionary];
}

void saveBookmarkStore(NSDictionary<NSString*, NSData*>* store)
{
    NSURL* url = bookmarkStoreURL();
    if (url == nil) return;
    [store writeToURL:url atomically:YES];
}

} // namespace

void rememberOpenedFile(std::string const& path)
{
    if (path.empty() || !isSandboxed()) return;
    @autoreleasepool {
        NSString* p = [NSString stringWithUTF8String:path.c_str()];
        if (p == nil) return;
        NSURL* fileURL = [NSURL fileURLWithPath:p];

        // Create an app-scoped security-scoped bookmark. The current process
        // already has access to this URL (LaunchServices/Powerbox granted it
        // when the user opened the file), which is what lets the bookmark be
        // created. Persist it so a later launch can re-acquire access.
        NSError* err = nil;
        NSData* bm = [fileURL
                bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
         includingResourceValuesForKeys:nil
                          relativeToURL:nil
                                  error:&err];
        if (bm == nil) return; // no access / not user-selected -- nothing to save

        NSMutableDictionary<NSString*, NSData*>* store = loadBookmarkStore();
        store[p] = bm;
        saveBookmarkStore(store);
    }
}

void restoreSecurityScopedFiles()
{
    if (!isSandboxed()) return;
    @autoreleasepool {
        NSMutableDictionary<NSString*, NSData*>* store = loadBookmarkStore();
        if (store.count == 0) return;

        NSMutableArray<NSString*>* dead = [NSMutableArray array];
        BOOL storeChanged = NO;

        for (NSString* p in store) {
            NSData* bm = store[p];
            BOOL stale = NO;
            NSError* err = nil;
            NSURL* url = [NSURL
                URLByResolvingBookmarkData:bm
                                   options:NSURLBookmarkResolutionWithSecurityScope
                             relativeToURL:nil
                       bookmarkDataIsStale:&stale
                                     error:&err];
            if (url == nil) {
                // Unresolvable (file deleted/moved beyond recovery): drop it so
                // the store does not grow without bound.
                [dead addObject:p];
                continue;
            }
            // Start accessing and NEVER stop: this is a media player, so we want
            // the granted paths readable for the whole session. The kernel drops
            // the extension when the process exits.
            [url startAccessingSecurityScopedResource];

            if (stale) {
                NSError* rerr = nil;
                NSData* fresh = [url
                        bookmarkDataWithOptions:NSURLBookmarkCreationWithSecurityScope
                 includingResourceValuesForKeys:nil
                                  relativeToURL:nil
                                          error:&rerr];
                if (fresh != nil) {
                    store[p] = fresh;
                    storeChanged = YES;
                }
            }
        }

        if (dead.count > 0) {
            [store removeObjectsForKeys:dead];
            storeChanged = YES;
        }
        if (storeChanged) saveBookmarkStore(store);
    }
}

// --- Companion-file (multi-file format) folder access ---------------------
//
// The Powerbox grant that comes with an "Open With" / double-click / drag-drop
// is STRICTLY PER-FILE. Verified empirically with a minimal app carrying the
// same entitlements as the mas build: reading the opened file succeeds, while
// reading a SIBLING in the same folder fails with errno 1 (EPERM), and even
// listing the folder is denied.
//
// That breaks every multi-file format when the tune is opened from outside the
// container -- TFMX (mdat./smpl.), the PSF family's .psflib/.gsflib/.usflib,
// MDX .pdx, KSS .SM1/.SM2, SoundSmith .W, AdPlug banks, MaxTrax shared banks --
// because for a LOCAL song the companions are read in place beside it rather
// than fetched (see MusicPlayerList: a local mirror yields an empty fetch list).
//
// The only way to widen the grant is to have the user select the FOLDER, so we
// ask for it explicitly, once, with the panel already pointed at the right
// place. The resulting folder bookmark is stored in the same app-scoped store
// as file bookmarks and re-acquired at launch by restoreSecurityScopedFiles().

namespace {

// Folders the user has already declined this session. Without this a user who
// cancels would be re-prompted for every song in that folder.
NSMutableSet<NSString*>* declinedFolders()
{
    static NSMutableSet<NSString*>* s = [NSMutableSet set];
    return s;
}

// Can we enumerate this directory? This is the cheapest true test of whether
// the sandbox will let the decoders reach companions: the empirical run showed
// listing fails before any sibling read is attempted.
bool folderIsReadable(NSString* dir)
{
    NSError* err = nil;
    return [[NSFileManager defaultManager] contentsOfDirectoryAtPath:dir
                                                               error:&err] != nil;
}

} // namespace

bool ensureFolderAccess(std::string const& filePath)
{
    if (filePath.empty()) return true;
    if (!isSandboxed()) return true; // plus / dev build: nothing to grant

    __block bool ok = false;
    @autoreleasepool {
        NSString* p = [NSString stringWithUTF8String:filePath.c_str()];
        if (p == nil) return true;
        NSString* dir = [p stringByDeletingLastPathComponent];
        if (dir.length == 0) return true;

        // Already reachable: the folder was granted earlier in this session, or
        // restored from a bookmark at launch, or the song lives in our own
        // container (everything downloaded/cached does).
        if (folderIsReadable(dir)) return true;
        if ([declinedFolders() containsObject:dir]) return false;

        // Must run on the main thread -- NSOpenPanel is AppKit UI. Callers are
        // expected to be on it already (ChipMachine::update()); the dispatch is
        // belt-and-braces so a future caller on a worker thread cannot deadlock
        // the render loop or trip AppKit's main-thread assertion.
        void (^ask)(void) = ^{
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            [panel setCanChooseFiles:NO];
            [panel setCanChooseDirectories:YES];
            [panel setAllowsMultipleSelection:NO];
            [panel setPrompt:@"Grant Access"];
            [panel setMessage:
                @"This tune loads samples or instrument banks from other files "
                 "in the same folder. macOS only granted access to the single "
                 "file you opened, so please grant access to its folder."];
            [panel setDirectoryURL:[NSURL fileURLWithPath:dir]];
            [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];

            if ([panel runModal] == NSModalResponseOK) {
                NSURL* url = [[panel URLs] firstObject];
                if (url != nil) {
                    // Keep the extension alive for the session, then persist it
                    // so later launches do not have to ask again.
                    [url startAccessingSecurityScopedResource];
                    rememberOpenedFile([[url path] UTF8String]);
                    ok = folderIsReadable([url path]);
                }
            }
            if (!ok) [declinedFolders() addObject:dir];
        };

        if ([NSThread isMainThread]) {
            ask();
        } else {
            dispatch_sync(dispatch_get_main_queue(), ask);
        }
    }
    return ok;
}

} // namespace chipmachine
