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
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/fontutil.h>
#include <wx/fontpicker.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/windowptr.h>
#include <wx/sizer.h>
#include <wx/settings.h>
#include <wx/spinctrl.h>
#include <wx/textwrapper.h>
#include <wx/progdlg.h>
#include <wx/xrc/xmlres.h>
#include <wx/numformatter.h>

#include "prefsdlg.h"
#include "edapp.h"
#include "edframe.h"
#include "catalog.h"
#include "tm/transmem.h"
#include "chooselang.h"
#include "errors.h"
#include "extractor.h"
#include "spellchecking.h"
#include "utility.h"
#include "customcontrols.h"

#ifdef __WXMSW__
#include <winsparkle.h>
#endif

#ifdef USE_SPARKLE
#include "osx_helpers.h"
#endif // USE_SPARKLE

#if defined(USE_SPARKLE) || defined(__WXMSW__)
    #define HAS_UPDATES_CHECK
#endif

namespace
{

class PrefsPanel : public wxPanel
{
public:
    PrefsPanel(wxWindow *parent)
        : wxPanel(parent), m_supressDataTransfer(0)
    {
#ifdef __WXOSX__
        // Refresh the content of prefs panels when re-opening it.
        // TODO: Use proper config settings notifications or user defaults bindings instead
        parent->Bind(wxEVT_ACTIVATE, [=](wxActivateEvent& e){
            e.Skip();
            if (e.GetActive())
                TransferDataToWindow();
        });
        Bind(wxEVT_SHOW, [=](wxShowEvent& e){
            e.Skip();
            if (e.IsShown())
                TransferDataToWindow();
        });
#endif // __WXOSX__
    }

    bool TransferDataToWindow() override
    {
        if (m_supressDataTransfer)
            return false;
        m_supressDataTransfer++;
        InitValues(*wxConfig::Get());
        m_supressDataTransfer--;

        // This is a "bit" of a hack: we take advantage of being in the last point before
        // showing the window and re-layout it on the off chance that some data transfered
        // into the window affected its size. And, currently more importantly, to reflect
        // ExplanationLabel instances' rewrapping.
        Fit();
#ifndef __WXOSX__
        GetParent()->GetParent()->Fit();
#endif
        return true;
    }

    bool TransferDataFromWindow() override
    {
        if (m_supressDataTransfer)
            return false;
        m_supressDataTransfer++;
        SaveValues(*wxConfig::Get());
        m_supressDataTransfer--;
        return true;
    }

protected:
    void TransferDataFromWindowAndUpdateUI(wxCommandEvent&)
    {
        TransferDataFromWindow();
        PoeditFrame::UpdateAllAfterPreferencesChange();
    }

    virtual void InitValues(const wxConfigBase& cfg) = 0;
    virtual void SaveValues(wxConfigBase& cfg) = 0;

    int m_supressDataTransfer;
};


#ifdef __WXOSX__
    #define BORDER_WIN(dir, n) Border(dir, 0)
    #define BORDER_OSX(dir, n) Border(dir, n)
#else
    #define BORDER_WIN(dir, n) Border(dir, n)
    #define BORDER_OSX(dir, n) Border(dir, 0)
#endif


class GeneralPageWindow : public PrefsPanel
{
public:
    GeneralPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->SetMinSize(400, -1);

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags(1).Expand().DoubleBorder());
        SetSizer(topsizer);

        sizer->Add(new HeadingLabel(this, _("Information about the translator")));
        sizer->AddSpacer(10);

        auto translator = new wxFlexGridSizer(2, wxSize(5,6));
        translator->AddGrowableCol(1);
        sizer->Add(translator, wxSizerFlags().Expand());

        auto nameLabel = new wxStaticText(this, wxID_ANY, _("Name:"));
        translator->Add(nameLabel, wxSizerFlags().Center().Right().BORDER_OSX(wxTOP, 1));
        m_userName = new wxTextCtrl(this, wxID_ANY);
        m_userName->SetHint(_("Your Name"));
        translator->Add(m_userName, wxSizerFlags(1).Expand().Center());
        auto emailLabel = new wxStaticText(this, wxID_ANY, _("Email:"));
        translator->Add(emailLabel, wxSizerFlags().Center().Right().BORDER_OSX(wxTOP, 1));
        m_userEmail = new wxTextCtrl(this, wxID_ANY);
        m_userEmail->SetHint(_("your_email@example.com"));
        translator->Add(m_userEmail, wxSizerFlags(1).Expand().Center());
        translator->AddSpacer(1);
        translator->Add(new ExplanationLabel(this, _("Your name and email address are only used to set the Last-Translator header of GNU gettext files.")), wxSizerFlags(1).Expand().Border(wxRIGHT));
#ifdef __WXOSX__
        nameLabel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        emailLabel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        m_userName->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        m_userEmail->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

        sizer->AddSpacer(10);
        sizer->Add(new HeadingLabel(this, _("Editing")));
        sizer->AddSpacer(10);

        m_compileMo = new wxCheckBox(this, wxID_ANY, _("Automatically compile MO file when saving"));
        sizer->Add(m_compileMo);
        m_showSummary = new wxCheckBox(this, wxID_ANY, _("Show summary after catalog update"));
        sizer->Add(m_showSummary, wxSizerFlags().Border(wxTOP));

        sizer->AddSpacer(10);

        m_spellchecking = new wxCheckBox(this, wxID_ANY, _("Check spelling"));
        sizer->Add(m_spellchecking, wxSizerFlags().Border(wxTOP));
        m_focusToText = new wxCheckBox(this, wxID_ANY, _("Always change focus to text input field"));
        sizer->Add(m_focusToText, wxSizerFlags().Border(wxTOP));
        wxString explainFocus(_("Never let the list of strings take focus. If enabled, you must use Ctrl-arrows for keyboard navigation but you can also type text immediately, without having to press Tab to change focus."));
#ifdef __WXOSX__
        explainFocus.Replace("Ctrl", "Cmd");
#endif
        sizer->AddSpacer(5);
        sizer->Add(new ExplanationLabel(this, explainFocus), wxSizerFlags().Expand().Border(wxLEFT, ExplanationLabel::CHECKBOX_INDENT));

        sizer->AddSpacer(10);
        sizer->Add(new HeadingLabel(this, _("Appearance")));
        sizer->AddSpacer(4);

        auto appearance = new wxFlexGridSizer(2, wxSize(5,1));
        appearance->AddGrowableCol(1);
        sizer->Add(appearance, wxSizerFlags().Expand());

        m_useFontList = new wxCheckBox(this, wxID_ANY, _("Use custom list font:"));
        m_fontList = new wxFontPickerCtrl(this, wxID_ANY);
        m_fontList->SetMinSize(wxSize(120, -1));
        m_useFontText = new wxCheckBox(this, wxID_ANY, _("Use custom text fields font:"));
        m_fontText = new wxFontPickerCtrl(this, wxID_ANY);
        m_fontText->SetMinSize(wxSize(120, -1));

        appearance->Add(m_useFontList, wxSizerFlags().Center().Left());
        appearance->Add(m_fontList, wxSizerFlags().Center().Expand());
        appearance->Add(m_useFontText, wxSizerFlags().Center().Left());
        appearance->Add(m_fontText, wxSizerFlags().Center().Expand());

#if NEED_CHOOSELANG_UI 
        m_uiLanguage = new wxButton(this, wxID_ANY, _("Change UI language"));
        sizer->Add(m_uiLanguage, wxSizerFlags().Border(wxTOP));
#endif

#ifdef __WXMSW__
        if (!IsSpellcheckingAvailable())
        {
            m_spellchecking->Disable();
            m_spellchecking->SetValue(false);
            // TRANSLATORS: This is a note appended to "Check spelling" when running on older Windows versions
            m_spellchecking->SetLabel(m_spellchecking->GetLabel() + " " + _("(requires Windows 8 or newer)"));
        }
#endif

        Fit();

        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
        {
            Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&){ TransferDataFromWindow(); });
            Bind(wxEVT_TEXT, [=](wxCommandEvent&){ TransferDataFromWindow(); });

            // Some settings directly affect the UI, so need a more expensive handler:
            m_useFontList->Bind(wxEVT_CHECKBOX, &GeneralPageWindow::TransferDataFromWindowAndUpdateUI, this);
            m_useFontText->Bind(wxEVT_CHECKBOX, &GeneralPageWindow::TransferDataFromWindowAndUpdateUI, this);
            Bind(wxEVT_FONTPICKER_CHANGED, &GeneralPageWindow::TransferDataFromWindowAndUpdateUI, this);
            m_focusToText->Bind(wxEVT_CHECKBOX, &GeneralPageWindow::TransferDataFromWindowAndUpdateUI, this);
            m_spellchecking->Bind(wxEVT_CHECKBOX, &GeneralPageWindow::TransferDataFromWindowAndUpdateUI, this);
        }

        // handle UI updates:
        m_fontList->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable(m_useFontList->GetValue()); });
        m_fontText->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable(m_useFontText->GetValue()); });

#if NEED_CHOOSELANG_UI
        m_uiLanguage->Bind(wxEVT_BUTTON, [=](wxCommandEvent&){ ChangeUILanguage(); });
#endif
    }

    void InitValues(const wxConfigBase& cfg) override
    {
        m_userName->SetValue(cfg.Read("translator_name", wxEmptyString));
        m_userEmail->SetValue(cfg.Read("translator_email", wxEmptyString));
        m_compileMo->SetValue(cfg.ReadBool("compile_mo", true));
        m_showSummary->SetValue(cfg.ReadBool("show_summary", false));
        m_focusToText->SetValue(cfg.ReadBool("focus_to_text", false));

        if (IsSpellcheckingAvailable())
        {
            m_spellchecking->SetValue(cfg.ReadBool("enable_spellchecking", true));
        }

        m_useFontList->SetValue(cfg.ReadBool("custom_font_list_use", false));
        m_useFontText->SetValue(cfg.ReadBool("custom_font_text_use", false));

        #if defined(__WXOSX__)
            #define DEFAULT_FONT "Helvetica"
        #elif defined(__WXMSW__)
            #define DEFAULT_FONT "Arial"
        #elif defined(__WXGTK__)
            #define DEFAULT_FONT "sans serif"
        #endif
        auto listFont = wxFont(cfg.Read("custom_font_list_name", ""));
        if (!listFont.IsOk())
            listFont = wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, DEFAULT_FONT);
        auto textFont = wxFont(cfg.Read("custom_font_text_name", ""));
        if (!textFont.IsOk())
            textFont = wxFont(11, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, DEFAULT_FONT);
        m_fontList->SetSelectedFont(listFont);
        m_fontText->SetSelectedFont(textFont);
    }

    void SaveValues(wxConfigBase& cfg) override
    {
        cfg.Write("translator_name", m_userName->GetValue());
        cfg.Write("translator_email", m_userEmail->GetValue());
        cfg.Write("compile_mo", m_compileMo->GetValue());
        cfg.Write("show_summary", m_showSummary->GetValue());
        cfg.Write("focus_to_text", m_focusToText->GetValue());

        if (IsSpellcheckingAvailable())
        {
            cfg.Write("enable_spellchecking", m_spellchecking->GetValue());
        }
       
        wxFont listFont = m_fontList->GetSelectedFont();
        wxFont textFont = m_fontText->GetSelectedFont();

        cfg.Write("custom_font_list_use", m_useFontList->GetValue());
        cfg.Write("custom_font_text_use", m_useFontText->GetValue());
        if ( listFont.IsOk() )
            cfg.Write("custom_font_list_name", listFont.GetNativeFontInfoDesc());
        if ( textFont.IsOk() )
            cfg.Write("custom_font_text_name", textFont.GetNativeFontInfoDesc());

        // On Windows, we must update the UI here; on other platforms, it was done
        // via TransferDataFromWindowAndUpdateUI immediately:
        if (!wxPreferencesEditor::ShouldApplyChangesImmediately())
        {
            PoeditFrame::UpdateAllAfterPreferencesChange();
        }
    }

private:
    wxTextCtrl *m_userName, *m_userEmail;
    wxCheckBox *m_compileMo, *m_showSummary, *m_focusToText, *m_spellchecking;
    wxCheckBox *m_useFontList, *m_useFontText;
    wxFontPickerCtrl *m_fontList, *m_fontText;
#if NEED_CHOOSELANG_UI
    wxButton *m_uiLanguage;
#endif
};

class GeneralPage : public wxPreferencesPage
{
public:
    wxString GetName() const override { return _("General"); }
    wxBitmap GetLargeIcon() const override { return wxArtProvider::GetBitmap("Prefs-General"); }
    wxWindow *CreateWindow(wxWindow *parent) override { return new GeneralPageWindow(parent); }
};



class TMPageWindow : public PrefsPanel
{
public:
    TMPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
#ifdef __WXOSX__
        topsizer->SetMinSize(410, -1); // for OS X look
#endif

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags(1).Expand().DoubleBorder());
        SetSizer(topsizer);

        sizer->AddSpacer(5);
        m_useTM = new wxCheckBox(this, wxID_ANY, _("Use translation memory"));
        sizer->Add(m_useTM, wxSizerFlags().Expand());

        m_stats = new wxStaticText(this, wxID_ANY, "--\n--", wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
        sizer->AddSpacer(10);
        sizer->Add(m_stats, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 30));
        sizer->AddSpacer(10);

        auto buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        auto import = new wxButton(this, wxID_ANY,
                                   MSW_OR_OTHER(_("Learn from files..."), _("Learn From Files...")));
        buttonsSizer->Add(import, wxSizerFlags());
        // TRANSLATORS: This is a button that deletes everything in the translation memory (i.e. clears/resets it).
        auto clear = new wxButton(this, wxID_ANY, _("Reset"));
        buttonsSizer->Add(clear, wxSizerFlags().Border(wxLEFT, 5));

        sizer->Add(buttonsSizer, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, 30));
        sizer->AddSpacer(10);

        m_useTMWhenUpdating = new wxCheckBox(this, wxID_ANY, _("Consult TM when updating from sources"));
        sizer->Add(m_useTMWhenUpdating, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM));

        auto explainTxt = _("If enabled, Poedit will try to fill in new entries using your previous\n"
                            "translations stored in the translation memory. If the TM is\n"
                            "near-empty, it will not be very effective. The more translations\n"
                            "you edit and the larger the TM grows, the better it gets.");
        auto explain = new ExplanationLabel(this, explainTxt);
        sizer->Add(explain, wxSizerFlags().Expand().Border(wxLEFT, ExplanationLabel::CHECKBOX_INDENT));

        auto learnMore = new LearnMoreLink(this, "http://poedit.net/trac/wiki/Doc/TranslationMemory");
        sizer->AddSpacer(5);
        sizer->Add(learnMore, wxSizerFlags().Border(wxLEFT, ExplanationLabel::CHECKBOX_INDENT + LearnMoreLink::EXTRA_INDENT));
        sizer->AddSpacer(10);

#ifdef __WXOSX__
        m_stats->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        import->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        clear->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

        m_useTMWhenUpdating->Bind(wxEVT_UPDATE_UI, &TMPageWindow::OnUpdateUI, this);
        m_stats->Bind(wxEVT_UPDATE_UI, &TMPageWindow::OnUpdateUI, this);
        import->Bind(wxEVT_UPDATE_UI, &TMPageWindow::OnUpdateUI, this);
        clear->Bind(wxEVT_UPDATE_UI, &TMPageWindow::OnUpdateUI, this);

        import->Bind(wxEVT_BUTTON, &TMPageWindow::OnImportIntoTM, this);
        clear->Bind(wxEVT_BUTTON, &TMPageWindow::OnResetTM, this);

        UpdateStats();

        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
        {
            m_useTMWhenUpdating->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&){ TransferDataFromWindow(); });
            // Some settings directly affect the UI, so need a more expensive handler:
            m_useTM->Bind(wxEVT_CHECKBOX, &TMPageWindow::TransferDataFromWindowAndUpdateUI, this);
        }
    }

    void InitValues(const wxConfigBase& cfg) override
    {
        m_useTM->SetValue(cfg.ReadBool("use_tm", true));
        m_useTMWhenUpdating->SetValue(cfg.ReadBool("use_tm_when_updating", false));
    }

    void SaveValues(wxConfigBase& cfg) override
    {
        cfg.Write("use_tm", m_useTM->GetValue());
        cfg.Write("use_tm_when_updating", m_useTMWhenUpdating->GetValue());
    }

private:
    void UpdateStats()
    {
        wxString sDocs("--");
        wxString sFileSize("--");
        if (wxConfig::Get()->ReadBool("use_tm", true))
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

    void OnResetTM(wxCommandEvent&)
    {
        auto title = _("Reset translation memory");
        auto main = _("Are you sure you want to reset the translation memory?");
        auto details = _(L"Resetting the translation memory will irrevocably delete all stored translations from it. You can’t undo this operation.");

        wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog(this, main, title, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION));
        dlg->SetExtendedMessage(details);
        dlg->SetYesNoLabels(_("Reset"), _("Cancel"));

        dlg->ShowWindowModalThenDo([this,dlg](int retcode){
            if (retcode == wxID_YES) {
                wxBusyCursor bcur;
                auto tm = TranslationMemory::Get().CreateWriter();
                tm->DeleteAll();
                tm->Commit();
                UpdateStats();
            }
        });
    }

    void OnUpdateUI(wxUpdateUIEvent& e)
    {
        e.Enable(m_useTM->GetValue());
    }

    wxCheckBox *m_useTM, *m_useTMWhenUpdating;
    wxStaticText *m_stats;
};

class TMPage : public wxPreferencesPage
{
public:
    wxString GetName() const override
    {
#ifdef __WXOSX__
        // TRANSLATORS: This is abbreviation of "Translation Memory" used in Preferences on OS X.
        // Long text looks weird there, too short (like TM) too, but less so. "General" is about ideal
        // length there.
        return _("TM");
#else
        return _("Translation Memory");
#endif
    }
    wxBitmap GetLargeIcon() const override { return wxArtProvider::GetBitmap("Prefs-TM"); }
    wxWindow *CreateWindow(wxWindow *parent) override { return new TMPageWindow(parent); }
};



class ExtractorsPageWindow : public PrefsPanel
{
public:
    ExtractorsPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags(1).Expand().DoubleBorder());
        SetSizer(topsizer);

        sizer->Add(new ExplanationLabel(this, _("Source code extractors are used to find translatable strings in the source code files and extract them so that they can be translated.")),
                   wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM));
        sizer->AddSpacer(10);

        auto horizontal = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(horizontal, wxSizerFlags(1).Expand());

        m_list = new wxListBox(this, wxID_ANY);
        m_list->SetMinSize(wxSize(250,300));
#ifdef __WXOSX__
        m_list->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
        horizontal->Add(m_list, wxSizerFlags(1).Expand().Border(wxRIGHT));

        auto buttons = new wxBoxSizer(wxVERTICAL);
        horizontal->Add(buttons, wxSizerFlags().Expand());

        m_new = new wxButton(this, wxID_ANY, _("New"));
        m_edit = new wxButton(this, wxID_ANY, _("Edit"));
        m_delete = new wxButton(this, wxID_ANY, _("Delete"));
        buttons->Add(m_new, wxSizerFlags().Border(wxBOTTOM));
        buttons->Add(m_edit, wxSizerFlags().Border(wxBOTTOM));
        buttons->Add(m_delete, wxSizerFlags().Border(wxBOTTOM));

        m_new->Bind(wxEVT_BUTTON, &ExtractorsPageWindow::OnNewExtractor, this);
        m_edit->Bind(wxEVT_BUTTON, &ExtractorsPageWindow::OnEditExtractor, this);
        m_delete->Bind(wxEVT_BUTTON, &ExtractorsPageWindow::OnDeleteExtractor, this);

        m_edit->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e) { e.Enable(m_list->GetSelection() != wxNOT_FOUND); });
        m_delete->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e) { e.Enable(m_list->GetSelection() != wxNOT_FOUND); });
    }

    void InitValues(const wxConfigBase& cfg) override
    {
        m_extractors.Read(const_cast<wxConfigBase*>(&cfg));

        m_list->Clear();
        for (const auto& item: m_extractors.Data)
            m_list->Append(item.Name);

        if (!m_extractors.Data.empty())
        {
            m_list->SetSelection(0);
            m_list->EnsureVisible(0);
        }
    }

    void SaveValues(wxConfigBase& cfg) override
    {
        m_extractors.Write(&cfg);
    }

private:
    /// Called to launch dialog for editting parser properties.
    template<typename TFunctor>
    void EditExtractor(int num, TFunctor completionHandler)
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

        m_supressDataTransfer++;
        dlg->ShowWindowModalThenDo([=](int retcode){
            m_supressDataTransfer--;
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
                m_list->SetString(num, nfo.Name);
            }
            completionHandler(retcode == wxID_OK);
        });
    }

    void OnNewExtractor(wxCommandEvent&)
    {
        m_supressDataTransfer++;

        Extractor info;
        m_extractors.Data.push_back(info);
        m_list->Append(wxEmptyString);
        int index = (int)m_extractors.Data.size()-1;
        EditExtractor(index, [=](bool added){
            if (added)
            {
                m_edit->Enable(true);
                m_delete->Enable(true);
            }
            else
            {
                m_list->Delete(index);
                m_extractors.Data.erase(m_extractors.Data.begin() + index);
            }

            m_supressDataTransfer--;

            if (wxPreferencesEditor::ShouldApplyChangesImmediately())
                TransferDataFromWindow();
        });
    }

    void OnEditExtractor(wxCommandEvent&)
    {
        EditExtractor(m_list->GetSelection(), [=](bool changed){
            if (changed && wxPreferencesEditor::ShouldApplyChangesImmediately())
                TransferDataFromWindow();
        });
    }

    void OnDeleteExtractor(wxCommandEvent&)
    {
        int index = m_list->GetSelection();
        m_extractors.Data.erase(m_extractors.Data.begin() + index);
        m_list->Delete(index);
        if (m_extractors.Data.empty())
        {
            m_list->Enable(false);
            m_list->Enable(false);
        }

        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
            TransferDataFromWindow();
    }

    ExtractorsDB m_extractors;

    wxListBox *m_list;
    wxButton *m_new, *m_edit, *m_delete;
};

class ExtractorsPage : public wxPreferencesPage
{
public:
    wxString GetName() const override { return _("Extractors"); }
    wxBitmap GetLargeIcon() const override { return wxArtProvider::GetBitmap("Prefs-Extractors"); }
    wxWindow *CreateWindow(wxWindow *parent) override { return new ExtractorsPageWindow(parent); }
};




#ifdef HAS_UPDATES_CHECK
class UpdatesPageWindow : public PrefsPanel
{
public:
    UpdatesPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->SetMinSize(350, -1); // for OS X look, wouldn't fit the toolbar otherwise

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags().Expand().DoubleBorder());
        SetSizer(topsizer);

        m_updates = new wxCheckBox(this, wxID_ANY, _("Automatically check for updates"));
        sizer->Add(m_updates, wxSizerFlags().Expand().Border(wxTOP|wxBOTTOM));

        m_beta = new wxCheckBox(this, wxID_ANY, _("Include beta versions"));
        sizer->Add(m_beta, wxSizerFlags().Expand().Border(wxBOTTOM));
        
        sizer->Add(new ExplanationLabel(this, _("Beta versions contain the latest new features and improvements, but may be a bit less stable.")),
                   wxSizerFlags().Expand().Border(wxLEFT, ExplanationLabel::CHECKBOX_INDENT));
        sizer->AddSpacer(5);

        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
            Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&){ TransferDataFromWindow(); });
    }

    void InitValues(const wxConfigBase&) override
    {
#ifdef USE_SPARKLE
        m_updates->SetValue((bool)UserDefaults_GetBoolValue("SUEnableAutomaticChecks"));
#endif
#ifdef __WXMSW__
        m_updates->SetValue(win_sparkle_get_automatic_check_for_updates() != 0);
#endif
        m_beta->SetValue(wxGetApp().CheckForBetaUpdates());
        if (wxGetApp().IsBetaVersion())
            m_beta->Disable();
    }

    void SaveValues(wxConfigBase& cfg) override
    {
#ifdef __WXMSW__
        win_sparkle_set_automatic_check_for_updates(m_updates->GetValue());
#endif
        if (!wxGetApp().IsBetaVersion())
        {
            cfg.Write("check_for_beta_updates", m_beta->GetValue());
        }
#ifdef USE_SPARKLE
        UserDefaults_SetBoolValue("SUEnableAutomaticChecks", m_updates->GetValue());
        Sparkle_Initialize(wxGetApp().CheckForBetaUpdates());
#endif
    }

private:
    wxCheckBox *m_updates, *m_beta;
};

class UpdatesPage : public wxPreferencesPage
{
public:
    wxString GetName() const override { return _("Updates"); }
    wxBitmap GetLargeIcon() const override { return wxArtProvider::GetBitmap("Prefs-Updates"); }
    wxWindow *CreateWindow(wxWindow *parent) override { return new UpdatesPageWindow(parent); }
};
#endif // HAS_UPDATES_CHECK


class AdvancedPageWindow : public PrefsPanel
{
public:
    AdvancedPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags(1).Expand().DoubleBorder());
        SetSizer(topsizer);

        sizer->Add(new ExplanationLabel(this, _("These settings affect internal formatting of PO files. Adjust them if you have specific requirements e.g. because of version control.")), wxSizerFlags().Expand().Border(wxBOTTOM));

        auto crlfbox = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(crlfbox, wxSizerFlags().Expand().Border(wxTOP));
        crlfbox->Add(new wxStaticText(this, wxID_ANY, _("Line endings:")), wxSizerFlags().Center().BORDER_WIN(wxTOP, 1));
        m_crlf = new wxChoice(this, wxID_ANY);
        m_crlf->Append(_("Unix (recommended)"));
        m_crlf->Append(_("Windows"));
        crlfbox->Add(m_crlf, wxSizerFlags(1).Center().BORDER_OSX(wxLEFT, 3).BORDER_WIN(wxLEFT, 5));

        /// TRANSLATORS: Followed by text control for entering number; wraps text at given width
        m_wrap = new wxCheckBox(this, wxID_ANY, _("Wrap at:"));
        crlfbox->AddSpacer(10);
        crlfbox->Add(m_wrap, wxSizerFlags().Center().BORDER_WIN(wxTOP, 1));
        m_wrapWidth = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(50,-1));
        m_wrapWidth->SetRange(10, 1000);
        crlfbox->Add(m_wrapWidth, wxSizerFlags().Center().BORDER_OSX(wxLEFT, 3));

        m_keepFmt = new wxCheckBox(this, wxID_ANY, _("Preserve formatting of existing files"));
        sizer->Add(m_keepFmt, wxSizerFlags().Border(wxTOP));

        Fit();

        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
        {
            Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&){ TransferDataFromWindow(); });
            Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ TransferDataFromWindow(); });
            Bind(wxEVT_TEXT, [=](wxCommandEvent&){ TransferDataFromWindow(); });
        }

        // handle UI updates:
        m_wrapWidth->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable(m_wrap->GetValue()); });
    }

    void InitValues(const wxConfigBase& cfg) override
    {
        m_keepFmt->SetValue(cfg.ReadBool("keep_crlf", true));

        wxString format = cfg.Read("crlf_format", "unix");
        int sel;
        if (format == "win") sel = 1;
        else /* "unix" or obsolete settings */ sel = 0;
        m_crlf->SetSelection(sel);

        m_wrap->SetValue(cfg.ReadBool("wrap_po_files", true));
        m_wrapWidth->SetValue((int)cfg.ReadLong("wrap_po_files_width", 79));
    }

    void SaveValues(wxConfigBase& cfg) override
    {
        cfg.Write("keep_crlf", m_keepFmt->GetValue());

        static const char *formats[] = { "unix", "win" };
        cfg.Write("crlf_format", formats[m_crlf->GetSelection()]);

        cfg.Write("wrap_po_files", m_wrap->GetValue());
        cfg.Write("wrap_po_files_width", m_wrapWidth->GetValue());
    }

private:
    wxChoice *m_crlf;
    wxCheckBox *m_wrap;
    wxSpinCtrl *m_wrapWidth;
    wxCheckBox *m_keepFmt;
};

class AdvancedPage : public wxStockPreferencesPage
{
public:
    AdvancedPage() : wxStockPreferencesPage(Kind_Advanced) {}
    wxString GetName() const override { return _("Advanced"); }
    wxWindow *CreateWindow(wxWindow *parent) override { return new AdvancedPageWindow(parent); }
};


} // anonymous namespace



std::unique_ptr<PoeditPreferencesEditor> PoeditPreferencesEditor::Create()
{
    std::unique_ptr<PoeditPreferencesEditor> p(new PoeditPreferencesEditor);
    p->AddPage(new GeneralPage);
    p->AddPage(new TMPage);
    p->AddPage(new ExtractorsPage);
#ifdef HAS_UPDATES_CHECK
    p->AddPage(new UpdatesPage);
#endif
    p->AddPage(new AdvancedPage);
    return p;
}

