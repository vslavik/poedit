/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2024 Vaclav Slavik
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

#ifndef Poedit_static_ids_h
#define Poedit_static_ids_h

#include <wx/defs.h>

namespace WinID
{

/**
	IDs of all controls in the application that need them fixed.

	This is in small part for code convenience, but primarily to accomodate
	NVDA screen reader, which needs them to be able to lookup UI elements.

	See https://github.com/vslavik/poedit/issues/845

    See https://github.com/nvaccess/nvda/blob/master/source/appModules/poedit.py
    for NVDA's Poedit-specific code.

	!!! DO NOT CHANGE NUMERIC VALUES !!!
 */
enum ID
{
	// Not a real ID; this value is smaller than any actual ID in this list
	MIN                                          = 10000,

    // Since version 3.5:

	// Menu items in the list's context menu, used for file reference entries
	ListContextReferencesStart                   = /*10000*/ MIN,
	ListContextReferencesEnd                     = /*10100*/ ListContextReferencesStart + 100,


	// ...enter new IDs here...

	// Not a real ID; this value is larger than any actual ID in this list
	MAX
};

static_assert((int)WinID::MIN > (int)wxID_HIGHEST, "static IDs conflict with wxWidgets");
static_assert((int)WinID::MIN > (int)wxID_AUTO_HIGHEST, "static IDs conflict with wxWidgets");

} // WinID

#endif // Poedit_static_ids_h
