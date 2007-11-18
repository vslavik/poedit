/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2007 Vaclav Slavik
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
 *  $Id$
 *
 *  Simple interface for accessing NSUserDefaults
 *
 */

#include "userdefaults.h"

#import <AppKit/NSApplication.h>
#import <Foundation/NSString.h>
#import <Foundation/NSAutoreleasePool.h>
#import <Foundation/NSUserDefaults.h>

void UserDefaults_SetBoolValue(const char *key, int value)
{
    NSApplicationLoad();

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *nskey = [NSString stringWithUTF8String: key];

    [[NSUserDefaults standardUserDefaults] setBool:value forKey:nskey];
    [[NSUserDefaults standardUserDefaults] synchronize];

    [pool release];
}

int UserDefaults_GetBoolValue(const char *key)
{
    NSApplicationLoad();

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *nskey = [NSString stringWithUTF8String: key];

    int rv = [[NSUserDefaults standardUserDefaults] boolForKey:nskey];

    [pool release];

    return rv;
}
