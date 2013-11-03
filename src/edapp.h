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


#ifndef _EDAPP_H_
#define _EDAPP_H_

#include <wx/app.h>
#include <wx/string.h>
#include <wx/intl.h>
#include <wx/docview.h>

class WXDLLIMPEXP_FWD_BASE wxConfigBase;


/// wxApp for use with 
class PoeditApp : public wxApp
{
    public:
        /** wxWin initalization hook. Shows PoeditFrame and initializes
            configuration entries to default values if they were missing.
         */
        virtual bool OnInit();
        virtual int OnExit();

        /// Returns Poedit version string.
        wxString GetAppVersion() const;
        bool IsBetaVersion() const;
        bool CheckForBetaUpdates() const;

        // opens files in new frame
        void OpenFiles(const wxArrayString& filenames);
        // opens empty frame or catalogs manager
        void OpenNewFile();

        wxFileHistory& FileHistory() { return m_history; }

#ifdef __WXMAC__
        virtual void MacOpenFiles(const wxArrayString& names) { OpenFiles(names); }
        virtual void MacNewFile() { OpenNewFile(); }
#endif

        void EditPreferences();

        virtual bool OnExceptionInMainLoop();

        // Open page on poedit.net in the browser
        void OpenPoeditWeb(const wxString& path);

    protected:
        /** Sets default values to configuration items that don't
            have anything set. (This may happen after fresh installation or
            upgrade to new version.)
         */
        void SetDefaultCfg(wxConfigBase *cfg);
        void SetDefaultParsers(wxConfigBase *cfg);
        
        void OnInitCmdLine(wxCmdLineParser& parser);
        bool OnCmdLineParsed(wxCmdLineParser& parser);
        
    private:
        void SetupLanguage();
        void AskForDonations(wxWindow *parent);

        // App-global menu commands:
        void OnNew(wxCommandEvent& event);
        void OnOpen(wxCommandEvent& event);
        void OnOpenHist(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        void OnManager(wxCommandEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnPreferences(wxCommandEvent& event);
        void OnHelp(wxCommandEvent& event);
#ifdef __WXMSW__
        void OnWinsparkleCheck(wxCommandEvent& event);
#endif

        DECLARE_EVENT_TABLE()

        wxFileHistory m_history;
};

DECLARE_APP(PoeditApp);


#endif // _EDAPP_H_
