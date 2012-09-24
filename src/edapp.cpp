/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2012 Vaclav Slavik
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
#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/datetime.h>
#include <wx/intl.h>
#if wxCHECK_VERSION(2,9,1)
#include <wx/translation.h>
#endif

#ifdef USE_SPARKLE
#include "osx_helpers.h"
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
#include "transmem.h"
#include "utility.h"
#include "prefsdlg.h"

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


static wxArrayString gs_filesToOpen;

extern void InitXmlResource();

bool PoeditApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

#if defined(__WXMAC__) && wxCHECK_VERSION(2,8,5)
    wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_SPELL_CHECKER, 1);
#endif

#ifdef __WXMAC__
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

#if wxCHECK_VERSION(2,9,0)
    wxArtProvider::PushBack(new PoeditArtProvider);
#else    
    wxArtProvider::Insert(new PoeditArtProvider);
#endif

    SetupLanguage();

#ifdef __WXMAC__
    wxMenuBar::MacSetCommonMenuBar(wxXmlResource::Get()->LoadMenuBar(_T("mainmenu_mac_global")));
    // so that help menu is correctly merged with system-provided menu
    // (see http://sourceforge.net/tracker/index.php?func=detail&aid=1600747&group_id=9863&atid=309863)
    s_macHelpMenuTitleName = _("&Help");
#endif

    FileHistory().Load(*wxConfig::Get());

#ifdef USE_TRANSMEM
    // NB: It's important to do this before TM is used for the first time.
    TranslationMemory::MoveLegacyDbIfNeeded();
#endif

    // NB: opening files or creating empty window is handled differently on
    //     Macs, using MacOpenFile() and MacNewFile(), so don't create empty
    //     window if no files are given on command line; but still support
    //     passing files on command line
    if (!gs_filesToOpen.empty())
    {
        for (size_t i = 0; i < gs_filesToOpen.GetCount(); i++)
            OpenFile(gs_filesToOpen[i]);
        gs_filesToOpen.clear();
    }
#ifndef __WXMAC__
    else
    {
        OpenNewFile();
    }
#endif // !__WXMAC__

#ifdef USE_SPARKLE
    Sparkle_Initialize();
#endif // USE_SPARKLE

#ifdef __WXMSW__
    const char *appcast = "https://dl.updatica.com/poedit-win/appcast";

    if ( GetAppVersion().Contains(_T("beta")) ||
         GetAppVersion().Contains(_T("rc")) )
    {
        // Beta versions use unstable feed.
        appcast = "https://dl.updatica.com/poedit-win/appcast/beta";
    }

    win_sparkle_set_appcast_url(appcast);
    win_sparkle_init();
#endif

    return true;
}

int PoeditApp::OnExit()
{
#ifdef USE_SPARKLE
    Sparkle_Cleanup();
#endif // USE_SPARKLE
#ifdef __WXMSW__
    win_sparkle_cleanup();
#endif

    return wxApp::OnExit();
}


void PoeditApp::SetupLanguage()
{
#ifdef __WXMAC__
    wxLocale::AddCatalogLookupPathPrefix(
        wxStandardPaths::Get().GetResourcesDir() + _T("/locale"));
#else
    wxLocale::AddCatalogLookupPathPrefix(GetAppPath() + _T("/share/locale"));
#endif

#if wxCHECK_VERSION(2,9,1)

    wxTranslations *trans = new wxTranslations();
    wxTranslations::Set(trans);
    #if NEED_CHOOSELANG_UI
    trans->SetLanguage(GetUILanguage());
    #endif
    trans->AddCatalog("poedit");
    trans->AddStdCatalog();

#else // wx < 2.9.1

    m_locale.Init(wxLANGUAGE_DEFAULT);
    m_locale.AddCatalog(_T("poedit"));
    #ifdef __WXMSW__
    // Italian version of Windows uses just "?" for "Help" menu. This is
    // correctly handled by wxWidgets catalogs, but Poedit catalog has "Help"
    // entry too, so we add wxmsw.mo catalog again so that it takes
    // precedence over poedit.mo:
    m_locale.AddCatalog(_T("wxmsw"));
    #endif

#endif // wx < 2.9.1
}


void PoeditApp::OpenNewFile()
{
    wxWindow *win;
    if (wxConfig::Get()->Read(_T("manager_startup"), (long)false))
    {
        win = ManagerFrame::Create();
        win->Show(true);
    }
    else
    {
        win = PoeditFrame::Create();
    }

    AskForDonations(win);
}

void PoeditApp::OpenFile(const wxString& name)
{
    wxWindow *win = PoeditFrame::Create(name);
    AskForDonations(win);
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
        { _T("C#"),       _T("*.cs") },
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

        wxString langflag;
        if ( wxStrcmp(s_gettextLangs[i].name, _T("C/C++")) == 0 )
            langflag = _T(" --language=C++");
        else
            langflag = wxString(_T(" --language=")) + s_gettextLangs[i].name;

        // otherwise add new parser:
        Parser p;
        p.Name = s_gettextLangs[i].name;
        p.Extensions = s_gettextLangs[i].exts;
        p.Command = wxString(_T("xgettext")) + langflag + _T(" --force-po -o %o %C %K %F");
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


namespace
{
const wxChar *CL_KEEP_TEMP_FILES = _T("keep-temp-files");
}

void PoeditApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    wxApp::OnInitCmdLine(parser);

    parser.AddSwitch(_T(""), CL_KEEP_TEMP_FILES,
                     _("don't delete temporary files (for debugging)"));

    parser.AddParam(_T("catalog.po"), wxCMD_LINE_VAL_STRING, 
                    wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
}

bool PoeditApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if (!wxApp::OnCmdLineParsed(parser))
        return false;

    if ( parser.Found(CL_KEEP_TEMP_FILES) )
        TempDirectory::KeepFiles();

    for (size_t i = 0; i < parser.GetParamCount(); i++)
        gs_filesToOpen.Add(parser.GetParam(i));

    return true;
}


// ---------------------------------------------------------------------------
// event handlers for app-global menu actions
// ---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PoeditApp, wxApp)
#ifndef __WXMSW__
   EVT_MENU           (wxID_NEW,                  PoeditApp::OnNew)
   EVT_MENU           (XRCID("menu_new_from_pot"),PoeditApp::OnNew)
   EVT_MENU           (wxID_OPEN,                 PoeditApp::OnOpen)
   EVT_MENU_RANGE     (wxID_FILE1, wxID_FILE9,    PoeditApp::OnOpenHist)
#endif // !__WXMSW__
   EVT_MENU           (wxID_ABOUT,                PoeditApp::OnAbout)
   EVT_MENU           (XRCID("menu_manager"),     PoeditApp::OnManager)
   EVT_MENU           (wxID_EXIT,                 PoeditApp::OnQuit)
   EVT_MENU           (wxID_PREFERENCES,          PoeditApp::OnPreferences)
   EVT_MENU           (wxID_HELP,                 PoeditApp::OnHelp)
END_EVENT_TABLE()


// OS X and GNOME apps should open new documents in a new window. On Windows,
// however, the usual thing to do is to open the new document in the already
// open window and replace the current document.
#ifndef __WXMSW__

#define TRY_FORWARD_TO_ACTIVE_WINDOW(funcCall)                          \
    {                                                                   \
        PoeditFrame *active = PoeditFrame::UnusedActiveWindow();        \
        if ( active )                                                   \
        {                                                               \
            active->funcCall;                                           \
            return;                                                     \
        }                                                               \
    }

void PoeditApp::OnNew(wxCommandEvent& event)
{
    TRY_FORWARD_TO_ACTIVE_WINDOW( OnNew(event) );

    PoeditFrame *f = PoeditFrame::Create();
    f->OnNew(event);
}


void PoeditApp::OnOpen(wxCommandEvent& event)
{
    TRY_FORWARD_TO_ACTIVE_WINDOW( OnOpen(event) );

    wxString path = wxConfig::Get()->Read(_T("last_file_path"), wxEmptyString);
    wxString name = wxFileSelector(_("Open catalog"),
                    path, wxEmptyString, wxEmptyString,
                    _("GNU gettext catalogs (*.po)|*.po|All files (*.*)|*.*"),
                    wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (!name.empty())
    {
        wxConfig::Get()->Write(_T("last_file_path"), wxPathOnly(name));
        OpenFile(name);
    }
}


void PoeditApp::OnOpenHist(wxCommandEvent& event)
{
    TRY_FORWARD_TO_ACTIVE_WINDOW( OnOpenHist(event) );

    wxString f(FileHistory().GetHistoryFile(event.GetId() - wxID_FILE1));
    if ( !wxFileExists(f) )
    {
        wxLogError(_("File '%s' doesn't exist."), f.c_str());
        return;
    }

    OpenFile(f);
}

#endif // !__WXMSW__


void PoeditApp::OnAbout(wxCommandEvent&)
{
#if 0
    // Forces translation of several strings that are used for about
    // dialog internally by wx, but are frequently not translate due to
    // state of wx's translations:

    // TRANSLATORS: This is titlebar of about dialog, "%s" is application name
    //              ("Poedit" here, but please use "%s")
    _("About %s");
    // TRANSLATORS: This is version information in about dialog, "%s" will be
    //              version number when used
    _("Version %s");
    // TRANSLATORS: This is version information in about dialog, it is followed
    //              by version number when used (wxWidgets 2.8)
    _(" Version ");
    // TRANSLATORS: This is titlebar of about dialog, the string ends with space
    //              and is followed by application name when used ("Poedit",
    //              but don't add it to this translation yourself) (wxWidgets 2.8)
    _("About ");
#endif

    wxAboutDialogInfo about;

    about.SetName(_T("Poedit"));
    about.SetVersion(wxGetApp().GetAppVersion());
#ifndef __WXMAC__
    about.SetDescription(_("Poedit is an easy to use translations editor."));
#endif
    about.SetCopyright(_T("Copyright \u00a9 1999-2012 Vaclav Slavik"));
#ifdef __WXGTK__ // other ports would show non-native about dlg
    about.SetWebSite(_T("http://www.poedit.net"));
#endif

    wxAboutBox(about);
}


void PoeditApp::OnManager(wxCommandEvent&)
{
    wxFrame *f = ManagerFrame::Create();
    f->Raise();
}


void PoeditApp::OnQuit(wxCommandEvent&)
{
    for ( wxWindowList::iterator i = wxTopLevelWindows.begin(); i != wxTopLevelWindows.end(); ++i )
    {
        if ( !(*i)->Close() )
            return;
    }

    ExitMainLoop();
}


void PoeditApp::EditPreferences()
{
    PreferencesDialog dlg(NULL);

    dlg.TransferTo(wxConfig::Get());
    if (dlg.ShowModal() == wxID_OK)
    {
        dlg.TransferFrom(wxConfig::Get());
        PoeditFrame::UpdateAllAfterPreferencesChange();
    }
}

void PoeditApp::OnPreferences(wxCommandEvent&)
{
    EditPreferences();
}


void PoeditApp::OnHelp(wxCommandEvent&)
{
    wxLaunchDefaultBrowser(_T("http://www.poedit.net/trac/wiki/Doc"));
}


void PoeditApp::AskForDonations(wxWindow *parent)
{
    wxConfigBase *cfg = wxConfigBase::Get();

    if ( (bool)cfg->Read(_T("donate/dont_bug"), (long)false) )
        return; // the user doesn't like us, don't be a bother
    if ( (bool)cfg->Read(_T("donate/donated"), (long)false) )
        return; // the user likes us a lot, don't be a bother

    wxDateTime lastAsked((time_t)cfg->Read(_T("donate/last_asked"), (long)0));

    wxDateTime now = wxDateTime::Now();
    if ( lastAsked.Add(wxDateSpan::Days(7)) >= now )
        return; // don't ask too frequently

    // let's ask nicely:
    wxDialog dlg(parent, wxID_ANY, _T(""));
    wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);

    wxIcon icon(wxArtProvider::GetIcon(_T("poedit"), wxART_OTHER, wxSize(64,64)));
    topsizer->Add(new wxStaticBitmap(&dlg, wxID_ANY, icon), wxSizerFlags().DoubleBorder());

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(sizer, wxSizerFlags(1).Expand().DoubleBorder());

    wxStaticText *big = new wxStaticText(&dlg, wxID_ANY, _T("Support Open Source software"));
    wxFont font = big->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
#if defined(__WXMSW__) && wxCHECK_VERSION(2,9,1)
    font.MakeLarger();
#endif
    big->SetFont(font);
    sizer->Add(big);

    wxStaticText *desc = new wxStaticText(&dlg, wxID_ANY,
            _T("A lot of time and effort have gone into development\n")
            _T("of Poedit. If it's useful to you, please consider showing\n")
            _T("your appreciation with a donation.\n")
            _T("\n")
            _T("Donate or not, there will be no difference in Poedit's\n")
            _T("features and functionality.")
            );
#ifdef __WXMAC__
    desc->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
    sizer->Add(desc, wxSizerFlags(1).Expand().DoubleBorder(wxTOP|wxBOTTOM|wxRIGHT));

    wxCheckBox *checkbox = new wxCheckBox(&dlg, wxID_ANY, _T("Don't bug me about this again"));
    sizer->Add(checkbox);

    wxStdDialogButtonSizer *buttons = new wxStdDialogButtonSizer();
    wxButton *ok = new wxButton(&dlg, wxID_OK, _("Donate..."));
    wxButton *cancel = new wxButton(&dlg, wxID_CANCEL, _("No, thanks"));
    cancel->SetDefault();
    buttons->AddButton(ok);
    buttons->AddButton(cancel);
    buttons->Realize();
    sizer->Add(buttons, wxSizerFlags().Right().DoubleBorder(wxTOP));
    dlg.SetSizerAndFit(topsizer);
    dlg.Centre();

    if ( dlg.ShowModal() == wxID_OK )
    {
        wxLaunchDefaultBrowser(_T("http://www.poedit.net/donate.php"));
        cfg->Write(_T("donate/donated"), true);
    }
    else
    {
        cfg->Write(_T("donate/dont_bug"), checkbox->GetValue());
    }

    cfg->Write(_T("donate/last_asked"), (long)now.GetTicks());

    // re-asking after a crash wouldn't be a good idea:
    cfg->Flush();
}
