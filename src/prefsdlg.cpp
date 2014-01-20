/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2014 Vaclav Slavik
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

#include <memory>

#include <wx/editlbox.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/config.h>
#include <wx/choicdlg.h>
#include <wx/spinctrl.h>
#include <wx/notebook.h>
#include <wx/fontutil.h>
#include <wx/fontpicker.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/windowptr.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/progdlg.h>
#include <wx/xrc/xmlres.h>
#include <wx/numformatter.h>

#include "prefsdlg.h"
#include "edapp.h"
#include "catalog.h"
#include "tm/transmem.h"
#include "chooselang.h"
#include "errors.h"

#ifdef __WXMSW__
#include <winsparkle.h>
#endif

#ifdef USE_SPARKLE
#include "osx_helpers.h"
#endif // USE_SPARKLE

class PreferencesPage : public wxPanel
{
public:
    virtual void LoadSettings() = 0;
    virtual void WriteSettings() = 0;

protected:
    PreferencesPage(wxWindow *parent)
        : wxPanel(parent, wxID_ANY),
          m_cfg(wxConfig::Get())
    {}

    wxConfigBase *m_cfg;
};


namespace
{

class TMPage : public PreferencesPage
{
public:
    TMPage(wxWindow *parent) : PreferencesPage(parent)
    {
        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        SetSizer(sizer);

        sizer->AddSpacer(10);
        m_useTM = new wxCheckBox(this, wxID_ANY, _("Use translation memory"));
        sizer->Add(m_useTM, wxSizerFlags().Expand().Border(wxALL));

        m_stats = new wxStaticText(this, wxID_ANY, "--\n--", wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
        sizer->AddSpacer(10);
        sizer->Add(m_stats, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 25));
        sizer->AddSpacer(10);

        auto import = new wxButton(this, wxID_ANY, _("Learn From Files..."));
        sizer->Add(import, wxSizerFlags().Border(wxLEFT|wxRIGHT, 25));
        sizer->AddSpacer(10);

        m_useTMWhenUpdating = new wxCheckBox(this, wxID_ANY, _("Consult TM when updating from sources"));
        sizer->Add(m_useTMWhenUpdating, wxSizerFlags().Expand().Border(wxALL));
        auto explain = new wxStaticText(this, wxID_ANY, _("If enabled, Poedit will try to fill in missing translations."));
        sizer->Add(explain, wxSizerFlags().Expand().Border(wxLEFT, 25));

#ifdef __WXOSX__
        m_stats->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        import->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        explain->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#else
        explain->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
#endif

        m_useTMWhenUpdating->Bind(wxEVT_UPDATE_UI, &TMPage::OnUpdateUI, this);
        m_stats->Bind(wxEVT_UPDATE_UI, &TMPage::OnUpdateUI, this);
        explain->Bind(wxEVT_UPDATE_UI, &TMPage::OnUpdateUI, this);
        import->Bind(wxEVT_UPDATE_UI, &TMPage::OnUpdateUI, this);

        import->Bind(wxEVT_BUTTON, &TMPage::OnImportIntoTM, this);

        UpdateStats();
    }

    virtual void LoadSettings()
    {
        m_useTM->SetValue(m_cfg->ReadBool("use_tm", true));
        m_useTMWhenUpdating->SetValue(m_cfg->ReadBool("use_tm_when_updating", false));
    }

    virtual void WriteSettings()
    {
        m_cfg->Write("use_tm", m_useTM->GetValue());
        m_cfg->Write("use_tm_when_updating", m_useTMWhenUpdating->GetValue());
    }

private:
    void UpdateStats()
    {
        wxString sDocs("--");
        wxString sFileSize("--");
        if (m_cfg->ReadBool("use_tm", true))
        {
            try
            {
                long docs, fileSize;
                TranslationMemory::Get().GetStats(docs, fileSize);
                sDocs.Printf("<b>%s</b>", wxNumberFormatter::ToString(docs));
                sFileSize.Printf("<b>%s</b>", wxFileName::GetHumanReadableSize(fileSize, "--", 1, wxSIZE_CONV_SI));
            }
            catch (Exception&)
            {
                // ignore Lucene errors -- if the index doesn't exist yet, just show --
            }
        }

        m_stats->SetLabelMarkup(wxString::Format(
            "%s %s\n%s %s",
            _("Stored translations:"),      sDocs,
            _("Database size on disk:"),    sFileSize
        ));
    }

    void OnImportIntoTM(wxCommandEvent&)
    {
        wxWindowPtr<wxFileDialog> dlg(new wxFileDialog(
            this,
            _("Select translation files to import"),
            wxEmptyString,
            wxEmptyString,
            _("GNU gettext catalogs (*.po)|*.po|All files (*.*)|*.*"),
            wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE));

        dlg->ShowWindowModalThenDo([=](int retcode){
            if (retcode != wxID_OK)
                return;

            wxArrayString paths;
            dlg->GetPaths(paths);

            wxProgressDialog progress(_("Translation Memory"),
                                      _("Importing translations..."),
                                      (int)paths.size() * 2 + 1,
                                      this,
                                      wxPD_APP_MODAL|wxPD_AUTO_HIDE|wxPD_CAN_ABORT);
            auto tm = TranslationMemory::Get().CreateWriter();
            int step = 0;
            for (size_t i = 0; i < paths.size(); i++)
            {
                std::unique_ptr<Catalog> cat(new Catalog(paths[i]));
                if (!progress.Update(++step))
                    break;
                if (cat->IsOk())
                    tm->Insert(*cat);
                if (!progress.Update(++step))
                    break;
            }
            progress.Pulse(_("Finalizing..."));
            tm->Commit();
            UpdateStats();
        });
    }

    void OnUpdateUI(wxUpdateUIEvent& e)
    {
        e.Enable(m_useTM->GetValue());
    }

    wxCheckBox *m_useTM, *m_useTMWhenUpdating;
    wxStaticText *m_stats;
};

} // anonymous namespace


PreferencesDialog::PreferencesDialog(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "preferences");

    wxNotebook *nb = XRCCTRL(*this, "prefs_notebook", wxNotebook);
    m_pageTM = new TMPage(nb);
    nb->InsertPage(2, m_pageTM, _("Translation Memory"));

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
                cfg->ReadBool("show_summary", false));
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

    m_pageTM->LoadSettings();
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

    m_pageTM->WriteSettings();
}



BEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
   EVT_BUTTON(XRCID("parser_new"), PreferencesDialog::OnNewParser)
   EVT_BUTTON(XRCID("parser_edit"), PreferencesDialog::OnEditParser)
   EVT_BUTTON(XRCID("parser_delete"), PreferencesDialog::OnDeleteParser)
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

