/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2025 Vaclav Slavik
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
#include <wx/textfile.h>

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
            return fn == path || fn.starts_with(path + "/");
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

inline void CheckReadPermissions(const wxString& basepath, const wxString& path)
{
    if (!wxIsReadable(basepath + path))
    {
        BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::PermissionDenied));
    }
}

inline bool IsVCSDir(const wxString& d)
{
    return d == ".git" || d == ".svn" || d == ".hg" || d == ".bzr" || d == "CVS";
}


int FindInDir(const wxString& basepath, const wxString& dirname, const PathsToMatch& excludedPaths,
              Extractor::FilesList& output)
{
    if (dirname.empty())
        return 0;

    wxDir dir(basepath + dirname);

    CheckReadPermissions(basepath, dirname);

    if (!dir.IsOpened())
        return 0;

    bool cont;
    wxString iter;
    int found = 0;
    
    cont = dir.GetFirst(&iter, wxEmptyString, wxDIR_FILES);
    while (cont)
    {
        const wxString filename = iter;
        const wxString fullpath = (dirname == ".") ? filename : dirname + "/" + filename;
        cont = dir.GetNext(&iter);

        if (excludedPaths.MatchesFile(fullpath))
            continue;

        // Normally, a file enumerated by wxDir exists, but in one special case, it may
        // not: if it is a broken symlink. FileExists() follows the symlink to check.
        if (!wxFileName::FileExists(basepath + fullpath))
            continue;
        
        CheckReadPermissions(basepath, fullpath);
        wxLogTrace("poedit.extractor", "  - %s", fullpath);
        output.push_back(fullpath);
        found++;
    }

    cont = dir.GetFirst(&iter, wxEmptyString, wxDIR_DIRS);
    while (cont)
    {
        const wxString filename = iter;
        const wxString fullpath = (dirname == ".") ? filename : dirname + "/" + filename;
        cont = dir.GetNext(&iter);

        if (IsVCSDir(filename))
            continue;

        if (excludedPaths.MatchesFile(fullpath))
            continue;

        CheckReadPermissions(basepath, fullpath);
        found += FindInDir(basepath, fullpath, excludedPaths, output);
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
            CheckReadPermissions(basepath, path);
            wxLogTrace("poedit.extractor", "  - %s", path);
            output.push_back(path);
        }
        else if (wxFileName::DirExists(basepath + path))
        {
            if (!FindInDir(basepath, path, excludedPaths, output))
            {
                wxLogTrace("poedit.extractor", "no files found in '%s'", path);
            }
        }
        else
        {
            BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::NoSourcesFound, path));
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


ExtractionOutput Extractor::ExtractWithAll(TempDirectory& tmpdir,
                                           const SourceCodeSpec& sourceSpec,
                                           const std::vector<wxString>& files_)
{
    auto files = files_;
    wxLogTrace("poedit.extractor", "extracting from %d files", (int)files.size());

    std::vector<ExtractionOutput> partials;

    for (auto ex: CreateAllExtractors(sourceSpec))
    {
        const auto ex_files = ex->FilterFiles(files);
        if (ex_files.empty())
            continue;

        wxLogTrace("poedit.extractor", " .. using extractor '%s' for %d files", ex->GetId(), (int)ex_files.size());
        auto sub = ex->Extract(tmpdir, sourceSpec, ex_files);
        if (sub)
            partials.push_back(sub);

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

    wxLogTrace("poedit.extractor", "extraction finished with %d unrecognized files and %d sub-POTs", (int)files.size(), (int)partials.size());

    if (partials.empty())
    {
        BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::NoSourcesFound));
    }
    else if (partials.size() == 1)
    {
        return partials.front();
    }
    else
    {
        wxLogTrace("poedit.extractor", "merging %d subPOTs", (int)partials.size());
        return ConcatPartials(tmpdir, partials);
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


ExtractionOutput Extractor::ConcatPartials(TempDirectory& tmpdir, const std::vector<ExtractionOutput>& partials)
{
    if (partials.empty())
    {
        return {};
    }
    else if (partials.size() == 1)
    {
        return partials.front();
    }

    ExtractionOutput result;
    result.pot_file = tmpdir.CreateFileName("concatenated.pot");

    wxTextFile filelist;
    filelist.Create(tmpdir.CreateFileName("gettext_filelist.txt"));
    for (auto& p: partials)
    {
        auto fn = p.pot_file;
#ifdef __WXMSW__
        // Gettext tools can't handle Unicode filenames well (due to using
        // char* arguments), so work around this by using the short names.
        if (!fn.IsAscii())
        {
            fn = CliSafeFileName(fn);
            fn.Replace("\\", "/");
        }
#endif
        filelist.AddLine(fn);
        result.errors += p.errors;
    }
    filelist.Write(wxTextFileType_Unix, wxConvFile);

    GettextRunner gt;
    auto output = gt.run_sync("msgcat",
                              "--force-po",
                              // avoid corrupted content of duplicate "" headers
                              // FIXME: merge headers intelligently, e.g. pick latest POT-Creation-Date
                              "--use-first",
                              "-o", result.pot_file,
                              "--files-from", filelist.GetName());
    if (output.failed())
    {
        wxLogError("msgcat: %s", output.std_err);
        wxLogError(_("Failed to merge gettext catalogs."));
        BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::Unspecified));
    }

    return result;
}


Extractor::ExtractorsList Extractor::CreateAllExtractors(const SourceCodeSpec& sources)
{
    ExtractorsList all;

    // User-defined "legacy" extractors customizing the behavior:
    CreateAllLegacyExtractors(all, sources);

    // Standard builtin extractors follow
    CreateGettextExtractors(all, sources);

    std::stable_sort(all.begin(), all.end(), [](const auto& a, const auto& b)
    {
        return a->GetPriority() < b->GetPriority();
    });

    return all;
}
