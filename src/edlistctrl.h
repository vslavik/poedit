/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2014 Vaclav Slavik
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

#ifndef _ED_LIST_CTRL_H_
#define _ED_LIST_CTRL_H_

#include <wx/listctrl.h>
#include <wx/frame.h>

#include <vector>

class WXDLLIMPEXP_FWD_CORE wxListCtrl;
class WXDLLIMPEXP_FWD_CORE wxListEvent;

#include "catalog.h"
#include "cat_sorting.h"

// list control with both columns equally wide:
class PoeditListCtrl : public wxListView
{
    public:
        PoeditListCtrl(wxWindow *parent,
                       wxWindowID id = -1,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize,
                       long style = wxLC_ICON,
                       bool dispIDs = false,
                       const wxValidator& validator = wxDefaultValidator,
                       const wxString &name = "listctrl");

        virtual ~PoeditListCtrl();

        /// Re-sort the control according to user-specified criteria.
        void Sort();

        void SizeColumns();

        void SetDisplayLines(bool dl);

        void CatalogChanged(Catalog* catalog);

        virtual wxString OnGetItemText(long item, long column) const;
        virtual wxListItemAttr *OnGetItemAttr(long item) const;
        virtual wxListItemAttr *OnGetItemColumnAttr(long item, long column) const;
        virtual int OnGetItemImage(long item) const;

        /// Returns the list item index for the given catalog index
        int CatalogIndexToList(int index) const
        {
            if ( index < 0 || index >= (int)m_mapCatalogToList.size() )
                return index;
            else
                return m_mapCatalogToList[index];
        }

        /// Returns item's index in the catalog
        int ListIndexToCatalog(int index) const
        {
            if ( index < 0 || index >= (int)m_mapListToCatalog.size() )
                return index;
            else
                return m_mapListToCatalog[index];
        }

        /// Returns item from the catalog based on list index
        const CatalogItem& ListIndexToCatalogItem(int index) const
        {
            return (*m_catalog)[ListIndexToCatalog(index)];
        }


        /// Returns current list selection (as list item index)
        int GetSelection() const { return (int)GetFirstSelected(); }

        /// Returns index of selected catalog item
        int GetSelectedCatalogItem() const
        {
            return ListIndexToCatalog(GetSelection());
        }

        /// Selects given catalog item
        void SelectCatalogItem(int catalogIndex)
        {
            Select(CatalogIndexToList(catalogIndex));
        }

        void Select(long n, bool on = true)
        {
#ifdef __WXMAC__
            SetItemState(GetSelection(), 0, wxLIST_STATE_SELECTED);
#endif
            wxListView::Select(n, on);
            EnsureVisible(n);
        }

        void SetCustomFont(wxFont font);

        // Order used for sorting
        SortOrder sortOrder;

    private:
        // Returns average width of one column in number of characters:
        size_t GetMaxColChars() const
        {
            return m_colWidth * 2/*safety coefficient*/;
        }

        void CreateSortMap();
        void CreateColumns();
        void ReadCatalog();
        void OnSize(wxSizeEvent& event);

        bool m_displayIDs;
        unsigned m_colWidth;
        bool m_isRTL, m_appIsRTL;

        Catalog* m_catalog;

        std::vector<int> m_mapListToCatalog;
        std::vector<int> m_mapCatalogToList;

        wxListItemAttr m_attrId;
        wxListItemAttr m_attrNormal[2];
        wxListItemAttr m_attrUntranslated[2];
        wxListItemAttr m_attrFuzzy[2];
        wxListItemAttr m_attrInvalid[2];

        DECLARE_EVENT_TABLE()
};

#endif // _ED_LIST_CTRL_H_
