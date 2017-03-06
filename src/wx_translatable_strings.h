/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2017 Vaclav Slavik
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


// This is a dummy file that is not included anywhere or used for compilation.
//
// Its sole purpose is to make some strings from wxWidgets appear in Poedit's
// source code. This is done to force translation of wx strings that are used
// by Poedit (which is a small subset of wx's full translation). Poedit has
// many more and more complete translations than wxWidgets, so including them
// here ensures better localization.


// --------------------------------------------------------------------------
// Stock windows
// --------------------------------------------------------------------------

// TRANSLATORS: This is titlebar of about dialog, "%s" is application name
//              ("Poedit" here, but please use "%s")
_("About %s");
// TRANSLATORS: This is version information in about dialog, "%s" will be
//              version number when used
_("Version %s");

// TRANSLATORS: Title of the preferences window on Windows and Linux. "%s" is replaced with "Poedit" when running.
_("%s Preferences");


// --------------------------------------------------------------------------
// macOS
// --------------------------------------------------------------------------

// application menu:

// TRANSLATORS: macOS item in app menu
_("Services");
// TRANSLATORS: macOS item in app menu, %s is replaced with "Poedit"
_("Hide %s");
// TRANSLATORS: macOS item in app menu
_("Hide Others");
// TRANSLATORS: macOS item in app menu
_("Show All");
// TRANSLATORS: macOS item in app menu, %s is replaced with "Poedit"
_("Quit %s");
// TRANSLATORS: macOS item in app menu
_(L"Preferences…");
_("Preferences...");


// --------------------------------------------------------------------------
// Windows
// --------------------------------------------------------------------------

// system taskbar categories:

// TRANSLATORS: Title of a category in Windows task menu (right-click icon in taskbar)
_("Recent");
// TRANSLATORS: Title of a category in Windows task menu (right-click icon in taskbar)
_("Frequent");


// --------------------------------------------------------------------------
// Stock menu/button items
// --------------------------------------------------------------------------

STOCKITEM(wxID_APPLY,               _("&Apply"),              _("Apply"));
STOCKITEM(wxID_BACKWARD,            _("&Back"),               _("Back"));
STOCKITEM(wxID_CANCEL,              _("&Cancel"),             _("Cancel"));
STOCKITEM(wxID_CLEAR,               _("&Clear"),              _("Clear"));
STOCKITEM(wxID_CLOSE,               _("&Close"),              _("Close"));

STOCKITEM(wxID_COPY,                _("&Copy"),               _("Copy"));
STOCKITEM(wxID_CUT,                 _("Cu&t"),                _("Cut"));
STOCKITEM(wxID_DELETE,              _("&Delete"),             _("Delete"));

STOCKITEM(wxID_EDIT,                _("&Edit"),               _("Edit"));
STOCKITEM(wxID_EXIT,                _("&Quit"),               _("Quit"));
STOCKITEM(wxID_FILE,                _("&File"),               _("File"));
STOCKITEM(wxID_HELP,                _("&Help"),               _("Help"));
STOCKITEM(wxID_NEW,                 _("&New"),                _("New"));
STOCKITEM(wxID_NO,                  _("&No"),                 _("No"));
STOCKITEM(wxID_OK,                  _("&OK"),                 _("OK"));
STOCKITEM(wxID_OPEN,                _(L"&Open…"),             _(L"Open…"));
STOCKITEM(wxID_OPEN,                _("&Open..."),            _("Open..."));
STOCKITEM(wxID_PASTE,               _("&Paste"),              _("Paste"));
STOCKITEM(wxID_PREFERENCES,         _("&Preferences"),        _("Preferences"));
STOCKITEM(wxID_REDO,                _("&Redo"),               _("Redo"));
STOCKITEM(wxID_REFRESH,             _("Refresh"),             _("Refresh"));
STOCKITEM(wxID_SAVE,                _("&Save"),               _("Save"));
STOCKITEM(wxID_SAVEAS,              _("&Save as"),            _("Save as"));
STOCKITEM(wxID_SELECTALL,           _("Select &All"),         _("Select All"));
STOCKITEM(wxID_UNDO,                _("&Undo"),               _("Undo"));
STOCKITEM(wxID_YES,                 _("&Yes"),                _("Yes"));


/// TRANSLATORS: Keyboard shortcut for display in Windows menus
_("Ctrl+");
/// TRANSLATORS: Keyboard shortcut for display in Windows menus
_("Alt+");
/// TRANSLATORS: Keyboard shortcut for display in Windows menus
_("Shift+");
/// TRANSLATORS: Keyboard shortcut for display in Windows menus
_("Enter");
/// TRANSLATORS: Keyboard shortcut for display in Windows menus
_("Up");
/// TRANSLATORS: Keyboard shortcut for display in Windows menus
_("Down");

/// TRANSLATORS: Keyboard shortcut, must correspond to translation of "Ctrl+"
_("ctrl");
/// TRANSLATORS: Keyboard shortcut, must correspond to translation of "Alt+"
_("alt");
/// TRANSLATORS: Keyboard shortcut, must correspond to translation of "Shift+"
_("shift");
