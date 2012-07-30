/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2012 Vaclav Slavik
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

#include "sparkle.h"

#include "userdefaults.h"

#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSUserDefaults.h>
#import <Sparkle/Sparkle.h>


void Sparkle_Initialize()
{
    /* Remove config key for Sparkle < 1.5. */
    UserDefaults_RemoveValue("SUCheckAtStartup");

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    SUUpdater *updater = [SUUpdater sharedUpdater];
    [pool release];
}


void Sparkle_AddMenuItem(const char *title)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    NSString *nstitle = [NSString stringWithUTF8String: title];
    NSMenu *appmenu = [[[[NSApplication sharedApplication] mainMenu] itemAtIndex:0] submenu];
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:nstitle
                                           action:@selector(checkForUpdates:)
                                           keyEquivalent:@""];
    SUUpdater *updater = [SUUpdater sharedUpdater];
    [item setEnabled:YES];
    [item setTarget:updater];
    [appmenu insertItem:item atIndex:1];

    [pool release];
}


void Sparkle_Cleanup()
{
    /*  Make sure that Sparkle's updates to plist preferences are saved: */
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    [[NSUserDefaults standardUserDefaults] synchronize];
    [pool release];
}
