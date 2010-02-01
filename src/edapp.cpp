/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2010 Vaclav Slavik
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
 *  Application class
 *
 */

#include <wx/wxprec.h>

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/log.h>
#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_all.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/sysopt.h>

#ifdef USE_SPARKLE
#include <Sparkle/SUCarbonAPI.h>
#include "osx/userdefaults.h"
#endif // USE_SPARKLE

#ifdef __WXMSW__
#include <winsparkle.h>
#endif

#if !wxUSE_UNICODE
    #error "Unicode build of wxWidgets is required by Poedit"
#endif

#include "edapp.h"
#include "edframe.h"
#include "manager.h"
#include "prefsdlg.h"
#include "parser.h"
#include "chooselang.h"
#include "icons.h"
#include "version.h"


IMPLEMENT_APP(PoeditApp);

wxString PoeditApp::GetAppPath() const
{
#if defined(__UNIX__)
    wxString home;
    if (!wxGetEnv(_T("POEDIT_PREFIX"), &home))
        home = wxString::FromAscii(POEDIT_PREFIX);
    return home;
#elif defined(__WXMSW__)
    wxString exedir;
    wxFileName::SplitPath(wxStandardPaths::Get().GetExecutablePath(),
                          &exedir, NULL, NULL);
    wxFileName fn = wxFileName::DirName(exedir);
    fn.RemoveLastDir();
    return fn.GetFullPath();
#else
#error "Unsupported platform!"
#endif
}

wxString PoeditApp::GetAppVersion() const
{
    return wxString::FromAscii(POEDIT_VERSION);
}


#ifndef __WXMAC__
static wxArrayString gs_filesToOpen;
#endif

extern void InitXmlResource();

bool PoeditApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

#if defined(__WXMAC__) && wxCHECK_VERSION(2,8,5)
    wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_SPELL_CHECKER, 1);
#endif

#ifdef __WXMAC__
    // so that help menu is correctly merged with system-provided menu
    // (see http://sourceforge.net/tracker/index.php?func=detail&aid=1600747&group_id=9863&atid=309863)
    s_macHelpMenuTitleName = _("&Help");

    SetExitOnFrameDelete(false);
#endif

#if defined(__UNIX__) && !defined(__WXMAC__)
    wxString home = wxGetHomeDir() + _T("/");

    // create Poedit cfg dir, move ~/.poedit to ~/.poedit/config
    // (upgrade from older versions of Poedit which used ~/.poedit file)
    if (!wxDirExists(home + _T(".poedit")))
    {
        if (wxFileExists(home + _T(".poedit")))
            wxRenameFile(home + _T(".poedit"), home + _T(".poedit2"));
        wxMkdir(home + _T(".poedit"));
        if (wxFileExists(home + _T(".poedit2")))
            wxRenameFile(home + _T(".poedit2"), home + _T(".poedit/config"));
    }
#endif

    SetVendorName(_T("Vaclav Slavik"));
    SetAppName(_T("Poedit"));

#if defined(__WXMAC__)
    #define CFG_FILE (wxStandardPaths::Get().GetUserConfigDir() + _T("/net.poedit.Poedit.cfg"))
#elif defined(__UNIX__)
    #define CFG_FILE (home + _T(".poedit/config"))
#else
    #define CFG_FILE wxEmptyString
#endif

#ifdef __WXMAC__
    // upgrade from the old location of config file:
    wxString oldcfgfile = wxStandardPaths::Get().GetUserConfigDir() + _T("/poedit.cfg");
    if (wxFileExists(oldcfgfile) && !wxFileExists(CFG_FILE))
    {
        wxRenameFile(oldcfgfile, CFG_FILE);
    }
#endif

    wxConfigBase::Set(
        new wxConfig(wxEmptyString, wxEmptyString, CFG_FILE, wxEmptyString, 
                     wxCONFIG_USE_GLOBAL_FILE | wxCONFIG_USE_LOCAL_FILE));
    wxConfigBase::Get()->SetExpandEnvVars(false);

    wxImage::AddHandler(new wxGIFHandler);
    wxImage::AddHandler(new wxPNGHandler);
    wxXmlResource::Get()->InitAllHandlers();
    InitXmlResource();

    SetDefaultCfg(wxConfig::Get());

    wxArtProvider::Insert(new PoeditArtProvider);

#ifdef __WXMAC__
    wxLocale::AddCatalogLookupPathPrefix(
        wxStandardPaths::Get().GetResourcesDir() + _T("/locale"));
#else
    wxLocale::AddCatalogLookupPathPrefix(GetAppPath() + _T("/share/locale"));
#endif

    m_locale.Init(GetUILanguage());

    m_locale.AddCatalog(_T("poedit"));

    if (wxConfig::Get()->Read(_T("translator_name"), _T("nothing")) == _T("nothing"))
    {
        wxMessageBox(_("This is first time you run Poedit.\nPlease fill in your name and email address.\n(This information is used only in catalogs headers)"), _("Setup"),
                       wxOK | wxICON_INFORMATION);

        PreferencesDialog dlg;
        dlg.TransferTo(wxConfig::Get());
        if (dlg.ShowModal() == wxID_OK)
            dlg.TransferFrom(wxConfig::Get());
    }

    // opening files or creating empty window is handled differently on Macs,
    // using MacOpenFile() and MacNewFile(), so don't do it here:
#ifndef __WXMAC__
    if (gs_filesToOpen.GetCount() == 0)
    {
        OpenNewFile();
    }
    else
    {
        for (size_t i = 0; i < gs_filesToOpen.GetCount(); i++)
            OpenFile(gs_filesToOpen[i]);
        gs_filesToOpen.clear();
    }
#endif // !__WXMAC__

#ifdef USE_SPARKLE
    // Remove config key for Sparkle < 1.5.
    UserDefaults::RemoveValue("SUCheckAtStartup");

    SUSparkleInitializeForCarbon();
#endif // USE_SPARKLE

#ifdef __WXMSW__
    win_sparkle_set_appcast_url("http://releases.poedit.net/appcast_win.xml");
    win_sparkle_init();
#endif

    return true;
}

int PoeditApp::OnExit()
{
#ifdef __WXMSW__
    win_sparkle_cleanup();
#endif

    return wxApp::OnExit();
}


void PoeditApp::OpenNewFile()
{
    if (wxConfig::Get()->Read(_T("manager_startup"), (long)false))
        ManagerFrame::Create()->Show(true);
    else
        PoeditFrame::Create(wxEmptyString);
}

void PoeditApp::OpenFile(const wxString& name)
{
    PoeditFrame::Create(name);
}

void PoeditApp::SetDefaultParsers(wxConfigBase *cfg)
{
    ParsersDB pdb;
    bool changed = false;
    wxString defaultsVersion = cfg->Read(_T("Parsers/DefaultsVersion"),
                                         _T("1.2.x"));
    pdb.Read(cfg);

    // Add parsers for languages supported by gettext itself (but only if the
    // user didn't already add language with this name himself):
    static struct
    {
        const wxChar *name;
        const wxChar *exts;
    } s_gettextLangs[] = {
        { _T("C/C++"),    _T("*.c;*.cpp;*.h;*.hpp;*.cc;*.C;*.cxx;*.hxx") },
#ifndef __WINDOWS__
        // FIXME: not supported by 0.13.1 shipped with poedit on win32
        { _T("C#"),       _T("*.cs") },
#endif
        { _T("Java"),     _T("*.java") },
        { _T("Perl"),     _T("*.pl") },
        { _T("PHP"),      _T("*.php") },
        { _T("Python"),   _T("*.py") },
        { _T("TCL"),      _T("*.tcl") },
        { NULL, NULL }
    };
   
    for (size_t i = 0; s_gettextLangs[i].name != NULL; i++)
    {
        // if this lang is already registered, don't overwrite it:
        if (pdb.FindParser(s_gettextLangs[i].name) != -1)
            continue;

        // otherwise add new parser:
        Parser p;
        p.Name = s_gettextLangs[i].name;
        p.Extensions = s_gettextLangs[i].exts;
        p.Command = _T("xgettext --force-po -o %o %C %K %F");
        p.KeywordItem = _T("-k%k");
        p.FileItem = _T("%f");
        p.CharsetItem = _T("--from-code=%c");
        pdb.Add(p);
        changed = true;
    }

    // If upgrading Poedit to 1.2.4, add dxgettext parser for Delphi:
#ifdef __WINDOWS__
    if (defaultsVersion == _T("1.2.x"))
    {
        Parser p;
        p.Name = _T("Delphi (dxgettext)");
        p.Extensions = _T("*.pas;*.dpr;*.xfm;*.dfm");
        p.Command = _T("dxgettext --so %o %F");
        p.KeywordItem = wxEmptyString;
        p.FileItem = _T("%f");
        pdb.Add(p);
        changed = true;
    }
#endif

    // If upgrading Poedit to 1.2.5, update C++ parser to handle --from-code:
    if (defaultsVersion == _T("1.2.x") || defaultsVersion == _T("1.2.4"))
    {
        int cpp = pdb.FindParser(_T("C/C++"));
        if (cpp != -1)
        {
            if (pdb[cpp].Command == _T("xgettext --force-po -o %o %K %F"))
            {
                pdb[cpp].Command = _T("xgettext --force-po -o %o %C %K %F");
                pdb[cpp].CharsetItem = _T("--from-code=%c");
                changed = true;
            }
        }
    }

    if (changed)
    {
        pdb.Write(cfg);
        cfg->Write(_T("Parsers/DefaultsVersion"), GetAppVersion());
    }
}

void PoeditApp::SetDefaultCfg(wxConfigBase *cfg)
{
    SetDefaultParsers(cfg);

    if (cfg->Read(_T("version"), wxEmptyString) == GetAppVersion()) return;

    if (cfg->Read(_T("TM/database_path"), wxEmptyString).empty())
    {
        wxString dbpath;
#if defined(__WXMAC__)
        dbpath = wxStandardPaths::Get().GetUserDataDir() + _T("/tm");
#elif defined(__UNIX__)
        dbpath = wxGetHomeDir() + _T("/.poedit/tm");
#elif defined(__WXMSW__)
        dbpath = wxGetHomeDir() + _T("\\poedit_tm");
#endif
        cfg->Write(_T("TM/database_path"), dbpath);
    }

    if (cfg->Read(_T("TM/search_paths"), wxEmptyString).empty())
    {
        wxString paths;
#if defined(__UNIX__)
        paths = wxGetHomeDir() + _T(":/usr/share/locale:/usr/local/share/locale");
#elif defined(__WXMSW__)
        paths = _T("C:");
#endif
        cfg->Write(_T("TM/search_paths"), paths);
    }

    cfg->Write(_T("version"), GetAppVersion());
}

void PoeditApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    wxApp::OnInitCmdLine(parser);
    parser.AddParam(_T("catalog.po"), wxCMD_LINE_VAL_STRING, 
                    wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
}

bool PoeditApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if (!wxApp::OnCmdLineParsed(parser))
        return false;

#ifndef __WXMAC__
    for (size_t i = 0; i < parser.GetParamCount(); i++)
        gs_filesToOpen.Add(parser.GetParam(i));
#endif

    return true;
}
