/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2022 Vaclav Slavik
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
 *  Generated by PaintCode (www.paintcodeapp.com)
 *
 */

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>


@interface StyleKit : NSObject

// Drawing Methods
+ (void)drawSwitchButtonWithFrame: (NSRect)frame onColor: (NSColor*)onColor labelOffColor: (NSColor*)labelOffColor label: (NSString*)label togglePosition: (CGFloat)togglePosition isDarkMode: (BOOL)isDarkMode;
+ (void)drawTranslucentButtonWithFrame: (NSRect)frame label: (NSString*)label pressed: (BOOL)pressed;
+ (void)drawActionButtonWithFrame: (NSRect)frame buttonColor: (NSColor*)buttonColor hasIcon: (BOOL)hasIcon label: (NSString*)label description: (NSString*)description;

@end
