/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2016 Vaclav Slavik
 *  Copyright (C) 2005 Olivier Sannier
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

#include "edlistctrl.h"

#include "hidpi.h"
#include "language.h"
#include "cat_sorting.h"
#include "colorscheme.h"
#include "unicode_helpers.h"
#include "utility.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/wupdlock.h>

#ifdef __WXMSW__
#include <wx/headerctrl.h>
#include <wx/itemattr.h>
#include <wx/msw/uxtheme.h>
#include <vssym32.h>
#endif

#include <algorithm>


namespace
{

class SelectionPreserver
{
public:
    SelectionPreserver(PoeditListCtrl *list_) : list(list_), focus(-1)
    {
        if (!list)
            return;
        selection = list->GetSelectedCatalogItemIndexes();
        focus = list->ListItemToCatalogIndex(list->GetCurrentItem());
    }

    ~SelectionPreserver()
    {
        if (!list)
            return;
        if (!selection.empty())
            list->SetSelectedCatalogItemIndexes(selection);
        if (focus != -1)
        {
            auto item = list->CatalogIndexToListItem(focus);
            list->EnsureVisible(item);
            list->SetCurrentItem(item);
        }
    }

private:
    PoeditListCtrl *list;
    std::vector<int> selection;
    int focus;
};

} // anonymous namespace



PoeditListCtrl::Model::Model(TextDirection appTextDir, wxVisualAttributes visual)
    : m_sourceTextDir(TextDirection::LTR),
      m_transTextDir(TextDirection::LTR),
      m_appTextDir(appTextDir)
{
    sortOrder = SortOrder::Default();

    // configure items colors & fonts:

    m_clrID = ColorScheme::Get(Color::ItemID, visual);
    m_clrFuzzy = ColorScheme::Get(Color::ItemFuzzy, visual);
    m_clrInvalid = ColorScheme::Get(Color::ItemError, visual);

    m_iconPreTranslated = wxArtProvider::GetBitmap("poedit-status-automatic");
    m_iconComment = wxArtProvider::GetBitmap("poedit-status-comment");
    m_iconBookmark = wxArtProvider::GetBitmap("poedit-status-bookmark");
}


void PoeditListCtrl::Model::SetCatalog(CatalogPtr catalog)
{
    m_catalog = catalog;

    if (!catalog)
    {
        Reset(0);
        return;
    }

    auto srclang = catalog->GetSourceLanguage();
    auto lang = catalog->GetLanguage();
    m_sourceTextDir = srclang.Direction();
    m_transTextDir = lang.Direction();

    // sort catalog items, create indexes mapping
    CreateSortMap();

    Reset(catalog->GetCount());
}


void PoeditListCtrl::Model::UpdateSort()
{
    if (!m_catalog)
        return;
    CreateSortMap();
    Reset(m_catalog->GetCount());
}


wxString PoeditListCtrl::Model::GetColumnType(unsigned int col) const
{
    switch (col)
    {
        case Col_ID:
            return "string";

        case Col_Icon:
            return "wxBitmap";

        case Col_Source:
        case Col_Translation:
            return "string";

        default:
            return "null";
    }
}


void PoeditListCtrl::Model::GetValueByRow(wxVariant& variant, unsigned row, unsigned col) const
{
    auto d = Item(row);
    wxCHECK_RET(d, "invalid row");

    switch (col)
    {
        case Col_ID:
        {
            variant = wxString::Format("%d", d->GetId());
            break;
        }

        case Col_Icon:
        {
            if (d->GetBookmark() != NO_BOOKMARK)
                variant << m_iconBookmark;
            else if (d->HasComment() || d->HasExtractedComments())
                variant << m_iconComment;
            else if (d->IsPreTranslated())
                variant << m_iconPreTranslated;
            else
                variant = wxNullVariant;
            break;
        }

        case Col_Source:
        {
            wxString orig;
#if wxCHECK_VERSION(3,1,1)
            if (d->HasContext())
            {
                // Work around a problem with GTK+'s coloring of markup that begins with colorizing <span>:
                #ifdef __WXGTK__
                    #define MARKUP(x) L"\u200B" L##x
                #else
                    #define MARKUP(x) x
                #endif
                orig.Printf(MARKUP("<span style=\"italic\" color=\"#2B6F16\">[%s]</span> %s"),
                            EscapeMarkup(d->GetContext()), EscapeMarkup(d->GetString()));
            }
            else
            {
                orig = EscapeMarkup(d->GetString());
            }
#else // wx 3.0
            if (d->HasContext())
                orig.Printf("[%s] %s", d->GetContext(), d->GetString());
            else
                orig = d->GetString();
#endif

            // FIXME: use syntax highlighting or typographic marks
            orig.Replace("\n", " ");

            // Add RTL Unicode mark to render bidi texts correctly
            if (m_appTextDir != m_sourceTextDir)
                variant = bidi::mark_direction(orig, m_sourceTextDir);
            else
                variant = orig;
            break;
        }

        case Col_Translation:
        {
            auto trans = d->GetTranslation();

            // FIXME: use syntax highlighting or typographic marks
            trans.Replace("\n", " ");

            // Add RTL Unicode mark to render bidi texts correctly
            if (m_appTextDir != m_transTextDir)
                variant = bidi::mark_direction(trans, m_transTextDir);
            else
                variant = trans;
            break;
        }

        default:
            variant.Clear();
            break;
    };
}

bool PoeditListCtrl::Model::SetValueByRow(const wxVariant&, unsigned, unsigned)
{
    wxFAIL_MSG("setting values in dataview not implemented");
    return false;
}

bool PoeditListCtrl::Model::GetAttrByRow(unsigned row, unsigned col, wxDataViewItemAttr& attr) const
{
    if (!m_catalog)
        return false;

    switch (col)
    {
        case Col_ID:
        {
            attr.SetColour(m_clrID);
            return true;
        }

        case Col_Source:
        case Col_Translation:
        {
            auto d = Item(row);
            if (d->GetValidity() == CatalogItem::Val_Invalid)
            {
                attr.SetColour(m_clrInvalid);
                return true;
            }
            else if (d->IsFuzzy())
            {
                attr.SetColour(m_clrFuzzy);
                return true;
            }
            else
            {
                return false;
            }
        }

        default:
            return false;
    }
}


void PoeditListCtrl::Model::CreateSortMap()
{
    // FIXME: Use native wxDataViewCtrl sorting instead

    int count = (int)m_catalog->GetCount();

    m_mapListToCatalog.resize(count);
    m_mapCatalogToList.resize(count);

    // First create identity mapping for the sort order.
    for ( int i = 0; i < count; i++ )
        m_mapListToCatalog[i] = i;

    // m_mapListToCatalog will hold our desired sort order. Sort it in place
    // now, using the desired sort criteria.
    CatalogItemsComparator comparator(*m_catalog, sortOrder);
    std::sort
    (
        m_mapListToCatalog.begin(),
        m_mapListToCatalog.end(),
        std::ref(comparator)
    );

    // Finally, construct m_mapCatalogToList to be the inverse mapping to
    // m_mapListToCatalog.
    for ( int i = 0; i < count; i++ )
        m_mapCatalogToList[m_mapListToCatalog[i]] = i;
}




PoeditListCtrl::PoeditListCtrl(wxWindow *parent, wxWindowID id, bool dispIDs)
     : wxDataViewCtrl(parent, id, wxDefaultPosition, wxDefaultSize, wxDV_MULTIPLE | wxDV_ROW_LINES | wxNO_BORDER, wxDefaultValidator, "translations list")
{
    m_displayIDs = dispIDs;

    m_appTextDir = (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft) ? TextDirection::RTL : TextDirection::LTR;

    m_model.reset(new Model(m_appTextDir, GetDefaultAttributes()));
    AssociateModel(m_model.get());

    CreateColumns();

    UpdateHeaderAttrs();

    Bind(wxEVT_SIZE, &PoeditListCtrl::OnSize, this);
}

PoeditListCtrl::~PoeditListCtrl()
{
    sortOrder().Save();
}

void PoeditListCtrl::UpdateHeaderAttrs()
{
#ifdef __WXMSW__
    // Setup custom header font & color on Windows 10, where the default look is a bit odd
    if (IsWindows10OrGreater())
    {
        // Use the same text color as Explorer's headers use
        const wxUxThemeEngine* theme = wxUxThemeEngine::GetIfActive();
        if (theme)
        {
            wxUxThemeHandle hTheme(this->GetParent(), L"ItemsView::Header");
            COLORREF clr;
            HRESULT hr = theme->GetThemeColor(hTheme, HP_HEADERITEM, 0, TMT_TEXTCOLOR, &clr);
            if (SUCCEEDED(hr))
            {
                wxItemAttr headerAttr;
                headerAttr.SetTextColour(wxRGBToColour(clr));
                SetHeaderAttr(headerAttr);
            }
        }

        // Standard header has smaller height than Explorer's and it isn't
        // separated from the content well -- especially in HiDPI modes.
        // Match Explorer's header size too:
        int headerHeight = HiDPIScalingFactor() > 1.0 ? PX(26) : PX(25);
        GenericGetHeader()->SetMinSize(wxSize(-1, headerHeight));
    }
#endif // __WXMSW__
}

void PoeditListCtrl::SetCustomFont(wxFont font_)
{
    wxFont font(font_);

    if ( !font.IsOk() )
        font = GetDefaultAttributes().font;

    SetFont(font);

    UpdateHeaderAttrs();
    CreateColumns();
}

void PoeditListCtrl::SetDisplayLines(bool dl)
{
    m_displayIDs = dl;
    CreateColumns();
}

void PoeditListCtrl::CreateColumns()
{
    if (GetColumnCount() > 0)
        ClearColumns();

    m_colID = m_colIcon = m_colSource = m_colTrans = nullptr;

    Language srclang, lang;
    if (m_catalog)
    {
        srclang = m_catalog->GetSourceLanguage();
        lang = m_catalog->GetLanguage();
    }

    auto isRTL = lang.IsRTL();
#ifdef __WXMSW__
    // a quirk of wx API: if the current locale is RTL, the meaning of L and R is reversed
    if (m_appTextDir == TextDirection::RTL)
        isRTL = !isRTL;
#endif

    m_colIcon = AppendBitmapColumn(L"∙", Model::Col_Icon, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_CENTER, 0);

    wxString sourceTitle = srclang.IsValid()
                             ? wxString::Format(_(L"Source text — %s"), srclang.DisplayName())
                             : _("Source text");
    auto sourceRenderer = new wxDataViewTextRenderer();
    sourceRenderer->EnableEllipsize(wxELLIPSIZE_END);
#if wxCHECK_VERSION(3,1,1)
    sourceRenderer->EnableMarkup();
#endif
    m_colSource = new wxDataViewColumn(sourceTitle, sourceRenderer, Model::Col_Source, wxCOL_WIDTH_DEFAULT, wxALIGN_LEFT, 0);
    AppendColumn(m_colSource);

    if (m_model->m_catalog && m_model->m_catalog->HasCapability(Catalog::Cap::Translations))
    {
        wxString langname = lang.IsValid() ? lang.DisplayName() : _("unknown language");;
        wxString transTitle = wxString::Format(_(L"Translation — %s"), langname);
        auto transRenderer = new wxDataViewTextRenderer();
        transRenderer->EnableEllipsize(wxELLIPSIZE_END);
        m_colTrans = new wxDataViewColumn(transTitle, transRenderer, Model::Col_Translation, wxCOL_WIDTH_DEFAULT, isRTL ? wxALIGN_RIGHT : wxALIGN_LEFT, 0);
        AppendColumn(m_colTrans);
    }

    if (m_displayIDs)
    {
        m_colID = AppendTextColumn(_("ID"), Model::Col_ID, wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_RIGHT, 0);
    }

    // wxDVC insists on having an expander column, but we really don't want one:
    auto fake = AppendTextColumn("", Model::Col_ID);
    fake->SetHidden(true);
    SetExpanderColumn(fake);

#ifdef __WXMSW__
    if (m_appTextDir == TextDirection::RTL)
        m_colSource->SetAlignment(wxALIGN_RIGHT);
#endif

    SizeColumns();

#ifdef __WXGTK__
    // wxGTK has delayed sizing computation, apparently
    CallAfter([=]{ SizeColumns(); });
#endif
}

void PoeditListCtrl::SizeColumns()
{
    int w = GetClientSize().x;

#if defined(__WXOSX__)
    w -= (GetColumnCount() - 1) * 3;
#elif defined(__WXGTK__)
    w -= 2;
#endif

    if (m_colIcon)
        w -= m_colIcon->GetWidth();

    if (m_colID)
        w -= m_colID->GetWidth();

    if (m_colTrans)
    {
        m_colSource->SetWidth(w / 2);
        m_colTrans->SetWidth(w - w / 2);
    }
    else
    {
        m_colSource->SetWidth(w);
    }
}


void PoeditListCtrl::CatalogChanged(const CatalogPtr& catalog)
{
    wxWindowUpdateLocker no_updates(this);

    const int oldCount = m_catalog ? m_catalog->GetCount() : 0;
    const int newCount = catalog ? catalog->GetCount() : 0;
    const bool isSameCatalog = (catalog == m_catalog);
    const bool sizeOrCatalogChanged = !isSameCatalog || (oldCount != newCount);

    SelectionPreserver preserve(!sizeOrCatalogChanged ? this : nullptr);

    // now read the new catalog:
    m_catalog = catalog;
    m_model->SetCatalog(catalog);

    if (!m_catalog)
        return;

    CreateColumns();

    if (sizeOrCatalogChanged && GetItemCount() > 0)
        CallAfter([=]{ SelectAndFocus(0); });
}


void PoeditListCtrl::Sort()
{
    if (!m_catalog)
        return;

    SelectionPreserver preserve(this);
    m_model->UpdateSort();
}


void PoeditListCtrl::OnSize(wxSizeEvent& event)
{
    wxWindowUpdateLocker lock(this);

    SizeColumns();
    event.Skip();
}
