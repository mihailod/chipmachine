#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#include "version.h"
#include <string>

/**
 * Helper to extract a normalized version string (e.g. "1.4.3") from a tag or VERSION_STR.
 * It takes the first sequence of digits and dots it finds.
 */
static NSString* ExtractVersionNumber(NSString* source) {
    if (!source) return nil;
    
    NSError *error = nil;
    NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"[0-9]+(\\.[0-9]+)+" options:0 error:&error];
    NSTextCheckingResult *match = [regex firstMatchInString:source options:0 range:NSMakeRange(0, [source length])];
    
    if (match) {
        return [source substringWithRange:match.range];
    }
    return source;
}

static void ProcessVersionComparison(NSString *latestTag) {
    if (!latestTag) return;
    
    // Normalize versions for accurate numeric comparison
    NSString *cleanedRemoteVersion = ExtractVersionNumber(latestTag);
    NSString *currentLocalVersion = ExtractVersionNumber([NSString stringWithUTF8String:VERSION_STR]);
    
    if (!cleanedRemoteVersion || !currentLocalVersion) return;
    
    // Use NSNumericSearch to handle version logic (e.g. 1.0.10 > 1.0.2)
    NSComparisonResult result = [cleanedRemoteVersion compare:currentLocalVersion options:NSNumericSearch];
    
    if (result == NSOrderedDescending) {
        // A newer version is definitively available
        dispatch_async(dispatch_get_main_queue(), ^{
            NSAlert *alert = [[NSAlert alloc] init];
            [alert setMessageText:@"ChipMachineAS Update"];
            
            NSString *informativeText = [NSString stringWithFormat:
                @"You are running version %@. A newer version (%@) is now available.",
                [NSString stringWithUTF8String:VERSION_STR], latestTag];
                
            [alert setInformativeText:informativeText];
            [alert addButtonWithTitle:@"View on GitHub"];
            [alert addButtonWithTitle:@"Later"];
            [alert setAlertStyle:NSAlertStyleInformational];
            
            [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
            if ([alert runModal] == NSAlertFirstButtonReturn) {
                NSURL *githubURL = [NSURL URLWithString:@"https://github.com/mihailod/chipmachine/releases/latest"];
                [[NSWorkspace sharedWorkspace] openURL:githubURL];
            }
        });
    }
}

// Internal implementation of the update check using modern NSURLSession
static void PerformUpdateCheck() {
    // --- DEBUG OVERRIDE START ---
    // Check for a local file named "DEBUG_UPDATE_VERSION" in the executable directory
    NSString *exeDir = [[NSBundle mainBundle] executablePath].stringByDeletingLastPathComponent;
    NSString *debugFilePath = [exeDir stringByAppendingPathComponent:@"DEBUG_UPDATE_VERSION"];
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:debugFilePath]) {
        NSString *debugVersion = [NSString stringWithContentsOfFile:debugFilePath encoding:NSUTF8StringEncoding error:nil];
        if (debugVersion) {
            debugVersion = [debugVersion stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
            NSLog(@"[Updater] DEBUG OVERRIDE: Simulating remote version %@", debugVersion);
            ProcessVersionComparison(debugVersion);
            return;
        }
    }
    // --- DEBUG OVERRIDE END ---

    NSString *urlStr = @"https://api.github.com/repos/mihailod/chipmachine/releases/latest";
    NSURL *url = [NSURL URLWithString:urlStr];
    
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setHTTPMethod:@"GET"];
    [request setValue:@"ChipMachineAS-Updater" forHTTPHeaderField:@"User-Agent"];
    [request setCachePolicy:NSURLRequestReloadIgnoringLocalCacheData];

    NSURLSession *session = [NSURLSession sharedSession];
    [[session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
        if (error || !data) return;
        
        NSError *jsonError = nil;
        NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:kNilOptions error:&jsonError];
        if (jsonError || !json || ![json isKindOfClass:[NSDictionary class]]) return;
        
        NSString *latestTag = [json objectForKey:@"tag_name"];
        ProcessVersionComparison(latestTag);
    }] resume];
}


// Public entry point for C++ callers
extern "C" void InitializeUpdateVerificationSubsystem() {
    // NSURLSession data tasks run on a background queue automatically,
    // so we can call this directly without spawning a std::thread.
    PerformUpdateCheck();
}
