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

#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/gizmos/editlbox.h>
#include <wx/xml/xmlres.h>

#include "iso639.h"
#include "settingsdlg.h"


SettingsDialog::SettingsDialog(wxWindow *parent)
{
    wxTheXmlResource->LoadDialog(this, parent, "settings");

    m_Team = XMLCTRL(*this, "team_name", wxTextCtrl);
    m_TeamEmail = XMLCTRL(*this, "team_email", wxTextCtrl);
    m_Project = XMLCTRL(*this, "prj_name", wxTextCtrl);
    m_Language = XMLCTRL(*this, "language", wxComboBox);
    m_Charset = XMLCTRL(*this, "charset", wxComboBox);
    m_BasePath = XMLCTRL(*this, "basepath", wxTextCtrl);

    const LanguageStruct *lang = isoLanguages; /*from iso639.h*/ 
    m_Language->Append("");
    while (lang->lang != NULL)
        m_Language->Append((lang++)->lang);
        
    // my custom controls:
    wxPanel *panel;
    wxSizer *sizer;
    
    panel = XMLCTRL(*this, "keywords_list_panel", wxPanel);
    sizer = new wxBoxSizer(wxHORIZONTAL);
    m_Keywords = new wxEditableListBox(panel, -1, _("Keywords"));
    sizer->Add(m_Keywords, 1, wxEXPAND);
    panel->SetSizer(sizer);
    panel->SetAutoLayout(TRUE);
    panel->Layout();

    panel = XMLCTRL(*this, "paths_list_panel", wxPanel);
    sizer = new wxBoxSizer(wxHORIZONTAL);
    m_Paths = new wxEditableListBox(panel, -1, _("Paths"));
    sizer->Add(m_Paths, 1, wxEXPAND);
    panel->SetSizer(sizer);
    panel->SetAutoLayout(TRUE);
    panel->Layout();
}


void SettingsDialog::TransferTo(Catalog *cat)
{
    #define SET_VAL(what) m_##what->SetValue(cat->Header().what)
    SET_VAL(Team);
    SET_VAL(TeamEmail);
    SET_VAL(Project);
    SET_VAL(BasePath);
    #undef SET_VAL
    m_Language->SetSelection(m_Language->FindString(cat->Header().Language));
    m_Charset->SetSelection(m_Charset->FindString(cat->Header().Charset));
    
    m_Paths->SetStrings(cat->Header().SearchPaths);
    m_Keywords->SetStrings(cat->Header().Keywords);
}

 
            
void SettingsDialog::TransferFrom(Catalog *cat)
{
    #define GET_VAL(what) cat->Header().what = m_##what->GetValue()
    GET_VAL(Language);
    GET_VAL(Charset);
    GET_VAL(Team);
    GET_VAL(TeamEmail);
    GET_VAL(Project);
    GET_VAL(BasePath);
    #undef GET_VAL

    wxString dummy;
    wxArrayString arr;   
    
    cat->Header().SearchPaths.Clear();
    cat->Header().Keywords.Clear();

    m_Paths->GetStrings(arr);
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

    m_Keywords->GetStrings(arr);
    cat->Header().Keywords = arr;
}
