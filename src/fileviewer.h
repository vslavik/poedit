/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2021 Vaclav Slavik
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

#include "catalog.h"

#include <wx/frame.h>

#include <memory>

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxChoice;
class WXDLLIMPEXP_FWD_CORE wxStaticText;
class WXDLLIMPEXP_FWD_CORE wxFileName;
class WXDLLIMPEXP_FWD_CORE wxWebView;
class WXDLLIMPEXP_FWD_CORE wxSizer;


/** This class implements frame that shows part of file
    surrounding specified line (40 lines in both directions).
 */
class FileViewer : public wxFrame
{
protected:
    FileViewer(wxWindow *parent);
    ~FileViewer();

public:
    static FileViewer *GetAndActivate();
    static FileViewer *GetIfExists() { return ms_instance; }

    /// Shows given reference, i.e. loads the file
    void ShowReferences(CatalogPtr catalog, CatalogItemPtr item, int defaultReference = 0);

private:
    wxFileName GetFilename(wxString ref) const;

    void SelectReference(const wxString& ref);
    void ShowHTMLContent(const wxString& markup);
    void ShowError(const char *icon, const wxString& msg, const wxString& description = "", const wxString& references = "");

private:
    wxString m_basePath;
    wxArrayString m_references;

    wxChoice *m_file;
    wxStaticText *m_description;
    wxButton *m_openInEditor;
    wxWebView *m_content;
    wxSizer *m_topBarSizer;

    void OnChoice(wxCommandEvent &event);
    void OnEditFile(wxCommandEvent &event);

    static FileViewer *ms_instance;

#ifdef __WXMSW__
    struct TempFile;
    std::shared_ptr<TempFile> m_tmpFile;
#endif
};

#endif // _FILEVIEWER_H_
