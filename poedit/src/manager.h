
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      manager.h
    
      Catalogs manager frame
    
      (c) Vaclav Slavik, 2001,2004

*/

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <wx/frame.h>
#include <wx/string.h>

class WXDLLEXPORT wxListCtrl;
class WXDLLEXPORT wxListBox;
class Catalog;

/** ManagerFrame provides a convenient way to manage PO catalogs.
    The frame contains two lists: a list of projects and list of catalogs
    in active project, together with their statistics.
 */
class ManagerFrame : public wxFrame
{
    public:
        /// Creates instance of manager or returns pointer to existing one.
        static ManagerFrame* Create();
        /** Returns pointer to existing instance or NULL if there's no one. 
            (I.e. unlike Create, this one doesn't create a new instance.)
         */
        static ManagerFrame* Get() { return ms_instance; }
        
        /** Used to notify the manager that one of files changed and it
            has to update the list control.
         */
        void NotifyFileChanged(const wxString& catalog);

    private:
        ManagerFrame();
        ~ManagerFrame();

        /** Pops up project settings dialog for project #id.
            \return false if user pressed Cancel, true otherwise
         */
        bool EditProject(int id);
        /// Deletes project
        void DeleteProject(int id);
        /** Updates projects list
            \param select id of project to be selected
         */
        void UpdateListPrj(int select = 0);
        /// Updates catalogs list for given project
        void UpdateListCat(int id = -1);
        
        DECLARE_EVENT_TABLE()
        void OnNewProject(wxCommandEvent& event);
        void OnEditProject(wxCommandEvent& event);
        void OnDeleteProject(wxCommandEvent& event);
        void OnUpdateProject(wxCommandEvent& event);
        void OnSelectProject(wxCommandEvent& event);
        void OnOpenCatalog(wxListEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);

        wxListCtrl *m_listCat;
        wxListBox  *m_listPrj;      
        wxArrayString m_catalogs;
        int m_curPrj;

        static ManagerFrame *ms_instance;
};


#endif // _EDFRAME_H_
