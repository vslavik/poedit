/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2012 Vaclav Slavik
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

#include <wx/string.h>
#include <wx/config.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/filename.h>

#include "digger.h"
#include "parser.h"
#include "catalog.h"
#include "progressinfo.h"
#include "gexecute.h"
#include "utility.h"

namespace
{

// concatenates catalogs using msgcat
bool ConcatCatalogs(const wxArrayString& files, const wxString& outfile)
{
    wxString list;
    for ( wxArrayString::const_iterator i = files.begin();
          i != files.end(); ++i )
    {
        list += wxString::Format(_T(" \"%s\""), i->c_str());
    }

    wxString cmd =
        wxString::Format(_T("msgcat --force-po -o \"%s\" %s"),
                         outfile.c_str(),
                         list.c_str());
    bool succ = ExecuteGettext(cmd);

    if ( !succ )
    {
        wxLogError(_("Failed command: %s"), cmd.c_str());
        wxLogError(_("Failed to merge gettext catalogs."));
        return false;
    }

    return true;
}

} // anonymous namespace

Catalog *SourceDigger::Dig(const wxArrayString& paths,
                           const wxArrayString& keywords,
                           const wxString& charset)
{
    ParsersDB pdb;
    pdb.Read(wxConfig::Get());

    m_progressInfo->UpdateMessage(_("Scanning files..."));

    wxArrayString *all_files = FindFiles(paths, pdb);
    if (all_files == NULL)
        return NULL;

    TempDirectory tmpdir;
    wxArrayString partials;

    for (size_t i = 0; i < pdb.GetCount(); i++)
    {
        if ( all_files[i].empty() )
            continue; // no files of this kind

        m_progressInfo->UpdateMessage(
            wxString::Format(_("Parsing %s files..."), pdb[i].Name.c_str()));
        if (!DigFiles(tmpdir, partials, all_files[i], pdb[i], keywords, charset))
        {
            delete[] all_files;
            return NULL;
        }
    }

    delete[] all_files;

    if ( partials.empty() )
        return NULL; // couldn't parse any source files

    wxString mergedFile = tmpdir.CreateFileName(_T("merged.pot"));

    if ( !ConcatCatalogs(partials, mergedFile) )
        return NULL;

    Catalog *c = new Catalog(mergedFile, Catalog::CreationFlag_IgnoreHeader);

    if ( !c->IsOk() )
    {
        wxLogError(_("Failed to load extracted catalog."));
        delete c;
        return NULL;
    }

    return c;
}



// cmdline's length is limited by OS/shell, this is maximal number
// of files we'll pass to the parser at one run:
#define BATCH_SIZE  16

bool SourceDigger::DigFiles(TempDirectory& tmpdir,
                            wxArrayString& outFiles,
                            const wxArrayString& files,
                            Parser &parser, const wxArrayString& keywords,
                            const wxString& charset)
{
    wxArrayString batchfiles;
    wxArrayString tempfiles;
    size_t i, last = 0;

    while (last < files.GetCount())
    {
        batchfiles.clear();
        for (i = last; i < last + BATCH_SIZE && i < files.size(); i++)
            batchfiles.Add(files[i]);
        last = i;

        wxString tempfile = tmpdir.CreateFileName(_T("extracted.pot"));
        if (!ExecuteGettext(
                    parser.GetCommand(batchfiles, keywords, tempfile, charset)))
        {
            return false;
        }

        tempfiles.push_back(tempfile);

        m_progressInfo->UpdateGauge(batchfiles.GetCount());

        if (m_progressInfo->Cancelled())
            return false;
    }

    if ( tempfiles.empty() )
        return false; // failed to parse any source files

    wxString outfile = tmpdir.CreateFileName(_T("merged_chunks.pot"));
    if ( !ConcatCatalogs(tempfiles, outfile) )
        return false;

    outFiles.push_back(outfile);
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
        wxLogError(_("Poedit did not find any files in scanned directories."));
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

