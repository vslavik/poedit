/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2014 Vaclav Slavik
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

#ifdef __WXOSX__
#include "osx_helpers.h"
#include <wx/cocoa/string.h>
#endif

#ifdef __WXMSW__
#include <winsparkle.h>
#endif

#include <unicode/uclean.h>
#include <unicode/putil.h>

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
#include "language.h"

#ifdef __WXOSX__
struct PoeditApp::RecentMenuData
{
    NSMenu *menu = nullptr;
    NSMenuItem *menuItem = nullptr;
    wxMenuBar *menuBar = nullptr;
};
#endif

extern bool MigrateLegacyTranslationMemory();

IMPLEMENT_APP(PoeditApp);

PoeditApp::PoeditApp()
{
#ifdef __WXOSX__
    m_recentMenuData.reset(new RecentMenuData);
#endif
}

PoeditApp::~PoeditApp()
{
}

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

#ifdef __WXOSX__
    MoveToApplicationsFolderIfNecessary();

    wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_SPELL_CHECKER, 1);

    SetExitOnFrameDelete(false);
#endif

    // timestamps on the logs are of not use for Poedit:
    wxLog::DisableTimestamp();

#if defined(__UNIX__) && !defined(__WXMAC__)
    wxString installPrefix = POEDIT_PREFIX;
#ifdef __linux__
    wxFileName::SplitPath(wxStandardPaths::Get().GetExecutablePath(), &installPrefix, nullptr, nullptr);
    wxFileName fn = wxFileName::DirName(installPrefix);
    fn.RemoveLastDir();
    installPrefix = fn.GetFullPath();
#endif
    wxStandardPaths::Get().SetInstallPrefix(installPrefix);

    wxString xdgConfigHome;
    if (!wxGetEnv("XDG_CONFIG_HOME", &xdgConfigHome))
        xdgConfigHome = wxGetHomeDir() + "/.config";
    wxString configDir = xdgConfigHome + "/poedit";
    if (!wxFileName::DirExists(configDir))
        wxFileName::Mkdir(configDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    wxString configFile = configDir + "/config";

    // Move legacy config locations to XDG compatible ones:
    if (!wxFileExists(configFile))
    {
        wxString oldconfig = wxGetHomeDir() + "/.poedit";
        if (wxDirExists(oldconfig))
        {
            if (wxFileExists(oldconfig + "/config"))
                wxRenameFile(oldconfig + "/config", configFile);
            {
                wxLogNull null;
                wxRmdir(oldconfig);
            }
        }
        else if (wxFileExists(oldconfig))
        {
            // even older dotfile
            wxRenameFile(oldconfig, configDir + "/config");
        }
    }
#endif // __UNIX__

    SetVendorName("Vaclav Slavik");
    SetAppName("Poedit");

#if defined(__WXMAC__)
    #define CFG_FILE (wxStandardPaths::Get().GetUserConfigDir() + "/net.poedit.Poedit.cfg")
#elif defined(__UNIX__)
    #define CFG_FILE configFile
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
#ifdef __WXMSW__
    wxImage::AddHandler(new wxICOHandler);
#endif
    wxXmlResource::Get()->InitAllHandlers();

#if defined(__WXMAC__)
    wxXmlResource::Get()->LoadAllFiles(wxStandardPaths::Get().GetResourcesDir());
#elif defined(__WXMSW__)
	wxStandardPaths::Get().DontIgnoreAppSubDir();
    wxXmlResource::Get()->LoadAllFiles(wxStandardPaths::Get().GetResourcesDir() + "\\Resources");
#else
    InitXmlResource();
#endif

    SetDefaultCfg(wxConfig::Get());

#if defined(__WXMAC__) || defined(__WXMSW__)
    u_setDataDirectory(wxStandardPaths::Get().GetResourcesDir().mb_str());
#endif

#ifndef __WXMAC__
    wxArtProvider::PushBack(new PoeditArtProvider);
#endif

    SetupLanguage();

#ifdef __WXMAC__
    wxMenuBar *bar = wxXmlResource::Get()->LoadMenuBar("mainmenu_mac_global");
    TweakOSXMenuBar(bar);
    wxMenuBar::MacSetCommonMenuBar(bar);
    // so that help menu is correctly merged with system-provided menu
    // (see http://sourceforge.net/tracker/index.php?func=detail&aid=1600747&group_id=9863&atid=309863)
    s_macHelpMenuTitleName = _("&Help");
#endif

#ifndef __WXOSX__
    FileHistory().Load(*wxConfig::Get());
#endif

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
    win_sparkle_set_can_shutdown_callback(&PoeditApp::WinSparkle_CanShutdown);
    win_sparkle_set_shutdown_request_callback(&PoeditApp::WinSparkle_Shutdown);
    win_sparkle_init();
#endif

    return true;
}

int PoeditApp::OnExit()
{
    // Make sure PoeditFrame instances schedules for deletion are deleted
    // early -- e.g. before wxConfig is destroyed, so they can save changes
    DeletePendingObjects();

    TranslationMemory::CleanUp();

#ifdef USE_SPARKLE
    Sparkle_Cleanup();
#endif // USE_SPARKLE
#ifdef __WXMSW__
    win_sparkle_cleanup();
#endif

    u_cleanup();

    return wxApp::OnExit();
}


static wxLayoutDirection g_layoutDirection = wxLayout_Default;

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

    wxString bestTrans = trans->GetBestTranslation("poedit");
    Language uiLang = Language::TryParse(bestTrans.ToStdWstring());
    UErrorCode err = U_ZERO_ERROR;
    icu::Locale::setDefault(uiLang.ToIcu(), err);

    const wxLanguageInfo *info = wxLocale::FindLanguageInfo(bestTrans);
    g_layoutDirection = info ? info->LayoutDirection : wxLayout_Default;
}

wxLayoutDirection PoeditApp::GetLayoutDirection() const
{
    return g_layoutDirection;
}


void PoeditApp::OpenNewFile()
{
    PoeditFrame::CreateWelcome();
}

void PoeditApp::OpenFiles(const wxArrayString& names)
{
    PoeditFrame *active = PoeditFrame::UnusedActiveWindow();
    if (names.size() == 1 && active)
    {
        active->OpenFile(names[0]);
        return;
    }

    for ( auto name: names )
        PoeditFrame::Create(name);
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
        { "PHP",      "*.php;*.phtml" },
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
        p.Command = wxString("xgettext") + langflag + " --add-comments=TRANSLATORS --add-comments=translators: --force-po -o %o %C %K %F";
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
        wxLogError(_("Unhandled exception occurred: %s"), e.what());
    }
    catch ( std::exception& e )
    {
        wxLogError(_("Unhandled exception occurred: %s"), e.what());
    }
#ifdef __WXOSX__
    catch ( NSException *e )
    {
        wxLogError(_("Unhandled exception occurred: %s"), wxStringWithNSString([e reason]));
    }
#endif
    catch ( ... )
    {
        wxLogError(_("Unhandled exception occurred."));
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
 #ifndef __WXOSX__
   EVT_MENU_RANGE     (wxID_FILE1, wxID_FILE9,    PoeditApp::OnOpenHist)
 #endif
#endif // !__WXMSW__
   EVT_MENU           (wxID_ABOUT,                PoeditApp::OnAbout)
   EVT_MENU           (XRCID("menu_manager"),     PoeditApp::OnManager)
   EVT_MENU           (wxID_EXIT,                 PoeditApp::OnQuit)
   EVT_MENU           (wxID_PREFERENCES,          PoeditApp::OnPreferences)
   EVT_MENU           (wxID_HELP,                 PoeditApp::OnHelp)
   EVT_MENU           (XRCID("menu_gettext_manual"), PoeditApp::OnGettextManual)
#ifdef __WXMSW__
   EVT_MENU           (XRCID("menu_check_for_updates"), PoeditApp::OnWinsparkleCheck)
#endif
#ifdef __WXOSX__
   EVT_IDLE           (PoeditApp::OnIdleInstallOpenRecentMenu)
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

    PoeditFrame *f = PoeditFrame::CreateEmpty();
    f->OnNew(event);
}


void PoeditApp::OnOpen(wxCommandEvent&)
{
    PoeditFrame *active = PoeditFrame::UnusedActiveWindow();

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

        if (paths.size() == 1 && active)
        {
            active->OpenFile(paths[0]);
            return;
        }

        OpenFiles(paths);
    }
}


#ifndef __WXOSX__
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
#endif // !__WXOSX__

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
#endif

    wxAboutDialogInfo about;

#ifndef __WXMAC__
    about.SetName("Poedit");
    about.SetVersion(wxGetApp().GetAppVersion());
    about.SetDescription(_("Poedit is an easy to use translations editor."));
#endif
    about.SetCopyright(L"Copyright \u00a9 1999-2014 Václav Slavík");
#ifdef __WXGTK__ // other ports would show non-native about dlg
    about.SetWebSite("http://poedit.net");
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
    // The Close() calls below may not terminate immediately, they may ask for
    // confirmation window-modally on OS X. So change the behavior to terminate
    // the app when the last window is closed now, instead of calling
    // ExitMainLoop(). This will terminate the app automagically when all the
    // windows are closed.

    bool delayed = false;
    for ( auto& i : wxTopLevelWindows )
    {
        if (!i->Close())
            delayed = true;
    }

    if (delayed)
        SetExitOnFrameDelete(true);
    else
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
    OpenPoeditWeb("/trac/wiki/Doc");
}

void PoeditApp::OnGettextManual(wxCommandEvent&)
{
    wxLaunchDefaultBrowser("http://www.gnu.org/software/gettext/manual/");
}


void PoeditApp::OpenPoeditWeb(const wxString& path)
{
    wxLaunchDefaultBrowser
    (
        wxString::Format("http://poedit.net%s?fromVersion=%s",
                         path,
                         GetAppVersion())
    );
}

#ifdef __WXOSX__

static NSMenuItem *AddNativeItem(NSMenu *menu, int pos, const wxString&text, SEL ac, NSString *key)
{
    NSString *str = wxNSStringWithWxString(text);
    if (pos == -1)
        return [menu addItemWithTitle:str action:ac keyEquivalent:key];
    else
        return [menu insertItemWithTitle:str action:ac keyEquivalent:key atIndex:pos];
}

void PoeditApp::TweakOSXMenuBar(wxMenuBar *bar)
{
    wxMenu *apple = bar->OSXGetAppleMenu();
    if (!apple)
        return; // huh

    apple->Insert(3, XRCID("menu_manager"), _("Catalogs Manager"));
    apple->InsertSeparator(3);

#if USE_SPARKLE
    Sparkle_AddMenuItem(apple->GetHMenu(), _("Check for Updates...").utf8_str());
#endif

    int editMenuPos = bar->FindMenu(_("Edit"));
    if (editMenuPos == wxNOT_FOUND)
        editMenuPos = 1;
    wxMenu *edit = bar->GetMenu(editMenuPos);
    int pasteItem = -1;
    int findItem = -1;
    int pos = 0;
    for (auto& i : edit->GetMenuItems())
    {
        if (i->GetId() == wxID_PASTE)
            pasteItem = pos;
        else if (i->GetId() == XRCID("menu_sub_find"))
            findItem = pos;
        pos++;
    }

    NSMenu *editNS = edit->GetHMenu();

    AddNativeItem(editNS, 0, _("Undo"), @selector(undo:), @"z");
    AddNativeItem(editNS, 1, _("Redo"), @selector(redo:), @"Z");
    [editNS insertItem:[NSMenuItem separatorItem] atIndex:2];
    if (pasteItem != -1) pasteItem += 3;
    if (findItem != -1)  findItem += 3;

    NSMenuItem *item;
    if (pasteItem != -1)
    {
        item = AddNativeItem(editNS, pasteItem+1, _("Paste and Match Style"),
                             @selector(pasteAsPlainText:), @"V");
        [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
        item = AddNativeItem(editNS, pasteItem+2, _("Delete"),
                             @selector(delete:), @"");
        [item setKeyEquivalentModifierMask:NSCommandKeyMask];
        if (findItem != -1) findItem += 2;
    }

    #define FIND_PLUS(ofset) ((findItem != -1) ? (findItem+ofset) : -1)
    if (findItem == -1)
        [editNS addItem:[NSMenuItem separatorItem]];
    item = AddNativeItem(editNS, FIND_PLUS(1), _("Spelling and Grammar"), NULL, @"");
    NSMenu *spelling = [[NSMenu alloc] initWithTitle:@"Spelling and Grammar"];
    AddNativeItem(spelling, -1, _("Show Spelling and Grammar"), @selector(showGuessPanel:), @":");
    AddNativeItem(spelling, -1, _("Check Document Now"), @selector(checkSpelling:), @";");
    [spelling addItem:[NSMenuItem separatorItem]];
    AddNativeItem(spelling, -1, _("Check Spelling While Typing"), @selector(toggleContinuousSpellChecking:), @"");
    AddNativeItem(spelling, -1, _("Check Grammar With Spelling"), @selector(toggleGrammarChecking:), @"");
    AddNativeItem(spelling, -1, _("Correct Spelling Automatically"), @selector(toggleAutomaticSpellingCorrection:), @"");
    [editNS setSubmenu:spelling forItem:item];

    item = AddNativeItem(editNS, FIND_PLUS(2), _("Substitutions"), NULL, @"");
    NSMenu *subst = [[NSMenu alloc] initWithTitle:@"Substitutions"];
    AddNativeItem(subst, -1, _("Show Substitutions"), @selector(orderFrontSubstitutionsPanel:), @"");
    [subst addItem:[NSMenuItem separatorItem]];
    AddNativeItem(subst, -1, _("Smart Copy/Paste"), @selector(toggleSmartInsertDelete:), @"");
    AddNativeItem(subst, -1, _("Smart Quotes"), @selector(toggleAutomaticQuoteSubstitution:), @"");
    AddNativeItem(subst, -1, _("Smart Dashes"), @selector(toggleAutomaticDashSubstitution:), @"");
    AddNativeItem(subst, -1, _("Smart Links"), @selector(toggleAutomaticLinkDetection:), @"");
    AddNativeItem(subst, -1, _("Text Replacement"), @selector(toggleAutomaticTextReplacement:), @"");
    [editNS setSubmenu:subst forItem:item];

    item = AddNativeItem(editNS, FIND_PLUS(3), _("Transformations"), NULL, @"");
    NSMenu *trans = [[NSMenu alloc] initWithTitle:@"Transformations"];
    AddNativeItem(trans, -1, _("Make Upper Case"), @selector(uppercaseWord:), @"");
    AddNativeItem(trans, -1, _("Make Lower Case"), @selector(lowercaseWord:), @"");
    AddNativeItem(trans, -1, _("Capitalize"), @selector(capitalizeWord:), @"");
    [editNS setSubmenu:trans forItem:item];

    item = AddNativeItem(editNS, FIND_PLUS(4), _("Speech"), NULL, @"");
    NSMenu *speech = [[NSMenu alloc] initWithTitle:@"Speech"];
    AddNativeItem(speech, -1, _("Start Speaking"), @selector(startSpeaking:), @"");
    AddNativeItem(speech, -1, _("Stop Speaking"), @selector(stopSpeaking:), @"");
    [editNS setSubmenu:speech forItem:item];

    int windowMenuPos = bar->FindMenu(_("Window"));
    if (windowMenuPos != wxNOT_FOUND)
    {
        NSMenu *windowNS = bar->GetMenu(windowMenuPos)->GetHMenu();
        AddNativeItem(windowNS, -1, _("Minimize"), @selector(performMiniaturize:), @"m");
        AddNativeItem(windowNS, -1, _("Zoom"), @selector(performZoom:), @"");
        [windowNS addItem:[NSMenuItem separatorItem]];
        AddNativeItem(windowNS, -1, _("Bring All to Front"), @selector(arrangeInFront:), @"");
        [NSApp setWindowsMenu:windowNS];
    }
}

void PoeditApp::CreateFakeOpenRecentMenu()
{
    // Populate the menu with a hack that will be replaced.
    NSMenu *mainMenu = [NSApp mainMenu];
 
    NSMenuItem *item = [mainMenu addItemWithTitle:@"File" action:NULL keyEquivalent:@""];
	NSMenu *menu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"File", nil)];
 
	item = [menu addItemWithTitle:NSLocalizedString(@"Open Recent", nil)
						   action:NULL
					keyEquivalent:@""];
	NSMenu *openRecentMenu = [[NSMenu alloc] initWithTitle:@"Open Recent"];
	[openRecentMenu performSelector:@selector(_setMenuName:) withObject:@"NSRecentDocumentsMenu"];
	[menu setSubmenu:openRecentMenu forItem:item];
    m_recentMenuData->menuItem = item;
    m_recentMenuData->menu = openRecentMenu;
 
	item = [openRecentMenu addItemWithTitle:NSLocalizedString(@"Clear Menu", nil)
									 action:@selector(clearRecentDocuments:)
							  keyEquivalent:@""];
}

void PoeditApp::InstallOpenRecentMenu(wxMenuBar *bar)
{
    if (m_recentMenuData->menuItem)
        [m_recentMenuData->menuItem setSubmenu:nil];
    m_recentMenuData->menuBar = nullptr;

    if (!bar)
        return;

    wxMenu *fileMenu;
    wxMenuItem *item = bar->FindItem(XRCID("open_recent"), &fileMenu);
    if (!item)
        return;

    NSMenu *native = fileMenu->GetHMenu();
    NSMenuItem *nativeItem = [native itemWithTitle:wxNSStringWithWxString(item->GetItemLabelText())];
    if (!nativeItem)
        return;

    [nativeItem setSubmenu:m_recentMenuData->menu];
    m_recentMenuData->menuItem = nativeItem;
    m_recentMenuData->menuBar = bar;
}

void PoeditApp::OnIdleInstallOpenRecentMenu(wxIdleEvent& event)
{
    event.Skip();
    auto installed = wxMenuBar::MacGetInstalledMenuBar();
    if (m_recentMenuData->menuBar != installed)
        InstallOpenRecentMenu(installed);
}

void PoeditApp::OSXOnWillFinishLaunching()
{
    wxApp::OSXOnWillFinishLaunching();
    CreateFakeOpenRecentMenu();
}


#endif // __WXOSX__


#ifdef __WXMSW__
void PoeditApp::OnWinsparkleCheck(wxCommandEvent& event)
{
    win_sparkle_check_update_with_ui();
}

// WinSparkle callbacks:
int PoeditApp::WinSparkle_CanShutdown()
{
    return !PoeditFrame::AnyWindowIsModified();
}

void PoeditApp::WinSparkle_Shutdown()
{
    wxGetApp().OnQuit(wxCommandEvent());
}
#endif // __WXMSW__

