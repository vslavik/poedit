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

#ifndef Poedit_custom_buttons_h
#define Poedit_custom_buttons_h

#include <wx/button.h>
#include <wx/tglbtn.h>

#ifdef __WXOSX__
    #include <wx/nativewin.h>
    #if !wxCHECK_VERSION(3,1,0)
        #include "wx_backports/nativewin.h"
    #endif
#endif

#include <memory>


#ifdef __WXOSX__

/**
    iOS/Windows10/GtkSwitch style button.
    
    Mimics wxToggleButton with API and sent events.
 */
class SwitchButton : public wxNativeWindow
{
public:
    SwitchButton(wxWindow *parent, wxWindowID winid, const wxString& label);
    ~SwitchButton();

    void SetColors(const wxColour& on, const wxColour& offLabel);

    void SetValue(bool value);
    bool GetValue() const;

    void SendToggleEvent();

private:
    class impl;
    std::unique_ptr<impl> m_impl;
};


/**
    A partially transparent button.
 */
class TranslucentButton : public wxNativeWindow
{
public:
    TranslucentButton(wxWindow *parent, wxWindowID winid, const wxString& label);
};


#else // !__WXOSX__


class SwitchButton : public wxToggleButton
{
public:
    SwitchButton(wxWindow *parent, wxWindowID winid, const wxString& label);

    void SetColors(const wxColour& on, const wxColour& offLabel);

#ifdef __WXMSW__
    bool ShouldInheritColours() const override { return true; }
    void OnMouseClick(wxMouseEvent& e);
    bool MSWOnDraw(WXDRAWITEMSTRUCT *wxdis) override;
    wxSize DoGetBestSize() const override;

private:
    wxColour m_clrOn, m_clrOffLabel;
#endif // __WXMSW__
};


class TranslucentButton : public wxButton
{
public:
    TranslucentButton(wxWindow *parent, wxWindowID winid, const wxString& label);
};

#endif // !__WXOSX__


#endif // Poedit_custom_buttons_h
