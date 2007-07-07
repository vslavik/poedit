/*
 *  This file is part of poEdit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2005 Vaclav Slavik
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 *
 *  $Id$
 *
 *  Preferences settings dialog
 *
 */

#include <wx/wxprec.h>
#include "wxeditablelistbox.h"
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <wx/choicdlg.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/fontdlg.h>
#include <wx/fontutil.h>
#include <wx/xrc/xmlres.h>

#include "prefsdlg.h"
#include "isocodes.h"
#include "transmem.h"
#include "transmemupd.h"
#include "transmemupd_wizard.h"
#include "chooselang.h"

PreferencesDialog::PreferencesDialog(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, _T("preferences"));
#ifdef USE_TRANSMEM
    wxXmlResource::Get()->AttachUnknownControl(_T("tm_langs"), 
                new wxEditableListBox(this, -1, _("My Languages")));
#else
    // remove "Translation Memory" page if support not compiled-in
	// Must first look for the index of the TM page and then delete
	// it from the notebook.
	// We must rely on the fact that the TM page has a name from
	// the XRC resources because it's text may have been localized.
	wxNotebook* nb = XRCCTRL(*this, "notebook", wxNotebook);
	wxNotebookPage* tmPage = XRCCTRL(*this, "tm_page", wxWindow);
	int tmIndex = -1;
	int i = 0;
	while (i < nb->GetPageCount() && tmIndex == -1)
	{
		if (nb->GetPage(i) == tmPage)
			tmIndex = i;
		i++;
	}

	// this test may be a bit superfluous, but is a safeguard in case 
	// the page isn't even available from the resources
	if (tmIndex != -1)	
		XRCCTRL(*this, "notebook", wxNotebook)->DeletePage(tmIndex);
#endif

#if !USE_SPELLCHECKING
    // remove "Automatic spellchecking" checkbox:
    wxWindow *spellcheck = XRCCTRL(*this, "enable_spellchecking", wxCheckBox);
    spellcheck->GetContainingSizer()->Show(spellcheck, false);
    // re-layout the dialog:
    GetSizer()->Fit(this);
#endif

#if !NEED_CHOOSELANG_UI
    // remove (defunct on Unix) "Change UI language" button:
    XRCCTRL(*this, "ui_language", wxButton)->Show(false);
#endif

#if defined(__WXMSW__) || defined(__WXMAC__)
    // FIXME
    SetSize(GetSize().x+1,GetSize().y+1);
#endif
}


void PreferencesDialog::TransferTo(wxConfigBase *cfg)
{
    XRCCTRL(*this, "user_name", wxTextCtrl)->SetValue(
                cfg->Read(_T("translator_name"), wxEmptyString));
    XRCCTRL(*this, "user_email", wxTextCtrl)->SetValue(
                cfg->Read(_T("translator_email"), wxEmptyString));
    XRCCTRL(*this, "compile_mo", wxCheckBox)->SetValue(
                cfg->Read(_T("compile_mo"), true));
    XRCCTRL(*this, "show_summary", wxCheckBox)->SetValue(
                cfg->Read(_T("show_summary"), true));
    XRCCTRL(*this, "manager_startup", wxCheckBox)->SetValue(
                (bool)cfg->Read(_T("manager_startup"), (long)false));
    XRCCTRL(*this, "focus_to_text", wxCheckBox)->SetValue(
                (bool)cfg->Read(_T("focus_to_text"), (long)false));
    XRCCTRL(*this, "comment_window_editable", wxCheckBox)->SetValue(
                (bool)cfg->Read(_T("comment_window_editable"), (long)false));
    XRCCTRL(*this, "ext_editor", wxComboBox)->SetValue(
                cfg->Read(_T("ext_editor"), wxEmptyString));
    XRCCTRL(*this, "open_editor_immediately", wxCheckBox)->SetValue(
                cfg->Read(_T("open_editor_immediately"), (long)false));
    XRCCTRL(*this, "keep_crlf", wxCheckBox)->SetValue(
                (bool)cfg->Read(_T("keep_crlf"), true));
#if USE_SPELLCHECKING
    XRCCTRL(*this, "enable_spellchecking", wxCheckBox)->SetValue(
                (bool)cfg->Read(_T("enable_spellchecking"), true));
#endif
    
    XRCCTRL(*this, "use_font_list", wxCheckBox)->SetValue(
                (bool)cfg->Read(_T("custom_font_list_use"), (long)false));
    XRCCTRL(*this, "use_font_text", wxCheckBox)->SetValue(
                (bool)cfg->Read(_T("custom_font_text_use"), (long)false));
    XRCCTRL(*this, "fontname_list", wxTextCtrl)->SetValue(
            cfg->Read(_T("custom_font_list_name"), wxEmptyString));
    XRCCTRL(*this, "fontname_text", wxTextCtrl)->SetValue(
            cfg->Read(_T("custom_font_text_name"), wxEmptyString));

    wxString format = cfg->Read(_T("crlf_format"), _T("unix"));
    int sel;
    if (format == _T("win")) sel = 1;
    else if (format == _T("mac")) sel = 2;
    else if (format == _T("native")) sel = 3;
    else /* _T("unix") */ sel = 0;

    XRCCTRL(*this, "crlf_format", wxChoice)->SetSelection(sel);

    m_parsers.Read(cfg);               
    
    wxListBox *list = XRCCTRL(*this, "parsers_list", wxListBox);
    for (unsigned i = 0; i < m_parsers.GetCount(); i++)
        list->Append(m_parsers[i].Name);
    
    if (m_parsers.GetCount() == 0)
    {
        XRCCTRL(*this, "parser_edit", wxButton)->Enable(false);
        XRCCTRL(*this, "parser_delete", wxButton)->Enable(false);
    }
    else
        list->SetSelection(0);

#ifdef USE_TRANSMEM        
    XRCCTRL(*this, "tm_dbpath", wxTextCtrl)->SetValue(
                cfg->Read(_T("TM/database_path"), wxEmptyString));

    wxStringTokenizer tkn(cfg->Read(_T("TM/languages"), wxEmptyString), _T(":"));
    wxArrayString langs;
    while (tkn.HasMoreTokens()) langs.Add(tkn.GetNextToken());
    XRCCTRL(*this, "tm_langs", wxEditableListBox)->SetStrings(langs);

    XRCCTRL(*this, "tm_omits", wxSpinCtrl)->SetValue(
                cfg->Read(_T("TM/max_omitted"), 2));
    XRCCTRL(*this, "tm_delta", wxSpinCtrl)->SetValue(
                cfg->Read(_T("TM/max_delta"), 2));
    XRCCTRL(*this, "tm_automatic", wxCheckBox)->SetValue(
                cfg->Read(_T("use_tm_when_updating"), true));
#endif
}
 
            
void PreferencesDialog::TransferFrom(wxConfigBase *cfg)
{
    cfg->Write(_T("translator_name"), 
                XRCCTRL(*this, "user_name", wxTextCtrl)->GetValue());
    cfg->Write(_T("translator_email"), 
                XRCCTRL(*this, "user_email", wxTextCtrl)->GetValue());
    cfg->Write(_T("compile_mo"), 
                XRCCTRL(*this, "compile_mo", wxCheckBox)->GetValue());
    cfg->Write(_T("show_summary"), 
                XRCCTRL(*this, "show_summary", wxCheckBox)->GetValue());
    cfg->Write(_T("manager_startup"), 
                XRCCTRL(*this, "manager_startup", wxCheckBox)->GetValue());
    cfg->Write(_T("focus_to_text"), 
                XRCCTRL(*this, "focus_to_text", wxCheckBox)->GetValue());
    cfg->Write(_T("comment_window_editable"), 
                XRCCTRL(*this, "comment_window_editable", wxCheckBox)->GetValue());
    cfg->Write(_T("ext_editor"), 
                XRCCTRL(*this, "ext_editor", wxComboBox)->GetValue());
    cfg->Write(_T("open_editor_immediately"), 
                XRCCTRL(*this, "open_editor_immediately", wxCheckBox)->GetValue());
    cfg->Write(_T("keep_crlf"), 
                XRCCTRL(*this, "keep_crlf", wxCheckBox)->GetValue());
#if USE_SPELLCHECKING
    cfg->Write(_T("enable_spellchecking"), 
                XRCCTRL(*this, "enable_spellchecking", wxCheckBox)->GetValue());
#endif
    
    cfg->Write(_T("custom_font_list_use"),
                XRCCTRL(*this, "use_font_list", wxCheckBox)->GetValue());
    cfg->Write(_T("custom_font_text_use"),
                XRCCTRL(*this, "use_font_text", wxCheckBox)->GetValue());
    cfg->Write(_T("custom_font_list_name"),
                XRCCTRL(*this, "fontname_list", wxTextCtrl)->GetValue());
    cfg->Write(_T("custom_font_text_name"),
                XRCCTRL(*this, "fontname_text", wxTextCtrl)->GetValue());
 
    static const wxChar *formats[] = 
        { _T("unix"), _T("win"), _T("mac"), _T("native") };
    cfg->Write(_T("crlf_format"), formats[
                XRCCTRL(*this, "crlf_format", wxChoice)->GetSelection()]);
               
    m_parsers.Write(cfg);

#ifdef USE_TRANSMEM
    wxArrayString langs;
    XRCCTRL(*this, "tm_langs", wxEditableListBox)->GetStrings(langs);
    wxString languages;
    for (size_t i = 0; i < langs.GetCount(); i++)
    {
        if (i != 0) languages << _T(':');
        languages << langs[i];
    }
    cfg->Write(_T("TM/languages"), languages);
    cfg->Write(_T("TM/database_path"),
                XRCCTRL(*this, "tm_dbpath", wxTextCtrl)->GetValue());
    cfg->Write(_T("TM/max_omitted"), 
                (long)XRCCTRL(*this, "tm_omits", wxSpinCtrl)->GetValue());
    cfg->Write(_T("TM/max_delta"), 
                (long)XRCCTRL(*this, "tm_delta", wxSpinCtrl)->GetValue());
    cfg->Write(_T("use_tm_when_updating"), 
                XRCCTRL(*this, "tm_automatic", wxCheckBox)->GetValue());
#endif
}



BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
   EVT_BUTTON(XRCID("parser_new"), PreferencesDialog::OnNewParser)
   EVT_BUTTON(XRCID("parser_edit"), PreferencesDialog::OnEditParser)
   EVT_BUTTON(XRCID("parser_delete"), PreferencesDialog::OnDeleteParser)
   EVT_BUTTON(XRCID("fontpicker_list"), PreferencesDialog::OnChooseListFont)
   EVT_BUTTON(XRCID("fontpicker_text"), PreferencesDialog::OnChooseTextFont)
#ifdef USE_TRANSMEM
   EVT_BUTTON(XRCID("tm_addlang"), PreferencesDialog::OnTMAddLang)
   EVT_BUTTON(XRCID("tm_browsedbpath"), PreferencesDialog::OnTMBrowseDbPath)
   EVT_BUTTON(XRCID("tm_generate"), PreferencesDialog::OnTMGenerate)
#endif
#if NEED_CHOOSELANG_UI
   EVT_BUTTON(XRCID("ui_language"), PreferencesDialog::OnUILanguage)
#endif
END_EVENT_TABLE()

#if NEED_CHOOSELANG_UI
void PreferencesDialog::OnUILanguage(wxCommandEvent& event)
{
    ChangeUILanguage();
}
#endif

bool PreferencesDialog::EditParser(int num)
{
    wxDialog dlg;
    
    wxXmlResource::Get()->LoadDialog(&dlg, this, _T("edit_parser"));
    dlg.Centre();
    
    Parser& nfo = m_parsers[num];
    XRCCTRL(dlg, "parser_language", wxTextCtrl)->SetValue(nfo.Name);
    XRCCTRL(dlg, "parser_extensions", wxTextCtrl)->SetValue(nfo.Extensions);
    XRCCTRL(dlg, "parser_command", wxTextCtrl)->SetValue(nfo.Command);
    XRCCTRL(dlg, "parser_keywords", wxTextCtrl)->SetValue(nfo.KeywordItem);
    XRCCTRL(dlg, "parser_files", wxTextCtrl)->SetValue(nfo.FileItem);
    XRCCTRL(dlg, "parser_charset", wxTextCtrl)->SetValue(nfo.CharsetItem);
    
    if (dlg.ShowModal() == wxID_OK)
    {
        nfo.Name = XRCCTRL(dlg, "parser_language", wxTextCtrl)->GetValue();
        nfo.Extensions = XRCCTRL(dlg, "parser_extensions", wxTextCtrl)->GetValue();
        nfo.Command = XRCCTRL(dlg, "parser_command", wxTextCtrl)->GetValue();
        nfo.KeywordItem = XRCCTRL(dlg, "parser_keywords", wxTextCtrl)->GetValue();
        nfo.FileItem = XRCCTRL(dlg, "parser_files", wxTextCtrl)->GetValue();
        nfo.CharsetItem = XRCCTRL(dlg, "parser_charset", wxTextCtrl)->GetValue();
        XRCCTRL(*this, "parsers_list", wxListBox)->SetString(num, nfo.Name);
        return true;
    }
    else
        return false;
}

void PreferencesDialog::OnNewParser(wxCommandEvent& event)
{
    Parser info;
    m_parsers.Add(info);
    XRCCTRL(*this, "parsers_list", wxListBox)->Append(wxEmptyString);
    size_t index = m_parsers.GetCount()-1;
    if (!EditParser(index))
    {
        XRCCTRL(*this, "parsers_list", wxListBox)->Delete(index);
        m_parsers.RemoveAt(index);
    }
    else
    {
        XRCCTRL(*this, "parser_edit", wxButton)->Enable(true);
        XRCCTRL(*this, "parser_delete", wxButton)->Enable(true);
    }
}

void PreferencesDialog::OnEditParser(wxCommandEvent& event)
{
    EditParser(XRCCTRL(*this, "parsers_list", wxListBox)->GetSelection());
}

void PreferencesDialog::OnDeleteParser(wxCommandEvent& event)
{
    size_t index = XRCCTRL(*this, "parsers_list", wxListBox)->GetSelection();
    m_parsers.RemoveAt(index);
    XRCCTRL(*this, "parsers_list", wxListBox)->Delete(index);
    if (m_parsers.GetCount() == 0)
    {
        XRCCTRL(*this, "parser_edit", wxButton)->Enable(false);
        XRCCTRL(*this, "parser_delete", wxButton)->Enable(false);
    }
}

#ifdef USE_TRANSMEM

void PreferencesDialog::OnTMAddLang(wxCommandEvent& event)
{
    wxArrayString lngs;
    int index;
    
    for (const LanguageStruct *i = isoLanguages; i->lang != NULL; i++)
        lngs.Add(wxString(i->iso) + _T(" (") + i->lang + _T(")"));
    index = wxGetSingleChoiceIndex(_("Select language"), 
                                   _("Please select language ISO code:"),
                                   lngs, this);
    if (index != -1)
    {
        wxArrayString a;
        XRCCTRL(*this, "tm_langs", wxEditableListBox)->GetStrings(a);
        a.Add(isoLanguages[index].iso);
        XRCCTRL(*this, "tm_langs", wxEditableListBox)->SetStrings(a);
    }
}

void PreferencesDialog::OnTMBrowseDbPath(wxCommandEvent& event)
{
    wxDirDialog dlg(this, _("Select directory"), 
                    XRCCTRL(*this, "tm_dbpath", wxTextCtrl)->GetValue());
    if (dlg.ShowModal() == wxID_OK)
        XRCCTRL(*this, "tm_dbpath", wxTextCtrl)->SetValue(dlg.GetPath());
}


void PreferencesDialog::OnTMGenerate(wxCommandEvent& event)
{
    wxString dbPath = XRCCTRL(*this, "tm_dbpath", wxTextCtrl)->GetValue();
        // VS: we can't get it from TM/database_path key in wxConfig object
        //     because it wasn't update yet with information from the dialog
        //     (which happens when the users presses OK) but we still won't
        //     to use the path entered by the user...

    wxArrayString langs;
    XRCCTRL(*this, "tm_langs", wxEditableListBox)->GetStrings(langs);

    RunTMUpdateWizard(this, dbPath, langs);
}

#endif // USE_TRANSMEM
        
void PreferencesDialog::DoChooseFont(wxTextCtrl *nameField)
{
    wxFont font;
    wxString fontname = nameField->GetValue();
    if (!fontname.empty())
    {
        wxNativeFontInfo fi;
        fi.FromString(fontname);
        font.SetNativeFontInfo(fi);
    }
    wxFontData data;
    data.EnableEffects(false);
    data.SetAllowSymbols(false);
    data.SetInitialFont(font);
    wxFontDialog dlg(this, data);

    if (dlg.ShowModal() == wxID_OK)
    {
        data = dlg.GetFontData();
        font = data.GetChosenFont();
        nameField->SetValue(font.GetNativeFontInfoDesc());
    }
}

void PreferencesDialog::OnChooseListFont(wxCommandEvent& event)
{
    DoChooseFont(XRCCTRL(*this, "fontname_list", wxTextCtrl));
}

void PreferencesDialog::OnChooseTextFont(wxCommandEvent& event)
{
    DoChooseFont(XRCCTRL(*this, "fontname_text", wxTextCtrl));
}
