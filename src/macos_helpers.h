/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2007-2017 Vaclav Slavik
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

#ifndef Poedit_macos_helpers_h
#define Poedit_macos_helpers_h

// FIXME: This is a hack to work around Automake's lack of support for ObjC++.
//        Remove it after switching build system to Bakefile.

#ifdef USE_SPARKLE
// Sparkle helpers
NSObject *Sparkle_Initialize();
void Sparkle_AddMenuItem(NSMenu *appmenu, const char *title);
void Sparkle_Cleanup();
#endif // USE_SPARKLE

// Native preferences
void UserDefaults_SetBoolValue(const char *key, int value);
int  UserDefaults_GetBoolValue(const char *key);
void UserDefaults_RemoveValue(const char *key);

// Misc UI helpers
void MakeButtonRounded(void *button);

void MoveToApplicationsFolderIfNecessary();

#endif // Poedit_macos_helpers_h
