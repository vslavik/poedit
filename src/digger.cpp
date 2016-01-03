/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2000-2015 Vaclav Slavik
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
#include "extractor.h"
#include "progressinfo.h"
#include "gexecute.h"
#include "utility.h"

namespace
{

// concatenates catalogs using msgcat
bool ConcatCatalogs(const wxArrayString& files, TempDirectory& tmpdir, wxString *outfile)
{
    if (files.empty())
        return false;

    if (files.size() == 1)
    {
        *outfile = files.front();
        return true;
    }

    *outfile = tmpdir.CreateFileName("merged.pot");

    wxString list;
    for ( wxArrayString::const_iterator i = files.begin();
          i != files.end(); ++i )
    {
        list += wxString::Format(" %s", QuoteCmdlineArg(*i));
    }

    wxString cmd =
        wxString::Format("msgcat --force-po -o %s %s",
                         QuoteCmdlineArg(*outfile),
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

CatalogPtr SourceDigger::Dig(const wxArrayString& paths,
                             const wxArrayString& excludePaths,
                             const wxArrayString& keywords,
                             const wxString& charset,
                             UpdateResultReason& reason)
{
    ExtractorsDB db;
    db.Read(wxConfig::Get());

    m_progressInfo->UpdateMessage(_("Scanning files..."));

    wxArrayString *all_files = FindFiles(paths, PathsToMatch(excludePaths), db);
    if (all_files == nullptr)
    {
        reason = UpdateResultReason::NoSourcesFound;
        return nullptr;
    }

    TempDirectory tmpdir;
    wxArrayString partials;

    for (size_t i = 0; i < db.Data.size(); i++)
    {
        if ( all_files[i].empty() )
            continue; // no files of this kind

        m_progressInfo->UpdateMessage(
            // TRANSLATORS: '%s' is replaced with the kind of the files (e.g. C++, PHP, ...)
            wxString::Format(_("Parsing %s files..."), db.Data[i].Name.c_str()));
        if (!DigFiles(tmpdir, partials, all_files[i], db.Data[i], keywords, charset))
        {
            delete[] all_files;
            return nullptr;
        }
    }

    delete[] all_files;

    wxString mergedFile;
    if ( !ConcatCatalogs(partials, tmpdir, &mergedFile) )
        return nullptr; // couldn't parse any source files

    CatalogPtr c = std::make_shared<Catalog>(mergedFile, Catalog::CreationFlag_IgnoreHeader);

    if ( !c->IsOk() )
    {
        wxLogError(_("Failed to load extracted catalog."));
        return nullptr;
    }

    return c;
}



// cmdline's length is limited by OS/shell, this is maximal number
// of files we'll pass to the parser at one run:
#define BATCH_SIZE  16

bool SourceDigger::DigFiles(TempDirectory& tmpdir,
                            wxArrayString& outFiles,
                            const wxArrayString& files,
                            Extractor &extract, const wxArrayString& keywords,
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

        wxString tempfile = tmpdir.CreateFileName("extracted.pot");
        if (!ExecuteGettext(
                    extract.GetCommand(batchfiles, keywords, tempfile, charset)))
        {
            return false;
        }

        tempfiles.push_back(tempfile);

        m_progressInfo->UpdateGauge((int)batchfiles.GetCount());

        if (m_progressInfo->Cancelled())
            return false;
    }

    wxString outfile;
    if ( !ConcatCatalogs(tempfiles, tmpdir, &outfile) )
        return false; // failed to parse any source files

    outFiles.push_back(outfile);
    return true;
}



wxArrayString *SourceDigger::FindFiles(const wxArrayString& paths,
                                       const PathsToMatch& excludePaths,
                                       ExtractorsDB& db)
{
    if (db.Data.empty())
      return NULL;

    wxArrayString *p_files = new wxArrayString[db.Data.size()];
    wxArrayString files;
    size_t i;
    
    for (i = 0; i < paths.GetCount(); i++)
    {
        if (wxFileName::FileExists(paths[i]))
        {
            if (excludePaths.MatchesFile(paths[i]))
            {
                wxLogTrace("poedit", "no files found in '%s'", paths[i]);
                continue;
            }
            files.Add(paths[i]);
        }
        else if ( !FindInDir(paths[i], excludePaths, files) )
        {
            wxLogTrace("poedit", "no files found in '%s'", paths[i]);
        }
    }

    // Sort the filenames in some well-defined order. This is because directory
    // traversal has, generally speaking, undefined order, and the order differs
    // between filesystems. Finally, the order is reflected in the created PO
    // files and it is much better for diffs if it remains consistent.
    files.Sort();

    size_t filescnt = 0;
    for (i = 0; i < db.Data.size(); i++)
    {
        if (!db.Data[i].Enabled)
            continue;
        p_files[i] = db.Data[i].SelectParsable(files);
        filescnt += p_files[i].GetCount();
    }
    m_progressInfo->SetGaugeMax((int)filescnt);
    
    if (filescnt == 0)
        return nullptr;

    return p_files;
}



int SourceDigger::FindInDir(const wxString& dirname,
                            const PathsToMatch& excludePaths,
                            wxArrayString& files)
{
    if (dirname.empty())
        return 0;
    wxDir dir(dirname);
    if (!dir.IsOpened()) 
        return 0;
    bool cont;
    wxString filename;
    int found = 0;
    
    cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
    while (cont)
    {
        const wxString f = (dirname == ".") ? filename : dirname + "/" + filename;
        cont = dir.GetNext(&filename);

        if (excludePaths.MatchesFile(f))
            continue;

        files.Add(f);
        found++;
    }

    cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
    while (cont)
    {
        const wxString f = (dirname == ".") ? filename : dirname + "/" + filename;
        cont = dir.GetNext(&filename);

        if (excludePaths.MatchesFile(f))
            continue;

        found += FindInDir(f, excludePaths, files);
    }

    return found;
}

