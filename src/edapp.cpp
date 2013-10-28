/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2013 Vaclav Slavik
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
#include <wx/filedlg.h>
#include <wx/sysopt.h>
#include <wx/stdpaths.h>
#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/datetime.h>
#include <wx/intl.h>
#include <wx/translation.h>

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
#include "tm/transmem.h"
#include "utility.h"
#include "prefsdlg.h"
#include "errors.h"

extern bool MigrateLegacyTranslationMemory();

IMPLEMENT_APP(PoeditApp);

wxString PoeditApp::GetAppVersion() const
{
    return wxString::FromAscii(POEDIT_VERSION);
}

bool PoeditApp::IsBetaVersion() const
{
    wxString v = GetAppVersion();
    return  v.Contains("beta") || v.Contains("rc");
}

bool PoeditApp::CheckForBetaUpdates() const
{
    return IsBetaVersion() ||
           wxConfigBase::Get()->ReadBool("check_for_beta_updates", false);
}


static wxArrayString gs_filesToOpen;

extern void InitXmlResource();

bool PoeditApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

#if defined(__WXMAC__)
    wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_SPELL_CHECKER, 1);
#endif

#ifdef __WXMAC__
    SetExitOnFrameDelete(false);
#endif

#if defined(__UNIX__) && !defined(__WXMAC__)
    wxString home = wxGetHomeDir() + "/";

    // create Poedit cfg dir, move ~/.poedit to ~/.poedit/config
    // (upgrade from older versions of Poedit which used ~/.poedit file)
    if (!wxDirExists(home + ".poedit"))
    {
        if (wxFileExists(home + ".poedit"))
            wxRenameFile(home + ".poedit", home + ".poedit2");
        wxMkdir(home + ".poedit");
        if (wxFileExists(home + ".poedit2"))
            wxRenameFile(home + ".poedit2", home + ".poedit/config");
    }
#endif

    SetVendorName("Vaclav Slavik");
    SetAppName("Poedit");

#if defined(__WXMAC__)
    #define CFG_FILE (wxStandardPaths::Get().GetUserConfigDir() + "/net.poedit.Poedit.cfg")
#elif defined(__UNIX__)
    #define CFG_FILE (home + ".poedit/config")
#else
    #define CFG_FILE wxEmptyString
#endif

#ifdef __WXMAC__
    // upgrade from the old location of config file:
    wxString oldcfgfile = wxStandardPaths::Get().GetUserConfigDir() + "/poedit.cfg";
    if (wxFileExists(oldcfgfile) && !wxFileExists(CFG_FILE))
    {
        wxRenameFile(oldcfgfile, CFG_FILE);
    }
#endif

    wxConfigBase::Set(
        new wxConfig(wxEmptyString, wxEmptyString, CFG_FILE, wxEmptyString, 
                     wxCONFIG_USE_GLOBAL_FILE | wxCONFIG_USE_LOCAL_FILE));
    wxConfigBase::Get()->SetExpandEnvVars(false);

    wxImage::AddHandler(new wxPNGHandler);
    wxXmlResource::Get()->InitAllHandlers();

#if defined(__WXMAC__)
    wxXmlResource::Get()->Load(wxStandardPaths::Get().GetResourcesDir() + "/*.xrc");
#elif defined(__WXMSW__)
	wxStandardPaths::Get().DontIgnoreAppSubDir();
    wxXmlResource::Get()->Load(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\*.xrc");
#else
    InitXmlResource();
#endif

    SetDefaultCfg(wxConfig::Get());

    wxArtProvider::PushBack(new PoeditArtProvider);

    SetupLanguage();

#ifdef __WXMAC__
    wxMenuBar::MacSetCommonMenuBar(wxXmlResource::Get()->LoadMenuBar("mainmenu_mac_global"));
    // so that help menu is correctly merged with system-provided menu
    // (see http://sourceforge.net/tracker/index.php?func=detail&aid=1600747&group_id=9863&atid=309863)
    s_macHelpMenuTitleName = _("&Help");
#endif

    FileHistory().Load(*wxConfig::Get());

    // NB: It's important to do this before TM is used for the first time.
    if ( !MigrateLegacyTranslationMemory() )
        return false;

    // NB: opening files or creating empty window is handled differently on
    //     Macs, using MacOpenFiles() and MacNewFile(), so don't create empty
    //     window if no files are given on command line; but still support
    //     passing files on command line
    if (!gs_filesToOpen.empty())
    {
        OpenFiles(gs_filesToOpen);
        gs_filesToOpen.clear();
    }
#ifndef __WXMAC__
    else
    {
        OpenNewFile();
    }
#endif // !__WXMAC__

#ifdef USE_SPARKLE
    Sparkle_Initialize(CheckForBetaUpdates());
#endif // USE_SPARKLE

#ifdef __WXMSW__
    const char *appcast = "http://releases.poedit.net/appcast-win";

    if ( CheckForBetaUpdates() )
    {
        // Beta versions use unstable feed.
        appcast = "http://releases.poedit.net/appcast-win/beta";
    }

    win_sparkle_set_appcast_url(appcast);
    win_sparkle_init();
#endif

    return true;
}

int PoeditApp::OnExit()
{
    TranslationMemory::CleanUp();

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
#if defined(__WXMSW__)
	wxLocale::AddCatalogLookupPathPrefix(wxStandardPaths::Get().GetResourcesDir() + "\\Translations");
#elif !defined(__WXMAC__)
    wxLocale::AddCatalogLookupPathPrefix(wxStandardPaths::Get().GetInstallPrefix() + "/share/locale");
#endif

    wxTranslations *trans = new wxTranslations();
    wxTranslations::Set(trans);
    #if NEED_CHOOSELANG_UI
    trans->SetLanguage(GetUILanguage());
    #endif
    trans->AddCatalog("poedit");
    trans->AddStdCatalog();
}


void PoeditApp::OpenNewFile()
{
    wxWindow *win;
    if (wxConfig::Get()->Read("manager_startup", (long)false))
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

void PoeditApp::OpenFiles(const wxArrayString& names)
{
    wxWindow *win = nullptr;
    for ( auto name: names )
        win = PoeditFrame::Create(name);

    AskForDonations(win);
}

void PoeditApp::SetDefaultParsers(wxConfigBase *cfg)
{
    ParsersDB pdb;
    bool changed = false;
    wxString defaultsVersion = cfg->Read("Parsers/DefaultsVersion",
                                         "1.2.x");
    pdb.Read(cfg);

    // Add parsers for languages supported by gettext itself (but only if the
    // user didn't already add language with this name himself):
    static struct
    {
        const char *name;
        const char *exts;
    } s_gettextLangs[] = {
        { "C/C++",    "*.c;*.cpp;*.h;*.hpp;*.cc;*.C;*.cxx;*.hxx" },
        { "C#",       "*.cs" },
        { "Java",     "*.java" },
        { "Perl",     "*.pl" },
        { "PHP",      "*.php" },
        { "Python",   "*.py" },
        { "TCL",      "*.tcl" },
        { NULL, NULL }
    };

    for (size_t i = 0; s_gettextLangs[i].name != NULL; i++)
    {
        // if this lang is already registered, don't overwrite it:
        if (pdb.FindParser(s_gettextLangs[i].name) != -1)
            continue;

        wxString langflag;
        if ( wxStrcmp(s_gettextLangs[i].name, "C/C++") == 0 )
            langflag = " --language=C++";
        else
            langflag = wxString(" --language=") + s_gettextLangs[i].name;

        // otherwise add new parser:
        Parser p;
        p.Name = s_gettextLangs[i].name;
        p.Extensions = s_gettextLangs[i].exts;
        p.Command = wxString("xgettext") + langflag + " --add-comments=TRANSLATORS --force-po -o %o %C %K %F";
        p.KeywordItem = "-k%k";
        p.FileItem = "%f";
        p.CharsetItem = "--from-code=%c";
        pdb.Add(p);
        changed = true;
    }

    // If upgrading Poedit to 1.2.4, add dxgettext parser for Delphi:
#ifdef __WINDOWS__
    if (defaultsVersion == "1.2.x")
    {
        Parser p;
        p.Name = "Delphi (dxgettext)";
        p.Extensions = "*.pas;*.dpr;*.xfm;*.dfm";
        p.Command = "dxgettext --so %o %F";
        p.KeywordItem = wxEmptyString;
        p.FileItem = "%f";
        pdb.Add(p);
        changed = true;
    }
#endif

    // If upgrading Poedit to 1.2.5, update C++ parser to handle --from-code:
    if (defaultsVersion == "1.2.x" || defaultsVersion == "1.2.4")
    {
        int cpp = pdb.FindParser("C/C++");
        if (cpp != -1)
        {
            if (pdb[cpp].Command == "xgettext --force-po -o %o %K %F")
            {
                pdb[cpp].Command = "xgettext --force-po -o %o %C %K %F";
                pdb[cpp].CharsetItem = "--from-code=%c";
                changed = true;
            }
        }
    }

    // Poedit 1.5.6 had a breakage, it add --add-comments without space in front of it.
    // Repair this automatically:
    for (size_t i = 0; i < pdb.GetCount(); i++)
    {
        wxString& cmd = pdb[i].Command;
        if (cmd.Contains("--add-comments=") && !cmd.Contains(" --add-comments="))
        {
            cmd.Replace("--add-comments=", " --add-comments=");
            changed = true;
        }
    }

    if (changed)
    {
        pdb.Write(cfg);
        cfg->Write("Parsers/DefaultsVersion", GetAppVersion());
    }
}

void PoeditApp::SetDefaultCfg(wxConfigBase *cfg)
{
    SetDefaultParsers(cfg);

    if (cfg->Read("version", wxEmptyString) == GetAppVersion()) return;

    if (cfg->Read("TM/search_paths", wxEmptyString).empty())
    {
        wxString paths;
#if defined(__UNIX__)
        paths = wxGetHomeDir() + ":/usr/share/locale:/usr/local/share/locale";
#elif defined(__WXMSW__)
        paths = "C:";
#endif
        cfg->Write("TM/search_paths", paths);
    }

    cfg->Write("version", GetAppVersion());
}


namespace
{
const char *CL_KEEP_TEMP_FILES = "keep-temp-files";
}

void PoeditApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    wxApp::OnInitCmdLine(parser);

    parser.AddSwitch("", CL_KEEP_TEMP_FILES,
                     _("don't delete temporary files (for debugging)"));

    parser.AddParam("catalog.po", wxCMD_LINE_VAL_STRING, 
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
// exceptions handling
// ---------------------------------------------------------------------------

bool PoeditApp::OnExceptionInMainLoop()
{
    try
    {
        throw;
    }
    catch ( Exception& e )
    {
        wxLogError("%s", e.what());
    }
    catch ( ... )
    {
        throw;
    }

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
#ifdef __WXMSW__
   EVT_MENU           (XRCID("menu_check_for_updates"), PoeditApp::OnWinsparkleCheck)
#endif
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


void PoeditApp::OnOpen(wxCommandEvent&)
{
    wxString path = wxConfig::Get()->Read("last_file_path", wxEmptyString);

    wxFileDialog dlg(nullptr,
                     _("Open catalog"),
                     path,
                     wxEmptyString,
                     _("GNU gettext catalogs (*.po)|*.po|All files (*.*)|*.*"),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (dlg.ShowModal() == wxID_OK)
    {
        wxConfig::Get()->Write("last_file_path", dlg.GetDirectory());
        wxArrayString paths;
        dlg.GetPaths(paths);

        if (paths.size() == 1)
        {
            TRY_FORWARD_TO_ACTIVE_WINDOW( OpenFile(paths[0]) );
        }

        OpenFiles(paths);
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

    OpenFiles(wxArrayString(1, &f));
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

#ifndef __WXMAC__
    about.SetName("Poedit");
    about.SetVersion(wxGetApp().GetAppVersion());
    about.SetDescription(_("Poedit is an easy to use translations editor."));
#endif
    about.SetCopyright("Copyright \u00a9 1999-2013 Vaclav Slavik");
#ifdef __WXGTK__ // other ports would show non-native about dlg
    about.SetWebSite("http://www.poedit.net");
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
    wxLaunchDefaultBrowser("http://www.poedit.net/trac/wiki/Doc");
}


#ifdef __WXMSW__
void PoeditApp::OnWinsparkleCheck(wxCommandEvent& event)
{
    win_sparkle_check_update_with_ui();
}
#endif // __WXMSW__


void PoeditApp::AskForDonations(wxWindow *parent)
{
    wxConfigBase *cfg = wxConfigBase::Get();

    if ( cfg->ReadBool("donate/dont_bug", false) )
        return; // the user doesn't like us, don't be a bother
    if ( cfg->ReadBool("donate/donated", false) )
        return; // the user likes us a lot, don't be a bother

    wxDateTime lastAsked((time_t)cfg->Read("donate/last_asked", (long)0));

    wxDateTime now = wxDateTime::Now();
    if ( lastAsked.Add(wxDateSpan::Days(7)) >= now )
        return; // don't ask too frequently

    // let's ask nicely:
    wxDialog dlg(parent, wxID_ANY, "");
    wxBoxSizer *topsizer = new wxBoxSizer(wxHORIZONTAL);

    wxIcon icon(wxArtProvider::GetIcon("poedit", wxART_OTHER, wxSize(64,64)));
    topsizer->Add(new wxStaticBitmap(&dlg, wxID_ANY, icon), wxSizerFlags().DoubleBorder());

    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    topsizer->Add(sizer, wxSizerFlags(1).Expand().DoubleBorder());

    wxStaticText *big = new wxStaticText(&dlg, wxID_ANY, "Support Open Source software");
    wxFont font = big->GetFont();
    font.SetWeight(wxFONTWEIGHT_BOLD);
#if defined(__WXMSW__)
    font.MakeLarger();
#endif
    big->SetFont(font);
    sizer->Add(big);

    wxStaticText *desc = new wxStaticText(&dlg, wxID_ANY,
            "A lot of time and effort has gone into the development\n"
            "of Poedit. If you find it useful, please consider showing\n"
            "your appreciation with a donation.\n"
            "\n"
            "Donation or not, there will be no difference in Poedit's\n"
            "features and functionality."
            );
#ifdef __WXMAC__
    desc->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
    sizer->Add(desc, wxSizerFlags(1).Expand().DoubleBorder(wxTOP|wxBOTTOM|wxRIGHT));

    wxCheckBox *checkbox = new wxCheckBox(&dlg, wxID_ANY, "Don't bug me about this again");
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
        wxLaunchDefaultBrowser("http://www.poedit.net/donate.php");
        cfg->Write("donate/donated", true);
    }
    else
    {
        cfg->Write("donate/dont_bug", checkbox->GetValue());
    }

    cfg->Write("donate/last_asked", (long)now.GetTicks());

    // re-asking after a crash wouldn't be a good idea:
    cfg->Flush();
}
