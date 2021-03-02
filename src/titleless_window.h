/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2020-2021 Vaclav Slavik
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

#ifndef Poedit_titleless_window_h
#define Poedit_titleless_window_h

#include <wx/frame.h>
#include <wx/dialog.h>

class WXDLLIMPEXP_CORE wxButton;


/**
    Window without a titlebar.

    Used for windows with redundant titlebar (welcome, progress etc.).
    On Windows, only shown on modern versions (Windows 10) and when no
    accessibility reader is present, to avoid degrading usability.
 */
template<typename T>
class TitlelessWindowBase : public T
{
public:
    TitlelessWindowBase(wxWindow* parent, wxWindowID id, const wxString& title,
                        const wxPoint& pos, const wxSize& size,
                        long style, const wxString& name);

protected:
    /// Returns close button if it is presnet; may be null
    wxButton* GetCloseButton() const { return m_closeButton; }

    bool Layout() override;

#ifdef __WXMSW__
    wxPoint GetClientAreaOrigin() const override;
    void DoGetClientSize(int *width, int *height) const override;
    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override;
    void OnPaintBackground(wxPaintEvent& event);

private:
    static bool ShouldRemoveChrome();
    bool m_isTitleless = false;
#endif // __WXMSW__

private:
    wxButton* m_closeButton = nullptr;

    typedef T BaseClass;
};


class TitlelessWindow : public TitlelessWindowBase<wxFrame>
{
public:
    TitlelessWindow(wxWindow* parent,
                    wxWindowID id,
                    const wxString& title,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_FRAME_STYLE,
                    const wxString& name = wxFrameNameStr)
        : TitlelessWindowBase(parent, id, title, pos, size, style, name)
    {}
};


class TitlelessDialog : public TitlelessWindowBase<wxDialog>
{
public:
    TitlelessDialog(wxWindow* parent,
                    wxWindowID id,
                    const wxString& title,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = wxDEFAULT_DIALOG_STYLE,
                    const wxString& name = wxDialogNameStr)
        : TitlelessWindowBase(parent, id, title, pos, size, style, name)
    {}
};

#endif // Poedit_titleless_window_h
