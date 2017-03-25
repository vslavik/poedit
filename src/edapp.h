/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
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


#ifndef _EDAPP_H_
#define _EDAPP_H_

#include <wx/app.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/docview.h>

#include "prefsdlg.h"

class WXDLLIMPEXP_FWD_BASE wxConfigBase;
class WXDLLIMPEXP_FWD_BASE wxSingleInstanceChecker;
class WXDLLIMPEXP_FWD_CORE wxMenuBar;


/// wxApp for use with 
class PoeditApp : public wxApp
{
    public:
        PoeditApp();
        ~PoeditApp();

        /** wxWin initialization hook. Shows PoeditFrame and initializes
            configuration entries to default values if they were missing.
         */
        virtual bool OnInit();
        virtual int OnExit();

        virtual wxLayoutDirection GetLayoutDirection() const;

        /// Returns Poedit version string.
        wxString GetAppVersion() const;
        wxString GetAppBuildNumber() const;
        bool IsBetaVersion() const;
        bool CheckForBetaUpdates() const;

        // opens files in new frame
        void OpenFiles(const wxArrayString& filenames, int lineno = 0);
        // opens empty frame or catalogs manager
        void OpenNewFile();

#ifndef __WXOSX__
        wxFileHistory& FileHistory() { return m_history; }
#endif

#ifdef __WXOSX__
        virtual void MacOpenFiles(const wxArrayString& names);
        virtual void MacNewFile() { OpenNewFile(); }
        virtual void MacOpenURL(const wxString &url) { HandleCustomURI(url); }
#endif

        void EditPreferences();

        virtual bool OnExceptionInMainLoop();

        // Open page on poedit.net in the browser
        void OpenPoeditWeb(const wxString& path);

#ifdef __WXOSX__
        // Make OSX-specific modifications to the menus, e.g. adding items into
        // the apple menu etc. Call on every newly created menubar
        void TweakOSXMenuBar(wxMenuBar *bar);
        void CreateFakeOpenRecentMenu();
        void InstallOpenRecentMenu(wxMenuBar *bar);
        void OnIdleInstallOpenRecentMenu(wxIdleEvent& event);
        virtual void OSXOnWillFinishLaunching();
        void OnCloseWindowCommand(wxCommandEvent& event);
#endif

    protected:
        /** Sets default values to configuration items that don't
            have anything set. (This may happen after fresh installation or
            upgrade to new version.)
         */
        void SetDefaultCfg(wxConfigBase *cfg);
        
        void OnInitCmdLine(wxCmdLineParser& parser);
        bool OnCmdLineParsed(wxCmdLineParser& parser);
        
    private:
        void HandleCustomURI(const wxString& uri);

        void SetupLanguage();

        // App-global menu commands:
        void OnNew(wxCommandEvent& event);
        void OnOpen(wxCommandEvent& event);
        void OnOpenFromCrowdin(wxCommandEvent& event);
#ifndef __WXOSX__
        void OnOpenHist(wxCommandEvent& event);
#endif
        void OnAbout(wxCommandEvent& event);
        void OnManager(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnHelp(wxCommandEvent& event);
        void OnGettextManual(wxCommandEvent& event);

#ifdef __WXMSW__
        void AssociateFileTypeIfNeeded();
        void OnWinsparkleCheck(wxCommandEvent& event);
        static int WinSparkle_CanShutdown();
        static void WinSparkle_Shutdown();
#endif

        DECLARE_EVENT_TABLE()

#ifdef __WXOSX__
        class NativeMacAppData;
        std::unique_ptr<NativeMacAppData> m_nativeMacAppData;
#else
        wxFileHistory m_history;
#endif

        std::unique_ptr<PoeditPreferencesEditor> m_preferences;

        std::unique_ptr<wxLocale> m_locale;

#ifndef __WXOSX__
        class RemoteServer;
        class RemoteClient;
        std::unique_ptr<RemoteServer> m_remoteServer;
        std::unique_ptr<wxSingleInstanceChecker> m_instanceChecker;
#endif
};

DECLARE_APP(PoeditApp);


#endif // _EDAPP_H_
