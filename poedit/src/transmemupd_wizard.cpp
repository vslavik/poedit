
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      transmemupd_wizard.cpp
    
      Translation memory database update wizard
    
      (c) Vaclav Slavik, 2003

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#ifdef USE_TRANSMEM

#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <wx/intl.h>
#include <wx/wizard.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/stattext.h>

#include <wx/gizmos/editlbox.h>
#include <wx/xrc/xmlres.h>

#include "transmem.h"
#include "transmemupd.h"
#include "progressinfo.h"

class UpdateWizard : public wxWizard
{
public:
    UpdateWizard() {}

    void Setup()
    {
        m_paths = new wxEditableListBox(
                XRCCTRL(*this, "tm_update_1", wxWizardPage),
                -1, _("Search Paths"));
        wxXmlResource::Get()->AttachUnknownControl(_T("search_paths"), m_paths);
        m_files = new wxEditableListBox(
                XRCCTRL(*this, "tm_update_2", wxWizardPage),
                -1, _("Files List"));
        wxXmlResource::Get()->AttachUnknownControl(_T("files_list"), m_files);

        FitToPage(XRCCTRL(*this, "tm_update_2", wxWizardPage));
    
        // Setup search paths:
        wxString dirsStr = 
            wxConfig::Get()->Read(_T("TM/search_paths"), wxEmptyString);
        wxArrayString dirsArray;
        wxStringTokenizer tkn(dirsStr, wxPATH_SEP);

        while (tkn.HasMoreTokens()) dirsArray.Add(tkn.GetNextToken());
        m_paths->SetStrings(dirsArray);
    }

    void SetLang(const wxString& lang)
    {
        m_lang = lang;
        XRCCTRL(*this, "language1", wxStaticText)->SetLabel(lang);
        XRCCTRL(*this, "language2", wxStaticText)->SetLabel(lang);
    }

    void GetSearchPaths(wxArrayString& arr) const
        { m_paths->GetStrings(arr); }
    void GetFiles(wxArrayString& arr) const
        { m_files->GetStrings(arr); }

private:
    void OnPageChange(wxWizardEvent& event)
    {
        if (event.GetDirection() == true/*fwd*/ &&
            event.GetPage() == XRCCTRL(*this, "tm_update_1", wxWizardPage))
        {
            wxBusyCursor bcur;
            
            wxArrayString dirsArray;
            GetSearchPaths(dirsArray);
            wxArrayString files;
            
            if (TranslationMemoryUpdater::FindFilesInPaths(dirsArray, files,
                                                           m_lang))
            {
                m_files->SetStrings(files);
            }
        }
        else
            event.Skip();
    }
    
    void OnBrowse(wxCommandEvent& event)
    {
        wxDirDialog dlg(this, _("Select directory"));
        if (dlg.ShowModal() == wxID_OK)
        {
            wxArrayString a;
            m_paths->GetStrings(a);
            a.Add(dlg.GetPath());
            m_paths->SetStrings(a);
        }
    }

    void OnDefaults(wxCommandEvent& event)
    {
        wxArrayString a;
#if defined(__UNIX__)
        a.Add(wxGetHomeDir());
        a.Add(_T("/usr/share/locale"));
        a.Add(_T("/usr/local/share/locale"));
#elif defined(__WXMSW__)
        a.Add(_T("C:"));
#endif
        m_paths->SetStrings(a);
    }
    
    void OnAddFiles(wxCommandEvent& event)
    {
        wxFileDialog dlg(this,
                         _("Add files"),
                         m_defaultDir,
                         wxEmptyString,
#ifdef __UNIX__
                   _("Translation files (*.po;*.mo;*.rpm)|*.po;*.mo;*.rpm"), 
#else
                   _("Translation files (*.po;*.mo)|*.po;*.mo"), 
#endif
                         wxOPEN | wxMULTIPLE);
        if (dlg.ShowModal() == wxID_OK)
        {
            wxArrayString f, f2;

            dlg.GetPaths(f);
            
            m_files->GetStrings(f2);
            WX_APPEND_ARRAY(f2, f);
            m_files->SetStrings(f2);

            m_defaultDir = dlg.GetDirectory();            
        }
    }

    wxString m_defaultDir;
    wxString m_lang;
    wxEditableListBox *m_paths, *m_files;

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(UpdateWizard, wxWizard)
    EVT_WIZARD_PAGE_CHANGING(-1, UpdateWizard::OnPageChange)
    EVT_BUTTON(XRCID("browse"), UpdateWizard::OnBrowse)
    EVT_BUTTON(XRCID("reset"), UpdateWizard::OnDefaults)
    EVT_BUTTON(XRCID("add_files"), UpdateWizard::OnAddFiles)
END_EVENT_TABLE()



void RunTMUpdateWizard(wxWindow *parent,
                       const wxString& dbPath, const wxArrayString& langs)
{
    UpdateWizard wizard;

    wxXmlResource::Get()->LoadObject(&wizard, parent,
                                     _T("tm_update_wizard"), _T("wxWizard"));
    wizard.Setup();
    for (size_t i = 0; i < langs.GetCount(); i++)
    {
        wizard.SetLang(langs[0]);
        if (!wizard.RunWizard(XRCCTRL(wizard, "tm_update_1", wxWizardPage)))
        {
            wizard.Destroy();
            return;
        }
        
        TranslationMemory *tm = 
            TranslationMemory::Create(langs[i], dbPath);
        if (tm)
        {
            ProgressInfo *pi = new ProgressInfo;
            TranslationMemoryUpdater u(tm, pi);
            wxArrayString files;
            wizard.GetFiles(files);
            if (!u.Update(files)) 
                { tm->Release(); break; }
            tm->Release();
            delete pi;
        }
    }

    // Save the directories:
    wxArrayString dirsArray;
    wizard.GetSearchPaths(dirsArray);
    wxString dirsStr;
    for (size_t i = 0; i < dirsArray.GetCount(); i++)
    {
        if (i != 0) dirsStr << wxPATH_SEP;
        dirsStr << dirsArray[i];
    }
    wxConfig::Get()->Write(_T("TM/search_paths"), dirsStr);
    
    wizard.Destroy();
   
#if 0
    ProgressInfo *pi = new ProgressInfo;
    pi->SetTitle(_("Updating translation memory"));
    for (size_t i = 0; i < langs.GetCount(); i++)
    {
        TranslationMemory *tm = 
            TranslationMemory::Create(langs[i], dbPath);
        if (tm)
        {
            TranslationMemoryUpdater u(tm, pi);
            wxArrayString files;
            if (!u.FindFilesInPaths(dirsArray, files))
                { tm->Release(); break; }
            if (!u.Update(files)) 
                { tm->Release(); break; }
            tm->Release();
        }
    }
    delete pi;
#endif
}

#endif //USE_TRANSMEM
