/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2007-2018 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE. 
 *
 */

#include "macos_helpers.h"

#include "edapp.h"

#import <Foundation/NSString.h>
#import <Foundation/NSUserDefaults.h>
#import <Foundation/NSAutoreleasePool.h>

#import <AppKit/NSApplication.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSSpellChecker.h>

#import "PFMoveApplication.h"

#ifdef USE_SPARKLE
#import <Sparkle/Sparkle.h>

// --------------------------------------------------------------------------------
// Sparkle helpers
// --------------------------------------------------------------------------------

@interface PoeditSparkleDelegate : NSObject <SUUpdaterDelegate>
@end

@implementation PoeditSparkleDelegate

- (NSArray *)feedParametersForUpdater:(SUUpdater *)updater sendingSystemProfile:(BOOL)sendingProfile
{
    #pragma unused(updater, sendingProfile)
    if (wxGetApp().CheckForBetaUpdates())
    {
        return @[ @{
            @"key":           @"beta",
            @"value":         @"1",
            @"displayKey":    @"Beta Versions",
            @"displayValue":  @"Yes"
        } ];
    }
    else
    {
        return @[];
    }
}

@end


NSObject *Sparkle_Initialize()
{
    @autoreleasepool {
        // Poedit < 2.0 stored this in preferences, which was wrong - it overrode
        // changes to Info.plist. Undo the damage:
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"SUFeedURL"];

        // For Preferences window, have default in sync with Info.plist:
        NSDictionary *sparkleDefaults = @{ @"SUEnableAutomaticChecks": @YES };
        [[NSUserDefaults standardUserDefaults] registerDefaults:sparkleDefaults];

        NSObject<SUUpdaterDelegate> *delegate = [PoeditSparkleDelegate new];
        SUUpdater.sharedUpdater.delegate = delegate;
        return delegate;
    }
}


void Sparkle_AddMenuItem(NSMenu *appmenu, const char *title)
{
    @autoreleasepool {
        NSString *nstitle = [NSString stringWithUTF8String: title];
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:nstitle
                                               action:@selector(checkForUpdates:)
                                               keyEquivalent:@""];
        SUUpdater *updater = [SUUpdater sharedUpdater];
        [item setEnabled:YES];
        [item setTarget:updater];
        [appmenu insertItem:item atIndex:1];
    }
}


void Sparkle_Cleanup()
{
    /*  Make sure that Sparkle's updates to plist preferences are saved: */
    @autoreleasepool {
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
}
#endif // USE_SPARKLE


// --------------------------------------------------------------------------------
// Native preferences
// --------------------------------------------------------------------------------

int UserDefaults_GetBoolValue(const char *key)
{
    @autoreleasepool {
        NSString *nskey = [NSString stringWithUTF8String: key];
        return [[NSUserDefaults standardUserDefaults] boolForKey:nskey];
    }
}


void UserDefaults_SetBoolValue(const char *key, int value)
{
    @autoreleasepool {
        NSString *nskey = [NSString stringWithUTF8String: key];

        [[NSUserDefaults standardUserDefaults] setBool:value forKey:nskey];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
}


void UserDefaults_RemoveValue(const char *key)
{
    @autoreleasepool {
        NSString *nskey = [NSString stringWithUTF8String: key];

        [[NSUserDefaults standardUserDefaults] removeObjectForKey:nskey];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
}


// --------------------------------------------------------------------------------
// Misc UI helpers
// --------------------------------------------------------------------------------

void MoveToApplicationsFolderIfNecessary()
{
    PFMoveToApplicationsFolderIfNecessary();
}

