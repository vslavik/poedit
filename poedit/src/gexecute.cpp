
/*
 
    poedit, a wxWindows i18n catalogs editor
 
    ---------------
      gexecute.cpp
    
      Gettext execution code
    
      (c) Vaclav Slavik, 2000-2004
 
*/


#ifdef __GNUG__
#pragma implementation
#endif 

#include <wx/wxprec.h>

#include <wx/utils.h> 
#include <wx/log.h> 
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/string.h> 
#include <wx/intl.h>


struct MyProcessData
{
        bool Running;
        int ExitCode;
        wxArrayString Stderr;
        wxArrayString Stdout;
};



class MyPipedProcess : public wxProcess
{
    public:
        MyPipedProcess(MyProcessData *data) : wxProcess(), m_data(data)
        {
            m_data->Running = true;
            m_data->Stderr.Empty();
            m_data->Stdout.Empty();
            Redirect();
        }
        
        bool HasInput()
        {
            bool hasInput = false;

            wxInputStream* is = GetInputStream();
            if (is && is->CanRead() && !is->Eof()) 
            {
                wxTextInputStream tis(*is);
                m_data->Stdout.Add(tis.ReadLine());
                hasInput = true;
            }

            wxInputStream* es = GetErrorStream();
            if (es && es->CanRead() && !es->Eof()) 
            {
                wxTextInputStream tis(*es);
                m_data->Stderr.Add(tis.ReadLine());
                hasInput = true;
            }

            return hasInput;
        }

        void OnTerminate(int pid, int status)
        {
            while (HasInput()) {}
            m_data->Running = false;
            m_data->ExitCode = status;
            wxProcess::OnTerminate(pid, status);
        }

    private:
        MyProcessData *m_data;
};


// we have to do this because otherwise xgettext might
// speak in native language, not English, and we cannot parse
// it correctly (not yet)
class TempLocaleSwitcher
{
    public:
        TempLocaleSwitcher(const wxString& locale)
        {
            wxGetEnv(_T("LC_ALL"), &m_all);
            wxGetEnv(_T("LC_MESSAGES"), &m_messages);
            wxGetEnv(_T("LANG"), &m_lang);
            wxGetEnv(_T("LANGUAGE"), &m_language);

            wxSetEnv(_T("LC_ALL"), locale);
            wxSetEnv(_T("LC_MESSAGES"), locale);
            wxSetEnv(_T("LANG"), locale);
            wxSetEnv(_T("LANGUAGE"), locale);
        }
        
        ~TempLocaleSwitcher()
        {
            wxSetEnv(_T("LC_ALL"), m_all);
            wxSetEnv(_T("LC_MESSAGES"), m_messages);
            wxSetEnv(_T("LANG"), m_lang);
            wxSetEnv(_T("LANGUAGE"), m_language);
        }
        
    private:
        wxString m_all, m_messages, m_lang, m_language;
};


bool ExecuteGettext(const wxString& cmdline)
{
    TempLocaleSwitcher localeSwitcher(_T("C"));

    size_t i;
    MyProcessData pdata;
    MyPipedProcess *process;

    process = new MyPipedProcess(&pdata);
    int pid = wxExecute(cmdline, false, process);

    if (pid == 0)
    {
        wxLogError(_("Cannot execute program: ") + cmdline.BeforeFirst(_T(' ')));
        return false;
    }

    while (pdata.Running)
    {
        process->HasInput();
        wxUsleep(50);
        wxYield();
    }

    bool isMsgmerge = (cmdline.BeforeFirst(_T(' ')) == _T("msgmerge"));
    wxString dummy;
    
    for (i = 0; i < pdata.Stderr.GetCount(); i++) 
    {
        if (pdata.Stderr[i].empty()) continue;
        if (isMsgmerge)
        {
            dummy = pdata.Stderr[i];
            dummy.Replace(_T("."), wxEmptyString);
            if (dummy.empty() || dummy == _T(" done")) continue;
            //msgmerge outputs *progress* to stderr, damn it!
        }
        wxLogError(_T("%s"), pdata.Stderr[i].c_str());
    }
    
    return pdata.ExitCode == 0;
}

