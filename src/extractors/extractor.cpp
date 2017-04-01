/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2017 Vaclav Slavik
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

#include "extractor.h"

#include "extractor_legacy.h"

#include "gexecute.h"

#include <wx/dir.h>
#include <wx/filename.h>

#include <algorithm>

namespace
{

// Path matching with support for wildcards

struct PathToMatch
{
    wxString path;
    bool isWildcard;

    bool MatchesFile(const wxString& fn) const
    {
        if (isWildcard)
            return wxMatchWild(path, fn);
        else
            return fn == path || fn.StartsWith(path + "/");
    }
};

class PathsToMatch
{
public:
    PathsToMatch() {}
    explicit PathsToMatch(const wxArrayString& a)
    {
        paths.reserve(a.size());
        for (auto& p: a)
            paths.push_back({p, wxIsWild(p)});
    }

    bool MatchesFile(const wxString& fn) const
    {
        for (auto& p: paths)
        {
            if (p.MatchesFile(fn))
                return true;
        }
        return false;
    }

private:
    std::vector<PathToMatch> paths;
};


int FindInDir(const wxString& basepath, const wxString& dirname, const PathsToMatch& excludedPaths,
              Extractor::FilesList& output)
{
    if (dirname.empty())
        return 0;

    wxDir dir(basepath + dirname);
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

        if (excludedPaths.MatchesFile(f))
            continue;

        wxLogTrace("poedit.extractor", "  - %s", f);
        output.push_back(f);
        found++;
    }

    cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_DIRS);
    while (cont)
    {
        const wxString f = (dirname == ".") ? filename : dirname + "/" + filename;
        cont = dir.GetNext(&filename);

        if (excludedPaths.MatchesFile(f))
            continue;

        found += FindInDir(basepath, f, excludedPaths, output);
    }

    return found;
}

} // anonymous namespace


Extractor::FilesList Extractor::CollectAllFiles(const SourceCodeSpec& sources)
{
    // TODO: Only collect files with recognized extensions

    wxLogTrace("poedit.extractor", "collecting files:");

    const auto basepath = sources.BasePath;
    const auto excludedPaths = PathsToMatch(sources.ExcludedPaths);

    FilesList output;

    for (auto& path: sources.SearchPaths)
    {
        if (wxFileName::FileExists(basepath + path))
        {
            if (excludedPaths.MatchesFile(path))
            {
                wxLogTrace("poedit.extractor", "no files found in '%s'", path);
                continue;
            }
            wxLogTrace("poedit.extractor", "  - %s", path);
            output.push_back(path);
        }
        else if (!FindInDir(basepath, path, excludedPaths, output))
        {
            wxLogTrace("poedit.extractor", "no files found in '%s'", path);
        }
    }

    // Sort the filenames in some well-defined order. This is because directory
    // traversal has, generally speaking, undefined order, and the order differs
    // between filesystems. Finally, the order is reflected in the created PO
    // files and it is much better for diffs if it remains consistent.
    std::sort(output.begin(), output.end());

    wxLogTrace("poedit.extractor", "finished collecting %d files", (int)output.size());

    return output;
}


wxString Extractor::ExtractWithAll(TempDirectory& tmpdir,
                                   const SourceCodeSpec& sourceSpec,
                                   const std::vector<wxString>& files_)
{
    auto files = files_;
    wxLogTrace("poedit.extractor", "extracting from %d files", (int)files.size());

    std::vector<wxString> subPots;

    for (auto ex: CreateAllExtractors())
    {
        const auto ex_files = ex->FilterFiles(files);
        if (ex_files.empty())
            continue;

        wxLogTrace("poedit.extractor", " .. using extractor '%s' for %d files", ex->GetId(), (int)ex_files.size());
        auto subPot = ex->Extract(tmpdir, sourceSpec, ex_files);
        if (!subPot.empty())
            subPots.push_back(subPot);

        if (files.size() > ex_files.size())
        {
            FilesList remaining;
            remaining.reserve(files.size() - ex_files.size());
            // Note that this only works because the lists are sorted:
            std::set_difference(files.begin(), files.end(),
                                ex_files.begin(), ex_files.end(),
                                std::inserter(remaining, remaining.begin()));
            std::swap(files, remaining);
        }
        else
        {
            files.clear();
            break; // no more work to do
        }
    }

    wxLogTrace("poedit.extractor", "extraction finished with %d unrecognized files and %d sub-POTs", (int)files.size(), (int)subPots.size());

    if (subPots.empty())
    {
        return "";
    }
    else if (subPots.size() == 1)
    {
        return subPots.front();
    }
    else
    {
        wxLogTrace("poedit.extractor", "merging %d subPOTs", (int)subPots.size());
        return ConcatCatalogs(tmpdir, subPots);
    }
}


Extractor::FilesList Extractor::FilterFiles(const FilesList& files) const
{
    FilesList out;
    for (auto& f: files)
    {
        if (IsFileSupported(f))
            out.push_back(f);
    }
    return out;
}


bool Extractor::IsFileSupported(const wxString& file) const
{
#ifdef __WXMSW__
    auto f = file.Lower();
#else
    auto& f = file;
#endif

    auto ext = f.AfterLast('.');
    if (ext != f && m_extensions.find(ext) != m_extensions.end())
        return true;

    for (auto& w: m_wildcards)
    {
        if (f.Matches(w))
            return true;
    }

    return false;
}


void Extractor::RegisterExtension(const wxString& ext)
{
    if (ext.Contains("."))
    {
        RegisterWildcard("*." + ext);
        return;
    }

#ifdef __WXMSW__
    m_extensions.insert(ext.Lower());
#else
    m_extensions.insert(ext);
#endif
    wxLogTrace("poedit.extractor", "%s handles extension %s", GetId(), ext);
}

void Extractor::RegisterWildcard(const wxString& wildcard)
{
#ifdef __WXMSW__
    m_wildcards.push_back(wildcard.Lower());
#else
    m_wildcards.push_back(wildcard);
#endif
    wxLogTrace("poedit.extractor", "%s handles %s", GetId(), wildcard);
}


wxString Extractor::ConcatCatalogs(TempDirectory& tmpdir, const std::vector<wxString>& files)
{
    if (files.empty())
    {
        return "";
    }
    else if (files.size() == 1)
    {
        return files.front();
    }

    auto outfile = tmpdir.CreateFileName("concatenated.pot");

    wxString list;
    for (auto& f: files)
        list += wxString::Format(" %s", QuoteCmdlineArg(f));

    auto cmd = wxString::Format("msgcat --force-po -o %s %s", QuoteCmdlineArg(outfile), list);
    bool succ = ExecuteGettext(cmd);

    if (!succ)
    {
        wxLogError(_("Failed command: %s"), cmd.c_str());
        wxLogError(_("Failed to merge gettext catalogs."));
        return "";
    }

    return outfile;
}


Extractor::ExtractorsList Extractor::CreateAllExtractors()
{
    ExtractorsList all;

    // User-defined "legacy" extractors take precedence over anything else,
    // to allow customization of the behavior:
    CreateAllLegacyExtractors(all);

    // Standard builtin extractors follow
    CreateGettextExtractors(all);

    return all;
}
