
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      findframe.h
    
      Search frame
    
      (c) Vaclav Slavik, 2001

*/

#ifndef _FINDFRAME_H_
#define _FINDFRAME_H_

#include <wx/dialog.h>
class WXDLLEXPORT wxListCtrl;
class WXDLLEXPORT wxButton;
class Catalog;

/** FindFrame is small dialog frame that contains controls for searching
    in content of EditorFrame's wxListCtrl object and associated Catalog
    instance.
    
    This class assumes that list control's user data contains index
    into the catalog.
 */
class FindFrame : public wxDialog
{
    public:
        /** Ctor. 
            \param parent  Parent frame, FindFrame will float on it
            \param list    List control to search in
            \param catalog Catalog to search in
         */
        FindFrame(wxWindow *parent, wxListCtrl *list, Catalog *c);
        ~FindFrame();
        
        /** Resets the search to starting position and changes
            the catalog in use. Called by EditorFrame when the user
            reloads catalog.
         */
        void Reset(Catalog *c);

    private:
        void OnCancel(wxCommandEvent &event);
        void OnClose(wxCloseEvent &event);
        void OnPrev(wxCommandEvent &event);
        void OnNext(wxCommandEvent &event);
        void OnTextChange(wxCommandEvent &event);
        void OnCheckbox(wxCommandEvent &event);
        bool DoFind(int dir);
        DECLARE_EVENT_TABLE()
    
        wxListCtrl *m_listCtrl;
        Catalog *m_catalog;
        int m_position;
        wxString m_text;
        wxButton *m_btnPrev, *m_btnNext;
};


#endif // _FINDFRAME_H_
