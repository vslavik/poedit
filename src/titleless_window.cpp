/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2020 Vaclav Slavik
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

#include "titleless_window.h"

#include "hidpi.h"
#include "utility.h"

#include <wx/bmpbuttn.h>

#ifdef __WXMSW__
    #include <wx/msw/private.h>
    #include <wx/msw/uxtheme.h>
    #include <dwmapi.h>
    #pragma comment(lib, "Dwmapi.lib")
#endif


#ifdef __WXMSW__

namespace
{

// See https://docs.microsoft.com/en-us/windows/uwp/design/style/segoe-ui-symbol-font

wxFont CreateButtonFont()
{
    return wxFont(wxSize(PX(10), PX(10)), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Segoe MDL2 Assets");
}

const wchar_t* Symbol_ChromClose = L"\U0000E8BB";

wxBitmap RenderButton(const wxSize& size, const wxFont& font, const wxColour& bg, const wxColour& fg)
{
    wxBitmap bmp(size);
    {
        wxMemoryDC dc(bmp);
        dc.SetBackground(bg);
        dc.SetFont(font);
        dc.SetTextForeground(fg);
        
        dc.Clear();

        wxString text(Symbol_ChromClose);
        auto extent = dc.GetTextExtent(text);
        dc.DrawText(text, (size.x - extent.x) / 2, (size.y - extent.y) / 2);
    }
    return bmp;
}

} // anonymous namespace


class TitlelessWindow::CloseButton : public wxBitmapButton
{
public:
    CloseButton(wxWindow* parent, wxWindowID id)
    {
        wxBitmapButton::Create(parent, id, wxNullBitmap, wxDefaultPosition, GetButtonSize(), wxBORDER_NONE);
        SetToolTip(_("Close"));
        CreateBitmaps();

        wxGetTopLevelParent(parent)->Bind(wxEVT_ACTIVATE, [=](wxActivateEvent& e) {
            e.Skip();
            if (!IsBeingDeleted())
                CreateBitmaps(/*onlyBackgroundRelated:*/true, /*isActive:*/e.GetActive());
        });
    }

private:
    wxSize GetButtonSize() const { return wxSize(PX(46), PX(28)); }

    void CreateBitmaps(bool onlyBackgroundRelated = false, bool isActive = true)
    {
        // Minic Windows 10's caption buttons
        auto size = GetButtonSize();
        auto font = CreateButtonFont();
        m_normal = RenderButton(size, font, GetBackgroundColour(), *wxBLACK);
        if (!onlyBackgroundRelated)
            m_hover = RenderButton(size, font, wxColour(232, 17, 35), *wxWHITE);
        m_inactive = RenderButton(size, font, GetBackgroundColour(), wxColour(153,153,153));

        SetBitmap(isActive ? m_normal : m_inactive);
        SetBitmapHover(m_hover);
    }

    wxBitmap m_normal, m_hover, m_inactive;
};

#endif // __WXMSW__


TitlelessWindow::TitlelessWindow(wxWindow* parent,
                                 wxWindowID id,
                                 const wxString& title,
                                 const wxPoint& pos,
                                 const wxSize& size,
                                 long style,
                                 const wxString& name)
    : wxFrame(parent, id, title, pos, size, style, name)
{
#ifdef __WXOSX__
    // Pretify the window:
    NSWindow* wnd = (NSWindow*)GetWXWindow();
    wnd.styleMask |= NSFullSizeContentViewWindowMask;
    wnd.titleVisibility = NSWindowTitleHidden;
    wnd.titlebarAppearsTransparent = YES;
    wnd.movableByWindowBackground = YES;
    [wnd standardWindowButton:NSWindowMiniaturizeButton].hidden = YES;
    [wnd standardWindowButton:NSWindowZoomButton].hidden = YES;
#endif

#ifdef __WXMSW__
    m_isTitleless = ShouldRemoveChrome();
    if (m_isTitleless)
    {
        auto handle = GetHwnd();

        static MARGINS margins = { PX(1), PX(1), PX(1), PX(1) };
        DwmExtendFrameIntoClientArea(handle, &margins);

        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &TitlelessWindow::OnPaintBackground, this);

        if (style & wxCLOSE_BOX)
            m_closeButton = new CloseButton(this, wxID_CLOSE);
    }
#endif // __WXMSW__

#ifdef __WXGTK3__
    // TODO: Under WXGTK, use GtkButton with icon "window-close-symbolic" with style class "titlebutton"
    //       https://stackoverflow.com/questions/22069839/how-to-theme-the-header-bar-close-button-in-gtk3
#endif

    if (m_closeButton)
        m_closeButton->Bind(wxEVT_BUTTON, [=](wxCommandEvent&) { Close(); });
}

#ifdef __WXMSW__
bool TitlelessWindow::ShouldRemoveChrome()
{
    if (!IsWindows10OrGreater())
        return false;

    if (!wxUxThemeIsActive())
        return false;

    // Detect screen readers and use normal titlebars to not confuse them
    BOOL running;
    BOOL ret = SystemParametersInfo(SPI_GETSCREENREADER, 0, &running, 0);
    if (ret && running)
        return false;

    return true;
}

wxPoint TitlelessWindow::GetClientAreaOrigin() const
{
    if (m_isTitleless)
        return wxPoint(PX(1), PX(1));
    else
        return wxFrame::GetClientAreaOrigin();
}

void TitlelessWindow::DoGetClientSize(int *width, int *height) const
{
    if (m_isTitleless)
    {
        auto size = GetSize();
        if (width)
            *width = size.x - 2 * PX(1);
        if (height)
            *height = size.y - 2 * PX(1);
    }
    else
    {
        return wxFrame::DoGetClientSize(width, height);
    }
}

WXLRESULT TitlelessWindow::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if (m_isTitleless)
    {
        switch (nMsg)
        {
        case WM_NCCALCSIZE:
            if (wParam == TRUE)
            {
                // ((LPNCCALCSIZE_PARAMS)lParam)->rgrc[0] is window size on input and
                // client size on output; by doing nothing here, we set NC area to zero.
                return 0;
            }
            break;

        case WM_NCHITTEST:
            // When we have no border or title bar, we need to perform our
            // own hit testing to allow moving etc.
            // See https://docs.microsoft.com/en-us/windows/win32/dwm/customframe
            return HTCAPTION;
        }
    }

    return wxFrame::MSWWindowProc(nMsg, wParam, lParam);
}

void TitlelessWindow::OnPaintBackground(wxPaintEvent&)
{
    wxPaintDC dc(this);

    // 1pt margins around the window must be black for DwmExtendFrameIntoClientArea() to work.
    // It would have been better to instead set 1pt non-client area in WM_NCCALCSIZE, but that
    // doesn't work for the top side, unfortunately, so here we are.
    dc.SetPen(*wxTRANSPARENT_PEN);

    wxRect rect(wxPoint(0, 0), GetSize());
    dc.SetBrush(*wxBLACK);
    dc.DrawRectangle(rect);

    rect.Deflate(PX(1));
    dc.SetBrush(GetBackgroundColour());
    dc.DrawRectangle(rect);
}
#endif // __WXMSW__


bool TitlelessWindow::Layout()
{
    if (!wxFrame::Layout())
        return false;

    if (m_closeButton)
    {
        auto size = GetClientSize();
        m_closeButton->Move(size.x - m_closeButton->GetSize().x, 0);
    }

    return true;
}
