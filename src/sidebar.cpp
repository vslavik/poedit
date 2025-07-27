/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2025 Vaclav Slavik
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

#include "sidebar.h"

#include "catalog.h"
#include "customcontrols.h"
#include "colorscheme.h"
#include "commentdlg.h"
#include "concurrency.h"
#include "configuration.h"
#include "errors.h"
#include "hidpi.h"
#include "static_ids.h"
#include "utility.h"
#include "unicode_helpers.h"

#include "tm/suggestions.h"
#include "tm/transmem.h"

#include <wx/app.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/time.h>
#include <wx/wupdlock.h>

#include <algorithm>


class SidebarSeparator : public wxWindow
{
public:
    SidebarSeparator(wxWindow *parent) : wxWindow(parent, wxID_ANY)
    {
        Bind(wxEVT_PAINT, &SidebarSeparator::OnPaint, this);
    }

    wxSize DoGetBestSize() const override
    {
        return wxSize(-1, PX(1));
    }

    bool AcceptsFocus() const override { return false; }

private:
    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        auto clr = ColorScheme::Get(Color::SidebarBlockSeparator);
        dc.SetBrush(clr);
        dc.SetPen(clr);
        dc.DrawRectangle(PX(2), 0, dc.GetSize().x - PX(4), PX(1) + 1);
    }
};


SidebarBlock::SidebarBlock(Sidebar *parent, const wxString& label, int flags)
{
    m_parent = parent;
    m_sizer = new wxBoxSizer(wxVERTICAL);
    if (!(flags & NoUpperMargin))
        m_sizer->AddSpacer(PX(15));
    if (!label.empty())
    {
        if (!(flags & NoUpperMargin))
        {
            m_sizer->Add(new SidebarSeparator(parent),
                         wxSizerFlags().Expand().Border(wxBOTTOM|wxLEFT|wxRIGHT, PX(5)));
        }
        m_headerSizer = new wxBoxSizer(wxHORIZONTAL);
        m_headerSizer->Add(new HeadingLabel(parent, label), wxSizerFlags().Center());
        m_sizer->Add(m_headerSizer, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, SIDEBAR_PADDING));
    }
    m_innerSizer = new wxBoxSizer(wxVERTICAL);

    auto innerFlags = wxSizerFlags(1).Expand();
    if (!(flags & NoSideMargins))
        innerFlags.Border(wxLEFT|wxRIGHT, SIDEBAR_PADDING);
    m_sizer->Add(m_innerSizer, innerFlags);
}

void SidebarBlock::Show(bool show)
{
    m_sizer->ShowItems(show);
}

void SidebarBlock::SetItem(const CatalogItemPtr& item)
{
    if (!item)
    {
        Show(false);
        return;
    }

    bool use = ShouldShowForItem(item);
    if (use)
        Update(item);
    Show(use);
}


class OldMsgidSidebarBlock : public SidebarBlock
{
public:
    OldMsgidSidebarBlock(Sidebar *parent)
          /// TRANSLATORS: "Previous" as in used in the past, now replaced with newer.
        : SidebarBlock(parent, _("Previous source text"))
    {
        m_innerSizer->AddSpacer(PX(2));
        m_innerSizer->Add(new ExplanationLabel(parent, _("The old source text (before it changed during an update) that the now-inaccurate translation corresponds to.")),
                     wxSizerFlags().Expand());
        m_innerSizer->AddSpacer(PX(5));
        m_text = new SelectableAutoWrappingText(parent, WinID::PreviousSourceText, "");
        m_innerSizer->Add(m_text, wxSizerFlags().Expand());
    }

    bool ShouldShowForItem(const CatalogItemPtr& item) const override
    {
        return item->HasOldMsgid();
    }

    void Update(const CatalogItemPtr& item) override
    {
        m_text->SetAndWrapLabel(item->GetOldMsgid());
    }

private:
    SelectableAutoWrappingText *m_text;
};


class ExtractedCommentSidebarBlock : public SidebarBlock
{
public:
    ExtractedCommentSidebarBlock(Sidebar *parent)
        : SidebarBlock(parent, _("Notes for translators"))
    {
        m_innerSizer->AddSpacer(PX(5));
        m_comment = new SelectableAutoWrappingText(parent, WinID::NotesForTranslator, "");
        m_innerSizer->Add(m_comment, wxSizerFlags().Expand());
    }

    bool ShouldShowForItem(const CatalogItemPtr& item) const override
    {
        return item->HasExtractedComments();
    }

    void Update(const CatalogItemPtr& item) override
    {
        auto comment = wxJoin(item->GetExtractedComments(), '\n', '\0');
        if (comment.starts_with("TRANSLATORS:") || comment.starts_with("translators:"))
        {
            comment.Remove(0, 12);
            if (!comment.empty() && comment[0] == ' ')
                comment.Remove(0, 1);
        }
        m_comment->SetAndWrapLabel(comment);
    }

private:
    SelectableAutoWrappingText *m_comment;
};


class CommentSidebarBlock : public SidebarBlock
{
public:
    CommentSidebarBlock(Sidebar *parent)
        : SidebarBlock(parent, _("Comment"))
    {
        m_innerSizer->AddSpacer(PX(5));
        m_comment = new SelectableAutoWrappingText(parent, WinID::TranslatorComment, "");
        m_innerSizer->Add(m_comment, wxSizerFlags().Expand());
    }

    bool ShouldShowForItem(const CatalogItemPtr& item) const override
    {
        return item->HasComment();
    }

    void Update(const CatalogItemPtr& item) override
    {
        auto text = CommentDialog::RemoveStartHash(item->GetComment());
        text.Trim();
        m_comment->SetAndWrapLabel(text);
    }

private:
    SelectableAutoWrappingText *m_comment;
};


class AddCommentSidebarBlock : public SidebarBlock
{
public:
    AddCommentSidebarBlock(Sidebar *parent) : SidebarBlock(parent, "")
    {
    #ifdef __WXMSW__
        auto label = _("Add comment");
    #else
        auto label = _("Add Comment");
    #endif
        m_btn = new wxButton(parent, XRCID("menu_comment"), _("Add Comment"));
        m_innerSizer->AddStretchSpacer();
        m_innerSizer->Add(m_btn, wxSizerFlags().Right());
    }

    bool IsGrowable() const override { return true; }

    bool ShouldShowForItem(const CatalogItemPtr&) const override
    {
        return m_parent->FileHasCapability(Catalog::Cap::UserComments);
    }

    void Update(const CatalogItemPtr& item) override
    {
    #ifdef __WXMSW__
        auto add = _("Add comment");
        auto edit = _("Edit comment");
    #else
        auto add = _("Add Comment");
        auto edit = _("Edit Comment");
    #endif
        m_btn->SetLabel(item->HasComment() ? edit : add);
    }

private:
    wxButton *m_btn;
};


wxDEFINE_EVENT(EVT_SUGGESTION_SELECTED, wxCommandEvent);

class SuggestionWidget : public wxWindow
{
public:
    SuggestionWidget(Sidebar *sidebar, wxWindow *parent, SuggestionsSidebarBlock *block, bool isFirst) : wxWindow(parent, wxID_ANY)
    {
        m_sidebar = sidebar;
        m_parentBlock = block;
        m_isHighlighted = false;
        m_icon = new StaticBitmap(this, "SuggestionTMTemplate");
        m_text = new AutoWrappingText(this, wxID_ANY, "TEXT");
        m_info = new InfoStaticText(this);
        m_moreActions = new ImageButton(this, "DownvoteTemplate");

        m_isPerfect = isFirst
                      ? new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap("SuggestionPerfectMatch"))
                      : nullptr;

        // Calculate the correct DPI-dependent offset of m_icon vs m_text - we want the icon centered
        // on the first line of text.
        const int textPadding = PX(6);
#if defined(__WXOSX__)
        const int iconPadding = PX(7);
#elif defined(__WXMSW__)
        int iconPadding = 0;
        const auto hidpiFactor = HiDPIScalingFactor();
        if (hidpiFactor < 1.25)
            iconPadding = PX(7);
        else if (hidpiFactor < 1.5)
            iconPadding = PX(9)+1;
        else if (hidpiFactor < 1.75)
            iconPadding = PX(8)+1;
        else if (hidpiFactor < 2.0)
            iconPadding = PX(10);
        else
            iconPadding = PX(8)+1;
#else
        const int iconPadding = PX(7);
#endif

        auto top = new wxBoxSizer(wxHORIZONTAL);
        auto right = new wxBoxSizer(wxVERTICAL);
        top->AddSpacer(PX(6));
        top->Add(m_icon, wxSizerFlags().Top().Border(wxTOP, iconPadding));
        top->Add(right, wxSizerFlags(1).Expand().Border(wxLEFT, PX(8)));
        right->Add(m_text, wxSizerFlags().Expand().Border(wxTOP, textPadding));
        auto infoSizer = new wxBoxSizer(wxHORIZONTAL);
        infoSizer->Add(m_info, wxSizerFlags().Center());
        if (m_isPerfect)
            infoSizer->Add(m_isPerfect, wxSizerFlags().Center().Border(wxLEFT, PX(2)));
        right->Add(infoSizer, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(2)));

        infoSizer->AddStretchSpacer();
        infoSizer->Add(m_moreActions, wxSizerFlags().ReserveSpaceEvenIfHidden().CenterVertical().Border(wxRIGHT, MSW_OR_OTHER(PX(4), PX(2))));
        m_moreActions->Hide();

        SetSizerAndFit(top);

        ColorScheme::SetupWindowColors(this, [=]
        {
            // setup mouse hover highlighting:
            auto bg = parent->GetBackgroundColour();
            SetBackgroundColour(bg);
#ifndef __WXOSX__
            m_bg = bg;
            m_bgHighlight = ColorScheme::GetWindowMode(parent) == ColorScheme::Dark
                            ? m_bg.ChangeLightness(110)
                            : m_bg.ChangeLightness(95);
            for (auto c: GetChildren())
                c->SetBackgroundColour(m_isHighlighted ? m_bgHighlight : m_bg);
#endif
        });

        wxWindow* parts [] = { this, m_icon, m_text, m_info, m_moreActions };
        for (auto w : parts)
        {
            w->Bind(wxEVT_MOTION,       &SuggestionWidget::OnMouseMove, this);
            w->Bind(wxEVT_LEAVE_WINDOW, &SuggestionWidget::OnMouseMove, this);
            if (w != m_moreActions)
                w->Bind(wxEVT_LEFT_UP,  &SuggestionWidget::OnMouseClick, this);
            w->Bind(wxEVT_CONTEXT_MENU, &SuggestionWidget::OnMoreActions, this);
        }
        m_moreActions->Bind(wxEVT_BUTTON, &SuggestionWidget::OnMoreActions, this);
        Bind(wxEVT_PAINT, &SuggestionWidget::OnPaint, this);
    }

    void SetValue(int index, const Suggestion& s, Language lang, const wxString& icon, const wxString& tooltip)
    {
        m_value = s;

        int percent = int(100 * s.score);
        auto percentStr = wxString::Format("%d%%", percent);

        index++;
        if (index < 10)
        {
        #ifdef __WXOSX__
            auto shortcut = wxString::Format(L"⌘%d", index);
        #else
            // TRANSLATORS: This is the key shortcut used in menus on Windows, some languages call them differently
            auto shortcut = wxString::Format("%s%d", wxGETTEXT_IN_CONTEXT("keyboard key", "Ctrl+"), index);
        #endif
            m_info->SetLabel(wxString::Format(L"%s • %s", shortcut, percentStr));
        }
        else
        {
            m_info->SetLabel(percentStr);
        }

        m_icon->SetBitmapName(icon);

        if (m_isPerfect)
            m_isPerfect->GetContainingSizer()->Show(m_isPerfect, percent == 100);

        auto text = bidi::mark_direction(s.text, lang);

        m_text->SetLanguage(lang);
        m_text->SetAndWrapLabel(text);

#ifndef __WXOSX__
        // FIXME: Causes weird issues on macOS: tooltips appearing on the main list control,
        //        over toolbar, where the mouse just was etc.
        m_icon->SetToolTip(tooltip);
        m_text->SetToolTip(tooltip);
#endif
        (void)tooltip;

#ifndef __WXOSX__
        SetBackgroundColour(m_bg);
#endif

        Layout();
        InvalidateBestSize();
        SetMinSize(wxDefaultSize);
        SetMinSize(GetBestSize());
    }

    bool AcceptsFocus() const override { return false; }

private:
    class InfoStaticText : public wxStaticText
    {
    public:
        InfoStaticText(wxWindow *parent) : wxStaticText(parent, wxID_ANY, "100%")
        {
        #ifdef __WXMSW__
            SetFont(SmallerFont(GetFont()));
        #else
            SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        #endif
            ColorScheme::SetupWindowColors(this, [=]
            {
                SetForegroundColour(ExplanationLabel::GetTextColor());
            });
        }

        void DoEnable(bool) override {} // wxOSX's disabling would break color
    };

    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        if (m_isHighlighted)
        {
#ifdef __WXOSX__
            auto winbg = GetBackgroundColour();
            NSColor *bg = winbg.OSXGetNSColor();
            NSColor *osHighlight = [bg colorWithSystemEffect:NSColorSystemEffectRollover];
            // use only lighter version of the highlight by blending with the background
            auto highlight = wxColour([bg blendedColorWithFraction:0.2 ofColor:osHighlight]);
#else
            auto highlight = m_bgHighlight;
#endif
            std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
            gc->SetBrush(highlight);
            gc->SetPen(*wxTRANSPARENT_PEN);

            auto rect = GetClientRect();
            if (!rect.IsEmpty())
            {
#if defined(__WXOSX__)
                gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, PX(5));
#else
                gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, PX(2));
#endif
            }
        }
    }

    void OnMouseMove(wxMouseEvent& e)
    {
        auto rectWin = GetClientRect();
        rectWin.Deflate(1); // work around off-by-one issue on macOS
        auto evtWin = static_cast<wxWindow*>(e.GetEventObject());
        auto mpos = e.GetPosition();
        if (evtWin != this)
            mpos += evtWin->GetPosition();
        Highlight(rectWin.Contains(mpos));
    }

    void OnMouseClick(wxMouseEvent&)
    {
        wxCommandEvent event(EVT_SUGGESTION_SELECTED);
        event.SetEventObject(this);
        event.SetString(m_value.text);
        ProcessWindowEvent(event);
    }

    void OnMoreActions(wxCommandEvent& e)
    {
        if (!ShouldShowActions())
        {
            e.Skip();
            return;
        }

        auto sidebar = m_sidebar;
        auto suggestion = m_value;
        static wxWindowIDRef idDelete = NewControlId();

        wxMenu menu;
#ifdef __WXOSX__
        [menu.GetHMenu() setFont:[NSFont systemFontOfSize:13]];
#endif
        menu.Append(idDelete, MSW_OR_OTHER(_("Delete from translation memory"), _("Delete From Translation Memory")));
        menu.Bind(wxEVT_MENU, [sidebar,suggestion](wxCommandEvent&)
        {
            SuggestionsProvider::Delete(suggestion);
            sidebar->RefreshContent();
        }, idDelete);

        PopupMenu(&menu);
    }

    void Highlight(bool highlight)
    {
        m_isHighlighted = highlight;
#ifndef __WXOSX__
        for (auto c: GetChildren())
            c->SetBackgroundColour(highlight ? m_bgHighlight : m_bg);
#endif
        m_moreActions->Show(highlight && ShouldShowActions());
        Refresh();

        if (highlight)
        {
            for (auto widget: m_parentBlock->m_suggestionsWidgets)
            {
                if (widget != this)
                    widget->Highlight(false);
            }
        }
    }

    bool ShouldShowActions() const
    {
        return m_isHighlighted && !m_value.id.empty();
    }

    Sidebar *m_sidebar;
    SuggestionsSidebarBlock *m_parentBlock;
    Suggestion m_value;
    bool m_isHighlighted;
    StaticBitmap *m_icon;
    AutoWrappingText *m_text;
    wxStaticText *m_info;
    wxStaticBitmap *m_isPerfect;
    ImageButton *m_moreActions;
#ifndef __WXOSX__
    wxColour m_bg, m_bgHighlight;
#endif
};


SuggestionsSidebarBlock::SuggestionsSidebarBlock(Sidebar *parent, wxMenu *menu)
    : SidebarBlock(parent,
                   // TRANSLATORS: as in: translation suggestions, suggested translations; should be similarly short
                   _("Suggestions"),
                   #if 0
                   _("Translation suggestions"),
                   #endif
                   NoUpperMargin | NoSideMargins),
      m_suggestionsMenu(menu),
      m_msgPresent(false),
      m_suggestionsSeparator(nullptr),
      m_pendingQueries(0),
      m_latestQueryId(0),
      m_lastUpdateTime(0)
{
    m_provider.reset(new SuggestionsProvider);
}

void SuggestionsSidebarBlock::InitMainPanel()
{
    m_suggestionsPanel = new wxPanel(m_parent, wxID_ANY);
    m_panelSizer = new wxBoxSizer(wxVERTICAL);
    m_suggestionsPanel->SetSizer(m_panelSizer);

    m_innerSizer->Add(m_suggestionsPanel, wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT, SIDEBAR_PADDING));
}

void SuggestionsSidebarBlock::InitControls()
{
    InitMainPanel();

    m_msgSizer = new wxBoxSizer(wxHORIZONTAL);
    m_msgIcon = new StaticBitmap(m_suggestionsPanel, wxString());
    m_msgText = new ExplanationLabel(m_suggestionsPanel, "");
    m_msgSizer->Add(m_msgIcon, wxSizerFlags().Center().PXBorderAll());
    m_msgSizer->Add(m_msgText, wxSizerFlags(1).Center().PXBorder(wxTOP|wxBOTTOM));
    m_panelSizer->Add(m_msgSizer, wxSizerFlags().Expand().Border(wxBOTTOM, PX(10)));

    m_suggestionsSizer = new wxBoxSizer(wxVERTICAL);
    m_extrasSizer = new wxBoxSizer(wxVERTICAL);
    m_panelSizer->Add(m_suggestionsSizer, wxSizerFlags().Expand());
    m_panelSizer->Add(m_extrasSizer, wxSizerFlags().Expand());

    m_iGotNothing = new wxStaticText(m_suggestionsPanel, wxID_ANY,
                                #ifdef __WXMSW__
                                     // TRANSLATORS: This is shown when no translation suggestions can be found in the TM (Windows).
                                     _("No matches found")
                                #else
                                     // TRANSLATORS: This is shown when no translation suggestions can be found in the TM (macOS, Linux).
                                     _("No Matches Found")
                                #endif
                                     );
    m_iGotNothing->SetWindowVariant(wxWINDOW_VARIANT_NORMAL);
#ifdef __WXMSW__
    m_iGotNothing->SetFont(m_iGotNothing->GetFont().Larger());
#endif
    ColorScheme::SetupWindowColors(m_iGotNothing, [=]
    {
        m_suggestionsPanel->SetBackgroundColour(m_parent->GetBackgroundColour());
        m_iGotNothing->SetForegroundColour(ExplanationLabel::GetTextColor().ChangeLightness(150));
    });
    m_panelSizer->Add(m_iGotNothing, wxSizerFlags().Center().Border(wxTOP|wxBOTTOM, PX(100)));

    BuildSuggestionsMenu();

    m_suggestionsTimer.SetOwner(m_parent);
    m_parent->Bind(wxEVT_TIMER,
                 &SuggestionsSidebarBlock::OnDelayedShowSuggestionsForItem, this,
                 m_suggestionsTimer.GetId());
}

SuggestionsSidebarBlock::~SuggestionsSidebarBlock()
{
    if (m_suggestionsMenu)
    {
        ClearSuggestionsMenu();
        for (auto i : m_suggestionsMenuItems)
            delete i;
    }
    // else: m_suggestionsMenuItems are already deleted
}

wxString SuggestionsSidebarBlock::GetIconForSuggestion(const Suggestion&) const
{
    return "SuggestionTMTemplate";
}

wxString SuggestionsSidebarBlock::GetTooltipForSuggestion(const Suggestion&) const
{
    return _(L"This string was found in Poedit’s translation memory.");
}

void SuggestionsSidebarBlock::ClearMessage()
{
    m_msgPresent = false;
    m_msgText->SetAndWrapLabel("");
    UpdateVisibility();
    m_suggestionsPanel->Layout();
    m_panelSizer->Layout();
}

void SuggestionsSidebarBlock::SetMessage(const wxString& icon, const wxString& text)
{
    m_msgPresent = true;
    m_msgIcon->SetBitmapName(icon);
    m_msgText->SetAndWrapLabel(text);
    UpdateVisibility();
    m_suggestionsPanel->Layout();
    m_panelSizer->Layout();
}

void SuggestionsSidebarBlock::ReportError(SuggestionsBackend*, dispatch::exception_ptr e)
{
    SetMessage("SuggestionErrorTemplate", DescribeException(e));
}

void SuggestionsSidebarBlock::ClearSuggestions()
{
    m_suggestions.clear();
    UpdateSuggestionsMenu();
    UpdateVisibility();
}

void SuggestionsSidebarBlock::UpdateSuggestions(const SuggestionsList& hits)
{
    wxWindowUpdateLocker lock(m_suggestionsPanel);

    for (auto& h: hits)
    {
        // empty entries screw up menus (treated as stock items), don't use them:
        if (!h.text.empty())
            m_suggestions.push_back(h);
    }

    std::stable_sort(m_suggestions.begin(), m_suggestions.end());

    // create any necessary controls:
    while (m_suggestions.size() > m_suggestionsWidgets.size())
    {
        auto w = new SuggestionWidget(m_parent, m_suggestionsPanel, this, /*isFirst=*/m_suggestionsWidgets.empty());
        m_suggestionsSizer->Add(w, wxSizerFlags().Expand());
        m_suggestionsWidgets.push_back(w);
    }
    m_panelSizer->Layout();

    // update shown suggestions:

    if (m_suggestionsSeparator)
    {
        m_suggestionsSeparator->Hide();
        m_suggestionsSizer->Detach(m_suggestionsSeparator);
    }

    auto lang = m_parent->GetCurrentLanguage();
    int perfectMatches = 0;
    for (size_t i = 0; i < m_suggestions.size(); ++i)
    {
        auto s = m_suggestions[i];
        m_suggestionsWidgets[i]->SetValue((int)i, s, lang, GetIconForSuggestion(s), GetTooltipForSuggestion(s));

        if (s.IsExactMatch())
        {
            perfectMatches++;
        }
        else
        {
            if (perfectMatches > 1)
            {
                if (!m_suggestionsSeparator)
                    m_suggestionsSeparator = new SidebarSeparator(m_suggestionsPanel);
                m_suggestionsSeparator->Show();
                m_suggestionsSizer->Insert(i, m_suggestionsSeparator, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, MSW_OR_OTHER(PX(2), PX(4))));
            }
            perfectMatches = 0;
        }
    }

    m_panelSizer->Layout();
    UpdateVisibility();
    m_suggestionsPanel->Layout();

    UpdateSuggestionsMenu();
}

void SuggestionsSidebarBlock::BuildSuggestionsMenu(int count)
{
    m_suggestionsMenuItems.reserve(SUGGESTIONS_MENU_ENTRIES);
    auto menu = m_suggestionsMenu;
    if (!menu)
        return;

    for (int i = 0; i < count; i++)
    {
        auto text = wxString::Format("(empty)\t%s%d", wxGETTEXT_IN_CONTEXT("keyboard key", "Ctrl+"), i+1);
        auto item = new wxMenuItem(menu, wxID_ANY, text);
        item->SetBitmap(wxArtProvider::GetBitmap("SuggestionTMTemplate"));

        m_suggestionsMenuItems.push_back(item);
        menu->Append(item);

        m_suggestionsMenu->Bind(wxEVT_MENU, [this,i,menu](wxCommandEvent&){
            if (i >= (int)m_suggestions.size())
                return;
            wxCommandEvent event(EVT_SUGGESTION_SELECTED);
            event.SetEventObject(menu);
            event.SetString(m_suggestions[i].text);
            menu->GetWindow()->ProcessWindowEvent(event);
        }, item->GetId());
    }
}

void SuggestionsSidebarBlock::UpdateSuggestionsMenu()
{
    ClearSuggestionsMenu();

    bool isRTL = m_parent->GetCurrentLanguage().IsRTL();
    wxString formatMask;
    if (isRTL)
        formatMask = L"\u202b%s\u202c\t" + wxGETTEXT_IN_CONTEXT("keyboard key", "Ctrl+") + "%d";
    else
        formatMask = L"\u202a%s\u202c\t" + wxGETTEXT_IN_CONTEXT("keyboard key", "Ctrl+") + "%d";

    int index = 0;
    for (auto s: m_suggestions)
    {
        if (index >= SUGGESTIONS_MENU_ENTRIES)
            break;

        wxString text = s.text;
        text.Replace("\t", " ");
        text.Replace("\n", " ");
        if (text.length() > 100)
            text = text.substr(0, 100) + L"…";

        auto item = m_suggestionsMenuItems[index];
        m_suggestionsMenu->Append(item);

        auto label = wxControl::EscapeMnemonics(wxString::Format(formatMask, text, index+1));
        item->SetItemLabel(label);
        item->SetBitmap(wxArtProvider::GetBitmap(GetIconForSuggestion(s)));

        index++;
    }
}

void SuggestionsSidebarBlock::ClearSuggestionsMenu()
{
    auto m = m_suggestionsMenu;
    if (!m)
        return;

    auto menuItems = m->GetMenuItems();
    for (auto i: menuItems)
    {
        if (std::find(m_suggestionsMenuItems.begin(), m_suggestionsMenuItems.end(), i) != m_suggestionsMenuItems.end())
            m->Remove(i);
    }
}


void SuggestionsSidebarBlock::OnQueriesFinished()
{
    if (m_suggestions.empty())
    {
        m_panelSizer->Show(m_iGotNothing);
        m_suggestionsPanel->Layout();
    }
}

void SuggestionsSidebarBlock::UpdateVisibility()
{
    m_msgSizer->ShowItems(m_msgPresent);
    m_panelSizer->Show(m_iGotNothing, m_suggestions.empty() && !m_pendingQueries);

    int heightRemaining = m_panelSizer->GetSize().y;
    size_t w = 0;
    for (w = 0; w < m_suggestions.size(); w++)
    {
        heightRemaining -= m_suggestionsWidgets[w]->GetSize().y;
        // don't show suggestions that don't fit in the space, but always try to show at least 2
        if (heightRemaining < 20 && w > 2)
            break;
        m_suggestionsSizer->Show(m_suggestionsWidgets[w]);
    }

    for (; w < m_suggestionsWidgets.size(); w++)
        m_suggestionsSizer->Hide(m_suggestionsWidgets[w]);

}

void SuggestionsSidebarBlock::Show(bool show)
{
    SidebarBlock::Show(show);
    if (show)
    {
        UpdateVisibility();
    }
    else
    {
        ClearSuggestionsMenu();
    }
}

bool SuggestionsSidebarBlock::ShouldShowForItem(const CatalogItemPtr&) const
{
    return m_parent->FileHasCapability(Catalog::Cap::Translations) &&
           Config::UseTM();
}

void SuggestionsSidebarBlock::Update(const CatalogItemPtr& item)
{
    ClearMessage();
    ClearSuggestions();

    UpdateSuggestionsForItem(item);
}

void SuggestionsSidebarBlock::UpdateSuggestionsForItem(CatalogItemPtr item)
{
    if (!item)
        return;

    long long now = wxGetUTCTimeMillis().GetValue();
    long long delta = now - m_lastUpdateTime;
    m_lastUpdateTime = now;

    if (delta < 100)
    {
        // User is probably holding arrow down and going through the list as crazy
        // and not really caring for the suggestions. Throttle them a bit and call
        // this code after a small delay. Notice that this may repeat itself several
        // times, only continuing through to show suggestions after the dust settled
        // and the user didn't change the selection for a few milliseconds.
        if (!m_suggestionsTimer.IsRunning())
            m_suggestionsTimer.StartOnce(110);
        return;
    }

    m_pendingQueries = 0;

    // FIXME: Get catalog info from `item` once present there
    if (m_parent->GetCatalog()->UsesSymbolicIDsForSource())
    {
        SetMessage("SuggestionErrorTemplate", _(L"Translation suggestions require that source text is available. They don’t work if only IDs without the actual text are used."));
        return;
    }
    else if (!m_parent->GetCatalog()->GetSourceLanguage().IsValid())
    {
        SetMessage("SuggestionErrorTemplate", _(L"Translation suggestions require that source text’s language is known. Poedit couldn’t detect it in this file."));
        return;
    }

    auto srclang = m_parent->GetCurrentSourceLanguage();
    auto lang = m_parent->GetCurrentLanguage();
    if (!srclang.IsValid() || !lang.IsValid() || srclang == lang)
    {
        OnQueriesFinished();
        return;
    }

    QueryAllProviders(item);
}

void SuggestionsSidebarBlock::OnDelayedShowSuggestionsForItem(wxTimerEvent&)
{
    UpdateSuggestionsForItem(m_parent->GetSelectedItem());
}

void SuggestionsSidebarBlock::QueryAllProviders(const CatalogItemPtr& item)
{
    auto thisQueryId = ++m_latestQueryId;

    // At this point, we know we're not interested in any older results, but some might have
    // arrived asynchronously in between ClearSuggestions() call and now. So make sure there
    // are no old suggestions present right after increasing the query ID:
    m_suggestions.clear();

    QueryProvider(TranslationMemory::Get(), item, thisQueryId);
}

void SuggestionsSidebarBlock::QueryProvider(SuggestionsBackend& backend, const CatalogItemPtr& item, uint64_t queryId)
{
    m_pendingQueries++;

    // we need something to talk to GUI thread through that is guaranteed
    // to exist, and the app object is a good choice:
    auto backendPtr = &backend;
    std::weak_ptr<SuggestionsSidebarBlock> weakSelf = std::dynamic_pointer_cast<SuggestionsSidebarBlock>(shared_from_this());

    SuggestionQuery query {
        m_parent->GetCurrentSourceLanguage(),
        m_parent->GetCurrentLanguage(),
        item->GetString().ToStdWstring()
    };

    m_provider->SuggestTranslation(backend, std::move(query))
    .then_on_main([weakSelf,queryId](SuggestionsList hits)
    {
        auto self = weakSelf.lock();
        // maybe this call is already out of date:
        if (!self || self->m_latestQueryId != queryId)
            return;
        self->UpdateSuggestions(hits);
        if (--self->m_pendingQueries == 0)
            self->OnQueriesFinished();
    })
    .catch_all([weakSelf,queryId,backendPtr](dispatch::exception_ptr e)
    {
        auto self = weakSelf.lock();
        // maybe this call is already out of date:
        if (!self || self->m_latestQueryId != queryId)
            return;
        self->ReportError(backendPtr, e);
        if (--self->m_pendingQueries == 0)
            self->OnQueriesFinished();
    });
}



Sidebar::Sidebar(wxWindow *parent, wxMenu *suggestionsMenu)
    : wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE),
      m_catalog(nullptr),
      m_selectedItem(nullptr)
{
    ColorScheme::SetupWindowColors(this, [=]
    {
        SetBackgroundColour(ColorScheme::Get(Color::SidebarBackground));
    });

#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    Bind(wxEVT_PAINT, &Sidebar::OnPaint, this);
#ifdef __WXOSX__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    auto *topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize(wxSize(PX(300), -1));

    m_blocksSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_blocksSizer, wxSizerFlags(1).Expand());
    topSizer->AddSpacer(SIDEBAR_PADDING);

    m_topBlocksSizer = new wxBoxSizer(wxVERTICAL);
    m_bottomBlocksSizer = new wxBoxSizer(wxVERTICAL);

    m_blocksSizer->Add(m_topBlocksSizer, wxSizerFlags(1).Expand().ReserveSpaceEvenIfHidden());
    m_blocksSizer->Add(m_bottomBlocksSizer, wxSizerFlags().Expand());

    m_topBlocksSizer->AddSpacer(PXDefaultBorder);

    AddBlock(SuggestionsSidebarBlock::Create(this, suggestionsMenu), Top);
    AddBlock(new OldMsgidSidebarBlock(this), Bottom);
    AddBlock(new ExtractedCommentSidebarBlock(this), Bottom);
    AddBlock(new CommentSidebarBlock(this), Bottom);
    AddBlock(new AddCommentSidebarBlock(this), Bottom);

    SetSizerAndFit(topSizer);

    SetSelectedItem(nullptr, nullptr);
}


void Sidebar::AddBlock(SidebarBlock *block, BlockPos pos)
{
    m_blocks.emplace_back(block);

    auto sizer = (pos == Top) ? m_topBlocksSizer : m_bottomBlocksSizer;
    auto grow = (block->IsGrowable()) ? 1 : 0;
    sizer->Add(block->GetSizer(), wxSizerFlags(grow).Expand());
}


Sidebar::~Sidebar()
{
}


void Sidebar::SetSelectedItem(const CatalogPtr& catalog, const CatalogItemPtr& item)
{
    m_catalog = catalog;
    m_selectedItem = item;
    RefreshContent();
}

void Sidebar::SetMultipleSelection()
{
    SetSelectedItem(nullptr, nullptr);
}

Language Sidebar::GetCurrentLanguage() const
{
    if (!m_catalog)
        return Language();
    return m_catalog->GetLanguage();
}

Language Sidebar::GetCurrentSourceLanguage() const
{
    if (!m_catalog)
        return Language::English();
    return m_catalog->GetSourceLanguage();
}

bool Sidebar::FileHasCapability(Catalog::Cap cap) const
{
    return m_catalog && m_catalog->HasCapability(cap);
}

void Sidebar::RefreshContent()
{
    if (!IsShown())
        return;

    auto item = m_selectedItem;
    if (!IsThisEnabled())
        item = nullptr;

    wxWindowUpdateLocker lock(this);
    for (auto& b: m_blocks)
        b->SetItem(item);
    Layout();
}

void Sidebar::SetUpperHeight(int size)
{
    wxWindowUpdateLocker lock(this);

    int pos = GetSize().y - size;

    if (size < PX(400) || pos > size)
    {
        // Too little space for suggestions (either absolute size small or
        // bottom area larger than top). If that happens, align the top/bottom
        // separator with the Translation: field in editing area instead of
        // with its top.
        pos = pos / 2 - PX(1);
    }

    pos -= SIDEBAR_PADDING;
    pos += PX(15) ; // SidebarSeparator spacing

    m_bottomBlocksSizer->SetMinSize(wxSize(-1, pos));
    Layout();
}

void Sidebar::DoEnable(bool)
{
    RefreshContent();
}

void Sidebar::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);

#ifdef __WXOSX__
    dc.SetPen(ColorScheme::Get(Color::ToolbarSeparator));
    dc.DrawLine(0, 0, dc.GetSize().x - 1, 0);
#endif
}
