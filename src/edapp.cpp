/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2021 Vaclav Slavik
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
#include <wx/clipbrd.h>
#include <wx/config.h>
#include <wx/cpp.h>
#include <wx/filedlg.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/log.h>
#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_all.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/snglinst.h>
#include <wx/sysopt.h>
#include <wx/stdpaths.h>
#include <wx/aboutdlg.h>
#include <wx/artprov.h>
#include <wx/datetime.h>
#include <wx/intl.h>
#include <wx/ipc.h>
#include <wx/translation.h>

#ifdef __WXOSX__
#include "macos_helpers.h"
#endif

#ifdef __WXMSW__
#include <wx/msw/registry.h>
#include <wx/msw/wrapwin.h>
#include <Shlwapi.h>
#include <winsparkle.h>
#endif

#ifdef __WXGTK__
#include <glib.h>
#include <gtk/gtk.h>
#endif

#include <unicode/uclean.h>
#include <unicode/putil.h>

#if !wxUSE_UNICODE
    #error "Unicode build of wxWidgets is required by Poedit"
#endif

#include "colorscheme.h"
#include "concurrency.h"
#include "configuration.h"
#include "crowdin_gui.h"
#include "edapp.h"
#include "edframe.h"
#include "extractors/extractor_legacy.h"
#include "filemonitor.h"
#include "manager.h"
#include "prefsdlg.h"
#include "chooselang.h"
#include "customcontrols.h"
#include "gexecute.h"
#include "hidpi.h"
#include "http_client.h"
#include "icons.h"
#include "version.h"
#include "recent_files.h"
#include "str_helpers.h"
#include "tm/transmem.h"
#include "utility.h"
#include "prefsdlg.h"
#include "errors.h"
#include "language.h"
#include "crowdin_client.h"
#include "welcomescreen.h"

#ifdef __WXOSX__
struct PoeditApp::NativeMacAppData
{
#ifdef USE_SPARKLE
    NSObject *sparkleDelegate = nullptr;
#endif
};
#endif


#ifndef __WXOSX__

// IPC for ensuring that only one instance of Poedit runs at a time. This is
// handled native on macOS and GtkApplication could do it under GTK+3, but wx
// doesn't support that and we have to implement everything manually for both
// Windows and GTK+ ports.

namespace
{

#ifdef __UNIX__
wxString GetRuntimeDir()
{
    wxString dir;
    if (!wxGetEnv("XDG_RUNTIME_DIR", &dir))
        dir = wxGetHomeDir();
    if (dir.Last() != '/')
        dir += '/';
    return dir;
}
wxString RemoteService() { return GetRuntimeDir() + "poedit.ipc"; }
#else
wxString RemoteService() { return "Poedit"; }
#endif

const char *IPC_TOPIC = "cmdline";

} // anonymous namespace

class PoeditApp::RemoteServer
{
public:
    RemoteServer(PoeditApp *app) : m_server(app)
    {
        m_server.Create(RemoteService());
    }

private:
    class Connection : public wxConnection
    {
    public:
        Connection(PoeditApp *app) : m_app(app) {}

        bool OnExec(const wxString& topic, const wxString& data) override
        {
            if (topic != IPC_TOPIC)
                return false;
            wxString payload;
            if (data == "Activate")
            {
                dispatch::on_main([=] {
                    m_app->OpenNewFile();
                });
                return true;
            }
            if (data.StartsWith("OpenURI:", &payload))
            {
                dispatch::on_main([=] {
                    m_app->HandleCustomURI(payload);
                });
                return true;
            }
            if (data.StartsWith("OpenFile:", &payload))
            {
                long lineno = 0;
                payload.BeforeFirst(':').ToLong(&lineno);
                wxArrayString a;
                a.push_back(payload.AfterFirst(':'));
                dispatch::on_main([=] {
                    m_app->OpenFiles(a, lineno);
                });
                return true;
            }
            return false;
        }

    private:
        PoeditApp *m_app;
    };

    class Server : public wxServer
    {
    public:
        Server(PoeditApp *app) : m_app(app) {}

        wxConnectionBase *OnAcceptConnection(const wxString& topic) override
        {
            if (topic != IPC_TOPIC)
                return nullptr;
            return new Connection(m_app);
        }

    private:
        PoeditApp *m_app;
    };

    Server m_server;
};

class PoeditApp::RemoteClient
{
public:
    RemoteClient(wxSingleInstanceChecker *instCheck) : m_instCheck(instCheck), m_client()
    {
    }

    enum ConnectionStatus
    {
        NoOtherInstance,
        Connected,
        Failure
    };

    ConnectionStatus ConnectIfNeeded()
    {
        // Try several times in case of a race condition where one instance
        // is exiting and not communicating, while another one is just starting.
        for (int attempt = 0; attempt < 3; attempt++)
        {
            if (!m_instCheck->IsAnotherRunning())
                return NoOtherInstance;

            m_conn.reset(m_client.MakeConnection("localhost", RemoteService(), IPC_TOPIC));
            if (m_conn)
                return Connected;

            // else: Something went wrong, no connection despite other instance existing.
            //       Try again in a little while.
            wxMilliSleep(100);
        }

        return Failure;
    }

    void Activate()
    {
        Command("Activate");
    }

    void HandleCustomURI(const wxString& uri)
    {
        Command("OpenURI:" + uri);
    }

    void OpenFile(const wxString& filename, int lineno = 0)
    {
        wxFileName fn(filename);
        fn.MakeAbsolute();
        Command(wxString::Format("OpenFile:%d:%s", lineno, fn.GetFullPath()));
    }

private:
    void Command(const wxString cmd) { m_conn->Execute(cmd); }

    wxSingleInstanceChecker *m_instCheck;
    wxClient m_client;
    std::unique_ptr<wxConnectionBase> m_conn;
};

#endif // __WXOSX__


IMPLEMENT_APP(PoeditApp);

PoeditApp::PoeditApp()
{
#ifdef __WXOSX__
    m_nativeMacAppData.reset(new NativeMacAppData);
#endif
}

PoeditApp::~PoeditApp()
{
}

wxString PoeditApp::GetAppVersion() const
{
    return wxString::FromAscii(POEDIT_VERSION);
}

wxString PoeditApp::GetAppBuildNumber() const
{
#if defined(__WXOSX__)
    NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
    NSString *ver = [infoDict objectForKey:@"CFBundleVersion"];
    return str::to_wx(ver);
#elif defined(__WXMSW__)
    auto exe = wxStandardPaths::Get().GetExecutablePath();
    DWORD unusedHandle;
    DWORD fiSize = GetFileVersionInfoSize(exe.wx_str(), &unusedHandle);
    if (fiSize == 0)
        return "";
    wxCharBuffer fi(fiSize);
    if (!GetFileVersionInfo(exe.wx_str(), unusedHandle, fiSize, fi.data()))
        return "";
    void *ver;
    UINT sizeInfo;
    if (!VerQueryValue(fi.data(), L"\\", &ver, &sizeInfo))
        return "";
    VS_FIXEDFILEINFO *info = (VS_FIXEDFILEINFO*)ver;
    return wxString::Format("%d", LOWORD(info->dwFileVersionLS));
#else
    return "";
#endif
}

bool PoeditApp::IsBetaVersion() const
{
    return false;
}

bool PoeditApp::CheckForBetaUpdates() const
{
    return IsBetaVersion() ||
           wxConfigBase::Get()->ReadBool("check_for_beta_updates", false);
}


#ifndef __WXOSX__
static wxArrayString gs_filesToOpen;
#endif
static int gs_lineToOpen = 0;

extern void InitXmlResource();

bool PoeditApp::OnInit()
{
#ifdef __WXMSW__
    // remove the current directory from the default DLL search order
    SetDllDirectory(L"");
#endif

#ifdef __WXGTK__
    gtk_window_set_default_icon_name("net.poedit.Poedit");
    g_set_application_name("Poedit");
    // Wayland compatibility, see https://wiki.gnome.org/Projects/GnomeShell/ApplicationBased
    g_set_prgname("net.poedit.Poedit");
#endif

    SetVendorName("Vaclav Slavik");
    SetAppName("Poedit");

#ifndef __WXOSX__
    // create early for use in cmd line parsing
    m_instanceChecker.reset(new wxSingleInstanceChecker);
  #ifdef __UNIX__
    m_instanceChecker->Create("poedit.lock", GetRuntimeDir());
  #else
    m_instanceChecker->CreateDefault();
  #endif
#endif

    if (!wxApp::OnInit())
        return false;

#ifdef __WXOSX__
    // macOS 10.15 Vista throws a fit and bombards the user with scary UAC prompt
    // if a subprocess, shell or gettext, is launched with CWD within a "protected"
    // folder like Downloads or Desktop. Avoid by changing CWD to something harmless.
    // Note that this has to be done after wxApp::OnInit() above, which parser the
    // command line, including possible relative filenames.
    wxSetWorkingDirectory(wxGetHomeDir());
#endif

#ifndef __WXOSX__
    m_remoteServer.reset(new RemoteServer(this));
#endif

    InitHiDPIHandling();

#ifdef __WXOSX__
    MoveToApplicationsFolderIfNecessary();

    wxSystemOptions::SetOption(wxMAC_TEXTCONTROL_USE_SPELL_CHECKER, 1);

    SetExitOnFrameDelete(false);
#endif

    // timestamps on the logs are of not use for Poedit:
    wxLog::DisableTimestamp();

#if defined(__UNIX__) && !defined(__WXOSX__)
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
#endif // __UNIX__

#if defined(__WXOSX__)
    #define CFG_FILE (wxStandardPaths::Get().GetUserConfigDir() + "/net.poedit.Poedit.cfg")
#elif defined(__UNIX__)
    #define CFG_FILE configFile
#else
    #define CFG_FILE wxString()
#endif

#ifdef __WXOSX__
    // Remove legacy Sparkle updates folder that used to accumulate broken download
    // files for some users and eat up disk space:
    auto oldsparkle = wxStandardPaths::Get().GetUserDataDir() + "/.Sparkle";
    if (wxFileName::DirExists(oldsparkle))
        wxFileName::Rmdir(oldsparkle, wxPATH_RMDIR_RECURSIVE);
#endif

    Config::Initialize(CFG_FILE.ToStdWstring());

#ifndef __WXOSX__
    wxImage::AddHandler(new wxPNGHandler);
#endif

#ifdef __WXMSW__
    wxImage::AddHandler(new wxICOHandler);
#endif
    wxXmlResource::Get()->InitAllHandlers();
    wxXmlResource::Get()->AddHandler(new LearnMoreLinkXmlHandler);

#if defined(__WXOSX__)
    wxXmlResource::Get()->LoadAllFiles(wxStandardPaths::Get().GetResourcesDir());
#elif defined(__WXMSW__)
    wxStandardPaths::Get().DontIgnoreAppSubDir();
    wxXmlResource::Get()->LoadAllFiles(wxStandardPaths::Get().GetResourcesDir() + "\\Resources");
#else
    InitXmlResource();
#endif

    SetDefaultCfg(wxConfig::Get());

#if defined(__WXOSX__) || defined(__WXMSW__)
    u_setDataDirectory(wxStandardPaths::Get().GetResourcesDir().mb_str());
#endif

#ifndef __WXOSX__
    wxArtProvider::Push(new PoeditArtProvider);
#endif

    SetupLanguage();

#ifdef __WXOSX__
    CreateMenu(Menu::Global);
    // so that help menu is correctly merged with system-provided menu
    // (see http://sourceforge.net/tracker/index.php?func=detail&aid=1600747&group_id=9863&atid=309863)
    s_macHelpMenuTitleName = _("&Help");
#endif

#ifdef __WXMSW__
    AssociateFileTypeIfNeeded();
#endif

#ifndef __WXOSX__
    // NB: opening files or creating empty window is handled differently on
    //     Macs, using MacOpenFiles() and MacNewFile(), so don't create empty
    //     window if no files are given on command line; but still support
    //     passing files on command line
    if (!gs_filesToOpen.empty())
    {
        OpenFiles(gs_filesToOpen, gs_lineToOpen);
        gs_filesToOpen.clear();
        gs_lineToOpen = 0;
    }
    else
    {
        OpenNewFile();
    }
#endif // !__WXOSX__

#ifdef USE_SPARKLE
    m_nativeMacAppData->sparkleDelegate = Sparkle_Initialize();
#endif // USE_SPARKLE

#ifdef __WXMSW__
    wxString appcast = "https://poedit.net/updates_v2/win/appcast";
    if ( CheckForBetaUpdates() )
    {
        // Beta versions use unstable feed.
        appcast = "https://poedit.net/updates_v2/win/appcast/beta";
    }

    win_sparkle_set_appcast_url(appcast.utf8_str());
    win_sparkle_set_can_shutdown_callback(&PoeditApp::WinSparkle_CanShutdown);
    win_sparkle_set_shutdown_request_callback(&PoeditApp::WinSparkle_Shutdown);
    auto buildnum = GetAppBuildNumber();
    if (!buildnum.empty())
        win_sparkle_set_app_build_version(buildnum.wc_str());
    win_sparkle_init();
#endif

#ifndef __WXOSX__
    // If we failed to open any window during startup (e.g. because the user
    // attempted to open MO files), shut the app down. Don't do this on macOS
    // where a) the initialization is finished after OnInit() and b) apps
    // without windows are OK.
    if (!PoeditFrame::HasAnyWindow() && !WelcomeWindow::GetIfActive())
        return false;
#endif

    return true;
}

void PoeditApp::OnEventLoopEnter(wxEventLoopBase *loop)
{
    wxApp::OnEventLoopEnter(loop);
    FileMonitor::EventLoopStarted();
}

int PoeditApp::OnExit()
{
#ifndef __WXOSX__
    m_instanceChecker.reset();
    m_remoteServer.reset();
#endif

#ifdef __WXMSW__
    // Keep any clipboard data available on Windows after the app terminates:
    wxTheClipboard->Flush();
#endif

    // Make sure PoeditFrame instances schedules for deletion are deleted
    // early -- e.g. before wxConfig is destroyed, so they can save changes
    DeletePendingObjects();

    FileMonitor::CleanUp();
    ColorScheme::CleanUp();
    RecentFiles::CleanUp();
    TranslationMemory::CleanUp();

#ifdef HAVE_HTTP_CLIENT
    CrowdinClient::CleanUp();
#endif

    dispatch::cleanup();

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
    wxFileTranslationsLoader::AddCatalogLookupPathPrefix(wxStandardPaths::Get().GetResourcesDir() + "\\Translations");
    wxFileTranslationsLoader::AddCatalogLookupPathPrefix(GetGettextPackagePath() + "/share/locale");
#elif defined(__WXOSX__)
    wxFileTranslationsLoader::AddCatalogLookupPathPrefix(GetGettextPackagePath() + "/share/locale");
#else // UNIX
    wxFileTranslationsLoader::AddCatalogLookupPathPrefix(wxStandardPaths::Get().GetInstallPrefix() + "/share/locale");
#endif

    wxTranslations *trans = new wxTranslations();
    wxTranslations::Set(trans);

    int language = wxLANGUAGE_DEFAULT;

#if NEED_CHOOSELANG_UI
    auto uilang = GetUILanguage();
    if (!uilang.empty())
    {
        auto langinfo = wxLocale::FindLanguageInfo(uilang);
        if (langinfo)
        {
            language = langinfo->Language;
            uilang.clear(); // will go through locale
        }
    }
#endif

    // Properly set locale is important for some aspects of GTK+ as well as
    // other things. It's also the common thing to do, so don't break
    // expectations needlessly:
    {
        // suppress error logging because setting locale may fail and we want to
        // handle that gracefully and invisibly:
        wxLogNull null;
        m_locale.reset(new wxLocale());
        if (!m_locale->Init(language, wxLOCALE_DONT_LOAD_DEFAULT))
            m_locale.reset();

#if NEED_CHOOSELANG_UI
        if (!uilang.empty())
            trans->SetLanguage(uilang);
#endif
    }

    trans->AddStdCatalog();
    trans->AddCatalog("poedit");

    wxString bestTrans = trans->GetBestTranslation("poedit");
    Language uiLang = Language::TryParse(bestTrans.ToStdWstring());
    UErrorCode err = U_ZERO_ERROR;
    icu::Locale::setDefault(uiLang.ToIcu(), err);
#if defined(HAVE_HTTP_CLIENT) && !defined(__WXOSX__)
    http_client::set_ui_language(uiLang.LanguageTag());
#endif

    const wxLanguageInfo *info = wxLocale::FindLanguageInfo(bestTrans);
    g_layoutDirection = info ? info->LayoutDirection : wxLayout_Default;

#ifdef __WXMSW__
    win_sparkle_set_lang(bestTrans.utf8_str());
#endif
}

wxLayoutDirection PoeditApp::GetLayoutDirection() const
{
    return g_layoutDirection;
}


void PoeditApp::OpenNewFile()
{
    WelcomeWindow::GetAndActivate();
}

void PoeditApp::OpenFiles(const wxArrayString& names, int lineno)
{
    for ( auto name: names )
    {
        // MO files cannot be opened directly in Poedit (yet), but they are
        // associated with it, so that the app can provide explanation to users
        // not familiar with the MO/PO distinction.
        auto n = name.Lower();
        if (n.EndsWith(".mo") || n.EndsWith(".gmo"))
        {
            wxMessageDialog dlg(nullptr,
                                _(L"MO files can’t be directly edited in Poedit."),
                                _("Error opening file"),
                                wxOK);
            dlg.SetExtendedMessage(_("Please open and edit the corresponding PO file instead. When you save it, the MO file will be updated as well."));
            dlg.ShowModal();
            continue;
        }

        WelcomeWindow::HideActive();
        PoeditFrame::Create(name, lineno);
    }
}


void PoeditApp::SetDefaultCfg(wxConfigBase *cfg)
{
    LegacyExtractorsDB::RemoveObsoleteExtractors(cfg);

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
const char *CL_HANDLE_POEDIT_URI = "handle-poedit-uri";
const char *CL_LINE = "line";
}

void PoeditApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    wxApp::OnInitCmdLine(parser);

    parser.AddSwitch("", CL_KEEP_TEMP_FILES,
                     _(L"don’t delete temporary files (for debugging)"));
    parser.AddLongOption(CL_HANDLE_POEDIT_URI,
                     _("handle a poedit:// URI"), wxCMD_LINE_VAL_STRING);
    parser.AddLongOption(CL_LINE,
                     _("go to item at given line number"), wxCMD_LINE_VAL_NUMBER);
    parser.AddParam("translation.po", wxCMD_LINE_VAL_STRING,
                    wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
}

bool PoeditApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if (!wxApp::OnCmdLineParsed(parser))
        return false;

    long lineno = 0;
    if (parser.Found(CL_LINE, &lineno))
        gs_lineToOpen = (int)lineno;

    if ( parser.Found(CL_KEEP_TEMP_FILES) )
        TempDirectory::KeepFiles();

#ifndef __WXOSX__
    RemoteClient client(m_instanceChecker.get());
    switch (client.ConnectIfNeeded())
    {
        case RemoteClient::Failure:
        {
            wxLogError(_("Failed to communicate with Poedit process."));
            wxLog::FlushActive();
            return false; // terminate program
        }

        case RemoteClient::Connected:
        {
            wxString poeditURI;
            if (parser.Found(CL_HANDLE_POEDIT_URI, &poeditURI))
            {
                client.HandleCustomURI(poeditURI);
            }
            else if (parser.GetParamCount() == 0)
            {
                client.Activate();
            }
            else
            {
                for (size_t i = 0; i < parser.GetParamCount(); i++)
                {
                    auto fn = parser.GetParam(i);
                    if (fn.StartsWith("poedit://"))
                        client.HandleCustomURI(fn);
                    else
                        client.OpenFile(fn, (int)lineno);
                }
            }
            return false; // terminate program
        }

        case RemoteClient::NoOtherInstance:
            // fall through and handle normally
            break;
    }
#endif

    wxString poeditURI;
    if (parser.Found(CL_HANDLE_POEDIT_URI, &poeditURI))
    {
        // handling the URI shows UI, so do it after OnInit() initialization:
        CallAfter([=]{ HandleCustomURI(poeditURI); });
    }

#ifdef __WXOSX__
    wxArrayString filesToOpen;
    #define gs_filesToOpen filesToOpen
#endif
    for (size_t i = 0; i < parser.GetParamCount(); i++)
    {
        auto fn = parser.GetParam(i);
        if (fn.StartsWith("poedit://"))
        {
            CallAfter([=]{ HandleCustomURI(fn); });
        }
        else
        {
            wxFileName fnFull(fn);
            fnFull.MakeAbsolute();
            gs_filesToOpen.push_back(fnFull.GetFullPath());
        }
    }
#ifdef __WXOSX__
    if (!filesToOpen.empty())
        OSXStoreOpenFiles(filesToOpen);
#endif

    return true;
}


void PoeditApp::HandleCustomURI(const wxString& uri)
{
    if (!uri.StartsWith("poedit://"))
        return;

#ifdef HAVE_HTTP_CLIENT
    if (CrowdinClient::Get().IsOAuthCallback(uri.ToStdString()))
    {
        wxConfig::Get()->Write("/6p/crowdin_logged_in", true);
        CrowdinClient::Get().HandleOAuthCallback(uri.ToStdString());
        return;
    }
#endif
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
#ifdef __WXOSX__
    catch ( NSException *e )
    {
        wxLogError(_("Unhandled exception occurred: %s"), str::to_wx([e reason]));
    }
#endif
    catch ( ... )
    {
        wxLogError(_("Unhandled exception occurred: %s"), DescribeCurrentException());
    }

    return true;

}


// ---------------------------------------------------------------------------
// event handlers for app-global menu actions
// ---------------------------------------------------------------------------

BEGIN_EVENT_TABLE(PoeditApp, wxApp)
   EVT_MENU           (wxID_NEW,                  PoeditApp::OnNewFromScratch)
   EVT_MENU           (XRCID("menu_new_from_pot"),PoeditApp::OnNewFromPOT)
   EVT_MENU           (wxID_OPEN,                 PoeditApp::OnOpen)
 #ifdef HAVE_HTTP_CLIENT
   EVT_MENU           (XRCID("menu_open_crowdin"),PoeditApp::OnOpenFromCrowdin)
 #endif
   EVT_COMMAND        (wxID_ANY, EVT_OPEN_RECENT_FILE, PoeditApp::OnOpenHist)
   EVT_MENU           (wxID_ABOUT,                PoeditApp::OnAbout)
   EVT_MENU           (XRCID("menu_welcome"),     PoeditApp::OnWelcomeWindow)
   EVT_MENU           (XRCID("menu_manager"),     PoeditApp::OnManager)
   EVT_MENU           (wxID_EXIT,                 PoeditApp::OnQuit)
   EVT_MENU           (wxID_PREFERENCES,          PoeditApp::OnPreferences)
   EVT_MENU           (wxID_HELP,                 PoeditApp::OnHelp)
   EVT_MENU           (XRCID("menu_gettext_manual"), PoeditApp::OnGettextManual)
#ifdef __WXMSW__
   EVT_MENU           (XRCID("menu_check_for_updates"), PoeditApp::OnWinsparkleCheck)
#endif
#ifdef __WXOSX__
   EVT_MENU           (wxID_CLOSE, PoeditApp::OnCloseWindowCommand)
   EVT_IDLE           (PoeditApp::OnIdleFixupMenusForMac)
#endif
END_EVENT_TABLE()

namespace
{

/// Information about the window invoking an event. Handles the difference between
/// Windows (where opening a new file is done in the same file as the existing one,
/// and so the action must not destroy data) and elsewhere (where new window is created).
struct InvokingWindowProxy
{
    InvokingWindowProxy(const wxCommandEvent& e) : m_actionTarget(nullptr)
    {
        auto obj = e.GetEventObject();
        wxWindow* win = nullptr;
        auto menu = dynamic_cast<wxMenu*>(obj);
        if (menu)
            win = menu->GetWindow();
        else
            win = dynamic_cast<wxWindow*>(obj);

        if (win)
            win = wxGetTopLevelParent(win);

        m_shouldReactivateWelcomeWindow = false;
        m_isFromWelcomeWindow = dynamic_cast<WelcomeWindow*>(win) != nullptr;
#ifdef __WXOSX__
        // we can't detect the window from global menu, so always assume welcome window must be hidden:
        if (!win && menu)
            m_isFromWelcomeWindow = true;
#endif

        m_invokingWindow = win;
#ifdef __WXMSW__
        m_actionTarget = dynamic_cast<PoeditFrame*>(win);
#endif
    }

    bool IsPerformingActionAllowed()
    {
#ifdef __WXMSW__
        if (m_actionTarget)
            return m_actionTarget->AskIfCanDiscardCurrentDoc();
        else
#endif
            return true;
    }

    void NotifyIsStarting()
    {
        if (m_isFromWelcomeWindow)
            m_shouldReactivateWelcomeWindow = WelcomeWindow::HideActive();
    }

    void NotifyWasAborted() const
    {
        // restore welcome window if that's where the aborted action came from
        if (m_shouldReactivateWelcomeWindow)
            WelcomeWindow::GetAndActivate();
    }

    /// Window to perform actions (e.g. open files) in, or nullptr for new one
    PoeditFrame *GetActionTarget() const
    {
        return m_actionTarget ? m_actionTarget : PoeditFrame::CreateEmpty();
    }

    /// Gets PoeditFrame that the action was invoken from; useful for e.g. file open window's parent
    PoeditFrame *GetParentWindowIfAny() const { return m_actionTarget; }

    /// Gets any invoking window; useful e.g. for parent of Upgrade-to-Pro prompt
    wxWindow *GetInvokingWindow() const { return m_invokingWindow; }

private:
    bool m_shouldReactivateWelcomeWindow;
    bool m_isFromWelcomeWindow;
    wxWindow *m_invokingWindow;
    PoeditFrame *m_actionTarget;
};

} // anonymous namespace


void PoeditApp::OnNewFromScratch(wxCommandEvent& event)
{
    InvokingWindowProxy win(event);

    if (!win.IsPerformingActionAllowed())
        return;

    win.NotifyIsStarting();
    win.GetActionTarget()->NewFromScratch();
}


void PoeditApp::OnNewFromPOT(wxCommandEvent& event)
{
    InvokingWindowProxy win(event);

    if (!win.IsPerformingActionAllowed())
        return;

    win.NotifyIsStarting();
    wxString path = wxConfig::Get()->Read("last_file_path", wxEmptyString);

    wxFileDialog dlg(win.GetParentWindowIfAny(),
        MACOS_OR_OTHER("", _("Select translation template")),
        path,
        wxEmptyString,
        Catalog::GetTypesFileMask({ Catalog::Type::POT, Catalog::Type::PO }),
        wxFD_OPEN | wxFD_FILE_MUST_EXIST);

    if (dlg.ShowModal() != wxID_OK)
    {
        win.NotifyWasAborted();
        return;
    }

    wxConfig::Get()->Write("last_file_path", dlg.GetDirectory());

    auto pot = std::make_shared<POCatalog>(dlg.GetPath(), Catalog::CreationFlag_IgnoreTranslations);
    if (!pot->IsOk())
    {
        win.NotifyWasAborted();
        wxLogError(_(L"“%s” is not a valid POT file."), dlg.GetPath());
        return;
    }

    // Silently fix duplicates because they are common in WP world:
    if (pot->HasDuplicateItems())
        pot->FixDuplicateItems();

    win.GetActionTarget()->NewFromPOT(pot);
}


void PoeditApp::OnOpen(wxCommandEvent& event)
{
    InvokingWindowProxy win(event);

    if (!win.IsPerformingActionAllowed())
        return;

    win.NotifyIsStarting();

    wxString path = wxConfig::Get()->Read("last_file_path", wxEmptyString);
    wxFileDialog dlg(win.GetParentWindowIfAny(),
                     MACOS_OR_OTHER("", _("Select translation file")),
                     path,
                     wxEmptyString,
                     Catalog::GetAllTypesFileMask(),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

    if (dlg.ShowModal() != wxID_OK)
    {
        win.NotifyWasAborted();
        return;
    }

    wxConfig::Get()->Write("last_file_path", dlg.GetDirectory());
    wxArrayString paths;
    dlg.GetPaths(paths);

    win.GetActionTarget()->DoOpenFile(paths.front());
    paths.erase(paths.begin());

    if (!paths.empty())
        OpenFiles(paths);
}


#ifdef HAVE_HTTP_CLIENT
void PoeditApp::OnOpenFromCrowdin(wxCommandEvent& event)
{
    InvokingWindowProxy win(event);

    if (!win.IsPerformingActionAllowed())
        return;

    win.NotifyIsStarting();
    CrowdinOpenFile(win.GetParentWindowIfAny(), [=](int retval, wxString filename)
    {
        if (retval == wxID_OK)
            win.GetActionTarget()->NewFromCrowdin(filename);
        else
            win.NotifyWasAborted();
    });
}
#endif


void PoeditApp::OnOpenHist(wxCommandEvent& event)
{
    InvokingWindowProxy win(event);

    if (!win.IsPerformingActionAllowed())
        return;

    win.NotifyIsStarting();
    win.GetActionTarget()->DoOpenFile(event.GetString());
}


void PoeditApp::OnAbout(wxCommandEvent&)
{
    wxAboutDialogInfo about;

#ifndef __WXOSX__
    about.SetName("Poedit");
    about.SetVersion(wxGetApp().GetAppVersion());
    about.SetDescription(_("Poedit is an easy to use translation editor."));
#endif
    about.SetCopyright(L"Copyright \u00a9 1999-2020 Václav Slavík");
#ifdef __WXGTK__ // other ports would show non-native about dlg
    about.SetWebSite("https://poedit.net");
#endif

    wxAboutBox(about);
}


void PoeditApp::OnWelcomeWindow(wxCommandEvent&)
{
    WelcomeWindow::GetAndActivate();
}


void PoeditApp::OnManager(wxCommandEvent&)
{
    wxFrame *f = ManagerFrame::Create();
    f->Raise();
}


void PoeditApp::OnQuit(wxCommandEvent&)
{
#ifdef __WXOSX__
    // Native apps don't quit if there's any modal window or a sheet open; wx
    // only refuses to quit if a app-modal window is open:
    for (NSWindow *w in [NSApp windows])
    {
        if (w.sheet && w.visible && w.preventsApplicationTerminationWhenModal)
            return;
    }

    // The Close() calls below may not terminate immediately, they may ask for
    // confirmation window-modally on macOS. So change the behavior to terminate
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

#else // !__WXOSX__

    for ( auto& i : wxTopLevelWindows )
    {
        if (!i->Close())
            return;
    }

    ExitMainLoop();

#endif
}


void PoeditApp::EditPreferences()
{
    if (!m_preferences)
        m_preferences = PoeditPreferencesEditor::Create();
    m_preferences->Show(nullptr);
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
    wxLaunchDefaultBrowser("http://www.gnu.org/software/gettext/manual/html_node/");
}


void PoeditApp::OpenPoeditWeb(const wxString& path)
{
    wxLaunchDefaultBrowser
    (
        wxString::Format("https://poedit.net%s?fromVersion=%s",
                         path,
                         GetAppVersion())
    );
}

#ifdef __WXOSX__

void PoeditApp::MacOpenFiles(const wxArrayString& names)
{
    OpenFiles(names, gs_lineToOpen);
    gs_lineToOpen = 0;
}

void PoeditApp::OnIdleFixupMenusForMac(wxIdleEvent& event)
{
    event.Skip();
    FixupMenusForMacIfNeeded();
}

void PoeditApp::OSXOnWillFinishLaunching()
{
    wxApp::OSXOnWillFinishLaunching();
    RecentFiles::Get().MacCreateFakeOpenRecentMenu();
    // We already create the menu item, this would cause duplicates "thanks" to the weird
    // way wx's menubar works on macOS:
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];
}

// Handle Cmd+W closure of windows globally here
void PoeditApp::OnCloseWindowCommand(wxCommandEvent&)
{
    for (auto w: wxTopLevelWindows)
    {
        auto tlw = dynamic_cast<wxTopLevelWindow*>(w);
        if (tlw && tlw->IsActive())
        {
            tlw->Close();
            break;
        }
    }
}

#endif // __WXOSX__


#ifdef __WXMSW__

void PoeditApp::AssociateFileTypeIfNeeded()
{
    // If installed without admin privileges, the installer won't associate
    // Poedit with .po extension. Self-associate with it here in per-user
    // registry record in this case.

    wchar_t buf[1000];
    DWORD bufSize = WXSIZEOF(buf);
    HRESULT hr = AssocQueryString(ASSOCF_INIT_IGNOREUNKNOWN,
                                  ASSOCSTR_EXECUTABLE,
                                  L".po", NULL,
                                  buf, &bufSize);
    if (SUCCEEDED(hr))
        return; // already associated, nothing to do

    auto poCmd = wxString::Format("\"%s\" \"%%1\"", wxStandardPaths::Get().GetExecutablePath());
    wxRegKey key1(wxRegKey::HKCU, "Software\\Classes\\.po");
    key1.Create();
    key1.SetValue("", "Poedit.PO");
    wxRegKey key2(wxRegKey::HKCU, "Software\\Classes\\Poedit.PO");
    key2.Create();
    key2.SetValue("", /*TRANSLATORS:File kind displayed in Finder/Explorer*/_("PO Translation"));
    wxRegKey key3(wxRegKey::HKCU, "Software\\Classes\\Poedit.PO\\Shell\\Open\\Command");
    key3.Create();
    key3.SetValue("", poCmd);
}


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

