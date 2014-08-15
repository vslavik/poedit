/*
 *  This file is part of Poedit (http://poedit.net)
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
#include <wx/hyperlink.h>
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
#include "spellchecking.h"

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

        auto explainTxt = _("If enabled, Poedit will try to fill in new entries using your previous\n"
                            "translations stored in the translation memory. If the TM is\n"
                            "near-empty, it will not be very effective. The more translations\n"
                            "you edit and the larger the TM grows, the better it gets.");
        auto explain = new wxStaticText(this, wxID_ANY, explainTxt);
        sizer->Add(explain, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 25));

        auto learnMore = new wxHyperlinkCtrl(this, wxID_ANY, _("Learn more"), "http://poedit.net/trac/wiki/Doc/TranslationMemory");
        sizer->AddSpacer(5);
        sizer->Add(learnMore, wxSizerFlags().Border(wxLEFT, 25));

#ifdef __WXOSX__
        m_stats->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        import->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        explain->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        learnMore->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
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
			wxString::Format("%s (*.po)|*.po|%s (*.*)|*.*",
                _("PO Translation Files"), _("All Files")),
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

#ifndef USE_SPELLCHECKING
    // remove "Automatic spellchecking" checkbox:
    wxWindow *spellcheck = XRCCTRL(*this, "enable_spellchecking", wxCheckBox);
    spellcheck->GetContainingSizer()->Show(spellcheck, false);
#endif
#ifdef __WXMSW__
    if (!IsSpellcheckingAvailable())
    {
        auto spellcheck = XRCCTRL(*this, "enable_spellchecking", wxCheckBox);
        spellcheck->Disable();
        spellcheck->SetValue(false);
        spellcheck->SetLabel(spellcheck->GetLabel() + " (Windows 8)");
    }
#endif

#if !NEED_CHOOSELANG_UI
    // remove (defunct on Unix) "Change UI language" button:
    XRCCTRL(*this, "ui_language", wxButton)->Show(false);
#endif

    // re-layout the dialog:
    Layout();
    GetSizer()->SetSizeHints(this);
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
    if (IsSpellcheckingAvailable())
    {
        XRCCTRL(*this, "enable_spellchecking", wxCheckBox)->SetValue(
                    cfg->ReadBool("enable_spellchecking", true));
    }
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

    m_extractors.Read(cfg);
    
    wxListBox *list = XRCCTRL(*this, "extractors_list", wxListBox);
    for (const auto& item: m_extractors.Data)
        list->Append(item.Name);
    
    if (m_extractors.Data.empty())
    {
        XRCCTRL(*this, "extractor_edit", wxButton)->Enable(false);
        XRCCTRL(*this, "extractor_delete", wxButton)->Enable(false);
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

#if defined(__WXOSX__) && !defined(USE_SPARKLE)
    XRCCTRL(*this, "auto_updates", wxCheckBox)->Hide();
    XRCCTRL(*this, "beta_versions", wxCheckBox)->Hide();
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
    if (IsSpellcheckingAvailable())
    {
        cfg->Write("enable_spellchecking",
                    XRCCTRL(*this, "enable_spellchecking", wxCheckBox)->GetValue());
    }
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
               
    m_extractors.Write(cfg);

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
   EVT_BUTTON(XRCID("extractor_new"), PreferencesDialog::OnNewExtractor)
   EVT_BUTTON(XRCID("extractor_edit"), PreferencesDialog::OnEditExtractor)
   EVT_BUTTON(XRCID("extractor_delete"), PreferencesDialog::OnDeleteExtractor)
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
void PreferencesDialog::EditExtractor(int num, TFunctor completionHandler)
{
    wxWindowPtr<wxDialog> dlg(wxXmlResource::Get()->LoadDialog(this, "edit_extractor"));
    dlg->Centre();

    auto extractor_language = XRCCTRL(*dlg, "extractor_language", wxTextCtrl);
    auto extractor_extensions = XRCCTRL(*dlg, "extractor_extensions", wxTextCtrl);
    auto extractor_command = XRCCTRL(*dlg, "extractor_command", wxTextCtrl);
    auto extractor_keywords = XRCCTRL(*dlg, "extractor_keywords", wxTextCtrl);
    auto extractor_files = XRCCTRL(*dlg, "extractor_files", wxTextCtrl);
    auto extractor_charset = XRCCTRL(*dlg, "extractor_charset", wxTextCtrl);

    {
        const Extractor& nfo = m_extractors.Data[num];
        extractor_language->SetValue(nfo.Name);
        extractor_extensions->SetValue(nfo.Extensions);
        extractor_command->SetValue(nfo.Command);
        extractor_keywords->SetValue(nfo.KeywordItem);
        extractor_files->SetValue(nfo.FileItem);
        extractor_charset->SetValue(nfo.CharsetItem);
    }

    dlg->Bind
    (
        wxEVT_UPDATE_UI,
        [=](wxUpdateUIEvent& e){
            e.Enable(!extractor_language->IsEmpty() &&
                     !extractor_extensions->IsEmpty() &&
                     !extractor_command->IsEmpty() &&
                     !extractor_files->IsEmpty());
            // charset, keywords could in theory be empty if unsupported by the parser tool
        },
        wxID_OK
    );

    dlg->ShowWindowModalThenDo([=](int retcode){
        (void)dlg; // force use
        if (retcode == wxID_OK)
        {
            Extractor& nfo = m_extractors.Data[num];
            nfo.Name = extractor_language->GetValue();
            nfo.Extensions = extractor_extensions->GetValue();
            nfo.Command = extractor_command->GetValue();
            nfo.KeywordItem = extractor_keywords->GetValue();
            nfo.FileItem = extractor_files->GetValue();
            nfo.CharsetItem = extractor_charset->GetValue();
            XRCCTRL(*this, "extractors_list", wxListBox)->SetString(num, nfo.Name);
        }
        completionHandler(retcode == wxID_OK);
    });
}

void PreferencesDialog::OnNewExtractor(wxCommandEvent&)
{
    Extractor info;
    m_extractors.Data.push_back(info);
    XRCCTRL(*this, "extractors_list", wxListBox)->Append(wxEmptyString);
    int index = (int)m_extractors.Data.size()-1;
    EditExtractor(index, [=](bool added){
        if (added)
        {
            XRCCTRL(*this, "extractor_edit", wxButton)->Enable(true);
            XRCCTRL(*this, "extractor_delete", wxButton)->Enable(true);
        }
        else
        {
            XRCCTRL(*this, "extractors_list", wxListBox)->Delete(index);
            m_extractors.Data.erase(m_extractors.Data.begin() + index);
        }
    });
}

void PreferencesDialog::OnEditExtractor(wxCommandEvent&)
{
    EditExtractor(XRCCTRL(*this, "extractors_list", wxListBox)->GetSelection(), [](bool){});
}

void PreferencesDialog::OnDeleteExtractor(wxCommandEvent&)
{
    int index = XRCCTRL(*this, "extractors_list", wxListBox)->GetSelection();
    m_extractors.Data.erase(m_extractors.Data.begin() + index);
    XRCCTRL(*this, "extractors_list", wxListBox)->Delete(index);
    if (m_extractors.Data.empty())
    {
        XRCCTRL(*this, "extractor_edit", wxButton)->Enable(false);
        XRCCTRL(*this, "extractor_delete", wxButton)->Enable(false);
    }
}

