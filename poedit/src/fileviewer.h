
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      fileviewer.h
    
      Shows part of file around specified line
    
      (c) Vaclav Slavik, 2000, 2004

*/


#if defined(__GNUG__) && !defined(__APPLE__)
#pragma interface
#endif

#ifndef _FILEVIEWER_H_
#define _FILEVIEWER_H_

#include <wx/frame.h>
class WXDLLEXPORT wxListCtrl;


/** This class implements frame that shows part of file
    surrounding specified line (40 lines in both directions).
 */
class FileViewer : public wxFrame
{
    public:
        /** Ctor. 
            \param basePath   base directory that all entries in 
                              \i references are relative to
            \param references array of strings in format \i filename:linenum
                              that lists all occurences of given string
            \param openAt     number of the \i references entry to show
                              by default
         */
        FileViewer(wxWindow *parent, const wxString& basePath,
                   const wxArrayString& references, size_t openAt);
        ~FileViewer();
        
        /// Shows given reference, i.e. loads the file
        void ShowReference(const wxString& ref);
        
        /// Open file in an editor
        static void OpenInEditor(const wxString& basepath, 
                                 const wxString& reference);

        bool FileOk() { return !m_current.empty(); }
        
    private:
        wxString m_basePath;
        wxArrayString m_references;
        wxString m_current;
        
        wxListCtrl *m_list;
    
        void OnChoice(wxCommandEvent &event);
        void OnEditFile(wxCommandEvent &event);
        DECLARE_EVENT_TABLE();
};



#endif // _FILEVIEWER_H_
