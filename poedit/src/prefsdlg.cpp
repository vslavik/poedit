
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
#include <wx/gizmos/editlbox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <wx/choicdlg.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/xrc/xmlres.h>

#include "prefsdlg.h"
#include "iso639.h"
#include "transmemupd.h"
#include "transmem.h"
#include "progressinfo.h"

PreferencesDialog::PreferencesDialog(wxWindow *parent)
{
    wxTheXmlResource->LoadDialog(this, parent, "preferences");
#ifdef USE_TRANSMEM
    wxTheXmlResource->AttachUnknownControl("tm_langs", 
                new wxEditableListBox(this, -1, _("My Languages")));
#else
    // remove "Translation Memory" page if support not compiled-in
    XMLCTRL(*this, "notebook", wxNotebook)->DeletePage(1);
#endif                
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
    XMLCTRL(*this, "textctrl_style", wxRadioBox)->SetSelection(
                cfg->Read("multiline_textctrl", true) ? 0 : 1);

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

#ifdef USE_TRANSMEM        
    XMLCTRL(*this, "tm_dbpath", wxTextCtrl)->SetValue(
                cfg->Read("TM/database_path", ""));

    wxStringTokenizer tkn(cfg->Read("TM/languages", ""), ":");
    wxArrayString langs;
    while (tkn.HasMoreTokens()) langs.Add(tkn.GetNextToken());
    XMLCTRL(*this, "tm_langs", wxEditableListBox)->SetStrings(langs);

    XMLCTRL(*this, "tm_omits", wxSpinCtrl)->SetValue(
                cfg->Read("TM/max_omitted", 2));
    XMLCTRL(*this, "tm_delta", wxSpinCtrl)->SetValue(
                cfg->Read("TM/max_delta", 2));
    XMLCTRL(*this, "tm_automatic", wxCheckBox)->SetValue(
                cfg->Read("use_tm_when_updating", true));
#endif
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
    cfg->Write("multiline_textctrl",
                XMLCTRL(*this, "textctrl_style", wxRadioBox)->GetSelection()==0);
    
    static char *formats[] = { "unix", "win", "mac", "native" };
    cfg->Write("crlf_format", formats[
                XMLCTRL(*this, "crlf_format", wxChoice)->GetSelection()]);
               
    m_parsers.Write(cfg);

#ifdef USE_TRANSMEM
    wxArrayString langs;
    XMLCTRL(*this, "tm_langs", wxEditableListBox)->GetStrings(langs);
    wxString languages;
    for (size_t i = 0; i < langs.GetCount(); i++)
    {
        if (i != 0) languages << ':';
        languages << langs[i];
    }
    cfg->Write("TM/languages", languages);
    cfg->Write("TM/database_path",
                XMLCTRL(*this, "tm_dbpath", wxTextCtrl)->GetValue());
    cfg->Write("TM/max_omitted", 
                (long)XMLCTRL(*this, "tm_omits", wxSpinCtrl)->GetValue());
    cfg->Write("TM/max_delta", 
                (long)XMLCTRL(*this, "tm_delta", wxSpinCtrl)->GetValue());
    cfg->Write("use_tm_when_updating", 
                XMLCTRL(*this, "tm_automatic", wxCheckBox)->GetValue());
#endif
}



BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
   EVT_BUTTON(XMLID("parser_new"), PreferencesDialog::OnNewParser)
   EVT_BUTTON(XMLID("parser_edit"), PreferencesDialog::OnEditParser)
   EVT_BUTTON(XMLID("parser_delete"), PreferencesDialog::OnDeleteParser)
#ifdef USE_TRANSMEM
   EVT_BUTTON(XMLID("tm_addlang"), PreferencesDialog::OnTMAddLang)
   EVT_BUTTON(XMLID("tm_browsedbpath"), PreferencesDialog::OnTMBrowseDbPath)
   EVT_BUTTON(XMLID("tm_generate"), PreferencesDialog::OnTMGenerate)
#endif
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

#ifdef USE_TRANSMEM

void PreferencesDialog::OnTMAddLang(wxCommandEvent& event)
{
    wxArrayString lngs;
    int index;
    
    for (const LanguageStruct *i = isoLanguages; i->lang != NULL; i++)
        lngs.Add(wxString(i->iso) + " (" + i->lang + ")");
    index = wxGetSingleChoiceIndex(_("Select language"), 
                                   _("Please select language ISO code:"),
                                   lngs, this);
    if (index != -1)
    {
        wxArrayString a;
        XMLCTRL(*this, "tm_langs", wxEditableListBox)->GetStrings(a);
        a.Add(isoLanguages[index].iso);
        XMLCTRL(*this, "tm_langs", wxEditableListBox)->SetStrings(a);
    }
}

void PreferencesDialog::OnTMBrowseDbPath(wxCommandEvent& event)
{
    wxDirDialog dlg(this, _("Select directory"), 
                    XMLCTRL(*this, "tm_dbpath", wxTextCtrl)->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        XMLCTRL(*this, "tm_dbpath", wxTextCtrl)->SetValue(dlg.GetPath());
}



class TMSearchDlg : public wxDialog
{
    protected:
        DECLARE_EVENT_TABLE()
        void OnBrowse(wxCommandEvent& event);
};

BEGIN_EVENT_TABLE(TMSearchDlg, wxDialog)
   EVT_BUTTON(XMLID("tm_adddir"), TMSearchDlg::OnBrowse)
END_EVENT_TABLE()

void TMSearchDlg::OnBrowse(wxCommandEvent& event)
{
    wxDirDialog dlg(this, _("Select directory"));
    if (dlg.ShowModal() == wxID_OK)
    {
        wxArrayString a;
        wxEditableListBox *l = XMLCTRL(*this, "tm_dirs", wxEditableListBox);
        l->GetStrings(a);
        a.Add(dlg.GetPath());
        l->SetStrings(a);
    }
}

void PreferencesDialog::OnTMGenerate(wxCommandEvent& event)
{
    // 1. Get paths list from the user:
    wxConfigBase *cfg = wxConfig::Get();
    TMSearchDlg dlg;
    wxTheXmlResource->LoadDialog(&dlg, this, "dlg_generate_tm");

    wxEditableListBox *dirs = 
        new wxEditableListBox(&dlg, -1, _("Search Paths"));
    wxTheXmlResource->AttachUnknownControl("tm_dirs", dirs);

    wxString dirsStr = cfg->Read("TM/search_paths", "");
    wxArrayString dirsArray;
    wxStringTokenizer tkn(dirsStr, wxPATH_SEP);

    while (tkn.HasMoreTokens()) dirsArray.Add(tkn.GetNextToken());
    dirs->SetStrings(dirsArray);

    if (dlg.ShowModal() == wxID_OK)
    {
        dirs->GetStrings(dirsArray);
        dirsStr = "";
        for (size_t i = 0; i < dirsArray.GetCount(); i++)
        {
            if (i != 0) dirsStr << wxPATH_SEP;
            dirsStr << dirsArray[i];
        }
        cfg->Write("TM/search_paths", dirsStr);
    }
    else return;
    
    // 2. Update TM:
    wxString dbPath = cfg->Read("TM/database_path", "");
    tkn.SetString(cfg->Read("TM/languages", ""), ":");
    wxArrayString langs;
    while (tkn.HasMoreTokens()) langs.Add(tkn.GetNextToken());

    ProgressInfo *pi = new ProgressInfo;
    pi->SetTitle(_("Updating translation memory"));
    for (size_t i = 0; i < langs.GetCount(); i++)
    {
        TranslationMemory *tm = 
            new TranslationMemory(langs[i], dbPath);
        TranslationMemoryUpdater u(tm, pi);
        if (!u.Update(dirsArray)) 
            { delete tm; break; }
        delete tm;
    }
    delete pi;
}

#endif // USE_TRANSMEM
