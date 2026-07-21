#pragma once

// macOS "Open With" / double-click support.
//
// When the user opens a file from Finder, macOS does NOT append it to argv --
// it launches (or re-activates) the app and posts a kAEOpenDocuments Apple
// Event. GLFW installs its own NSApplication, but Apple Events are dispatched
// through the shared NSAppleEventManager, so we register our own handler after
// the app exists and it fires regardless of GLFW's delegate.
//
// The handler runs on the main thread during the normal event pump
// (glfwPollEvents), pushing delivered paths into a small queue that the render
// loop drains via drainPendingOpenFiles(). This covers both cold launch (the
// OS queues the event; we drain it on the first ready frame) and an
// already-running instance (the event fires live).
//
// Implemented in FileOpenHandler.mm and linked on Apple only; all call sites
// are guarded with #ifdef __APPLE__.

#include <string>
#include <vector>

namespace chipmachine {

// Install the kAEOpenDocuments handler. Call once, after the NSApplication
// exists (i.e. after grappix screen.open()). Idempotent.
void installFileOpenHandler();

// Return and clear any paths delivered by Finder since the last call.
// Thread-safe; empty when nothing is pending.
std::vector<std::string> drainPendingOpenFiles();

// --- App Sandbox (Mac App Store) file access -----------------------------
//
// Under the App Sandbox, a file the user opens via "Open With" / double-click /
// drag-and-drop is readable for the CURRENT process automatically (the
// com.apple.security.files.user-selected.read-only entitlement plus the
// LaunchServices/Powerbox grant), so the running session can decode it with no
// extra work. But that grant dies with the process: a path saved to Favorites
// or a playlist that points at such an external file would be unreachable next
// launch. To keep it reachable we persist an app-scoped security-scoped
// bookmark (com.apple.security.files.bookmarks.app-scoped) for every opened
// file and re-acquire access at startup.
//
// Both functions are Apple-only and IDENTICAL in the mas and plus builds (no
// CM_MAS divergence): they self-gate at runtime on whether the process is
// sandboxed, so callers guard them with #ifdef __APPLE__ alone and they are a
// harmless no-op in the non-sandboxed (plus) build. The only variant difference
// for this feature lives in the entitlements .plist.

// Persist a security-scoped bookmark for a user-opened file so a favorited or
// queued reference to it survives a relaunch. Safe to call repeatedly; a path
// already recorded is refreshed. Returns immediately when not sandboxed.
void rememberOpenedFile(std::string const& path);

// At launch, resolve every persisted bookmark and start accessing it (kept for
// the whole process lifetime -- never balanced with a stop, deliberately, for a
// media player). Missing/unresolvable entries are pruned. Call once, early,
// before Favorites/playlists are played. Returns immediately when not sandboxed.
void restoreSecurityScopedFiles();

} // namespace chipmachine
