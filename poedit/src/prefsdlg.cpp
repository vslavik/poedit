
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      prefsdlg.cpp
    
      Preferences dialog
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/xml/xmlres.h>

#include "prefsdlg.h"


    


PreferencesDialog::PreferencesDialog(wxWindow *parent)
{
    wxTheXmlResource->LoadDialog(this, parent, "preferences");
    Centre();    
}



void PreferencesDialog::TransferTo(wxConfigBase *cfg)
{
    XMLCTRL(*this, "user_name", wxTextCtrl)->SetValue(
                              cfg->Read("translator_name", ""));
    XMLCTRL(*this, "user_email", wxTextCtrl)->SetValue(
                              cfg->Read("translator_email", ""));
    XMLCTRL(*this, "compile_mo", wxCheckBox)->SetValue(
                              cfg->Read("compile_mo", true));
    XMLCTRL(*this, "show_summary", wxCheckBox)->SetValue(
                              cfg->Read("show_summary", true));


    m_Parsers.Read(cfg);               
    
    wxListBox *list = XMLCTRL(*this, "parsers_list", wxListBox);
    for (unsigned i = 0; i < m_Parsers.GetCount(); i++)
        list->Append(m_Parsers[i].Name);
    
    if (m_Parsers.GetCount() == 0)
    {
        XMLCTRL(*this, "parser_edit", wxButton)->Enable(false);
        XMLCTRL(*this, "parser_delete", wxButton)->Enable(false);
    }
    else
        list->SetSelection(0);
}

 
            
void PreferencesDialog::TransferFrom(wxConfigBase *cfg)
{
    cfg->Write("translator_name", 
               XMLCTRL(*this, "user_name", wxTextCtrl)->GetValue());
    cfg->Write("translator_email", 
               XMLCTRL(*this, "user_email", wxTextCtrl)->GetValue());
    cfg->Write("compile_mo", 
               XMLCTRL(*this, "compile_mo", wxCheckBox)->GetValue());
    cfg->Write("show_summary", 
               XMLCTRL(*this, "show_summary", wxCheckBox)->GetValue());
               
    m_Parsers.Write(cfg);
}



BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
   EVT_BUTTON(XMLID("parser_new"), PreferencesDialog::OnNewParser)
   EVT_BUTTON(XMLID("parser_edit"), PreferencesDialog::OnEditParser)
   EVT_BUTTON(XMLID("parser_delete"), PreferencesDialog::OnDeleteParser)
END_EVENT_TABLE()


bool PreferencesDialog::EditParser(int num)
{
    wxDialog dlg;
    
    wxTheXmlResource->LoadDialog(&dlg, this, "edit_parser");
    dlg.Centre();
    
    Parser& nfo = m_Parsers[num];
    XMLCTRL(dlg, "parser_language", wxTextCtrl)->SetValue(nfo.Name);
    XMLCTRL(dlg, "parser_extensions", wxTextCtrl)->SetValue(nfo.Extensions);
    XMLCTRL(dlg, "parser_command", wxTextCtrl)->SetValue(nfo.Command);
    XMLCTRL(dlg, "parser_keywords", wxTextCtrl)->SetValue(nfo.KeywordItem);
    XMLCTRL(dlg, "parser_files", wxTextCtrl)->SetValue(nfo.FileItem);
    
    if (dlg.ShowModal() == wxID_OK)
    {
        nfo.Name = XMLCTRL(dlg, "parser_language", wxTextCtrl)->GetValue();
        nfo.Extensions = XMLCTRL(dlg, "parser_extensions", wxTextCtrl)->GetValue();
        nfo.Command = XMLCTRL(dlg, "parser_command", wxTextCtrl)->GetValue();
        nfo.KeywordItem = XMLCTRL(dlg, "parser_keywords", wxTextCtrl)->GetValue();
        nfo.FileItem = XMLCTRL(dlg, "parser_files", wxTextCtrl)->GetValue();
        XMLCTRL(*this, "parsers_list", wxListBox)->SetString(num, nfo.Name);
        
        return true;
    }
    else return false;
}



void PreferencesDialog::OnNewParser(wxCommandEvent& event)
{
    Parser info;
    m_Parsers.Add(info);
    XMLCTRL(*this, "parsers_list", wxListBox)->Append("");
    size_t index = m_Parsers.GetCount()-1;
    if (!EditParser(index))
    {
        XMLCTRL(*this, "parsers_list", wxListBox)->Delete(index);
        m_Parsers.RemoveAt(index);
    }
    else
    {
        XMLCTRL(*this, "parser_edit", wxButton)->Enable(true);
        XMLCTRL(*this, "parser_delete", wxButton)->Enable(true);
    }
}



void PreferencesDialog::OnEditParser(wxCommandEvent& event)
{
    EditParser(XMLCTRL(*this, "parsers_list", wxListBox)->GetSelection());
}



void PreferencesDialog::OnDeleteParser(wxCommandEvent& event)
{
    size_t index = XMLCTRL(*this, "parsers_list", wxListBox)->GetSelection();
    m_Parsers.RemoveAt(index);
    XMLCTRL(*this, "parsers_list", wxListBox)->Delete(index);
    if (m_Parsers.GetCount() == 0)
    {
        XMLCTRL(*this, "parser_edit", wxButton)->Enable(false);
        XMLCTRL(*this, "parser_delete", wxButton)->Enable(false);
    }
}
