/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2013 Vaclav Slavik
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

#ifndef _FILEVIEWER_H_
#define _FILEVIEWER_H_

#include <wx/frame.h>

class WXDLLIMPEXP_FWD_CORE wxListCtrl;
class WXDLLIMPEXP_FWD_CORE wxStyledTextCtrl;
class WXDLLIMPEXP_FWD_CORE wxFileName;


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
               const wxArrayString& references, int openAt);
    ~FileViewer();

    /// Shows given reference, i.e. loads the file
    void ShowReference(const wxString& ref);

    bool FileOk() { return !m_current.empty(); }

private:
    void SetupTextCtrl();
    int GetLexer(const wxString& extension);
    wxFileName GetFilename(wxString ref) const;

private:
    wxString m_basePath;
    wxArrayString m_references;
    wxString m_current;

    wxStyledTextCtrl *m_text;

    void OnChoice(wxCommandEvent &event);
    void OnEditFile(wxCommandEvent &event);
};

#endif // _FILEVIEWER_H_
