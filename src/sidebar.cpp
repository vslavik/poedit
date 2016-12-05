/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2016 Vaclav Slavik
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
#include "commentdlg.h"
#include "concurrency.h"
#include "errors.h"
#include "hidpi.h"
#include "utility.h"
#include "unicode_helpers.h"

#include "tm/suggestions.h"
#include "tm/transmem.h"

#include <wx/app.h>
#include <wx/button.h>
#include <wx/config.h>
#include <wx/dcclient.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/time.h>
#include <wx/wupdlock.h>

#include <algorithm>

#define SIDEBAR_BACKGROUND      wxColour("#EDF0F4")
#define GRAY_LINES_COLOR        wxColour(220,220,220)
#define GRAY_LINES_COLOR_DARK   wxColour(180,180,180)


class SidebarSeparator : public wxWindow
{
public:
    SidebarSeparator(wxWindow *parent)
        : wxWindow(parent, wxID_ANY),
          m_sides(SIDEBAR_BACKGROUND),
          m_center(GRAY_LINES_COLOR_DARK)
    {
        Bind(wxEVT_PAINT, &SidebarSeparator::OnPaint, this);
    }

    wxSize DoGetBestSize() const override
    {
        return wxSize(-1, 1);
    }

    bool AcceptsFocus() const override { return false; }

private:
    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        auto w = dc.GetSize().x;
        dc.GradientFillLinear(wxRect(0,0,PX(15),PX(1)), m_sides, m_center);
        dc.GradientFillLinear(wxRect(PX(15),0,w,PX(1)), m_center, m_sides);
    }

    wxColour m_sides, m_center;
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
                         wxSizerFlags().Expand().Border(wxBOTTOM|wxLEFT, PX(2)));
        }
        m_headerSizer = new wxBoxSizer(wxHORIZONTAL);
        m_headerSizer->Add(new HeadingLabel(parent, label), wxSizerFlags().Expand());
        m_sizer->Add(m_headerSizer, wxSizerFlags().Expand().PXDoubleBorder(wxLEFT|wxRIGHT));
    }
    m_innerSizer = new wxBoxSizer(wxVERTICAL);
    m_sizer->Add(m_innerSizer, wxSizerFlags(1).Expand().PXDoubleBorder(wxLEFT|wxRIGHT));
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
        : SidebarBlock(parent, _("Previous source text:"))
    {
        m_innerSizer->AddSpacer(PX(2));
        m_innerSizer->Add(new ExplanationLabel(parent, _("The old source text (before it changed during an update) that the fuzzy translation corresponds to.")),
                     wxSizerFlags().Expand());
        m_innerSizer->AddSpacer(PX(5));
        m_text = new SelectableAutoWrappingText(parent, "");
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
        : SidebarBlock(parent, _("Notes for translators:"))
    {
        m_innerSizer->AddSpacer(PX(5));
        m_comment = new SelectableAutoWrappingText(parent, "");
        m_innerSizer->Add(m_comment, wxSizerFlags().Expand());
    }

    bool ShouldShowForItem(const CatalogItemPtr& item) const override
    {
        return item->HasExtractedComments();
    }

    void Update(const CatalogItemPtr& item) override
    {
        auto comment = wxJoin(item->GetExtractedComments(), '\n', '\0');
        if (comment.StartsWith("TRANSLATORS:") || comment.StartsWith("translators:"))
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
        : SidebarBlock(parent, _("Comment:"))
    {
        m_innerSizer->AddSpacer(PX(5));
        m_comment = new SelectableAutoWrappingText(parent, "");
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
    SuggestionWidget(wxWindow *parent) : wxWindow(parent, wxID_ANY)
    {
        m_icon = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);
        m_text = new AutoWrappingText(this, "TEXT");
        m_info = new InfoStaticText(this);

        auto top = new wxBoxSizer(wxHORIZONTAL);
        auto right = new wxBoxSizer(wxVERTICAL);
        top->AddSpacer(PX(2));
        top->Add(m_icon, wxSizerFlags().Top().PXBorder(wxTOP|wxBOTTOM));
        top->Add(right, wxSizerFlags(1).Expand().PXBorder(wxLEFT));
        right->Add(m_text, wxSizerFlags().Expand().Border(wxTOP, PX(4)));
        right->Add(m_info, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM, PX(2)));
        SetSizerAndFit(top);

        // setup mouse hover highlighting:
        m_bg = parent->GetBackgroundColour();
        m_bgHighlight = m_bg.ChangeLightness(160);

        wxWindow* parts [] = { this, m_icon, m_text, m_info };
        for (auto w : parts)
        {
            w->Bind(wxEVT_MOTION,       &SuggestionWidget::OnMouseMove, this);
            w->Bind(wxEVT_LEAVE_WINDOW, &SuggestionWidget::OnMouseMove, this);
            w->Bind(wxEVT_LEFT_UP,      &SuggestionWidget::OnMouseClick, this);
        }
    }

    void SetValue(int index, const Suggestion& s, Language lang, const wxBitmap& icon, const wxString& tooltip)
    {
        m_value = s;

        auto percent = wxString::Format("%d%%", int(100 * s.score));

        index++;
        if (index < 10)
        {
        #ifdef __WXOSX__
            auto shortcut = wxString::Format(L"⌘%d", index);
        #else
            // TRANSLATORS: This is the key shortcut used in menus on Windows, some languages call them differently
            auto shortcut = wxString::Format("%s%d", _("Ctrl+"), index);
        #endif
            m_info->SetLabel(wxString::Format(L"%s • %s", shortcut, percent));
        }
        else
        {
            m_info->SetLabel(percent);
        }

        m_icon->SetBitmap(icon);

        auto text = wxControl::EscapeMnemonics(bidi::mark_direction(s.text, lang));

        m_text->SetLanguage(lang);
        m_text->SetAndWrapLabel(text);

#ifndef __WXOSX__
        // FIXME: Causes weird issues on OS X: tooltips appearing on the main list control,
        //        over toolbar, where the mouse just was etc.
        m_icon->SetToolTip(tooltip);
        m_text->SetToolTip(tooltip);
#endif
        (void)tooltip;

        SetBackgroundColour(m_bg);
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
            SetForegroundColour(ExplanationLabel::GetTextColor());
        #ifdef __WXMSW__
            SetFont(GetFont().Smaller());
        #else
            SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        #endif
        }

        void DoEnable(bool) override {} // wxOSX's disabling would break color
    };

    void OnMouseMove(wxMouseEvent& e)
    {
        auto rectWin = GetClientRect();
        rectWin.Deflate(1); // work around off-by-one issue on OS X
        auto evtWin = static_cast<wxWindow*>(e.GetEventObject());
        auto mpos = e.GetPosition();
        if (evtWin != this)
            mpos += evtWin->GetPosition();
        bool highlighted = rectWin.Contains(mpos);
        SetBackgroundColour(highlighted ? m_bgHighlight : m_bg);
        Refresh();
    }

    void OnMouseClick(wxMouseEvent&)
    {
        wxCommandEvent event(EVT_SUGGESTION_SELECTED);
        event.SetEventObject(this);
        event.SetString(m_value.text);
        ProcessWindowEvent(event);
    }

    Suggestion m_value;
    wxStaticBitmap *m_icon;
    AutoWrappingText *m_text;
    wxStaticText *m_info;
    wxColour m_bg, m_bgHighlight;
};


SuggestionsSidebarBlock::SuggestionsSidebarBlock(Sidebar *parent, wxMenu *menu)
    : SidebarBlock(parent, _("Translation suggestions:"), NoUpperMargin),
      m_suggestionsMenu(menu),
      m_msgPresent(false),
      m_pendingQueries(0),
      m_latestQueryId(0),
      m_lastUpdateTime(0)
{
    m_provider.reset(new SuggestionsProvider);

    m_msgSizer = new wxBoxSizer(wxHORIZONTAL);
    m_msgIcon = new wxStaticBitmap(parent, wxID_ANY, wxNullBitmap);
    m_msgText = new ExplanationLabel(parent, "");
    m_msgSizer->Add(m_msgIcon, wxSizerFlags().Center().PXBorderAll());
    m_msgSizer->Add(m_msgText, wxSizerFlags(1).Center().PXBorder(wxTOP|wxBOTTOM));
    m_innerSizer->Add(m_msgSizer, wxSizerFlags().Expand());

    m_innerSizer->AddSpacer(PX(10));

    m_suggestionsSizer = new wxBoxSizer(wxVERTICAL);
    m_extrasSizer = new wxBoxSizer(wxVERTICAL);
    m_innerSizer->Add(m_suggestionsSizer, wxSizerFlags().Expand());
    m_innerSizer->Add(m_extrasSizer, wxSizerFlags().Expand());

    m_iGotNothing = new wxStaticText(parent, wxID_ANY,
                                #ifdef __WXMSW__
                                     // TRANSLATORS: This is shown when no translation suggestions can be found in the TM (Windows).
                                     _("No matches found")
                                #else
                                     // TRANSLATORS: This is shown when no translation suggestions can be found in the TM (OS X, Linux).
                                     _("No Matches Found")
                                #endif
                                     );
    m_iGotNothing->SetForegroundColour(ExplanationLabel::GetTextColor().ChangeLightness(150));
    m_iGotNothing->SetWindowVariant(wxWINDOW_VARIANT_NORMAL);
#ifdef __WXMSW__
    m_iGotNothing->SetFont(m_iGotNothing->GetFont().Larger());
#endif
    m_innerSizer->Add(m_iGotNothing, wxSizerFlags().Center().Border(wxTOP|wxBOTTOM, PX(100)));

    BuildSuggestionsMenu();

    m_suggestionsTimer.SetOwner(parent);
    parent->Bind(wxEVT_TIMER,
                 &SuggestionsSidebarBlock::OnDelayedShowSuggestionsForItem, this,
                 m_suggestionsTimer.GetId());
}

SuggestionsSidebarBlock::~SuggestionsSidebarBlock()
{
    ClearSuggestionsMenu();
    for (auto i : m_suggestionMenuItems)
        delete i;
}

wxBitmap SuggestionsSidebarBlock::GetIconForSuggestion(const Suggestion&) const
{
    return wxArtProvider::GetBitmap("SuggestionTM");
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
    m_parent->Layout();
}

void SuggestionsSidebarBlock::SetMessage(const wxString& icon, const wxString& text)
{
    m_msgPresent = true;
    m_msgIcon->SetBitmap(wxArtProvider::GetBitmap(icon));
    m_msgText->SetAndWrapLabel(text);
    UpdateVisibility();
    m_parent->Layout();
}

void SuggestionsSidebarBlock::ReportError(SuggestionsBackend*, std::exception_ptr e)
{
    SetMessage("SuggestionError", DescribeException(e));
}

void SuggestionsSidebarBlock::ClearSuggestions()
{
    m_suggestions.clear();
    UpdateSuggestionsMenu();
    UpdateVisibility();
}

void SuggestionsSidebarBlock::UpdateSuggestions(const SuggestionsList& hits)
{
    wxWindowUpdateLocker lock(m_parent);

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
        auto w = new SuggestionWidget(m_parent);
        m_suggestionsSizer->Add(w, wxSizerFlags().Expand());
        m_suggestionsWidgets.push_back(w);
    }
    m_innerSizer->Layout();

    // update shown suggestions:
    auto lang = m_parent->GetCurrentLanguage();
    for (size_t i = 0; i < m_suggestions.size(); ++i)
    {
        auto s = m_suggestions[i];
        m_suggestionsWidgets[i]->SetValue((int)i, s, lang, GetIconForSuggestion(s), GetTooltipForSuggestion(s));
    }

    m_innerSizer->Layout();
    UpdateVisibility();
    m_parent->Layout();

    UpdateSuggestionsMenu();
}

void SuggestionsSidebarBlock::BuildSuggestionsMenu(int count)
{
    m_suggestionMenuItems.reserve(SUGGESTIONS_MENU_ENTRIES);
    auto menu = m_suggestionsMenu;
    for (int i = 0; i < count; i++)
    {
        auto text = wxString::Format("(empty)\t%s%d", _("Ctrl+"), i+1);
        auto item = new wxMenuItem(menu, wxID_ANY, text);
        item->SetBitmap(wxArtProvider::GetBitmap("SuggestionTM"));

        m_suggestionMenuItems.push_back(item);
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
        formatMask = L"\u202b%s\u202c\t" + _("Ctrl+") + "%d";
    else
        formatMask = L"\u202a%s\u202c\t" + _("Ctrl+") + "%d";

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

        auto item = m_suggestionMenuItems[index];
        m_suggestionsMenu->Append(item);

        auto label = wxControl::EscapeMnemonics(wxString::Format(formatMask, text, index+1));
        item->SetItemLabel(label);
        item->SetBitmap(GetIconForSuggestion(s));

        index++;
    }
}

void SuggestionsSidebarBlock::ClearSuggestionsMenu()
{
    auto m = m_suggestionsMenu;

    auto menuItems = m->GetMenuItems();
    for (auto i: menuItems)
    {
        if (std::find(m_suggestionMenuItems.begin(), m_suggestionMenuItems.end(), i) != m_suggestionMenuItems.end())
            m->Remove(i);
    }
}


void SuggestionsSidebarBlock::OnQueriesFinished()
{
    if (m_suggestions.empty())
    {
        m_innerSizer->Show(m_iGotNothing);
        m_parent->Layout();
    }
}

void SuggestionsSidebarBlock::UpdateVisibility()
{
    m_msgSizer->ShowItems(m_msgPresent);
    m_innerSizer->Show(m_iGotNothing, m_suggestions.empty() && !m_pendingQueries);

    int heightRemaining = m_innerSizer->GetSize().y;
    size_t w = 0;
    for (w = 0; w < m_suggestions.size(); w++)
    {
        heightRemaining -= m_suggestionsWidgets[w]->GetSize().y;
        if (heightRemaining < 20)
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
           wxConfig::Get()->ReadBool("use_tm", true);
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
    QueryProvider(TranslationMemory::Get(), item, thisQueryId);
}

void SuggestionsSidebarBlock::QueryProvider(SuggestionsBackend& backend, const CatalogItemPtr& item, uint64_t queryId)
{
    m_pendingQueries++;

    // we need something to talk to GUI thread through that is guaranteed
    // to exist, and the app object is a good choice:
    auto backendPtr = &backend;
    std::weak_ptr<SuggestionsSidebarBlock> weakSelf = std::dynamic_pointer_cast<SuggestionsSidebarBlock>(shared_from_this());

    m_provider->SuggestTranslation
    (
        backend,
        m_parent->GetCurrentSourceLanguage(),
        m_parent->GetCurrentLanguage(),
        item->GetString().ToStdWstring(),

        // when receiving data
        [=](const SuggestionsList& hits){
            call_on_main_thread([weakSelf,queryId,hits]{
                auto self = weakSelf.lock();
                // maybe this call is already out of date:
                if (!self || self->m_latestQueryId != queryId)
                    return;
                self->UpdateSuggestions(hits);
                if (--self->m_pendingQueries == 0)
                    self->OnQueriesFinished();
            });
        },

        // on error:
        [=](std::exception_ptr e){
            call_on_main_thread([weakSelf,queryId,backendPtr,e]{
                auto self = weakSelf.lock();
                // maybe this call is already out of date:
                if (!self || self->m_latestQueryId != queryId)
                    return;
                self->ReportError(backendPtr, e);
                if (--self->m_pendingQueries == 0)
                    self->OnQueriesFinished();
            });
        }
    );
}



Sidebar::Sidebar(wxWindow *parent, wxMenu *suggestionsMenu)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxNO_BORDER | DoubleBufferingWindowStyle()),
      m_catalog(nullptr),
      m_selectedItem(nullptr)
{
    SetBackgroundColour(SIDEBAR_BACKGROUND);
#ifdef __WXMSW__
    if (!IsWindowsXP())
        SetDoubleBuffered(true);
#endif

    Bind(wxEVT_PAINT, &Sidebar::OnPaint, this);
#ifdef __WXOSX__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    auto *topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize(wxSize(PX(300), -1));

    m_blocksSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_blocksSizer, wxSizerFlags(1).Expand().PXDoubleBorder(wxTOP|wxBOTTOM));

    m_topBlocksSizer = new wxBoxSizer(wxVERTICAL);
    m_bottomBlocksSizer = new wxBoxSizer(wxVERTICAL);

    m_blocksSizer->Add(m_topBlocksSizer, wxSizerFlags(1).Expand().ReserveSpaceEvenIfHidden());
    m_blocksSizer->Add(m_bottomBlocksSizer, wxSizerFlags().Expand());

    AddBlock(new SuggestionsSidebarBlock(this, suggestionsMenu), Top);
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
#ifdef __WXOSX__
    pos += 4;
#else
    pos += 6;
#endif
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

    dc.SetPen(wxPen(GRAY_LINES_COLOR));
#ifndef __WXMSW__
    dc.DrawLine(0, 0, 0, dc.GetSize().y-1);
#endif
#ifndef __WXOSX__
    dc.DrawLine(0, 0, dc.GetSize().x - 1, 0);
#endif
}
