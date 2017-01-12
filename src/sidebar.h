/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2017 Vaclav Slavik
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
#include <wx/timer.h>

#include "catalog.h"
#include "language.h"
#include "tm/suggestions.h"

class WXDLLIMPEXP_FWD_CORE wxMenu;
class WXDLLIMPEXP_FWD_CORE wxMenuItem;
class WXDLLIMPEXP_FWD_CORE wxSizer;
class WXDLLIMPEXP_FWD_CORE wxStaticText;
class WXDLLIMPEXP_FWD_CORE wxStaticBitmap;

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

    void SetItem(const CatalogItemPtr& item);

    virtual bool ShouldShowForItem(const CatalogItemPtr& item) const = 0;

    virtual void Update(const CatalogItemPtr& item) = 0;

    virtual bool IsGrowable() const { return false; }

protected:
    enum Flags
    {
        NoUpperMargin = 1
    };

    SidebarBlock(Sidebar *parent, const wxString& label, int flags = 0);

    Sidebar *m_parent;
    wxSizer *m_headerSizer;
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
    bool ShouldShowForItem(const CatalogItemPtr& item) const override;
    void Update(const CatalogItemPtr& item) override;

protected:
    // How many entries can have shortcuts?
    static const int SUGGESTIONS_MENU_ENTRIES = 9;

    virtual void UpdateVisibility();

    virtual wxBitmap GetIconForSuggestion(const Suggestion& s) const;
    virtual wxString GetTooltipForSuggestion(const Suggestion& s) const;

    void ClearMessage();
    void SetMessage(const wxString& icon, const wxString& text);

    virtual void ReportError(SuggestionsBackend *backend, dispatch::exception_ptr e);
    virtual void ClearSuggestions();
    virtual void UpdateSuggestions(const SuggestionsList& hits);
    virtual void OnQueriesFinished();

    virtual void BuildSuggestionsMenu(int count = SUGGESTIONS_MENU_ENTRIES);
    virtual void UpdateSuggestionsMenu();
    virtual void ClearSuggestionsMenu();

    virtual void QueryAllProviders(const CatalogItemPtr& item);
    void QueryProvider(SuggestionsBackend& backend, const CatalogItemPtr& item, uint64_t queryId);

    // Handle showing of suggestions
    void UpdateSuggestionsForItem(CatalogItemPtr item);
    void OnDelayedShowSuggestionsForItem(wxTimerEvent& e);

protected:
    std::unique_ptr<SuggestionsProvider> m_provider;

    wxMenu *m_suggestionsMenu;

    wxSizer *m_msgSizer;
    bool m_msgPresent;
    wxStaticBitmap *m_msgIcon;
    ExplanationLabel *m_msgText;
    wxStaticText *m_iGotNothing;

    wxSizer *m_suggestionsSizer;
    // Additional sizer for derived classes, shown below suggestions
    wxSizer *m_extrasSizer;

    SuggestionsList m_suggestions;
    std::vector<SuggestionWidget*> m_suggestionsWidgets;
    std::vector<wxMenuItem*> m_suggestionMenuItems;
    int m_pendingQueries;
    uint64_t m_latestQueryId;

    // delayed showing of suggestions:
    long long m_lastUpdateTime;
    wxTimer m_suggestionsTimer;
};

/**
    Control showing Poedit's assistance sidebar.
    
    Contains TM suggestions, comments and possibly other auxiliary stuff.
 */
class Sidebar : public wxWindow
{
public:
    Sidebar(wxWindow *parent, wxMenu *suggestionsMenu);
    ~Sidebar();

    /// Update selected item, if there's a single one. May be nullptr.
    void SetSelectedItem(const CatalogPtr& catalog, const CatalogItemPtr& item);

    /// Tell the sidebar there's multiple selection.
    void SetMultipleSelection();

    /// Returns currently selected item
    CatalogItemPtr GetSelectedItem() const { return m_selectedItem; }
    Language GetCurrentSourceLanguage() const;
    Language GetCurrentLanguage() const;
    bool FileHasCapability(Catalog::Cap cap) const;

    /// Refreshes displayed content
    void RefreshContent();

    /// Call when catalog changes/is invalidated
    void ResetCatalog() { SetSelectedItem(nullptr, nullptr); }

    /// Set max height of the upper (not input-aligned) part.
    void SetUpperHeight(int size);

    bool AcceptsFocus() const override { return false; }

protected:
    void DoEnable(bool enable) override;

private:
    enum BlockPos { Top, Bottom };
    void AddBlock(SidebarBlock *block, BlockPos pos);

    void OnPaint(wxPaintEvent&);

private:
    CatalogPtr m_catalog;
    CatalogItemPtr m_selectedItem;

    std::vector<std::shared_ptr<SidebarBlock>> m_blocks;

    wxSizer *m_blocksSizer;
    wxSizer *m_topBlocksSizer, *m_bottomBlocksSizer;
};

#endif // Poedit_sidebar_h
