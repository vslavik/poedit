/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2021 Vaclav Slavik
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

#include "hidpi.h"
#include "utility.h"

#include <wx/config.h>
#include <wx/msw/uxtheme.h>
#include <wx/nativewin.h>
#include <wx/recguard.h>

#include <mCtrl/menubar.h>

namespace
{

int g_mctrlInitialized = 0;

const int MENUBAR_OFFSET = -2;

} // anonymous namespace


class Windows10MenubarMixin::MenuWindow::mCtrlWrapper : public wxNativeWindow
{
public:
    mCtrlWrapper(wxWindow* parent, WXHWND wnd) : wxNativeWindow(parent, wxID_ANY, wnd), m_flagReenterMctrl(0) {}

    WXLRESULT MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam) override
    {
        switch (nMsg)
        {
            case WM_COMMAND:
            case WM_NOTIFY:
            {
                wxRecursionGuard guard(m_flagReenterMctrl);

                if (!guard.IsInside())
                {
                    return MSWDefWindowProc(nMsg, wParam, lParam);
                }
                else
                {
                    return ::DefWindowProc(GetHwnd(), nMsg, wParam, lParam);
                }
            }
        }

        return wxNativeWindow::MSWWindowProc(nMsg, wParam, lParam);
    }

private:
    wxRecursionGuardFlag m_flagReenterMctrl;
};

Windows10MenubarMixin::MenuWindow::MenuWindow() : m_mctrlWin(nullptr), m_mctrlHandle(0)
{
}

void Windows10MenubarMixin::MenuWindow::Create(wxWindow *parent)
{
    wxWindow::Create(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE);

    if (g_mctrlInitialized++ == 0)
        mcMenubar_Initialize();

    m_mctrlHandle = CreateWindowEx
    (
        WS_EX_COMPOSITED,
        MC_WC_MENUBAR,
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NORESIZE | CCS_NOPARENTALIGN,
        0, 0, 1000, 2 * PX(23),
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
        SetBackgroundColour(wxRGBToColour(::GetThemeSysColor(hTheme, COLOR_WINDOW)));
    }

    // mCtrl menus get focus, which is not compatible with PoeditFrame::OnTextEditingCommandUpdate().
    // Remember previous focus for it.
    m_mctrlWin->Bind(wxEVT_SET_FOCUS,  [=](wxFocusEvent& e) { e.Skip(); m_previousFocus = e.GetWindow(); });
    m_mctrlWin->Bind(wxEVT_KILL_FOCUS, [=](wxFocusEvent& e) { e.Skip(); m_previousFocus = nullptr; });
}

Windows10MenubarMixin::MenuWindow::~MenuWindow()
{
    m_mctrlWin->Destroy();
    ::DestroyWindow(m_mctrlHandle);

    if (--g_mctrlInitialized == 0)
        mcMenubar_Terminate();
}

void Windows10MenubarMixin::MenuWindow::SetHMENU(WXHMENU menu)
{
    SendMessage(m_mctrlHandle, MC_MBM_SETMENU, 0, (LPARAM) menu);
    SendMessage(m_mctrlHandle, MC_MBM_REFRESH, 0, 0);
}

bool Windows10MenubarMixin::MenuWindow::TranslateMenubarMessage(WXMSG *pMsg)
{
    MSG *msg = (MSG *) pMsg;
    if (mcIsMenubarMessage(m_mctrlHandle, msg))
        return true;

    return false;
}

wxWindow* Windows10MenubarMixin::MenuWindow::AdjustEffectiveFocus(wxWindow* focus) const
{
    return (focus == m_mctrlWin) ? m_previousFocus.get() : focus;
}

WXDWORD Windows10MenubarMixin::MenuWindow::MSWGetStyle(long flags, WXDWORD* exstyle) const
{
    // The toolbar control used by mCtrl doesn't fully paint its area, so we need to do it
    // in this wrapper window. Because wx clips childen unconditionally these days, it is
    // necessary to remove WS_CLIPCHILDREN here.
    return wxWindow::MSWGetStyle(flags, exstyle) & ~WS_CLIPCHILDREN;
}

void Windows10MenubarMixin::MenuWindow::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    wxWindow::DoSetSize(x, y, width, height, sizeFlags);
    SendMessage(m_mctrlHandle, MC_MBM_REFRESH, 0, 0);
}

wxSize Windows10MenubarMixin::MenuWindow::DoGetBestSize() const
{
    wxSize sizeBest = wxDefaultSize;
    SIZE size;
    if (::SendMessage(m_mctrlHandle, TB_GETMAXSIZE, 0, (LPARAM) &size))
    {
        sizeBest.x = size.cx;
        sizeBest.y = size.cy + 1;
        CacheBestSize(sizeBest);
    }
    return sizeBest;
}


bool Windows10MenubarMixin::ShouldUseCustomMenu() const
{
    if (!IsWindows10OrGreater())
        return false;

    if (!wxUxThemeIsActive())
        return false;

    if (wxConfig::Get()->ReadBool("/disable_mctrl", false))
        return false;

    // Detect screen readers and use normal menubar with them, because the
    // mCtrl one isn't accessible.
    BOOL running;
    BOOL ret = SystemParametersInfo(SPI_GETSCREENREADER, 0, &running, 0);
    if (ret && running)
        return false;

    return true;
}

void Windows10MenubarMixin::CreateCustomMenu(wxWindow* parent)
{
    m_menuBar = new MenuWindow;
    m_menuBar->Create(parent);
}

template<typename T>
WithWindows10Menubar<T>::WithWindows10Menubar(wxWindow* parent,
    wxWindowID id,
    const wxString& title,
    const wxPoint& pos,
    const wxSize& size,
    long style,
    const wxString& name)
    : BaseClass(parent, id, title, pos, size, style, name)
{
    if (ShouldUseCustomMenu())
        CreateCustomMenu(this);
}

template<typename T>
wxPoint WithWindows10Menubar<T>::GetClientAreaOrigin() const
{
    wxPoint pt = BaseClass::GetClientAreaOrigin();
    if (IsCustomMenuUsed() && ShouldPlaceMenuInNCArea())
    {
        pt.y += GetMenuWindow()->GetBestSize().y + MENUBAR_OFFSET;
    }
    return pt;
}

template<typename T>
wxWindow* WithWindows10Menubar<T>::FindFocusNoMenu()
{
    auto focus = wxWindow::FindFocus();
    if (focus && IsCustomMenuUsed())
        focus = GetMenuWindow()->AdjustEffectiveFocus(focus);
    return focus;
}

template<typename T>
void WithWindows10Menubar<T>::PositionToolBar()
{
    // Position both the toolbar and our menu bar (which is really another toolbar) here.
    if (!IsCustomMenuUsed() || !ShouldPlaceMenuInNCArea())
    {
        BaseClass::PositionToolBar();
        return;
    }

    // don't call our (or even wxTopLevelWindow) version because we want
    // the real (full) client area size, not excluding the tool/status bar
    int width, height;
    wxWindow::DoGetClientSize(&width, &height);

    int y = MENUBAR_OFFSET;

    // use the 'real' MSW position here, don't offset relatively to the
    // client area origin
    int mch = GetMenuWindow()->GetBestSize().y;
    GetMenuWindow()->SetSize(0, y, width, mch, wxSIZE_NO_ADJUSTMENTS);
    y += mch;

    wxToolBar *toolbar = GetToolBar();
    if (toolbar && toolbar->IsShown())
    {
        int tbh = toolbar->GetSize().y;
        toolbar->SetSize(0, y, width + 8, tbh, wxSIZE_NO_ADJUSTMENTS);
    }
}

template<typename T>
bool WithWindows10Menubar<T>::MSWTranslateMessage(WXMSG *msg)
{
    if (BaseClass::MSWTranslateMessage(msg))
        return true;

    if (IsCustomMenuUsed() && GetMenuWindow()->TranslateMenubarMessage(msg))
        return true;

    return false;
}

template<typename T>
WXLRESULT WithWindows10Menubar<T>::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    if (IsCustomMenuUsed())
    {
        // mCtrl doesn't play nice with the wxMSW's menus interaction where accelerators are updated
        // when a menu is opened (which works because TranslateAccelerators() normally sends a fake
        // event for that... if there's a normal menu). We need to refresh menus before accelerators
        // are used so that e.g. disabled state is accurately updated.
        if (message == WM_COMMAND && HIWORD(wParam) == 1/*accel*/)
            GetMenuBar()->UpdateMenus();
    }

    return BaseClass::MSWWindowProc(message, wParam, lParam);
}

template<typename T>
void WithWindows10Menubar<T>::InternalSetMenuBar()
{
    if (IsCustomMenuUsed())
    {
        GetMenuWindow()->SetHMENU(m_hMenu);
    }
    else
    {
        BaseClass::InternalSetMenuBar();
    }
}


// We need to explicitly instantiate the template for all uses within Poedit because of all the code in .cpp file:
template class WithWindows10Menubar<wxFrame>;

#include "titleless_window.h"
template class WithWindows10Menubar<TitlelessWindow>;
