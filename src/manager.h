/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2001-2017 Vaclav Slavik
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

#ifndef _MANAGER_H_
#define _MANAGER_H_

#include <wx/frame.h>
#include <wx/string.h>

class WXDLLIMPEXP_FWD_CORE wxListCtrl;
class WXDLLIMPEXP_FWD_CORE wxListBox;
class WXDLLIMPEXP_FWD_CORE wxSplitterWindow;

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
        template<typename TFunctor>
        void EditProject(int id, TFunctor completionHandler);
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
        void OnCloseCmd(wxCommandEvent& event);

        wxListCtrl *m_listCat;
        wxListBox  *m_listPrj;
        wxSplitterWindow *m_splitter;
        wxArrayString m_catalogs;
        int m_curPrj;

        static ManagerFrame *ms_instance;
};


#endif // _EDFRAME_H_
