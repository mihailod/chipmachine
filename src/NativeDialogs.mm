#include "ChipMachine.h"
#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

namespace chipmachine {

std::string ChipMachine::open_file_dialog() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
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
