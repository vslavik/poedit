/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2014-2015 Vaclav Slavik
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

#ifndef Poedit_main_toolbar_h
#define Poedit_main_toolbar_h

#include <wx/frame.h>
#include <memory>

/// Abstract interface to the app's main toolbar.
class MainToolbar
{
public:
    virtual ~MainToolbar() {}

    virtual bool IsFuzzy() const = 0;
    virtual void SetFuzzy(bool on) = 0;

    virtual void EnableSyncWithCrowdin(bool on) = 0;

    static std::unique_ptr<MainToolbar> Create(wxFrame *parent);

protected:
    static std::unique_ptr<MainToolbar> CreateWX(wxFrame *parent);

    MainToolbar() {}
};

#endif // Poedit_main_toolbar_h
