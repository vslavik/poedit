/*
 *  This file is part of poEdit (http://www.poedit.net)
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
 *  wxEditableListBox inclusion
 *
 */

#ifndef _WXEDITABLELISTBOX_H_
#define _WXEDITABLELISTBOX_H_

// wxWidgets up to 2.8 didn't include wxEditableListBox in the main library,
// but later versions do and if we used our private version, we'd get class
// name conflicts from wxRTTI. This header includes either wxWidgets' version
// or our private copy, depending on the wxWidgets version used.

#include <wx/version.h>

#if wxCHECK_VERSION(2,9,0)
    #include <wx/editlbox.h>
#else
    #include "editlbox/editlbox.h"
#endif

#endif // _WXEDITABLELISTBOX_H_
