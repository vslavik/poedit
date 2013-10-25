/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2013 Vaclav Slavik
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
 */

#include <wx/editlbox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/tokenzr.h>
#include <wx/config.h>
#include <wx/choicdlg.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/fontutil.h>
#include <wx/fontpicker.h>
#include <wx/windowptr.h>
#include <wx/xrc/xmlres.h>

#include "prefsdlg.h"
#include "edapp.h"
#include "isocodes.h"
#include "transmem.h"
#include "chooselang.h"

#ifdef __WXMSW__
#include <winsparkle.h>
#endif

#ifdef USE_SPARKLE
#include "osx_helpers.h"
#endif // USE_SPARKLE

PreferencesDialog::PreferencesDialog(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "preferences");
    wxXmlResource::Get()->AttachUnknownControl("tm_langs",
                new wxEditableListBox(this, -1, _("My Languages")));

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
                cfg->Read("translator_name", wxEmptyString));
    XRCCTRL(*this, "user_email", wxTextCtrl)->SetValue(
                cfg->Read("translator_email", wxEmptyString));
    XRCCTRL(*this, "compile_mo", wxCheckBox)->SetValue(
                cfg->ReadBool("compile_mo", true));
    XRCCTRL(*this, "show_summary", wxCheckBox)->SetValue(
                cfg->ReadBool("show_summary", true));
    XRCCTRL(*this, "manager_startup", wxCheckBox)->SetValue(
                cfg->ReadBool("manager_startup", false));
    XRCCTRL(*this, "focus_to_text", wxCheckBox)->SetValue(
                cfg->ReadBool("focus_to_text", false));
    XRCCTRL(*this, "comment_window_editable", wxCheckBox)->SetValue(
                cfg->ReadBool("comment_window_editable", false));
    XRCCTRL(*this, "keep_crlf", wxCheckBox)->SetValue(
                cfg->ReadBool("keep_crlf", true));
#ifdef USE_SPELLCHECKING
    XRCCTRL(*this, "enable_spellchecking", wxCheckBox)->SetValue(
                cfg->ReadBool("enable_spellchecking", true));
#endif

    XRCCTRL(*this, "use_font_list", wxCheckBox)->SetValue(
                cfg->ReadBool("custom_font_list_use", false));
    XRCCTRL(*this, "use_font_text", wxCheckBox)->SetValue(
                cfg->ReadBool("custom_font_text_use", false));
    XRCCTRL(*this, "font_list", wxFontPickerCtrl)->SetSelectedFont(
            wxFont(cfg->Read("custom_font_list_name", wxEmptyString)));
    XRCCTRL(*this, "font_text", wxFontPickerCtrl)->SetSelectedFont(
            wxFont(cfg->Read("custom_font_text_name", wxEmptyString)));

    wxString format = cfg->Read("crlf_format", "unix");
    int sel;
    if (format == "win") sel = 1;
    else /* "unix" or obsolete settings */ sel = 0;

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

    wxStringTokenizer tkn(cfg->Read("TM/languages", wxEmptyString), ":");
    wxArrayString langs;
    while (tkn.HasMoreTokens()) langs.Add(tkn.GetNextToken());
    XRCCTRL(*this, "tm_langs", wxEditableListBox)->SetStrings(langs);

    XRCCTRL(*this, "tm_omits", wxSpinCtrl)->SetValue(
                (int)cfg->Read("TM/max_omitted", 2));
    XRCCTRL(*this, "tm_delta", wxSpinCtrl)->SetValue(
                (int)cfg->Read("TM/max_delta", 2));
    XRCCTRL(*this, "tm_automatic", wxCheckBox)->SetValue(
                (int)cfg->Read("use_tm_when_updating", true));

#ifdef USE_SPARKLE
    XRCCTRL(*this, "auto_updates", wxCheckBox)->SetValue(
                (bool)UserDefaults_GetBoolValue("SUEnableAutomaticChecks"));
#endif // USE_SPARKLE
#ifdef __WXMSW__
    XRCCTRL(*this, "auto_updates", wxCheckBox)->SetValue(
                win_sparkle_get_automatic_check_for_updates() != 0);
#endif
#if defined(USE_SPARKLE) || defined(__WXMSW__)
    wxCheckBox *betas = XRCCTRL(*this, "beta_versions", wxCheckBox);
    betas->SetValue(wxGetApp().CheckForBetaUpdates());
    if (wxGetApp().IsBetaVersion())
        betas->Disable();
#endif
}
 
            
void PreferencesDialog::TransferFrom(wxConfigBase *cfg)
{
    cfg->Write("translator_name", 
                XRCCTRL(*this, "user_name", wxTextCtrl)->GetValue());
    cfg->Write("translator_email", 
                XRCCTRL(*this, "user_email", wxTextCtrl)->GetValue());
    cfg->Write("compile_mo", 
                XRCCTRL(*this, "compile_mo", wxCheckBox)->GetValue());
    cfg->Write("show_summary", 
                XRCCTRL(*this, "show_summary", wxCheckBox)->GetValue());
    cfg->Write("manager_startup", 
                XRCCTRL(*this, "manager_startup", wxCheckBox)->GetValue());
    cfg->Write("focus_to_text", 
                XRCCTRL(*this, "focus_to_text", wxCheckBox)->GetValue());
    cfg->Write("comment_window_editable", 
                XRCCTRL(*this, "comment_window_editable", wxCheckBox)->GetValue());
    cfg->Write("keep_crlf", 
                XRCCTRL(*this, "keep_crlf", wxCheckBox)->GetValue());
#ifdef USE_SPELLCHECKING
    cfg->Write("enable_spellchecking", 
                XRCCTRL(*this, "enable_spellchecking", wxCheckBox)->GetValue());
#endif
   
    wxFont listFont = XRCCTRL(*this, "font_list", wxFontPickerCtrl)->GetSelectedFont();
    wxFont textFont = XRCCTRL(*this, "font_text", wxFontPickerCtrl)->GetSelectedFont();

    cfg->Write("custom_font_list_use",
               listFont.IsOk() && XRCCTRL(*this, "use_font_list", wxCheckBox)->GetValue());
    cfg->Write("custom_font_text_use",
               textFont.IsOk() && XRCCTRL(*this, "use_font_text", wxCheckBox)->GetValue());
    if ( listFont.IsOk() )
        cfg->Write("custom_font_list_name", listFont.GetNativeFontInfoDesc());
    if ( textFont.IsOk() )
        cfg->Write("custom_font_text_name", textFont.GetNativeFontInfoDesc());

    static const char *formats[] = { "unix", "win" };
    cfg->Write("crlf_format", formats[
                XRCCTRL(*this, "crlf_format", wxChoice)->GetSelection()]);
               
    m_parsers.Write(cfg);

    wxArrayString langs;
    XRCCTRL(*this, "tm_langs", wxEditableListBox)->GetStrings(langs);
    wxString languages;
    for (size_t i = 0; i < langs.GetCount(); i++)
    {
        if (i != 0) languages << _T(':');
        languages << langs[i];
    }
    cfg->Write("TM/languages", languages);
    cfg->Write("TM/max_omitted", 
                (long)XRCCTRL(*this, "tm_omits", wxSpinCtrl)->GetValue());
    cfg->Write("TM/max_delta", 
                (long)XRCCTRL(*this, "tm_delta", wxSpinCtrl)->GetValue());
    cfg->Write("use_tm_when_updating", 
                XRCCTRL(*this, "tm_automatic", wxCheckBox)->GetValue());

#ifdef USE_SPARKLE
    UserDefaults_SetBoolValue("SUEnableAutomaticChecks",
                XRCCTRL(*this, "auto_updates", wxCheckBox)->GetValue());
#endif // USE_SPARKLE
#ifdef __WXMSW__
    win_sparkle_set_automatic_check_for_updates(
                XRCCTRL(*this, "auto_updates", wxCheckBox)->GetValue());
#endif
#if defined(USE_SPARKLE) || defined(__WXMSW__)
    if (!wxGetApp().IsBetaVersion())
    {
        cfg->Write("check_for_beta_updates",
                   XRCCTRL(*this, "beta_versions", wxCheckBox)->GetValue());
    }
#endif
}



BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
   EVT_BUTTON(XRCID("parser_new"), PreferencesDialog::OnNewParser)
   EVT_BUTTON(XRCID("parser_edit"), PreferencesDialog::OnEditParser)
   EVT_BUTTON(XRCID("parser_delete"), PreferencesDialog::OnDeleteParser)
   EVT_BUTTON(XRCID("tm_addlang"), PreferencesDialog::OnTMAddLang)
#if NEED_CHOOSELANG_UI
   EVT_BUTTON(XRCID("ui_language"), PreferencesDialog::OnUILanguage)
#endif
   EVT_UPDATE_UI(XRCID("font_list"), PreferencesDialog::OnUpdateUIFontList)
   EVT_UPDATE_UI(XRCID("font_text"), PreferencesDialog::OnUpdateUIFontText)
END_EVENT_TABLE()

#if NEED_CHOOSELANG_UI
void PreferencesDialog::OnUILanguage(wxCommandEvent&)
{
    ChangeUILanguage();
}
#endif


void PreferencesDialog::OnUpdateUIFontList(wxUpdateUIEvent& event)
{
    event.Enable(XRCCTRL(*this, "use_font_list", wxCheckBox)->GetValue());
}

void PreferencesDialog::OnUpdateUIFontText(wxUpdateUIEvent& event)
{
    event.Enable(XRCCTRL(*this, "use_font_text", wxCheckBox)->GetValue());
}


template<typename TFunctor>
void PreferencesDialog::EditParser(int num, TFunctor completionHandler)
{
    wxWindowPtr<wxDialog> dlg(wxXmlResource::Get()->LoadDialog(this, "edit_parser"));
    dlg->Centre();
    
    const Parser& nfo = m_parsers[num];
    XRCCTRL(*dlg, "parser_language", wxTextCtrl)->SetValue(nfo.Name);
    XRCCTRL(*dlg, "parser_extensions", wxTextCtrl)->SetValue(nfo.Extensions);
    XRCCTRL(*dlg, "parser_command", wxTextCtrl)->SetValue(nfo.Command);
    XRCCTRL(*dlg, "parser_keywords", wxTextCtrl)->SetValue(nfo.KeywordItem);
    XRCCTRL(*dlg, "parser_files", wxTextCtrl)->SetValue(nfo.FileItem);
    XRCCTRL(*dlg, "parser_charset", wxTextCtrl)->SetValue(nfo.CharsetItem);

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            Parser& nfo2 = m_parsers[num];
            nfo2.Name = XRCCTRL(*dlg, "parser_language", wxTextCtrl)->GetValue();
            nfo2.Extensions = XRCCTRL(*dlg, "parser_extensions", wxTextCtrl)->GetValue();
            nfo2.Command = XRCCTRL(*dlg, "parser_command", wxTextCtrl)->GetValue();
            nfo2.KeywordItem = XRCCTRL(*dlg, "parser_keywords", wxTextCtrl)->GetValue();
            nfo2.FileItem = XRCCTRL(*dlg, "parser_files", wxTextCtrl)->GetValue();
            nfo2.CharsetItem = XRCCTRL(*dlg, "parser_charset", wxTextCtrl)->GetValue();
            XRCCTRL(*this, "parsers_list", wxListBox)->SetString(num, nfo2.Name);
        }
        completionHandler(retcode == wxID_OK);
    });
}

void PreferencesDialog::OnNewParser(wxCommandEvent&)
{
    Parser info;
    m_parsers.Add(info);
    XRCCTRL(*this, "parsers_list", wxListBox)->Append(wxEmptyString);
    int index = (int)m_parsers.GetCount()-1;
    EditParser(index, [=](bool added){
        if (added)
        {
            XRCCTRL(*this, "parser_edit", wxButton)->Enable(true);
            XRCCTRL(*this, "parser_delete", wxButton)->Enable(true);
        }
        else
        {
            XRCCTRL(*this, "parsers_list", wxListBox)->Delete(index);
            m_parsers.RemoveAt(index);
        }
    });
}

void PreferencesDialog::OnEditParser(wxCommandEvent&)
{
    EditParser(XRCCTRL(*this, "parsers_list", wxListBox)->GetSelection(), [](bool){});
}

void PreferencesDialog::OnDeleteParser(wxCommandEvent&)
{
    int index = XRCCTRL(*this, "parsers_list", wxListBox)->GetSelection();
    m_parsers.RemoveAt(index);
    XRCCTRL(*this, "parsers_list", wxListBox)->Delete(index);
    if (m_parsers.GetCount() == 0)
    {
        XRCCTRL(*this, "parser_edit", wxButton)->Enable(false);
        XRCCTRL(*this, "parser_delete", wxButton)->Enable(false);
    }
}

void PreferencesDialog::OnTMAddLang(wxCommandEvent&)
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
        XRCCTRL(*this, "tm_langs", wxEditableListBox)->GetStrings(a);
        a.Add(isoLanguages[index].iso);
        XRCCTRL(*this, "tm_langs", wxEditableListBox)->SetStrings(a);
    }
}

