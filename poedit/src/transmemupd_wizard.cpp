
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

#include <wx/gizmos/editlbox.h>
#include <wx/xrc/xmlres.h>

#include "transmem.h"
#include "transmemupd.h"
#include "progressinfo.h"

class DirListPage : public wxDialog
{
    protected:
        DECLARE_EVENT_TABLE()
        void OnBrowse(wxCommandEvent& event);
};

BEGIN_EVENT_TABLE(DirListPage, wxDialog)
   EVT_BUTTON(XRCID("tm_adddir"), DirListPage::OnBrowse)
END_EVENT_TABLE()

void DirListPage::OnBrowse(wxCommandEvent& event)
{
    wxDirDialog dlg(this, _("Select directory"));
    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString a;
        wxEditableListBox *l = XRCCTRL(*this, "tm_dirs", wxEditableListBox);
        l->GetStrings(a);
        a.Add(dlg.GetPath());
        l->SetStrings(a);
    }
}

void RunTMUpdateWizard(wxWindow *parent,
                       const wxString& dbPath, const wxArrayString& langs)
{
#if 0 // doesn't work yet, work in progress!
    wxWizard *wizard =
        (wxWizard*) wxXmlResource::Get()->LoadObject(parent,
                                                     _T("tm_update_wizard"),
                                                     _T("wxWizard"));
    wizard->RunWizard(XRCCTRL(*wizard, "tm_update_1", wxWizardPage));
    wizard->Destroy();
#endif
    
    // 1. Get paths list from the user:
    wxConfigBase *cfg = wxConfig::Get();
    DirListPage dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, parent, _T("dlg_generate_tm"));

    wxEditableListBox *dirs = 
        new wxEditableListBox(&dlg, -1, _("Search Paths"));
    wxXmlResource::Get()->AttachUnknownControl(_T("tm_dirs"), dirs);

    wxString dirsStr = cfg->Read(_T("TM/search_paths"), wxEmptyString);
    wxArrayString dirsArray;
    wxStringTokenizer tkn(dirsStr, wxPATH_SEP);

    while (tkn.HasMoreTokens()) dirsArray.Add(tkn.GetNextToken());
    dirs->SetStrings(dirsArray);

    if (dlg.ShowModal() == wxID_OK)
    {
        dirs->GetStrings(dirsArray);
        dirsStr = wxEmptyString;
        for (size_t i = 0; i < dirsArray.GetCount(); i++)
        {
            if (i != 0) dirsStr << wxPATH_SEP;
            dirsStr << dirsArray[i];
        }
        cfg->Write(_T("TM/search_paths"), dirsStr);
    }
    else return;
    
    
    
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
}

#endif //USE_TRANSMEM
