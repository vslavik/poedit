/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2015 Vaclav Slavik
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

#include "customcontrols.h"

#include "concurrency.h"
#include "errors.h"
#include "hidpi.h"

#include <wx/app.h>
#include <wx/clipbrd.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/weakref.h>
#include <wx/wupdlock.h>

#if wxCHECK_VERSION(3,1,0)
    #include <wx/activityindicator.h>
#else
    #include "wx_backports/activityindicator.h"
#endif

#include <unicode/brkiter.h>
#ifdef __WXGTK__
#include <gtk/gtk.h>
#endif

#include "str_helpers.h"

#include <memory>

namespace
{

wxString WrapTextAtWidth(const wxString& text_, int width, wxWindow *wnd)
{
    if (text_.empty())
        return text_;
    auto text = str::to_icu(text_);

    static std::unique_ptr<icu::BreakIterator> iter;
    if (!iter)
    {
        UErrorCode err = U_ZERO_ERROR;
        iter.reset(icu::BreakIterator::createLineInstance(icu::Locale(), err));
    }

    iter->setText(text);

    wxString out;
    out.reserve(text_.length() + 10);

    int32_t lineStart = 0;
    wxString previousSubstr;

    for (int32_t pos = iter->next(); pos != icu::BreakIterator::DONE; pos = iter->next())
    {
        auto substr = str::to_wx(text.tempSubStringBetween(lineStart, pos));

        if (wnd->GetTextExtent(substr).x > width)
        {
            auto previousPos = iter->previous();
            if (previousPos == lineStart || previousPos == icu::BreakIterator::DONE)
            {
                // line is too large but we can't break it, so have no choice but not to wrap
                out += substr;
                lineStart = pos;
            }
            else
            {
                // need to wrap at previous linebreak position
                out += previousSubstr;
                lineStart = previousPos;
            }
            out += '\n';
            previousSubstr.clear();
        }
        else if (pos > 0 && text[pos-1] == '\n') // forced line feed
        {
            out += substr;
            lineStart = pos;
            previousSubstr.clear();
        }
        else
        {
            previousSubstr = substr;
        }
    }

    if (!previousSubstr.empty())
    {
        out += previousSubstr;
    }

    if (out.Last() == '\n')
        out.RemoveLast();

    return out;
}


} // anonymous namespace


AutoWrappingText::AutoWrappingText(wxWindow *parent, const wxString& label)
    : wxStaticText(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE),
      m_text(label),
      m_wrapWidth(-1)
{
    m_text.Replace("\n", " ");

    SetInitialSize(wxSize(10,10));
    Bind(wxEVT_SIZE, &ExplanationLabel::OnSize, this);
}

void AutoWrappingText::SetAlignment(int align)
{
    if (GetWindowStyleFlag() & align)
        return;
    SetWindowStyleFlag(wxST_NO_AUTORESIZE | align);
}

void AutoWrappingText::SetAndWrapLabel(const wxString& label)
{
    wxWindowUpdateLocker lock(this);
    m_text = label;
    m_wrapWidth = GetSize().x;
    SetLabelText(WrapTextAtWidth(label, m_wrapWidth, this));

    InvalidateBestSize();
    SetMinSize(wxDefaultSize);
    SetMinSize(GetBestSize());
}

void AutoWrappingText::OnSize(wxSizeEvent& e)
{
    e.Skip();
    int w = wxMax(0, e.GetSize().x - PX(4));
    if (w == m_wrapWidth)
        return;

    wxWindowUpdateLocker lock(this);

    m_wrapWidth = w;
    SetLabel(WrapTextAtWidth(m_text, w, this));

    InvalidateBestSize();
    SetMinSize(wxDefaultSize);
    SetMinSize(GetBestSize());
}


SelectableAutoWrappingText::SelectableAutoWrappingText(wxWindow *parent, const wxString& label)
    : AutoWrappingText(parent, label)
{
#if defined(__WXOSX__)
    NSTextField *view = (NSTextField*)GetHandle();
    [view setSelectable:YES];
#elif defined(__WXGTK__)
    GtkLabel *view = GTK_LABEL(GetHandle());
    gtk_label_set_selectable(view, TRUE);
#else
    // at least allow copying
    static wxWindowID idCopy = wxNewId();
    Bind(wxEVT_CONTEXT_MENU, [=](wxContextMenuEvent&){
        wxMenu *menu = new wxMenu();
        menu->Append(idCopy, _("&Copy"));
        PopupMenu(menu);
    });
    Bind(wxEVT_MENU, [=](wxCommandEvent&){
        wxClipboardLocker lock;
        wxClipboard::Get()->SetData(new wxTextDataObject(m_text));
    }, idCopy);
#endif
}


ExplanationLabel::ExplanationLabel(wxWindow *parent, const wxString& label)
    : AutoWrappingText(parent, label)
{
#if defined(__WXOSX__) || defined(__WXGTK__)
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
#ifndef __WXGTK__
    SetForegroundColour(GetTextColor());
#endif
}

wxColour ExplanationLabel::GetTextColor()
{
#if defined(__WXOSX__)
    return wxColour("#777777");
#elif defined(__WXGTK__)
    return wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
#else
    return wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
#endif
}

SecondaryLabel::SecondaryLabel(wxWindow *parent, const wxString& label)
    : wxStaticText(parent, wxID_ANY, label)
{
#if defined(__WXOSX__) || defined(__WXGTK__)
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
#ifndef __WXGTK__
    SetForegroundColour(GetTextColor());
#endif
}


LearnMoreLink::LearnMoreLink(wxWindow *parent, const wxString& url, wxString label, wxWindowID winid)
{
    if (label.empty())
    {
#ifdef __WXMSW__
        label = _("Learn more");
#else
        label = _("Learn More");
#endif
    }

    wxHyperlinkCtrl::Create(parent, winid, label, url);
    SetNormalColour("#2F79BE");
    SetVisitedColour("#2F79BE");
    SetHoverColour("#3D8DD5");

#ifdef __WXOSX__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    SetFont(GetFont().Underlined());
#endif
}


wxObject *LearnMoreLinkXmlHandler::DoCreateResource()
{
    auto w = new LearnMoreLink(m_parentAsWindow, GetText("url"), GetText("label"), GetID());
    w->SetName(GetName());
    SetupWindow(w);
    return w;
}

bool LearnMoreLinkXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, "LearnMoreLink");
}


ActivityIndicator::ActivityIndicator(wxWindow *parent)
    : wxWindow(parent, wxID_ANY), m_running(false)
{
    auto sizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(sizer);

    m_spinner = new wxActivityIndicator(this, wxID_ANY);
    m_spinner->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    m_label = new wxStaticText(this, wxID_ANY, "");
#ifdef __WXOSX__
    m_label->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    sizer->Add(m_spinner, wxSizerFlags().Center().Border(wxRIGHT, PX(4)));
    sizer->Add(m_label, wxSizerFlags(1).Center());

    HandleError = on_main_thread_for_window<std::exception_ptr>(this, [=](std::exception_ptr e){
        StopWithError(DescribeException(e));
    });
}

void ActivityIndicator::Start(const wxString& msg)
{
    m_running = true;

    m_label->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    m_label->SetLabel(msg);

    auto sizer = GetSizer();
    sizer->Show(m_spinner);
    sizer->Show(m_label, !msg.empty());
    Layout();

    m_spinner->Start();
}

void ActivityIndicator::Stop()
{
    m_running = false;

    m_spinner->Stop();
    m_label->SetLabel("");

    auto sizer = GetSizer();
    sizer->Hide(m_spinner);
    sizer->Hide(m_label);
    Layout();
}

void ActivityIndicator::StopWithError(const wxString& msg)
{
    m_running = false;

    m_spinner->Stop();
    m_label->SetForegroundColour(*wxRED);
    m_label->SetLabel(msg);

    auto sizer = GetSizer();
    sizer->Hide(m_spinner);
    sizer->Show(m_label);
    Layout();
}
