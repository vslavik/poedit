/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      settingsdlg.cpp
    
      Settings dialog
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include <wx/gizmos/editlbox.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>

#include "iso639.h"
#include "settingsdlg.h"


SettingsDialog::SettingsDialog(wxWindow *parent)
{
    wxTheXmlResource->LoadDialog(this, parent, _T("settings"));

    m_team = XMLCTRL(*this, "team_name", wxTextCtrl);
    m_teamEmail = XMLCTRL(*this, "team_email", wxTextCtrl);
    m_project = XMLCTRL(*this, "prj_name", wxTextCtrl);
    m_language = XMLCTRL(*this, "language", wxComboBox);
    m_charset = XMLCTRL(*this, "charset", wxComboBox);
    m_basePath = XMLCTRL(*this, "basepath", wxTextCtrl);

    const LanguageStruct *lang = isoLanguages; /*from iso639.h*/ 
    m_language->Append(wxEmptyString);
    while (lang->lang != NULL)
        m_language->Append((lang++)->lang);
        
    // my custom controls:
    m_keywords = new wxEditableListBox(this, -1, _("Keywords"));
    wxTheXmlResource->AttachUnknownControl(_T("keywords"), m_keywords);
    m_paths = new wxEditableListBox(this, -1, _("Paths"));
    wxTheXmlResource->AttachUnknownControl(_T("paths"), m_paths);
}


void SettingsDialog::TransferTo(Catalog *cat)
{
    #define SET_VAL(what,what2) m_##what2->SetValue(cat->Header().what)
    SET_VAL(Team, team);
    SET_VAL(TeamEmail, teamEmail);
    SET_VAL(Project, project);
    SET_VAL(BasePath, basePath);
    SET_VAL(Language, language);
    SET_VAL(Charset, charset);
    #undef SET_VAL
    
    m_paths->SetStrings(cat->Header().SearchPaths);
    m_keywords->SetStrings(cat->Header().Keywords);
}

 
            
void SettingsDialog::TransferFrom(Catalog *cat)
{
    #define GET_VAL(what,what2) cat->Header().what = m_##what2->GetValue()
    GET_VAL(Language, language);
    GET_VAL(Charset, charset);
    GET_VAL(Team, team);
    GET_VAL(TeamEmail, teamEmail);
    GET_VAL(Project, project);
    GET_VAL(BasePath, basePath);
    #undef GET_VAL

    wxString dummy;
    wxArrayString arr;   
    
    cat->Header().SearchPaths.Clear();
    cat->Header().Keywords.Clear();

    m_paths->GetStrings(arr);
    for (size_t i = 0; i < arr.GetCount(); i++)
    {
        dummy = arr[i];
        if (dummy[dummy.Length() - 1] == _T('/') || 
                dummy[dummy.Length() - 1] == _T('\\')) 
            dummy.RemoveLast();
        cat->Header().SearchPaths.Add(dummy);
    }
    if (arr.GetCount() > 0 && cat->Header().BasePath.IsEmpty()) 
        cat->Header().BasePath = _T(".");

    m_keywords->GetStrings(arr);
    cat->Header().Keywords = arr;
}
