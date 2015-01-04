/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2015 Vaclav Slavik
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

#include "language.h"
#include "digits.h"
#include "cat_sorting.h"

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/dcmemory.h>
#include <wx/image.h>
#include <wx/wupdlock.h>

#include <algorithm>


namespace
{

// how much to darken the other color in shaded list (this value
// is what GTK+ uses in its tree view control)
#define DARKEN_FACTOR      0.95

// max difference in color to consider it "amost" same if it differs by most
// this amount (in 0..255 range) from the tested color:
#define COLOR_SIMILARITY_FACTOR  20

inline bool IsAlmostBlack(const wxColour& clr)
{
    return (clr.Red() <= COLOR_SIMILARITY_FACTOR &&
            clr.Green() <= COLOR_SIMILARITY_FACTOR &&
            clr.Blue() <= COLOR_SIMILARITY_FACTOR);
}

inline bool IsAlmostWhite(const wxColour& clr)
{
    return (clr.Red() >= 255-COLOR_SIMILARITY_FACTOR &&
            clr.Green() >= 255-COLOR_SIMILARITY_FACTOR &&
            clr.Blue() >= 255-COLOR_SIMILARITY_FACTOR);
}


// colours used in the list:

const wxColour gs_ErrorColor("#ff5050");

// colors for white list control background
const wxColour gs_UntranslatedForWhite("#103f67");
const wxColour gs_FuzzyForWhite("#a9861b");

// ditto for black background
const wxColour gs_UntranslatedForBlack("#1962a0");
const wxColour gs_FuzzyForBlack("#a9861b");


const wxColour gs_TranspColor(254, 0, 253); // FIXME: get rid of this

enum
{
    IMG_NOTHING,
    IMG_AUTOMATIC,
    IMG_COMMENT,
    IMG_BOOKMARK
};

} // anonymous namespace


BEGIN_EVENT_TABLE(PoeditListCtrl, wxListCtrl)
   EVT_SIZE(PoeditListCtrl::OnSize)
END_EVENT_TABLE()

PoeditListCtrl::PoeditListCtrl(wxWindow *parent,
               wxWindowID id,
               const wxPoint &pos,
               const wxSize &size,
               long style,
               bool dispIDs,
               const wxValidator& validator,
               const wxString &name)
     : wxListView(parent, id, pos, size, style | wxLC_VIRTUAL, validator, name)
{
    m_catalog = NULL;
    m_displayIDs = dispIDs;

    m_isRTL = false;
    m_appIsRTL = (wxTheApp->GetLayoutDirection() == wxLayout_RightToLeft);

    sortOrder = SortOrder::Default();

    CreateColumns();

    wxImageList *list = new wxImageList(16, 16);

    // IMG_XXX:
    list->Add(wxArtProvider::GetBitmap("poedit-status-nothing"));
    list->Add(wxArtProvider::GetBitmap("poedit-status-automatic"));
    list->Add(wxArtProvider::GetBitmap("poedit-status-comment"));
    list->Add(wxArtProvider::GetBitmap("poedit-status-bookmark"));

    AssignImageList(list, wxIMAGE_LIST_SMALL);

    // configure items colors & fonts:

    wxVisualAttributes visual = GetDefaultAttributes();
    wxColour shaded = visual.colBg;

#ifdef __WXMSW__
    // On Windows 7, shaded list items make it impossible to see the selection,
    // so use different color for it (see bug #336).
    int verMaj, verMin;
    wxGetOsVersion(&verMaj, &verMin);
    if ( verMaj > 6 || (verMaj == 6 && verMin >= 1) )
    {
        shaded.Set(int(0.99 * shaded.Red()),
                   int(0.99 * shaded.Green()),
                   shaded.Blue());
    }
    else
#endif // __WXMSW__
#ifdef __WXOSX__
    if ( shaded == *wxWHITE )
    {
        // use standard shaded color from finder/databrowser:
        shaded.Set("#f0f5fd");
    }
    else
#endif // __WXOSX__
    {
        shaded.Set(int(DARKEN_FACTOR * shaded.Red()),
                   int(DARKEN_FACTOR * shaded.Green()),
                   int(DARKEN_FACTOR * shaded.Blue()));
    }

    m_attrNormal[1].SetBackgroundColour(shaded);
    m_attrUntranslated[1].SetBackgroundColour(shaded);
    m_attrFuzzy[1].SetBackgroundColour(shaded);
    m_attrInvalid[1].SetBackgroundColour(shaded);

    // FIXME: make this user-configurable
    if ( IsAlmostWhite(visual.colBg) )
    {
        m_attrUntranslated[0].SetTextColour(gs_UntranslatedForWhite);
        m_attrUntranslated[1].SetTextColour(gs_UntranslatedForWhite);
        m_attrFuzzy[0].SetTextColour(gs_FuzzyForWhite);
        m_attrFuzzy[1].SetTextColour(gs_FuzzyForWhite);
    }
    else if ( IsAlmostBlack(visual.colBg) )
    {
        m_attrUntranslated[0].SetTextColour(gs_UntranslatedForBlack);
        m_attrUntranslated[1].SetTextColour(gs_UntranslatedForBlack);
        m_attrFuzzy[0].SetTextColour(gs_FuzzyForBlack);
        m_attrFuzzy[1].SetTextColour(gs_FuzzyForBlack);
    }
    // else: we don't know if the default colors would be well-visible on
    //       user's background color, so play it safe and don't highlight
    //       anything

    // FIXME: todo; use appropriate font for fuzzy/trans/untrans
    m_attrInvalid[0].SetBackgroundColour(gs_ErrorColor);
    m_attrInvalid[1].SetBackgroundColour(gs_ErrorColor);

    // Use gray for IDs
    if ( IsAlmostBlack(visual.colFg) )
        m_attrId.SetTextColour(wxColour("#A1A1A1"));

    SetCustomFont(wxNullFont);
}

PoeditListCtrl::~PoeditListCtrl()
{
    sortOrder.Save();
}

void PoeditListCtrl::SetCustomFont(wxFont font_)
{
    wxFont font(font_);

    if ( !font.IsOk() )
        font = GetDefaultAttributes().font;

    m_attrNormal[0].SetFont(font);
    m_attrNormal[1].SetFont(font);

    wxFont fontb = font;
    fontb.SetWeight(wxFONTWEIGHT_BOLD);

    m_attrUntranslated[0].SetFont(fontb);
    m_attrUntranslated[1].SetFont(fontb);

    m_attrFuzzy[0].SetFont(fontb);
    m_attrFuzzy[1].SetFont(fontb);

    m_attrInvalid[0].SetFont(fontb);
    m_attrInvalid[1].SetFont(fontb);
}

void PoeditListCtrl::SetDisplayLines(bool dl)
{
    m_displayIDs = dl;
    CreateColumns();
}

void PoeditListCtrl::CreateColumns()
{
    DeleteAllColumns();
    InsertColumn(0, _("Source text"));
    InsertColumn(1, _("Translation"));
    if (m_displayIDs)
        InsertColumn(2, _("ID"), wxLIST_FORMAT_RIGHT);

#ifdef __WXMSW__
    if (m_appIsRTL)
    {
        // another wx quirk: if we truly need left alignment, we must lie under RTL locales
        wxListItem colInfoOrig;
        colInfoOrig.SetAlign(wxLIST_FORMAT_RIGHT);
        SetColumn(0, colInfoOrig);
    }
#endif

    SizeColumns();
}

void PoeditListCtrl::SizeColumns()
{
    const int LINE_COL_SIZE = m_displayIDs ? 50 : 0;

    int w = GetSize().x
            - wxSystemSettings::GetMetric(wxSYS_VSCROLL_X) - 10
            - LINE_COL_SIZE;
    SetColumnWidth(0, w / 2);
    SetColumnWidth(1, w - w / 2);
    if (m_displayIDs)
        SetColumnWidth(2, LINE_COL_SIZE);

    m_colWidth = (w/2) / GetCharWidth();
}


void PoeditListCtrl::CatalogChanged(Catalog* catalog)
{
    wxWindowUpdateLocker no_updates(this);

    const bool isSameCatalog = (catalog == m_catalog);

    // this is to prevent crashes (wxMac at least) when shortening virtual
    // listctrl when its scrolled to the bottom:
    m_catalog = nullptr;
    SetItemCount(0);

    std::vector<int> selection;
    if (isSameCatalog)
        selection = GetSelectedCatalogItems();

    // now read the new catalog:
    m_catalog = catalog;
    ReadCatalog();

    if (isSameCatalog && !selection.empty())
        SetSelectedCatalogItems(selection);
}

void PoeditListCtrl::ReadCatalog()
{
    wxWindowUpdateLocker no_updates(this);

    // clear the list and its sort order too:
    SetItemCount(0);
    m_mapListToCatalog.clear();
    m_mapCatalogToList.clear();

    if (m_catalog == NULL)
    {
        Refresh();
        return;
    }

    auto lang = m_catalog->GetLanguage();
    auto isRTL = lang.IsRTL();
#ifdef __WXMSW__
    // a quirk of wx API: if the current locale is RTL, the meaning of L and R is reversed
    if (m_appIsRTL)
        isRTL = !isRTL;
#endif
    m_isRTL = isRTL;

    wxString langname = lang.IsValid() ? lang.DisplayName() : _("unknown language");
    wxListItem colInfo;
    colInfo.SetMask(wxLIST_MASK_TEXT);
    colInfo.SetText(wxString::Format(_(L"Translation — %s"), langname));
    colInfo.SetAlign(isRTL ? wxLIST_FORMAT_RIGHT : wxLIST_FORMAT_LEFT);
    SetColumn(1, colInfo);

    // sort catalog items, create indexes mapping
    CreateSortMap();

    // now that everything is prepared, we may set the item count
    SetItemCount(m_catalog->GetCount());

    // scroll to the top and refresh everything:
    if ( m_catalog->GetCount() )
    {
        SelectOnly(0);
        RefreshItems(0, m_catalog->GetCount()-1);
    }
    else
    {
        Refresh();
    }
}


void PoeditListCtrl::Sort()
{
    if ( m_catalog && m_catalog->GetCount() )
    {
        auto sel = GetSelectedCatalogItems();

        CreateSortMap();
        RefreshItems(0, m_catalog->GetCount()-1);

        SetSelectedCatalogItems(sel);
    }
    else
    {
        Refresh();
    }
}


void PoeditListCtrl::CreateSortMap()
{
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


wxString PoeditListCtrl::OnGetItemText(long item, long column) const
{
    if (m_catalog == NULL)
        return wxEmptyString;

    const CatalogItem& d = ListIndexToCatalogItem((int)item);

    switch (column)
    {
        case 0:
        {
            wxString orig;
            if ( d.HasContext() )
                orig.Printf("%s  [ %s ]",
                            d.GetString().c_str(), d.GetContext().c_str());
            else
                orig = d.GetString();

            // Add RTL Unicode mark to render bidi texts correctly
            if (m_appIsRTL)
                return L"\u202a" + orig.substr(0,GetMaxColChars());
            else
                return orig.substr(0,GetMaxColChars());
        }
        case 1:
        {
            // Add RTL Unicode mark to render bidi texts correctly
            if (m_isRTL && !m_appIsRTL)
                return L"\u202b" + d.GetTranslation();
            else
                return d.GetTranslation();
        }
        case 2:
            return wxString() << d.GetId();

        default:
            return wxEmptyString;
    }
}

wxListItemAttr *PoeditListCtrl::OnGetItemAttr(long item) const
{
    long idx = item % 2;

    if (m_catalog == NULL)
        return (wxListItemAttr*)&m_attrNormal[idx];

    const CatalogItem& d = ListIndexToCatalogItem((int)item);

    if (d.GetValidity() == CatalogItem::Val_Invalid)
        return (wxListItemAttr*)&m_attrInvalid[idx];
    else if (!d.IsTranslated())
        return (wxListItemAttr*)&m_attrUntranslated[idx];
    else if (d.IsFuzzy())
        return (wxListItemAttr*)&m_attrFuzzy[idx];
    else
        return (wxListItemAttr*)&m_attrNormal[idx];
}

wxListItemAttr *PoeditListCtrl::OnGetItemColumnAttr(long item, long column) const
{
    if (column == 2)
    {
        return (wxListItemAttr*)&m_attrId;
    }
    else
    {
        return OnGetItemAttr(item);
    }
}

int PoeditListCtrl::OnGetItemImage(long item) const
{
    if (m_catalog == NULL)
        return IMG_NOTHING;

    const CatalogItem& d = ListIndexToCatalogItem((int)item);

    if (d.GetBookmark() != NO_BOOKMARK)
        return IMG_BOOKMARK;
    else if (d.HasComment() || d.HasExtractedComments())
        return IMG_COMMENT;
    else if (d.IsAutomatic())
        return IMG_AUTOMATIC;
    else
        return IMG_NOTHING;
}

void PoeditListCtrl::OnSize(wxSizeEvent& event)
{
    wxWindowUpdateLocker lock(this);

    SizeColumns();
    event.Skip();
}