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

#include <wx/xml/xmlres.h>

#include "iso639.h"
#include "settingsdlg.h"


SettingsDialog::SettingsDialog(wxWindow *parent)
{
    wxTheXmlResource->LoadDialog(this, parent, "settings");
    Centre();    

    m_Team = XMLCTRL(*this, "team_name", wxTextCtrl);
    m_TeamEmail = XMLCTRL(*this, "team_email", wxTextCtrl);
    m_Project = XMLCTRL(*this, "prj_name", wxTextCtrl);
    m_Language = XMLCTRL(*this, "language", wxComboBox);
    m_Charset = XMLCTRL(*this, "charset", wxComboBox);
    m_OnePath = XMLCTRL(*this, "dir_input", wxTextCtrl);
    m_OneWild = XMLCTRL(*this, "keyword_input", wxTextCtrl);
    m_Paths = XMLCTRL(*this, "dirs_list", wxListBox);
    m_Keywords = XMLCTRL(*this, "keywords_list", wxListBox);
    m_BasePath = XMLCTRL(*this, "basepath", wxTextCtrl);


    const LanguageStruct *lang = isoLanguages; /*from iso639.h*/ 
    m_Language->Append("");
    while (lang->lang != NULL)
        m_Language->Append((lang++)->lang);
}

         
BEGIN_EVENT_TABLE(SettingsDialog, wxDialog)
   EVT_BUTTON(XMLID("keyword_add"), SettingsDialog::OnAddWild)
   EVT_BUTTON(XMLID("keyword_remove"), SettingsDialog::OnRemoveWild)
   EVT_BUTTON(XMLID("dir_add"), SettingsDialog::OnAddPath)
   EVT_BUTTON(XMLID("dir_remove"), SettingsDialog::OnRemovePath)
END_EVENT_TABLE()


void SettingsDialog::OnAddWild(wxCommandEvent&)
{
    wxString w = m_OneWild->GetValue();
    if (m_Keywords->FindString(w) == -1)
        m_Keywords->Append(w);
    m_OneWild->SetFocus();
    m_OneWild->SetValue("");
}



void SettingsDialog::OnRemoveWild(wxCommandEvent&)
{
    if (m_Keywords->Number() > 0) m_Keywords->Delete(m_Keywords->GetSelection());
}


void SettingsDialog::OnAddPath(wxCommandEvent&)
{
    wxString w = m_OnePath->GetValue();
    if (m_Paths->FindString(w) == -1)
        m_Paths->Append(w);
    m_OnePath->SetFocus();
    m_OnePath->SetValue("");
}



void SettingsDialog::OnRemovePath(wxCommandEvent&)
{
    if (m_Paths->Number() > 0) m_Paths->Delete(m_Paths->GetSelection());
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
    
    unsigned i;
    for (i = 0; i < cat->Header().SearchPaths.GetCount(); i++)
        m_Paths->Append(cat->Header().SearchPaths[i]);
    for (i = 0; i < cat->Header().Keywords.GetCount(); i++)
        m_Keywords->Append(cat->Header().Keywords[i]);

    if (m_Paths->GetCount() > 0) m_Paths->SetSelection(0);
    if (m_Keywords->GetCount() > 0) m_Keywords->SetSelection(0);
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
    int i;
    
    cat->Header().SearchPaths.Clear();
    cat->Header().Keywords.Clear();
    
    for (i = 0; i < m_Paths->GetCount(); i++)
    {
        dummy = m_Paths->GetString(i);
        if (dummy[dummy.Length() - 1] == '/' || dummy[dummy.Length() - 1] == '\\') dummy.RemoveLast();
        cat->Header().SearchPaths.Add(dummy);
    }
    if (m_Paths->GetCount() > 0 && cat->Header().BasePath.IsEmpty()) 
        cat->Header().BasePath = ".";

    for (i = 0; i < m_Keywords->GetCount(); i++)
    {
        dummy = m_Keywords->GetString(i);
        if (dummy[dummy.Length() - 1] == '/' || dummy[dummy.Length() - 1] == '\\') dummy.RemoveLast();
        cat->Header().Keywords.Add(dummy);
    }
}
