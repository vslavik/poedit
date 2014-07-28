/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2013-2014 Vaclav Slavik
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

#include "transmem.h"

#include "logcapture.h"
#include "errors.h"

#include <wx/app.h>
#include <wx/string.h>
#include <wx/log.h>
#include <wx/intl.h>
#include <wx/utils.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/msgdlg.h>
#include <wx/progdlg.h>
#include <wx/intl.h>
#include <wx/sstream.h>

#include <expat.h>

namespace
{

wxString GetLegacyDatabaseDirInternal()
{
    wxString data;
#if defined(__UNIX__) && !defined(__WXMAC__)
    if (!wxGetEnv("XDG_DATA_HOME", &data))
        data = wxGetHomeDir() + "/.local/share";
    data += "/poedit";
#else
    data = wxStandardPaths::Get().GetUserDataDir();
#endif

    data += wxFILE_SEP_PATH;
    data += "TM";

    if (wxFileName::DirExists(data))
        return data;
    else
        return wxString();
}

wxString GetLegacyDatabaseDir()
{
    wxString path = wxConfig::Get()->Read("/TM/database_path", "");
    if (path.empty() || !wxFileName::DirExists(path))
        path = GetLegacyDatabaseDirInternal();
#ifdef __WXMSW__
    return wxFileName(path).GetShortPath();
#else
    return path;
#endif
}


wxString GetDumpToolPath()
{
#if defined(__WXOSX__) || defined(__WXMSW__)
    wxFileName path(wxStandardPaths::Get().GetExecutablePath());
    path.SetName("dump-legacy-tm");
  #ifdef __WXMSW__
    path.SetExt("exe");
    return path.GetShortPath();
  #else
    return path.GetFullPath();
  #endif
#else
    return wxStandardPaths::Get().GetInstallPrefix() + "/libexec/poedit-dump-legacy-tm";
#endif
}


class DumpProcess : public wxProcess
{
public:
    DumpProcess() : wxProcess(), terminated(false), status(0) {}

    virtual void OnTerminate(int, int status_)
    {
        status = status_;
        terminated = true;
    }

    bool terminated;
    int status;
};


// Note: Expat is used for parsing, because the data may potentially be huge and
//       streaming the content avoids excessive memory usage.

struct ExpatContext
{
    wxProgressDialog *progress;
    wxString lang;
    int count;
    std::shared_ptr<TranslationMemory::Writer> tm;
};

void XMLCALL OnStartElement(void *data, const char *name, const char **attrs)
{
    ExpatContext& ctxt = *static_cast<ExpatContext*>(data);

    if ( strcmp(name, "language") == 0 )
    {
        for ( int i = 0; attrs[i]; i += 2 )
        {
            if (strcmp(attrs[i], "lang") == 0)
                ctxt.lang = wxString::FromUTF8(attrs[i+1]);
        }
    }
    if ( strcmp(name, "i") == 0 )
    {
        wxString s, t;
        for ( int i = 0; attrs[i]; i += 2 )
        {
            if (strcmp(attrs[i], "s") == 0)
                s = wxString::FromUTF8(attrs[i+1]);
            else if (strcmp(attrs[i], "t") == 0)
                t = wxString::FromUTF8(attrs[i+1]);
            if (!s.empty() && !t.empty())
            {
                ctxt.tm->Insert(ctxt.lang.ToStdWstring(), s.ToStdWstring(), t.ToStdWstring());
                if (ctxt.count++ % 47 == 0)
                    ctxt.progress->Pulse(wxString::Format(_("Importing translations: %d"), ctxt.count));
            }
        }
    }
}

void XMLCALL OnEndElement(void*, const char*)
{
    // nothing to do
}

void DoMigrate(const wxString& path, const wxString& languages)
{
    LogCapture log;

    const wxString tool = GetDumpToolPath();
    wxLogTrace("poedit.tm", "TM migration - tool: '%s'", tool);
    wxLogVerbose("%s \"%s\" \"%s\"", tool, path, languages);

    wxProgressDialog progress(_("Poedit Update"), _("Preparing migration..."));

    wxCharBuffer tool_utf8(tool.utf8_str());
    wxCharBuffer path_utf8(path.utf8_str());
    wxCharBuffer langs_utf8(languages.utf8_str());
    char *argv_utf8[] = { tool_utf8.data(), path_utf8.data(), langs_utf8.data(), nullptr };

    DumpProcess callback;
    callback.Redirect();
    long executeStatus = wxExecute(argv_utf8, wxEXEC_ASYNC, &callback);
    if (executeStatus <= 0)
        throw Exception(log.text);

    auto sout = callback.GetInputStream();
    auto serr = callback.GetErrorStream();

    ExpatContext ctxt;
    ctxt.progress = &progress;
    ctxt.count = 0;
    ctxt.tm = TranslationMemory::Get().CreateWriter();

    XML_Parser parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, &ctxt);
    XML_SetElementHandler(parser, OnStartElement, OnEndElement);

    while (!callback.terminated)
    {
#if defined(__UNIX__)
        if ( wxTheApp )
            wxTheApp->CheckSignal();
#elif defined(__WXMSW__)
        wxYield(); // so that OnTerminate() is called
#endif

        wxMilliSleep(1);

        while (callback.IsInputAvailable())
        {
            char buf[4096];
            sout->Read(buf, 4096);
            size_t read = sout->LastRead();
            XML_Parse(parser, buf, (int)read, XML_FALSE);
        }
        while (callback.IsErrorAvailable())
        {
            wxStringOutputStream logstream(&log.text);
            serr->Read(logstream);
        }
    }
    XML_Parse(parser, "", 0, XML_TRUE);
    XML_ParserFree(parser);

    if (callback.status == 0)
    {
        progress.Pulse(_("Finalizing..."));
        progress.Pulse();
        ctxt.tm->Commit();
    }

    if (callback.status != 0)
    {
        log.Append(wxString::Format(_("Migration exit status: %d"), callback.status));
        throw Exception(log.text);
    }
}

} // anonymous namespace

/**
    Migrates existing BerkeleyDB-based translation memory into the new Lucene
    format.
    
    @return false if the user declined to do it, true otherwise (even on failure!)
 */
bool MigrateLegacyTranslationMemory()
{
    if (wxConfig::Get()->ReadBool("/TM/legacy_migration_failed", false))
        return true; // failed migration shouldn't prevent Poedit from working

    const wxString languages = wxConfig::Get()->Read("/TM/languages", "");
    if (languages.empty())
        return true; // no migration to perform

    const wxString path = GetLegacyDatabaseDir();
    wxLogTrace("poedit.tm", "TM migration - path: '%s', languages: '%s'", path, languages);
    if (path.empty())
        return true; // no migration to perform

    {
        wxMessageDialog dlg
        (
            nullptr,
            _("Poedit needs to convert your translation memory to a new format."),
            _("Poedit Update"),
            wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION
        );
        dlg.SetExtendedMessage(_("This must be done before Poedit can start. It may take a few minutes if you have lots of translations stored, but should normally be much faster."));
        dlg.SetYesNoLabels(_("Proceed"), _("Quit"));
        if (dlg.ShowModal() != wxID_YES)
            return false;
    }

    try
    {
        DoMigrate(path, languages);

        // migration successed, remove old TM:
        if (path == GetLegacyDatabaseDirInternal())
        {
            wxLogNull null;
            wxFileName::Rmdir(path, wxPATH_RMDIR_RECURSIVE);
        }
        // else: points to a user directory; play it safe and don't delete

        wxConfigBase::Get()->DeleteGroup("/TM");
    }
    catch (Exception& e)
    {
        wxConfig::Get()->Write("/TM/legacy_migration_failed", true);
        wxMessageDialog dlg
        (
            nullptr,
            _("Translation memory migration failed."),
            _("Poedit Update"),
            wxOK | wxICON_WARNING
        );
        dlg.SetExtendedMessage(wxString::Format(
            _(L"Your translation memory data couldn't be migrated. The error was:\n\n%s\nPlease email help@poedit.net and we’ll get it fixed."),
            e.what()));
        dlg.ShowModal();
    }

    return true;
}
