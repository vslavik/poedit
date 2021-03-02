/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2021 Vaclav Slavik
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

#ifndef Poedit_menus_h
#define Poedit_menus_h

#include "catalog.h"

#include <wx/menu.h>


/// Kind of menu to load
enum class Menu
{
    Global, // app-global menu used on macOS
    Editor, // for PoeditFrame, editor window
    WelcomeWindow
};


/**
    Management of various menus in Poedit.

    Centralizes platform-specific hacks and deals with menu variants in
    different windows.
 */
class MenusManager
{
public:
    MenusManager();
    ~MenusManager();

    wxMenuBar *CreateMenu(Menu purpose);

#ifdef __WXOSX__
protected:
    void FixupMenusForMacIfNeeded();

private:
    // Make OSX-specific modifications to the menus, e.g. adding items into
    // the apple menu etc. Call on every newly created menubar
    void TweakOSXMenuBar(wxMenuBar *bar);

    class NativeMacData;
    std::unique_ptr<NativeMacData> m_nativeMacData;
#endif // __WXOSX__
};

#endif // Poedit_menus_h
