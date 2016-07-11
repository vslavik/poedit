/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016 Vaclav Slavik
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

#include "win10_menubar.h"

#include "utility.h"

#include <wx/msw/uxtheme.h>
#include <wx/nativewin.h>
#if !wxCHECK_VERSION(3,1,0)
  #include "wx_backports/nativewin.h"
#endif

#include <mCtrl/menubar.h>

namespace
{

int g_mctrlInitialized = 0;

} // anonymous namespace


class wxFrameWithWindows10Menubar::MenuBarWindow : public wxWindow
{
public:
    MenuBarWindow(wxWindow *parent)
        : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
          m_mctrlWin(nullptr), m_mctrlHandle(0)
    {
        if (g_mctrlInitialized++ == 0)
            mcMenubar_Initialize();

        m_mctrlHandle = CreateWindowEx
        (
            WS_EX_COMPOSITED,
            MC_WC_MENUBAR,
            _T(""),
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NORESIZE | CCS_NOPARENTALIGN,
            0, 0, 400, 23*2,
            (HWND)this->GetHWND(),
            (HMENU) -1,
            wxGetInstance(),
            NULL
        );
        SendMessage(m_mctrlHandle, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        SendMessage(m_mctrlHandle, CCM_SETNOTIFYWINDOW, (WPARAM)parent->GetHWND(), 0);

        m_mctrlWin = new mCtrlWrapper(this, m_mctrlHandle);

        {
            wxUxThemeHandle hTheme(this, L"ExplorerMenu::Toolbar");
            SetBackgroundColour(wxRGBToColour(wxUxThemeEngine::GetIfActive()->GetThemeSysColor(hTheme, COLOR_WINDOW)));
        }
    }

    ~MenuBarWindow()
    {
        m_mctrlWin->Destroy();
        ::DestroyWindow(m_mctrlHandle);

        if (--g_mctrlInitialized == 0)
            mcMenubar_Terminate();
    }

    void SetHMENU(WXHMENU menu)
    {
        SendMessage(m_mctrlHandle, MC_MBM_SETMENU, 0, (LPARAM) menu);
        SendMessage(m_mctrlHandle, MC_MBM_REFRESH, 0, 0);
    }

    bool TranslateMenubarMessage(WXMSG *pMsg)
    {
        MSG *msg = (MSG *) pMsg;
        if (mcIsMenubarMessage(m_mctrlHandle, msg))
            return true;
        return false;
    }

    void DoSetSize(int x, int y, int width, int height, int sizeFlags) override
    {
        wxWindow::DoSetSize(x, y, width, height, sizeFlags);
        SendMessage(m_mctrlHandle, MC_MBM_REFRESH, 0, 0);
    }

    wxSize DoGetBestSize() const override
    {
        wxSize sizeBest = wxDefaultSize;
        SIZE size;
        if (::SendMessage(m_mctrlHandle, TB_GETMAXSIZE, 0, (LPARAM) &size))
        {
            sizeBest.y = size.cy + 1;
            CacheBestSize(sizeBest);
        }
        return sizeBest;
    }

private:
    class mCtrlWrapper : public wxNativeWindow
    {
    public:
        mCtrlWrapper(wxWindow *parent, WXHWND wnd) : wxNativeWindow(parent, wxID_ANY, wnd) {}

        WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override
        {
            switch (nMsg)
            {
                case WM_COMMAND:
                case WM_NOTIFY:
                    return MSWDefWindowProc(nMsg, wParam, lParam);
            }
            return wxNativeWindow::MSWWindowProc(nMsg, wParam, lParam);
        }
    };

    mCtrlWrapper *m_mctrlWin;
    WXHWND m_mctrlHandle;
};


wxFrameWithWindows10Menubar::wxFrameWithWindows10Menubar(wxWindow *parent,
                                                         wxWindowID id,
                                                         const wxString& title,
                                                         const wxPoint& pos,
                                                         const wxSize& size,
                                                         long style,
                                                         const wxString& name)
    : wxFrame(parent, id, title, pos, size, style, name), m_menuBar(nullptr)
{
    if (ShouldUse())
    {
        m_menuBar = new MenuBarWindow(this);
    }
}

bool wxFrameWithWindows10Menubar::ShouldUse() const
{
    if (!IsWindows10OrGreater())
        return false;

    if (!wxUxThemeEngine::GetIfActive())
        return false;

    // Detect screen readers and use normal menubar with them, because the
    // mCtrl one isn't accessible.
    BOOL running;
    BOOL ret = SystemParametersInfo(SPI_GETSCREENREADER, 0, &running, 0);
    if (ret && running)
        return false;

    return true;
}

wxPoint wxFrameWithWindows10Menubar::GetClientAreaOrigin() const
{
    wxPoint pt = wxFrame::GetClientAreaOrigin();
    if (IsUsed())
    {
        pt.y += m_menuBar->GetSize().y;
    }
    return pt;
}

void wxFrameWithWindows10Menubar::PositionToolBar()
{
    // Position both the toolbar and our menu bar (which is really another toolbar) here.
    if (!IsUsed())
    {
        wxFrame::PositionToolBar();
        return;
    }

    // don't call our (or even wxTopLevelWindow) version because we want
    // the real (full) client area size, not excluding the tool/status bar
    int width, height;
    wxWindow::DoGetClientSize(&width, &height);

    int y = 0;

    // use the 'real' MSW position here, don't offset relatively to the
    // client area origin
    int mch = m_menuBar->GetBestSize().y;
    m_menuBar->SetSize(0, y, width, mch, wxSIZE_NO_ADJUSTMENTS);
    y += mch;

    wxToolBar *toolbar = GetToolBar();
    if (toolbar && toolbar->IsShown())
    {
        int tbh = toolbar->GetSize().y;
        toolbar->SetSize(0, y, width, tbh, wxSIZE_NO_ADJUSTMENTS);
    }
}

bool wxFrameWithWindows10Menubar::MSWTranslateMessage(WXMSG *msg)
{
    if (wxFrame::MSWTranslateMessage(msg))
        return true;

    if (IsUsed() && m_menuBar->TranslateMenubarMessage(msg))
        return true;

    return false;
}

void wxFrameWithWindows10Menubar::InternalSetMenuBar()
{
    if (IsUsed())
    {
        m_menuBar->SetHMENU(m_hMenu);
    }
    else
    {
        wxFrame::InternalSetMenuBar();
    }
}
