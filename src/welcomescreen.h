/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2017 Vaclav Slavik
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

#ifndef Poedit_welcomescreen_h
#define Poedit_welcomescreen_h

#include <wx/panel.h>

class PoeditFrame;

class WelcomeScreenBase : public wxPanel
{
protected:
    WelcomeScreenBase(wxWindow *parent);

    wxFont m_fntHeader, m_fntNorm, m_fntSub;
    wxColour m_clrHeader, m_clrNorm, m_clrSub;
};

/// Content view for initially opened Poedit, without a file
class WelcomeScreenPanel : public WelcomeScreenBase
{
public:
    WelcomeScreenPanel(wxWindow *parent);
};


/// Content view for an empty file (File->New)
class EmptyPOScreenPanel : public WelcomeScreenBase
{
public:
    EmptyPOScreenPanel(PoeditFrame *parent);
};


#endif // Poedit_welcomescreen_h
