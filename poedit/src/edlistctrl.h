
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edlistctrl.h
    
      List view control
    
      (c) Vaclav Slavik, 1999-2004
      (c) Olivier Sannier, 2005

*/

#ifndef _ED_LIST_CTRL_H_
#define _ED_LIST_CTRL_H_

#include <wx/listctrl.h>
#include <wx/frame.h>

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
        
        void SetCatalog(Catalog* catalog);
        
        virtual wxString OnGetItemText(long item, long column) const;
        virtual wxListItemAttr * OnGetItemAttr(long item) const;
        virtual int OnGetItemImage(long item) const;
        
        long GetItemData(long item) const;
        
    private:
        DECLARE_EVENT_TABLE()
        void OnSize(wxSizeEvent& event);
        
        void ReadCatalog();

        bool m_displayLines;
        unsigned m_colWidth;
        
        Catalog* m_catalog;
        
        wxArrayInt m_itemIndexToCatalogIndexArray;
};

#endif // _ED_LIST_CTRL_H_
