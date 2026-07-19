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

} // namespace chipmachine
