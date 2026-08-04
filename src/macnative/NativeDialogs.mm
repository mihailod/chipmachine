#include "ChipMachine.h"
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

namespace chipmachine {

std::string ChipMachine::open_file_dialog() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        // Directories are selectable so a user can grant a WHOLE FOLDER in one
        // step. Under the App Sandbox the grant that comes with picking a single
        // file does not extend to its siblings, so multi-file formats (TFMX
        // mdat./smpl., .psflib, .pdx, .SM1/.SM2, ...) cannot load their
        // companions from a file-only selection. Picking the folder instead
        // grants everything in it at once. The caller distinguishes the two
        // cases (see the "local_file_playback" command): a directory is
        // remembered as a grant rather than played.
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:NO];
        
        // Ensure the dialog appears on top
        [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] firstObject];
            NSString* path = [url path];
            return [path UTF8String];
        }
    }
    return "";
}

}
