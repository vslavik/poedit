
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

#include <wx/wxprec.h>

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
    XMLCTRL(*this, "keep_crlf", wxCheckBox)->SetValue(
                              cfg->Read("keep_crlf", true));

    wxString format = cfg->Read("crlf_format", "unix");
    int sel;
    if (format == "win") sel = 1;
    else if (format == "mac") sel = 2;
    else if (format == "native") sel = 3;
    else /* "unix" */ sel = 0;

    XMLCTRL(*this, "crlf_format", wxChoice)->SetSelection(sel);


    m_parsers.Read(cfg);               
    
    wxListBox *list = XMLCTRL(*this, "parsers_list", wxListBox);
    for (unsigned i = 0; i < m_parsers.GetCount(); i++)
        list->Append(m_parsers[i].Name);
    
    if (m_parsers.GetCount() == 0)
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
    cfg->Write("keep_crlf", 
               XMLCTRL(*this, "keep_crlf", wxCheckBox)->GetValue());
    
    static char *formats[] = { "unix", "win", "mac", "native" };
    cfg->Write("crlf_format", formats[
                XMLCTRL(*this, "crlf_format", wxChoice)->GetSelection()]);
               
    m_parsers.Write(cfg);
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
    
    Parser& nfo = m_parsers[num];
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
    m_parsers.Add(info);
    XMLCTRL(*this, "parsers_list", wxListBox)->Append("");
    size_t index = m_parsers.GetCount()-1;
    if (!EditParser(index))
    {
        XMLCTRL(*this, "parsers_list", wxListBox)->Delete(index);
        m_parsers.RemoveAt(index);
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
    m_parsers.RemoveAt(index);
    XMLCTRL(*this, "parsers_list", wxListBox)->Delete(index);
    if (m_parsers.GetCount() == 0)
    {
        XMLCTRL(*this, "parser_edit", wxButton)->Enable(false);
        XMLCTRL(*this, "parser_delete", wxButton)->Enable(false);
    }
}
