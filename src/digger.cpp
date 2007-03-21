/*
 *  This file is part of poEdit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2005 Vaclav Slavik
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
 *  Sources digging class (xgettext)
 *
 */

#include <wx/string.h>
#include <wx/filefn.h> // workaround for wx-2.5.1 bug
#include <wx/config.h>
#include <wx/dir.h>

#include "digger.h"
#include "parser.h"
#include "catalog.h"
#include "progressinfo.h"
#include "gexecute.h"

Catalog *SourceDigger::Dig(const wxArrayString& paths, 
                           const wxArrayString& keywords,
                           const wxString& charset)
{
    ParsersDB pdb;
    pdb.Read(wxConfig::Get());

    m_progressInfo->UpdateMessage(_("Scanning files..."));

    wxArrayString *all_files = FindFiles(paths, pdb);
    if (all_files == NULL) return NULL;

    Catalog *c = new Catalog;
    c->CreateNewHeader();
        
    for (size_t i = 0; i < pdb.GetCount(); i++)
    {
        m_progressInfo->UpdateMessage(_("Parsing ")+pdb[i].Name+_(" files..."));
        if (!DigFiles(c, all_files[i], pdb[i], keywords, charset))
        { 
            delete[] all_files;
            delete c;
            return NULL;
        }
    }
    
    delete[] all_files;
    return c;
}



// cmdline's length is limited by OS/shell, this is maximal number
// of files we'll pass to the parser at one run:
#define BATCH_SIZE  16

bool SourceDigger::DigFiles(Catalog *cat, const wxArrayString& files, 
                            Parser &parser, const wxArrayString& keywords,
                            const wxString& charset)
{
    wxArrayString batchfiles;
    wxString tempfile = wxGetTempFileName(_T("poedit"));
    Catalog *ctmp;
    size_t i, last = 0;

    while (last < files.GetCount())
    {
        batchfiles.Empty();
        for (i = last; i < last + BATCH_SIZE && i < files.GetCount(); i++)
            batchfiles.Add(files[i]);
        last = i;
        
        if (!ExecuteGettext(
                    parser.GetCommand(batchfiles, keywords, tempfile, charset)))
        {
            return false;
        }
        m_progressInfo->UpdateGauge(batchfiles.GetCount());
        if (m_progressInfo->Cancelled()) return false;
        
        ctmp = new Catalog(tempfile);
        cat->Append(*ctmp);
        delete ctmp;
        wxRemoveFile(tempfile);
    }

    return true;
}



wxArrayString *SourceDigger::FindFiles(const wxArrayString& paths, 
                                       ParsersDB& pdb)
{
    if (pdb.GetCount() == 0) return NULL;
    wxArrayString *p_files = new wxArrayString[pdb.GetCount()];
    wxArrayString files;
    size_t i;
    
    for (i = 0; i < paths.GetCount(); i++)
        if (!FindInDir(paths[i], files))
    {
        delete[] p_files;
        return NULL;
    }

    size_t filescnt = 0;
    for (i = 0; i < pdb.GetCount(); i++)
    {
        p_files[i] = pdb[i].SelectParsable(files);
        filescnt += p_files[i].GetCount();
    }
    m_progressInfo->SetGaugeMax(filescnt);
    
    if (filescnt == 0)
    {
        for (i = 0; i < paths.GetCount(); i++)
            wxLogWarning(_("No files found in: ") + paths[i]);
        wxLogError(_("poEdit did not find any files in scanned directories."));
    }

    return p_files;
}



bool SourceDigger::FindInDir(const wxString& dirname, wxArrayString& files)
{
    wxDir dir(dirname);
    if (!dir.IsOpened()) 
        return false;
    bool cont;
    wxString filename;
    
    cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
    while (cont)
    {
        files.Add(dirname + _T("/") + filename);
        cont = dir.GetNext(&filename);
    }    

    cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
    while (cont)
    {
        if (!FindInDir(dirname + _T("/") + filename, files))
            return false;
        cont = dir.GetNext(&filename);
    }
    return true;
}

