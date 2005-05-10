
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edlistctrl.cpp
    
      List view control
    
      (c) Vaclav Slavik, 1999-2004
      (c) Olivier Sannier, 2005

*/

#include <wx/wx.h>
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/dcmemory.h>
#include <wx/image.h>

#include "edlistctrl.h"
#include "digits.h"

// I don't like this global flag, but all poEditFrame instances should share it :(
bool gs_shadedList = false;

// colours used in the list:
#define g_darkColourFactor 0.95
#define DARKEN_COLOUR(r,g,b) (wxColour(int((r)*g_darkColourFactor),\
                                       int((g)*g_darkColourFactor),\
                                       int((b)*g_darkColourFactor)))
#define LIST_COLOURS(r,g,b) { wxColour(r,g,b), DARKEN_COLOUR(r,g,b) }
static const wxColour 
    g_ItemColourNormal[2] =       LIST_COLOURS(0xFF,0xFF,0xFF), // white
    g_ItemColourUntranslated[2] = LIST_COLOURS(0xA5,0xEA,0xEF), // blue
    g_ItemColourFuzzy[2] =        LIST_COLOURS(0xF4,0xF1,0xC1), // yellow
    g_ItemColourInvalid[2] =      LIST_COLOURS(0xFF,0x20,0x20); // red
    
static const wxColour g_TranspColour(254, 0, 253);
    
enum
{
    IMG_NOTHING   = 0x00,
    IMG_AUTOMATIC = 0x01,
    IMG_COMMENT   = 0x02,
    IMG_MODIFIED  = 0x04,
    IMG_BK0       =  1 << 3,
    IMG_BK1       =  2 << 3,
    IMG_BK2       =  3 << 3,
    IMG_BK3       =  4 << 3,
    IMG_BK4       =  5 << 3,
    IMG_BK5       =  6 << 3,
    IMG_BK6       =  7 << 3,
    IMG_BK7       =  8 << 3,
    IMG_BK8       =  9 << 3,
    IMG_BK9       = 10 << 3
};    
    
BEGIN_EVENT_TABLE(poEditListCtrl, wxListCtrl)
   EVT_SIZE(poEditListCtrl::OnSize)
END_EVENT_TABLE()


static inline wxString convertToLocalCharset(const wxString& str)
{
#if !wxUSE_UNICODE
    wxString s2(str.wc_str(wxConvUTF8), wxConvLocal);
    if (s2.empty() /*conversion failed*/)
        return str;
    else
        return s2;
#else
    return str;
#endif
}

wxBitmap AddDigit(char digit, int x, int y, const wxBitmap& bmp)
{
    wxMemoryDC dc;
    int width = bmp.GetWidth();
    int height = bmp.GetHeight();
    wxBitmap tmpBmp(width, height);
    dc.SelectObject(tmpBmp);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(g_TranspColour, wxSOLID));
    dc.DrawRectangle(0, 0, width, height);
    
    dc.DrawBitmap(bmp, 0,0,true);
    
    dc.SetPen(*wxBLACK_PEN);
    for(int i = 0; i < 5; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            if (g_digits[digit][i][j] == 1)
                dc.DrawPoint(x+j, y+i);
        }
    }
    
    dc.SelectObject(wxNullBitmap);
    tmpBmp.SetMask(new wxMask(tmpBmp, g_TranspColour));
    return tmpBmp;
}    

wxBitmap MergeBitmaps(const wxBitmap& bmp1, const wxBitmap& bmp2)
{
    wxMemoryDC dc;
/*    wxBitmap tmpBmp(bmp1);  // can't simply remove the mask as there is now way
    tmpBmp.SetMask(NULL);*/   // to predict which background color will be used
    wxBitmap tmpBmp(bmp1.GetWidth(), bmp1.GetHeight());
    
    dc.SelectObject(tmpBmp);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(g_TranspColour, wxSOLID));
    dc.DrawRectangle(0, 0, bmp1.GetWidth(), bmp1.GetHeight());
    dc.DrawBitmap(bmp1, 0, 0, true);
    dc.DrawBitmap(bmp2, 0, 0, true);
    dc.SelectObject(wxNullBitmap);  // detach tmpBmp
    
    tmpBmp.SetMask(new wxMask(tmpBmp, g_TranspColour));
    return tmpBmp;
}

wxBitmap BitmapFromList(wxImageList* list, int index)
{
    int width, height;
    list->GetSize(index, width, height);
    wxMemoryDC dc;
    wxBitmap bmp(width, height);  // Don't forget the size to have a valid bitmap
    dc.SelectObject(bmp);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(g_TranspColour, wxSOLID));
    dc.DrawRectangle(0, 0, width, height);
    
    list->Draw(index, dc, 0, 0, wxIMAGELIST_DRAW_TRANSPARENT);
    
    dc.SelectObject(wxNullBitmap);  // detach bmp
    bmp.SetMask(new wxMask(bmp, g_TranspColour));
    return bmp;
}    

poEditListCtrl::poEditListCtrl(wxWindow *parent,
               wxWindowID id,
               const wxPoint &pos,
               const wxSize &size,
               long style,
               bool dispLines,
               const wxValidator& validator,
               const wxString &name)
     : wxListView(parent, id, pos, size, style | wxLC_VIRTUAL, validator, name)
{
    m_catalog = NULL;
    m_displayLines = dispLines;
    CreateColumns();
    
    int i;
    wxImageList *list = new wxImageList(16, 16);
    list->Add(wxArtProvider::GetBitmap(_T("poedit-status-nothing")));                // IMG_NOTHING
    
    list->Add(wxArtProvider::GetBitmap(_T("poedit-status-automatic")));              // IMG_AUTOMATIC
    list->Add(wxArtProvider::GetBitmap(_T("poedit-status-comment")));                // IMG_COMMENT
    list->Add(MergeBitmaps(wxArtProvider::GetBitmap(_T("poedit-status-automatic")), 
                           wxArtProvider::GetBitmap(_T("poedit-status-comment"))));  // IMG_AUTOMATIC | IMG_COMMENT
    list->Add(wxArtProvider::GetBitmap(_T("poedit-status-modified")));               // IMG_MODIFIED 
    for(i = 1; i < IMG_MODIFIED; i++)                                                // IMG_MODIFIED variations
    {
        list->Add(MergeBitmaps(BitmapFromList(list, i),
                               wxArtProvider::GetBitmap(_T("poedit-status-modified"))));
    }
    
    for(int bk = 0; bk < 10; bk++)                                                   // BK_XX variations
    {
        for(i = 0; i <= (IMG_AUTOMATIC|IMG_COMMENT|IMG_MODIFIED); i++)                       
        {
            wxBitmap bmp = BitmapFromList(list, i);
            list->Add(AddDigit(bk, 0, 0, bmp));
        }    
    }    

    AssignImageList(list, wxIMAGE_LIST_SMALL);
}

poEditListCtrl::~poEditListCtrl()
{
}    
        
void poEditListCtrl::CreateColumns()
{
    ClearAll();
    InsertColumn(0, _("Original string"));
    InsertColumn(1, _("Translation"));
    if (m_displayLines)
        InsertColumn(2, _("Line"), wxLIST_FORMAT_RIGHT);
    ReadCatalog();
    SizeColumns();
}

void poEditListCtrl::SizeColumns()
{
    const int LINE_COL_SIZE = m_displayLines ? 50 : 0;

    int w = GetSize().x
            - wxSystemSettings::GetSystemMetric(wxSYS_VSCROLL_X) - 10
            - LINE_COL_SIZE;
    SetColumnWidth(0, w / 2);
    SetColumnWidth(1, w - w / 2);
    if (m_displayLines)
        SetColumnWidth(2, LINE_COL_SIZE);

    m_colWidth = (w/2) / GetCharWidth();
}


void poEditListCtrl::SetCatalog(Catalog* catalog)
{
    m_catalog = catalog;
    ReadCatalog();
}

void poEditListCtrl::ReadCatalog()
{
    if (m_catalog != NULL)
    {
        // set the item count
        SetItemCount(m_catalog->GetCount());
        
        // create the lookup arrays of Ids by first splitting it upon
        // four categories of items:
        // unstranslated, invalid, fuzzy and the rest
        m_itemIndexToCatalogIndexArray.Clear();
        
        wxArrayInt untranslatedIds;
        wxArrayInt invalidIds;
        wxArrayInt fuzzyIds;
        wxArrayInt restIds;
        int i;
        
        for(i = 0; i < m_catalog->GetCount(); i++)
        {
            CatalogData& d = (*m_catalog)[i];
            if (!d.IsTranslated())
              untranslatedIds.Add(i);
            else if (d.GetValidity() == CatalogData::Val_Invalid)
              invalidIds.Add(i);
            else if (d.IsFuzzy())
              fuzzyIds.Add(i);
            else
              restIds.Add(i);
        }    
        
        // Now fill the lookup array, not forgetting to set the appropriate
        // property in the catalog entry to be able to go back and forth
        // from one numbering system to the other
        m_itemIndexToCatalogIndexArray.Alloc(m_catalog->GetCount());
        m_catalogIndexToItemIndexArray.SetCount(m_catalog->GetCount());
        int listItemId = 0;
        for(i = 0; i < untranslatedIds.Count(); i++)
        {
            m_itemIndexToCatalogIndexArray.Add(untranslatedIds[i]);
            m_catalogIndexToItemIndexArray[untranslatedIds[i]] = listItemId++;
        }    
        for(i = 0; i < invalidIds.Count(); i++)
        {
            m_itemIndexToCatalogIndexArray.Add(invalidIds[i]);
            m_catalogIndexToItemIndexArray[invalidIds[i]] = listItemId++;
        }    
        for(i = 0; i < fuzzyIds.Count(); i++)
        {
            m_itemIndexToCatalogIndexArray.Add(fuzzyIds[i]);
            m_catalogIndexToItemIndexArray[fuzzyIds[i]] = listItemId++;
        }    
        for(i = 0; i < restIds.Count(); i++)
        {
            m_itemIndexToCatalogIndexArray.Add(restIds[i]);
            m_catalogIndexToItemIndexArray[restIds[i]] = listItemId++;
//            (*m_catalog)[restIds[i]].SetListItemId(listItemId++);
        }    
    }    
}    

wxString poEditListCtrl::OnGetItemText(long item, long column) const
{
    if (m_catalog == NULL)
        return wxEmptyString;
    
    CatalogData& d = (*m_catalog)[m_itemIndexToCatalogIndexArray[item]];
    switch (column)
    {
        case 0: 
        {
            wxString orig = convertToLocalCharset(d.GetString());
            return orig.substr(0,GetMaxColChars()); 
            break;
        }    
        case 1: 
        {
            wxString trans = convertToLocalCharset(d.GetTranslation());
            return trans; 
            break;
        }    
        case 2: 
        {
            return wxString() << d.GetLineNumber(); 
            break;
        }    
        default: 
            return wxEmptyString; 
            break;
    } 
}  

// this variable can't be a member of the class as the OnGetItemAttr function is const
static wxListItemAttr g_attr;  
wxListItemAttr * poEditListCtrl::OnGetItemAttr(long item) const
{
    CatalogData& d = (*m_catalog)[m_itemIndexToCatalogIndexArray[item]];
    if (gs_shadedList)
    {
        if (!d.IsTranslated())
            g_attr.SetBackgroundColour(g_ItemColourUntranslated[item % 2]);
        else if (d.IsFuzzy())
            g_attr.SetBackgroundColour(g_ItemColourFuzzy[item % 2]);
        else if (d.GetValidity() == CatalogData::Val_Invalid)
            g_attr.SetBackgroundColour(g_ItemColourInvalid[item % 2]);
        else
            g_attr.SetBackgroundColour(g_ItemColourNormal[item % 2]);
    }
    else
    {
        if (!d.IsTranslated())
            g_attr.SetBackgroundColour(g_ItemColourUntranslated[0]);
        else if (d.IsFuzzy())
            g_attr.SetBackgroundColour(g_ItemColourFuzzy[0]);
        else if (d.GetValidity() == CatalogData::Val_Invalid)
            g_attr.SetBackgroundColour(g_ItemColourInvalid[0]);
        else
            g_attr.SetBackgroundColour(g_ItemColourNormal[0]);
    }
    
    return &g_attr;
}    

int poEditListCtrl::OnGetItemImage(long item) const
{
    CatalogData& d = (*m_catalog)[m_itemIndexToCatalogIndexArray[item]];
    int index = IMG_NOTHING;
    
    if (d.IsAutomatic())
        index |= IMG_AUTOMATIC;
    if (d.HasComment())
        index |= IMG_COMMENT;
    if (d.IsModified())
        index |= IMG_MODIFIED;
    
    index |= (static_cast<int>(d.GetBookmark())+1) << 3;
        
    return index;
}
    
void poEditListCtrl::OnSize(wxSizeEvent& event)
{
    SizeColumns();
    event.Skip();
}

long poEditListCtrl::GetItemData(long item) const
{
    if (item < m_itemIndexToCatalogIndexArray.Count())
        return m_itemIndexToCatalogIndexArray[item];
    else
        return -1;
}    

int poEditListCtrl::GetItemIndex(int catalogIndex) const
{
    if (catalogIndex < m_catalogIndexToItemIndexArray.Count())
        return m_catalogIndexToItemIndexArray[catalogIndex];
    else
        return -1;
}



