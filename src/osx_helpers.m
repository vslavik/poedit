/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2007-2013 Vaclav Slavik
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

#include "osx_helpers.h"

#import <Foundation/NSString.h>
#import <Foundation/NSUserDefaults.h>
#import <Foundation/NSAutoreleasePool.h>

#import <AppKit/NSApplication.h>
#import <AppKit/NSButton.h>
#import <AppKit/NSSpellChecker.h>

#ifdef USE_SPARKLE
#import <Sparkle/Sparkle.h>

// --------------------------------------------------------------------------------
// Sparkle helpers
// --------------------------------------------------------------------------------

void Sparkle_Initialize(bool checkForBeta)
{
    /* Remove config key for Sparkle < 1.5. */
    UserDefaults_RemoveValue("SUCheckAtStartup");

    @autoreleasepool {
        SUUpdater *updater = [SUUpdater sharedUpdater];

        if (checkForBeta)
        {
            NSString *url = @"http://releases.poedit.net/appcast-osx/beta";
            [updater setFeedURL:[NSURL URLWithString:url]];
        }
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
// Spell checking
// --------------------------------------------------------------------------------

int SpellChecker_SetLang(const char *lang)
{
    @autoreleasepool {
        NSString *nslang = [NSString stringWithUTF8String: lang];
        return [[NSSpellChecker sharedSpellChecker] setLanguage: nslang];
    }
}


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

void MakeButtonRounded(void *button)
{
    [(__bridge NSButton*)button setBezelStyle:NSRoundRectBezelStyle];
}
