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
#include <wx/gizmos/editlbox.h>
#include <wx/xml/xmlres.h>

#include "iso639.h"
#include "settingsdlg.h"


SettingsDialog::SettingsDialog(wxWindow *parent)
{
    wxTheXmlResource->LoadDialog(this, parent, "settings");

    m_team = XMLCTRL(*this, "team_name", wxTextCtrl);
    m_teamEmail = XMLCTRL(*this, "team_email", wxTextCtrl);
    m_project = XMLCTRL(*this, "prj_name", wxTextCtrl);
    m_language = XMLCTRL(*this, "language", wxComboBox);
    m_charset = XMLCTRL(*this, "charset", wxComboBox);
    m_basePath = XMLCTRL(*this, "basepath", wxTextCtrl);

    const LanguageStruct *lang = isoLanguages; /*from iso639.h*/ 
    m_language->Append("");
    while (lang->lang != NULL)
        m_language->Append((lang++)->lang);
        
    // my custom controls:
    wxPanel *panel;
    wxSizer *sizer;
    
    panel = XMLCTRL(*this, "keywords_list_panel", wxPanel);
    sizer = new wxBoxSizer(wxHORIZONTAL);
    m_keywords = new wxEditableListBox(panel, -1, _("Keywords"));
    sizer->Add(m_keywords, 1, wxEXPAND);
    panel->SetSizer(sizer);
    panel->SetAutoLayout(TRUE);
    panel->Layout();

    panel = XMLCTRL(*this, "paths_list_panel", wxPanel);
    sizer = new wxBoxSizer(wxHORIZONTAL);
    m_paths = new wxEditableListBox(panel, -1, _("Paths"));
    sizer->Add(m_paths, 1, wxEXPAND);
    panel->SetSizer(sizer);
    panel->SetAutoLayout(TRUE);
    panel->Layout();
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
        if (dummy[dummy.Length() - 1] == '/' || 
                dummy[dummy.Length() - 1] == '\\') 
            dummy.RemoveLast();
        cat->Header().SearchPaths.Add(dummy);
    }
    if (arr.GetCount() > 0 && cat->Header().BasePath.IsEmpty()) 
        cat->Header().BasePath = ".";

    m_keywords->GetStrings(arr);
    cat->Header().Keywords = arr;
}
