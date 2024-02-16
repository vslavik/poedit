/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2023 Vaclav Slavik
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

#include "custom_notebook.h"

#include "colorscheme.h"
#include "hidpi.h"
#include "str_helpers.h"
#include "utility.h"

#include <wx/dcclient.h>
#include <wx/panel.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/tglbtn.h>

#ifdef __WXOSX__
#include <wx/nativewin.h>
#endif

#ifdef __WXMSW__
#include <wx/msw/dc.h>
#endif

#include <vector>


#ifdef HAS_SEGMENTED_NOTEBOOK

// Abstract interface to platform implementations of segmented tabbing
class SegmentedNotebook::TabsIface
{
public:
    virtual ~TabsIface() {}

    virtual wxSizer *GetExtensibleArea() const = 0;

    virtual void InsertPage(size_t n, const wxString& label) = 0;
    virtual void RemovePage(size_t n) = 0;
    virtual void RemoveAllPages() = 0;

    virtual void ChangeSelection(size_t n) = 0;

    virtual void UpdateBackgroundColour() = 0;
};


#ifdef __WXOSX__

@interface POSegmentedNotebookController : NSObject

@property wxSimplebook *book;

@end

@implementation POSegmentedNotebookController
- (void)tabSelected:(NSSegmentedControl*)sender
{
    // SetSelection() generates events:
    self.book->SetSelection((int)sender.selectedSegment);
}
@end

// NSSegmentedControl-based implementation
class SegmentedNotebook::SegmentedControlTabs : public wxNativeWindow,
                                                public SegmentedNotebook::TabsIface
{
public:
    SegmentedControlTabs(wxSimplebook *parent, SegmentStyle style) : wxNativeWindow()
    { 
        m_labels = [NSMutableArray new];
        m_control = [NSSegmentedControl new];
        Create(parent, wxID_ANY, m_control);

        m_controller = [POSegmentedNotebookController new];
        m_controller.book = parent;
        [m_control setAction:@selector(tabSelected:)];
        [m_control setTarget:m_controller];

        switch (style)
        {
            case SegmentStyle::SmallInline:
                m_control.segmentStyle = NSSegmentStyleRoundRect;
                SetWindowVariant(wxWINDOW_VARIANT_SMALL);
                break;
            case SegmentStyle::LargeFullWidth:
                m_control.segmentStyle = NSSegmentStyleTexturedRounded;
                SetWindowVariant(wxWINDOW_VARIANT_LARGE);
                break;
            case SegmentStyle::SidebarPanels:
                wxFAIL_MSG("this style can't be used with NSSegmentedControl");
                break;
        }
    }

    wxSizer *GetExtensibleArea() const override { return nullptr; }

    void InsertPage(size_t n, const wxString& label) override
    {
        [m_labels insertObject:str::to_NS(label) atIndex:n];
        UpdateLabels();
    }

    void RemovePage(size_t n) override
    {
        [m_labels removeObjectAtIndex:n];
        UpdateLabels();
    }

    void RemoveAllPages() override
    {
        [m_labels removeAllObjects];
        m_control.segmentCount = 0;
    }

    void ChangeSelection(size_t n) override
        { m_control.selectedSegment = n; }

    void UpdateBackgroundColour() override
        { /* no action needed with native NSSegmentedControl */ }

private:
    void UpdateLabels()
    {
        m_control.segmentCount = m_labels.count;
        [m_labels enumerateObjectsUsingBlock:^(NSString *label, NSUInteger i, BOOL *) {
            [m_control setLabel:label forSegment:i];
        }];
    }

    wxSimplebook *Book() { return (wxSimplebook*)GetParent(); }

private:
    NSMutableArray<NSString *> *m_labels;
    NSSegmentedControl *m_control;
    POSegmentedNotebookController *m_controller;
};

#endif // __WXOSX__


class SegmentedNotebook::TabButton : public wxToggleButton
{
public:
    TabButton(wxWindow *parent, const wxString& label)
        : wxToggleButton(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT)
    {
#if defined(__WXOSX__)
        auto native = static_cast<NSButton*>(GetHandle());
        native.bezelStyle = NSBezelStyleRecessed;
        native.showsBorderOnlyWhileMouseInside = YES;
#elif defined(__WXMSW__)
        MakeOwnerDrawn();
        SetFont(GetFont().Bold());
        m_clrHighlight = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
#endif
    }

#ifdef __WXMSW__
    bool ShouldInheritColours() const override { return true; }
    virtual bool HasTransparentBackground() wxOVERRIDE { return true; }

    wxSize DoGetBestSize() const override
    {
        auto size = GetTextExtent(GetLabel());
        size.y += PX(6);
        return size;
    }

    bool MSWOnDraw(WXDRAWITEMSTRUCT* wxdis) override
    {
        LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)wxdis;
        HDC hdc = lpDIS->hDC;

        UINT state = lpDIS->itemState;
        if (GetNormalState() == State_Pressed)
            state |= ODS_SELECTED;
        const bool highlighted = IsMouseInWindow();

        auto label = GetLabel();
        auto rect = wxRectFromRECT(lpDIS->rcItem);

        wxDCTemp dc((WXHDC)hdc);

        wxRect textRect(dc.GetTextExtent(label));
        textRect = textRect.CenterIn(rect, wxHORIZONTAL);
        textRect.Offset(0, PX(1));

        if (state & ODS_SELECTED)
        {
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.SetBrush(m_clrHighlight);
            dc.DrawRectangle(wxRect(textRect.x, rect.y + rect.height - PX(2), textRect.width, PX(2)));
        }

        dc.SetTextForeground(highlighted ? m_clrHighlight : GetForegroundColour());
        dc.SetFont(GetFont());
        dc.DrawText(label, textRect.x, textRect.y);

        // draw the focus rectangle if we need it
        if ((state & ODS_FOCUS) && !(state & ODS_NOFOCUSRECT))
        {
            RECT r = { rect.x, rect.y, rect.width, rect.height };
            DrawFocusRect(hdc, &r);
        }

        return true;
    }

private:
    wxColour m_clrHighlight;
#endif // __WXMSW__
};

class SegmentedNotebook::ButtonTabs : public wxPanel,
                                      public SegmentedNotebook::TabsIface
{
public:
    ButtonTabs(wxSimplebook *parent, SegmentStyle style)
        : wxPanel(parent, wxID_ANY)
    {
        m_book = parent;
        m_style = style;

        m_buttonsSizer = new wxBoxSizer(wxHORIZONTAL);
        m_wrappingSizer = new wxBoxSizer(wxHORIZONTAL);
        m_wrappingSizer->AddStretchSpacer();
        m_wrappingSizer->Add(m_buttonsSizer, wxSizerFlags().Expand());
        m_wrappingSizer->AddStretchSpacer();

        auto topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(m_wrappingSizer, wxSizerFlags(1).Expand());
        topsizer->AddSpacer(PX(5));
#ifdef __WXOSX__
        if (@available(macOS 11.0, *))
        {
            topsizer->InsertSpacer(0, PX(2));
            topsizer->AddSpacer(PX(1));
        }
#endif
        SetSizer(topsizer);

        Bind(wxEVT_PAINT, &ButtonTabs::OnPaint, this);
    }

    wxSizer *GetExtensibleArea() const override { return m_wrappingSizer; }

    void OnPaint(wxPaintEvent&)
    {
        auto children = GetChildren();

        wxPaintDC dc(this);
        auto clr = ColorScheme::Get(Color::SidebarBlockSeparator);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(clr);

        switch (m_style)
        {
            case SegmentStyle::SmallInline:
#ifdef __WXMSW__
                for (size_t i = 1; i < children.size(); ++i)
                {
                    auto r = children[i]->GetRect();
                    dc.DrawRectangle(r.x - PX(5) - PX(1), r.y, PX(1), r.height);
                }
#endif // __WXMSW__
                break;
            
            case SegmentStyle::LargeFullWidth:
#ifdef __WXMSW__
                if (!children.empty())
                {
                    auto c = children[0]->GetRect();
                    dc.DrawRectangle(0,  c.y + c.height, GetClientSize().x, PX(1));
                }
#endif // __WXMSW__
                break;

            case SegmentStyle::SidebarPanels:
                {
#ifdef __WXOSX__
                    auto size = GetClientSize();
                    dc.DrawRectangle(0, size.y - PX(2), size.x, PX(1));
#endif
                }
                break;
        }
    }

    void InsertPage(size_t n, const wxString& label) override
    {
        auto button = new TabButton(this, label);
        wxSizerFlags flags;

        if (n > 0)
            flags.Border(wxLEFT, MSW_OR_OTHER(PX(5) + PX(1) + PX(5) /* != PX(11) in some zoom levels! */,
                                              PX(3)));

        switch (m_style)
        {
            case SegmentStyle::SmallInline:
                button->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
                break;
            case SegmentStyle::LargeFullWidth:
                button->SetWindowVariant(wxWINDOW_VARIANT_LARGE);
                flags.Proportion(1);
                break;

            case SegmentStyle::SidebarPanels:
#ifdef __WXOSX__
                button->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
                break;
        }

        m_buttonsSizer->Add(button, flags);

        button->Bind(wxEVT_TOGGLEBUTTON, [=](wxCommandEvent& e)
        {
            if (e.IsChecked())
            {
                // SetSelection() generates events:
                m_book->SetSelection(n);
            }
            else
            {
                // don't un-toggle already toggled button / selection
                button->SetValue(true);
            }
        });
    }

    void RemovePage(size_t n) override
    {
        auto window = m_buttonsSizer->GetItem(n)->GetWindow();
        m_buttonsSizer->Remove((int)n);
        window->Destroy();
    }

    void RemoveAllPages() override
    {
        m_buttonsSizer->Clear(/*delete_windows=*/true);
    }

    void ChangeSelection(size_t n) override
    {
        for (size_t i = 0; i < m_buttonsSizer->GetItemCount(); ++i)
        {
            auto btn = static_cast<TabButton*>(m_buttonsSizer->GetItem(i)->GetWindow());
            btn->SetValue(i == n);
        }
    }

    void UpdateBackgroundColour() override
    {
        SetBackgroundColour(m_book->GetBackgroundColour());
    }

private:
    wxSimplebook *m_book;
    SegmentStyle m_style;
    wxSizer *m_wrappingSizer, *m_buttonsSizer;
};


SegmentedNotebook::SegmentedNotebook(wxWindow *parent, SegmentStyle style)
    : wxSimplebook(parent, wxID_ANY)
{
    switch (style)
    {
        case SegmentStyle::SmallInline:
        case SegmentStyle::LargeFullWidth:
#ifdef __WXOSX__
        {
            auto tabs = new SegmentedControlTabs(this, style);
            m_bookctrl = tabs;
            m_tabs = tabs;
            break;
        }
#endif
        case SegmentStyle::SidebarPanels:
        {
            auto tabs = new ButtonTabs(this, style);
            m_bookctrl = tabs;
            m_tabs = tabs;
            break;
        }
    }

    wxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    m_controlSizer = new wxBoxSizer(wxHORIZONTAL);
    m_controlSizer->Add(m_bookctrl, wxSizerFlags(1).Expand());
    switch (style)
    {
        case SegmentStyle::SmallInline:
            sizer->Add(m_controlSizer, wxSizerFlags().Left().Border(wxLEFT, 4));
            break;
        case SegmentStyle::LargeFullWidth:
        case SegmentStyle::SidebarPanels:
            sizer->Add(m_controlSizer, wxSizerFlags().Expand());
            break;
    }
    SetSizer(sizer);

    Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [=](wxBookCtrlEvent& e)
    {
        m_tabs->ChangeSelection(e.GetSelection());
        e.Skip();
    });
}

bool SegmentedNotebook::SetBackgroundColour(const wxColour& clr)
{
    if (!wxSimplebook::SetBackgroundColour(clr))
        return false;
    m_tabs->UpdateBackgroundColour();
    return true;
}

wxSizer *SegmentedNotebook::GetTabsExtensibleArea() const
{
    return m_tabs->GetExtensibleArea();
}

int SegmentedNotebook::ChangeSelection(size_t page)
{
    m_tabs->ChangeSelection(page);
    return wxSimplebook::ChangeSelection(page);
}

bool SegmentedNotebook::InsertPage(size_t n, wxWindow *page, const wxString& text, bool bSelect, int imageId)
{
    m_tabs->InsertPage(n, text);
    return wxSimplebook::InsertPage(n, page, text, bSelect, imageId);
}

wxWindow *SegmentedNotebook::DoRemovePage(size_t page)
{
    m_tabs->RemovePage(page);
    return wxSimplebook::DoRemovePage(page);
}

bool SegmentedNotebook::DeleteAllPages()
{
    m_tabs->RemoveAllPages();
    return wxSimplebook::DeleteAllPages();
}

int SegmentedNotebook::DoSetSelection(size_t n, int flags)
{
    // Is any page in the notebook currently focused?
    bool pageHadFocus = false;
    for (auto w = FindFocus(); w; w = w->GetParent())
    {
        if (w == this)
        {
            pageHadFocus = true;
            break;
        }
    }

    auto oldSel = wxSimplebook::DoSetSelection(n, flags);

    // If a page was focused, focus the newly shown page:
    if (oldSel != n && pageHadFocus)
        GetPage(n)->SetFocus();

    return oldSel;
}


#else // !HAS_SEGMENTED_NOTEBOOK

SegmentedNotebook::SegmentedNotebook(wxWindow *parent, SegmentStyle style)
    : wxNotebook(parent, -1, wxDefaultPosition, wxDefaultSize, wxNB_NOPAGETHEME)
{
    wxFont font = GetFont();
    double size = font.GetFractionalPointSize();

    switch (style)
    {
        case SegmentStyle::SmallInline:
            size /= 1.2;
            break;
        case SegmentStyle::LargeFullWidth:
            size *= 1.2;
            break;
        case SegmentStyle::SidebarPanels:
            // do nothing
            break;
    }

    font.SetFractionalPointSize(size);
    SetOwnFont(font);
}

#endif // !HAS_SEGMENTED_NOTEBOOK
