/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2001-2013 Vaclav Slavik
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
#include <wx/tokenzr.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/filename.h>

#include "catalog.h"
#include "transmem.h"
#include "transmemupd.h"
#include "gexecute.h"
#include "progressinfo.h"

/** Does given file contain catalogs in given language?
    Handles these cases:
      - foo/bar/lang.mo
      - foo/lang/bar.mo
      - foo/lang/LC_MESSAGES/bar.mo
    Futhermore, if \a lang is 2-letter code (e.g. "cs"), handles these:
      - foo/bar/lang_??.mo
      - foo/lang_??/bar.mo
      - foo/lang_??/LC_MESSAGES/bar.mo
    and if \a lang is five-letter code (e.g. "cs_CZ"), tries to match its
    first two letters (i.e. country-neutral variant of the language).
 */
static inline bool IsForLang(const wxString& filename, const wxString& lang,
                             bool variants = true)
{
#ifdef __WINDOWS__
    #define LC_MESSAGES_STR "/lc_messages"
#else
    #define LC_MESSAGES_STR "/LC_MESSAGES"
#endif
    wxString base, dir;
    wxSplitPath(filename, &dir, &base, NULL);
    dir.Replace(wxString(wxFILE_SEP_PATH), "/");
    return base.Matches(lang) ||
           dir.Matches("*/" + lang) ||
           dir.Matches("*/" + lang + LC_MESSAGES_STR) ||
           (variants && lang.Len() == 5 && lang[2] == _T('_') && 
                IsForLang(filename, lang.Mid(0, 2), false)) ||
           (variants && lang.Len() == 2 && 
                IsForLang(filename, lang + "_??", false));
    #undef LC_MESSAGES_STR
}

class TMUDirTraverser : public wxDirTraverser
{
    public:
        TMUDirTraverser(wxArrayString *files, const wxString& lang)
            : m_files(files), m_lang(lang) {}
        virtual ~TMUDirTraverser() {}
            
        virtual wxDirTraverseResult OnFile(const wxString& filename)
        {
#ifdef __WINDOWS__
            wxString f = filename.Lower();
#else
            wxString f = filename;
#endif
            if ((f.Matches("*.mo") && IsForLang(f, m_lang)) || 
                (f.Matches("*.po") && IsForLang(f, m_lang)) 
#if HAVE_RPM
                || f.Matches("*.rpm")
#endif
                )
            {
                m_files->Add(f);
            }
            return wxDIR_CONTINUE;
        }

        virtual wxDirTraverseResult OnDir(const wxString&)
            { return wxDIR_CONTINUE; }

    private:
        wxArrayString *m_files;
        wxString m_lang;
};

TranslationMemoryUpdater::TranslationMemoryUpdater(TranslationMemory *mem, 
                                                   ProgressInfo *pi)
    : m_progress(pi), m_mem(mem)
{
}

/*static*/
bool TranslationMemoryUpdater::FindFilesInPaths(const wxArrayString& paths,
                                                wxArrayString& files,
                                                const wxString& lang)
{
    bool rv = true;
    
    files.Clear();
    TMUDirTraverser trav(&files, lang);

    for (size_t i = 0; i < paths.GetCount(); i++)
    {
        wxDir dir(paths[i]);
        if (!dir.IsOpened() || dir.Traverse(trav) == (size_t)-1) 
            // return false, but don't terminate, try other directories
            // first:
            rv = false;
    }
    return rv;
}

bool TranslationMemoryUpdater::Update(const wxArrayString& files)
{
    m_progress->SetGaugeMax((int)files.GetCount());
    m_progress->ResetGauge();

    bool res = true;
    wxString f;
    size_t cnt = files.GetCount();
    for (size_t i = 0; i < cnt; i++)
    {
        f = files[i];
        m_progress->UpdateMessage(wxString(_("Scanning file: ")) + 
                                  wxFileNameFromPath(f));
        
        if (f.Matches("*.po"))
            res = UpdateFromPO(f);
        else if (f.Matches("*.mo"))
            res = UpdateFromMO(f);
#if HAVE_RPM
        else if (f.Matches("*.rpm"))
            res = UpdateFromRPM(f);
#endif

        m_progress->UpdateGauge();
        wxYield();
        if (m_progress->Cancelled()) return false;
    }
    return res;
}

bool TranslationMemoryUpdater::UpdateFromPO(const wxString& filename)
{
    return UpdateFromCatalog(filename);
}

bool TranslationMemoryUpdater::UpdateFromMO(const wxString& filename)
{
    wxString tmp = wxFileName::CreateTempFileName("poedit");
    wxLogNull null;

    if ( tmp.empty() )
        return false;
    if (!ExecuteGettext(_T("msgunfmt --force-po -o \"") + tmp + _T("\" \"") + 
                        filename + _T("\"")))
        return false;
    bool rt = UpdateFromCatalog(tmp);
    wxRemoveFile(tmp);
    return rt;
}

#if HAVE_RPM
bool TranslationMemoryUpdater::UpdateFromRPM(const wxString& filename)
{
    #define TMP_DIR "/tmp/poedit-rpm-tpm"
    if (!wxMkdir(TMP_DIR)) return false;
    
    wxString cmd;
    cmd.Printf(_T("sh -c '(cd %s ; rpm2cpio %s | cpio -i -d --quiet \"*.mo\")'"),
               TMP_DIR, filename.c_str());
    if (wxExecute(cmd, true) != 0)
    {
        wxLogError(_("Cannot extract catalogs from RPM file."));
        wxExecute("rm -rf " TMP_DIR, true);
        return false;
    }

    bool res = true;
    wxArrayString files;
    if (wxDir::GetAllFiles(TMP_DIR, &files, "*.mo") != (size_t)-1)
    {
        size_t cnt = files.GetCount();
        for (size_t i = 0; res && i < cnt; i++)
        {
            if (!IsForLang(files[i], m_mem->GetLanguage())) continue;
            if (!UpdateFromMO(files[i])) 
                res = false;
        }
    }

    wxLog::FlushActive();
    wxExecute("rm -rf " TMP_DIR, true);
    return res;
    #undef TMP_DIR
}
#endif

bool TranslationMemoryUpdater::UpdateFromCatalog(const wxString& filename)
{
    wxLogNull null;
    Catalog cat(filename);
    
    if (!cat.IsOk()) return true; // ignore

    unsigned cnt = cat.GetCount();
    for (unsigned i = 0; i < cnt; i++)
    {
        CatalogItem &dt = cat[i];
        if (!dt.IsTranslated() || dt.IsFuzzy()) continue;
        if (!m_mem->Store(dt.GetString(), dt.GetTranslation())) 
            return false;
    }
    return true;
}

