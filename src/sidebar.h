/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2014 Vaclav Slavik
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

#ifndef Poedit_sidebar_h
#define Poedit_sidebar_h

#include <cstdint>
#include <memory>
#include <vector>

#include <wx/event.h>
#include <wx/panel.h>

#include "language.h"
#include "tm/suggestions.h"

class WXDLLIMPEXP_CORE wxMenu;
class WXDLLIMPEXP_CORE wxMenuItem;
class WXDLLIMPEXP_CORE wxSizer;
class WXDLLIMPEXP_CORE wxStaticText;
class WXDLLIMPEXP_CORE wxStaticBitmap;

class Catalog;
class CatalogItem;
class ExplanationLabel;

struct Suggestion;
class SuggestionsProvider;
class SuggestionWidget;
class Sidebar;


/// Implements part of the sidebar.
class SidebarBlock : public std::enable_shared_from_this<SidebarBlock>
{
public:
    virtual ~SidebarBlock() {}

    wxSizer *GetSizer() const { return m_sizer; }

    virtual void Show(bool show);

    void SetItem(CatalogItem *item);

    virtual bool ShouldShowForItem(CatalogItem *item) const = 0;

    virtual void Update(CatalogItem *item) = 0;

    virtual bool IsGrowable() const { return false; }

protected:
    enum Flags
    {
        NoUpperMargin = 1
    };

    SidebarBlock(Sidebar *parent, const wxString& label, int flags = 0);

    Sidebar *m_parent;
    wxSizer *m_innerSizer;

private:
    wxSizer *m_sizer;
};


wxDECLARE_EVENT(EVT_SUGGESTION_SELECTED, wxCommandEvent);

/// Sidebar block implementation translation suggestions
class SuggestionsSidebarBlock : public SidebarBlock
{
public:
    SuggestionsSidebarBlock(Sidebar *parent, wxMenu *menu);
    ~SuggestionsSidebarBlock();

    void Show(bool show) override;
    bool IsGrowable() const override { return true; }
    bool ShouldShowForItem(CatalogItem *item) const override;
    void Update(CatalogItem *item) override;

protected:
    // How many entries to request from a single provider?
    static const int SUGGESTIONS_REQUEST_COUNT = 9;
    // How many entries can have shortcuts?
    static const int SUGGESTIONS_MENU_ENTRIES = 9;

    void UpdateVisibility();

    virtual wxBitmap GetIconForSuggestion(const Suggestion& s) const;
    virtual wxString GetTooltipForSuggestion(const Suggestion& s) const;

    void ClearMessage();
    void SetMessage(const wxString& icon, const wxString& text);

    virtual void ClearSuggestions();
    virtual void UpdateSuggestions(const SuggestionsList& hits);
    virtual void OnQueriesFinished();

    virtual void BuildSuggestionsMenu(int count = SUGGESTIONS_MENU_ENTRIES);
    virtual void UpdateSuggestionsMenu();
    virtual void ClearSuggestionsMenu();

    void QueryProvider(SuggestionsBackend& backend, CatalogItem *item);

protected:
    std::unique_ptr<SuggestionsProvider> m_provider;

    wxMenu *m_suggestionsMenu;

    wxSizer *m_msgSizer;
    bool m_msgPresent;
    wxStaticBitmap *m_msgIcon;
    ExplanationLabel *m_msgText;
    wxStaticText *m_iGotNothing;

    SuggestionsList m_suggestions;
    std::vector<SuggestionWidget*> m_suggestionsWidgets;
    std::vector<wxMenuItem*> m_suggestionMenuItems;
    int m_pendingQueries;
    uint64_t m_latestQueryId;
};

/**
    Control showing Poedit's assistance sidebar.
    
    Contains TM suggestions, comments and possibly other auxiliary stuff.
 */
class Sidebar : public wxPanel
{
public:
    Sidebar(wxWindow *parent, wxMenu *suggestionsMenu);
    ~Sidebar();

    /// Update selected item, if there's a single one. May be nullptr.
    void SetSelectedItem(Catalog *catalog, CatalogItem *item);

    /// Tell the sidebar there's multiple selection.
    void SetMultipleSelection();

    /// Returns currently selected item
    CatalogItem *GetSelectedItem() const { return m_selectedItem; }
    Language GetCurrentLanguage() const;

    /// Refreshes displayed content
    void RefreshContent();

    /// Set max height of the upper (not input-aligned) part.
    void SetUpperHeight(int size);

protected:
    void DoEnable(bool enable) override;

private:
    enum BlockPos { Top, Bottom };
    void AddBlock(SidebarBlock *block, BlockPos pos);

    void OnPaint(wxPaintEvent&);

private:
    Catalog *m_catalog;
    CatalogItem *m_selectedItem;

    std::vector<std::shared_ptr<SidebarBlock>> m_blocks;

    wxSizer *m_blocksSizer;
    wxSizer *m_topBlocksSizer, *m_bottomBlocksSizer;
};

#endif // Poedit_sidebar_h
