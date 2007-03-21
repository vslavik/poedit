/*
 *  This file is part of poEdit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2005 Vaclav Slavik
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
 *  $Id$
 *
 *  List view control
 *
 */

#ifndef _ED_LIST_CTRL_H_
#define _ED_LIST_CTRL_H_

#include <wx/listctrl.h>
#include <wx/frame.h>

#include <vector>

class WXDLLEXPORT wxListCtrl;
class WXDLLEXPORT wxListEvent;

#include "catalog.h"

extern bool gs_shadedList;

// list control with both columns equally wide:
class poEditListCtrl : public wxListView
{
    public:
        poEditListCtrl(wxWindow *parent,
                       wxWindowID id = -1,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize,
                       long style = wxLC_ICON,
                       bool dispLines = false,
                       const wxValidator& validator = wxDefaultValidator,
                       const wxString &name = _T("listctrl"));

        virtual ~poEditListCtrl();

        void CreateColumns();

        void SizeColumns();

        void SetDisplayLines(bool dl) { m_displayLines = dl; }

        // Returns average width of one column in number of characters:
        size_t GetMaxColChars() const
        {
            return m_colWidth * 2/*safety coefficient*/;
        }

        void CatalogChanged(Catalog* catalog);

        virtual wxString OnGetItemText(long item, long column) const;
        virtual wxListItemAttr * OnGetItemAttr(long item) const;
        virtual int OnGetItemImage(long item) const;

        /// Returns item's index in the catalog
        long GetIndexInCatalog(long item) const;

        /// Returns the list item index for the given catalog index
        int GetItemIndex(int catalogIndex) const;

    private:
        void OnSize(wxSizeEvent& event);

        void ReadCatalog();

        bool m_displayLines;
        unsigned m_colWidth;

        Catalog* m_catalog;

        std::vector<int> m_itemIndexToCatalogIndexArray;
        std::vector<int> m_catalogIndexToItemIndexArray;

        wxListItemAttr m_attrNormal[2];
        wxListItemAttr m_attrUntranslated[2];
        wxListItemAttr m_attrFuzzy[2];
        wxListItemAttr m_attrInvalid[2];

        DECLARE_EVENT_TABLE()
};

#endif // _ED_LIST_CTRL_H_
