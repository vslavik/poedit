
/*
 
    poedit, a wxWindows i18n catalogs editor
 
    ---------------
      gexecute.cpp
    
      Gettext execution code
    
      (c) Vaclav Slavik, 2000
 
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
            bool hasInput = FALSE;

            wxInputStream* is = GetInputStream();
            if (is && !is->Eof()) 
            {
                wxTextInputStream tis(*is);
                m_data->Stdout.Add(tis.ReadLine());
                hasInput = TRUE;
            }

            wxInputStream* es = GetErrorStream();
            if (es && !es->Eof()) 
            {
                wxTextInputStream tis(*es);
                m_data->Stderr.Add(tis.ReadLine());
                hasInput = TRUE;
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




bool ExecuteGettext(const wxString& cmdline)
{
    size_t i;
    MyProcessData pdata;
    MyPipedProcess *process;

    process = new MyPipedProcess(&pdata);
    int pid = wxExecute(cmdline, FALSE, process);

    if (pid == 0)
    {
        wxLogError(_("Cannot execute program: ") + cmdline.BeforeFirst(' '));
        return false;
    }

    while (pdata.Running)
    {
        process->HasInput();
        wxUsleep(50);
        wxYield();
    }

    bool isMsgmerge = (cmdline.BeforeFirst(' ') == "msgmerge");
    wxString dummy;
    
    for (i = 0; i < pdata.Stderr.GetCount(); i++) 
    {
        if (isMsgmerge)
        {
            dummy = pdata.Stderr[i];
            dummy.Replace(".", "");
            if (dummy.IsEmpty() || dummy == " done") continue;
            //msgmerge outputs *progress* to stderr, damn it!
        }
        wxLogError(pdata.Stderr[i]);
    }
    
    return pdata.ExitCode == 0;
}

