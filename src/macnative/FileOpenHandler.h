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

} // namespace chipmachine
