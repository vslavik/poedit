/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2017 Vaclav Slavik
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
#include <wx/checklst.h>
#include <wx/notebook.h>
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

#if !wxCHECK_VERSION(3,1,0)
    #define CenterVertical() Center()
#endif

#include "prefsdlg.h"
#include "edapp.h"
#include "edframe.h"
#include "catalog.h"
#include "configuration.h"
#include "crowdin_gui.h"
#include "hidpi.h"
#include "tm/transmem.h"
#include "chooselang.h"
#include "errors.h"
#include "extractors/extractor_legacy.h"
#include "spellchecking.h"
#include "utility.h"
#include "customcontrols.h"
#include "unicode_helpers.h"

#ifdef __WXMSW__
#include <winsparkle.h>
#endif

#ifdef USE_SPARKLE
#include "macos_helpers.h"
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
        : wxPanel(parent), m_suppressDataTransfer(0)
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
        if (m_suppressDataTransfer)
            return false;
        m_suppressDataTransfer++;
        InitValues(*wxConfig::Get());
        m_suppressDataTransfer--;

        // This is a "bit" of a hack: we take advantage of being in the last point before
        // showing the window and re-layout it on the off chance that some data transferred
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
        if (m_suppressDataTransfer)
            return false;
        m_suppressDataTransfer++;
        SaveValues(*wxConfig::Get());
        m_suppressDataTransfer--;
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

    int m_suppressDataTransfer;
};



class GeneralPageWindow : public PrefsPanel
{
public:
    GeneralPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->SetMinSize(PX(400), -1);

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags(1).Expand().PXDoubleBorderAll());
        SetSizer(topsizer);

        sizer->Add(new HeadingLabel(this, _("Information about the translator")));
        sizer->AddSpacer(PX(10));

        auto translator = new wxFlexGridSizer(2, wxSize(5,6));
        translator->AddGrowableCol(1);
        sizer->Add(translator, wxSizerFlags().Expand());

        auto nameLabel = new wxStaticText(this, wxID_ANY, _("Name:"));
        translator->Add(nameLabel, wxSizerFlags().CenterVertical().Right().BORDER_MACOS(wxTOP, 1));
        m_userName = new wxTextCtrl(this, wxID_ANY);
        m_userName->SetHint(_("Your Name"));
        translator->Add(m_userName, wxSizerFlags(1).Expand().CenterVertical());
        auto emailLabel = new wxStaticText(this, wxID_ANY, _("Email:"));
        translator->Add(emailLabel, wxSizerFlags().CenterVertical().Right().BORDER_MACOS(wxTOP, 1));
        m_userEmail = new wxTextCtrl(this, wxID_ANY);
        m_userEmail->SetHint(_("your_email@example.com"));
        translator->Add(m_userEmail, wxSizerFlags(1).Expand().CenterVertical());
        translator->AddSpacer(PX(1));
        translator->Add(new ExplanationLabel(this, _("Your name and email address are only used to set the Last-Translator header of GNU gettext files.")), wxSizerFlags(1).Expand().PXBorder(wxRIGHT));
#ifdef __WXOSX__
        nameLabel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        emailLabel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        m_userName->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        m_userEmail->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

        sizer->AddSpacer(PX(10));
        sizer->Add(new HeadingLabel(this, _("Editing")));
        sizer->AddSpacer(PX(10));

        m_compileMo = new wxCheckBox(this, wxID_ANY, _("Automatically compile MO file when saving"));
        sizer->Add(m_compileMo);
        m_showSummary = new wxCheckBox(this, wxID_ANY, _("Show summary after catalog update"));
        sizer->Add(m_showSummary, wxSizerFlags().PXBorder(wxTOP));

        sizer->AddSpacer(PX(10));

        m_spellchecking = new wxCheckBox(this, wxID_ANY, _("Check spelling"));
        sizer->Add(m_spellchecking, wxSizerFlags().PXBorder(wxTOP));
        m_focusToText = new wxCheckBox(this, wxID_ANY, _("Always change focus to text input field"));
        sizer->Add(m_focusToText, wxSizerFlags().PXBorder(wxTOP));
        wxString explainFocus(_("Never let the list of strings take focus. If enabled, you must use Ctrl-arrows for keyboard navigation but you can also type text immediately, without having to press Tab to change focus."));
#ifdef __WXOSX__
        explainFocus.Replace("Ctrl", "Cmd");
#endif
        sizer->AddSpacer(PX(5));
        sizer->Add(new ExplanationLabel(this, explainFocus), wxSizerFlags().Expand().Border(wxLEFT, PX(ExplanationLabel::CHECKBOX_INDENT)));

        sizer->AddSpacer(PX(10));
        sizer->Add(new HeadingLabel(this, _("Appearance")));
        sizer->AddSpacer(PX(4));

        auto appearance = new wxFlexGridSizer(2, wxSize(5,1));
        appearance->AddGrowableCol(1);
        sizer->Add(appearance, wxSizerFlags().Expand());

        m_useFontList = new wxCheckBox(this, wxID_ANY, _("Use custom list font:"));
        m_fontList = new wxFontPickerCtrl(this, wxID_ANY);
        m_fontList->SetMinSize(wxSize(PX(120), -1));
        m_useFontText = new wxCheckBox(this, wxID_ANY, _("Use custom text fields font:"));
        m_fontText = new wxFontPickerCtrl(this, wxID_ANY);
        m_fontText->SetMinSize(wxSize(PX(120), -1));

        appearance->Add(m_useFontList, wxSizerFlags().CenterVertical().Left());
        appearance->Add(m_fontList, wxSizerFlags().CenterVertical().Expand());
        appearance->Add(m_useFontText, wxSizerFlags().CenterVertical().Left());
        appearance->Add(m_fontText, wxSizerFlags().CenterVertical().Expand());

#if NEED_CHOOSELANG_UI 
        m_uiLanguage = new wxButton(this, wxID_ANY, _("Change UI language"));
        sizer->Add(m_uiLanguage, wxSizerFlags().PXBorder(wxTOP));
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
            #define DEFAULT_FONT "Helvetica Neue"
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
        topsizer->SetMinSize(PX(430), -1); // for macOS look
#endif

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags(1).Expand().PXDoubleBorderAll());
        SetSizer(topsizer);

        sizer->AddSpacer(PX(5));
        m_useTM = new wxCheckBox(this, wxID_ANY, _("Use translation memory"));
        sizer->Add(m_useTM, wxSizerFlags().Expand());

        m_stats = new wxStaticText(this, wxID_ANY, "--\n--", wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);
        sizer->AddSpacer(PX(10));
        sizer->Add(m_stats, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, PX(30)));
        sizer->AddSpacer(PX(10));

        auto buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        auto import = new wxButton(this, wxID_ANY,
                                   MSW_OR_OTHER(_("Learn from files..."), _("Learn From Files...")));
        buttonsSizer->Add(import, wxSizerFlags());
        // TRANSLATORS: This is a button that deletes everything in the translation memory (i.e. clears/resets it).
        auto clear = new wxButton(this, wxID_ANY, _("Reset"));
        buttonsSizer->Add(clear, wxSizerFlags().Border(wxLEFT, 5));

        sizer->Add(buttonsSizer, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT, PX(30)));
        sizer->AddSpacer(PX(10));

        // TRANSLATORS: Followed by "match translations within the file" or "pre-translate from TM"
        m_mergeUse = new wxCheckBox(this, wxID_ANY, _("When updating from sources"));
        wxString mergeValues[] = {
            // TRANSLATORS: Preceded by "When updating from sources"
            _("fuzzy match within the file"),
            // TRANSLATORS: Preceded by "When updating from sources"
            _("pre-translate from TM")
        };
        m_mergeBehavior = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, WXSIZEOF(mergeValues), mergeValues);

        auto mergeSizer = new wxBoxSizer(wxHORIZONTAL);
        mergeSizer->Add(m_mergeUse, wxSizerFlags().Center());
        mergeSizer->AddSpacer(PX(5));
        mergeSizer->Add(m_mergeBehavior, wxSizerFlags().Center().BORDER_MACOS(wxTOP, PX(1)).BORDER_WIN(wxBOTTOM, 1));
        sizer->Add(mergeSizer, wxSizerFlags().PXBorder(wxTOP|wxBOTTOM));

        auto explainTxt = _(L"Poedit can attempt to fill in new entries from only previous translations in "
                            L"the file or from your entire translation memory. Using the TM won’t be very effective if "
                            L"it’s near-empty, but it will get better as you add more translations to it.");
        auto explain = new ExplanationLabel(this, explainTxt);
        sizer->Add(explain, wxSizerFlags().Expand().Border(wxLEFT, PX(ExplanationLabel::CHECKBOX_INDENT)));

        auto learnMore = new LearnMoreLink(this, "https://poedit.net/trac/wiki/Doc/TranslationMemory");
        sizer->AddSpacer(PX(3));
        sizer->Add(learnMore, wxSizerFlags().Border(wxLEFT, PX(ExplanationLabel::CHECKBOX_INDENT + LearnMoreLink::EXTRA_INDENT)));
        sizer->AddSpacer(PX(10));

#ifdef __WXOSX__
        m_stats->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        import->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
        clear->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

        m_mergeBehavior->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e){ e.Enable(m_mergeUse->GetValue() == true); });

        m_stats->Bind(wxEVT_UPDATE_UI, &TMPageWindow::OnUpdateUI, this);
        import->Bind(wxEVT_UPDATE_UI, &TMPageWindow::OnUpdateUI, this);
        clear->Bind(wxEVT_UPDATE_UI, &TMPageWindow::OnUpdateUI, this);

        import->Bind(wxEVT_BUTTON, &TMPageWindow::OnImportIntoTM, this);
        clear->Bind(wxEVT_BUTTON, &TMPageWindow::OnResetTM, this);

        UpdateStats();

        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
        {
            m_mergeUse->Bind(wxEVT_CHECKBOX, [=](wxCommandEvent&){ TransferDataFromWindow(); });
            m_mergeBehavior->Bind(wxEVT_CHOICE, [=](wxCommandEvent&){ TransferDataFromWindow(); });
            // Some settings directly affect the UI, so need a more expensive handler:
            m_useTM->Bind(wxEVT_CHECKBOX, &TMPageWindow::TransferDataFromWindowAndUpdateUI, this);
        }
    }

    void InitValues(const wxConfigBase&) override
    {
        m_useTM->SetValue(Config::UseTM());
        auto merge = Config::MergeBehavior();
        m_mergeUse->SetValue(merge != Merge_None);
        m_mergeBehavior->SetSelection(merge == Merge_UseTM ? 1 : 0);
    }

    void SaveValues(wxConfigBase&) override
    {
        Config::UseTM(m_useTM->GetValue());
        if (m_mergeUse->GetValue() == true)
        {
            Config::MergeBehavior(m_mergeBehavior->GetSelection() == 1 ? Merge_UseTM : Merge_FuzzyMatch);
        }
        else
        {
            Config::MergeBehavior(Merge_None);
        }
    }

private:
    void UpdateStats()
    {
        wxString sDocs("--");
        wxString sFileSize("--");
        if (Config::UseTM())
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
			Catalog::GetTypesFileMask({Catalog::Type::PO}),
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
            auto tm = TranslationMemory::Get().GetWriter();
            int step = 0;
            for (size_t i = 0; i < paths.size(); i++)
            {
                CatalogPtr cat = std::make_shared<Catalog>(paths[i]);
                if (!progress.Update(++step))
                    break;
                if (cat->IsOk())
                    tm->Insert(cat);
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

        wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog(this, main, title, wxYES_NO | wxNO_DEFAULT | wxICON_WARNING));
        dlg->SetExtendedMessage(details);
        dlg->SetYesNoLabels(_("Reset"), _("Cancel"));

        dlg->ShowWindowModalThenDo([this,dlg](int retcode){
            if (retcode == wxID_YES) {
                wxBusyCursor bcur;
                auto tm = TranslationMemory::Get().GetWriter();
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

    wxCheckBox *m_useTM;
    wxCheckBox *m_mergeUse;
    wxChoice *m_mergeBehavior;
    wxStaticText *m_stats;
};

class TMPage : public wxPreferencesPage
{
public:
    wxString GetName() const override
    {
#if defined(__WXOSX__) || defined(__WXGTK__)
        // TRANSLATORS: This is abbreviation of "Translation Memory" used in Preferences on macOS.
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
        topsizer->Add(sizer, wxSizerFlags(1).Expand().PXDoubleBorderAll());
        SetSizer(topsizer);

        sizer->Add(new ExplanationLabel(this, _("Source code extractors are used to find translatable strings in the source code files and extract them so that they can be translated.")),
                   wxSizerFlags().Expand().PXDoubleBorder(wxBOTTOM));

        // FIXME: Neither wxBORDER_ flag produces correct results on macOS or Windows, would need to paint manually
        auto listPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));
        listPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_LISTBOX));
        auto listSizer = new wxBoxSizer(wxVERTICAL);
        listPanel->SetSizer(listSizer);

        CreateBuiltinExtractorsUI(listPanel, listSizer);

        auto customExLabel = new wxStaticText(listPanel, wxID_ANY, MSW_OR_OTHER(_("Custom extractors:"), _("Custom Extractors:")));
        customExLabel->SetForegroundColour(ExplanationLabel::GetTextColor());
#ifdef __WXOSX__
        customExLabel->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
        customExLabel->SetFont(customExLabel->GetFont().Bold());

        listSizer->AddSpacer(PX(5));
        listSizer->Add(customExLabel, wxSizerFlags().ReserveSpaceEvenIfHidden().Border(wxLEFT|wxRIGHT, PX(5)));
        listSizer->AddSpacer(PX(5));

        m_list = new wxCheckListBox(listPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxBORDER_NONE);
        m_list->SetMinSize(wxSize(PX(400),PX(200)));
#ifdef __WXOSX__
        m_list->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
        listSizer->Add(m_list, wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT, PX(5)));

        sizer->Add(listPanel, wxSizerFlags(1).Expand().BORDER_WIN(wxLEFT, 1));

#if defined(__WXOSX__)
        m_new = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("NSAddTemplate"), wxDefaultPosition, wxSize(18, 18), wxBORDER_SUNKEN);
        m_delete = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("NSRemoveTemplate"), wxDefaultPosition, wxSize(18,18), wxBORDER_SUNKEN);
        int editButtonStyle = wxBU_EXACTFIT | wxBORDER_SIMPLE;
#elif defined(__WXMSW__)
        m_new = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-add"), wxDefaultPosition, wxSize(PX(19),PX(19)));
        m_delete = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-remove"), wxDefaultPosition, wxSize(PX(19),PX(19)));
        int editButtonStyle = wxBU_EXACTFIT;
#elif defined(__WXGTK__)
        m_new = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-add"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        m_delete = new wxBitmapButton(this, wxID_ANY, wxArtProvider::GetBitmap("list-remove"), wxDefaultPosition, wxDefaultSize, wxNO_BORDER);
        int editButtonStyle = wxBU_EXACTFIT | wxBORDER_NONE;
#endif
        m_edit = new wxButton(this, wxID_ANY, _("Edit..."), wxDefaultPosition, wxSize(-1, MSW_OR_OTHER(PX(19), -1)), editButtonStyle);
#ifndef __WXGTK__
        m_edit->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

        auto buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        buttonSizer->Add(m_new);
#ifdef __WXOSX__
        buttonSizer->AddSpacer(PX(1));
#endif
        buttonSizer->Add(m_delete);
#ifdef __WXOSX__
        buttonSizer->AddSpacer(PX(1));
#endif
        buttonSizer->Add(m_edit);

        sizer->AddSpacer(PX(1));
        sizer->Add(buttonSizer, wxSizerFlags().BORDER_MACOS(wxLEFT, PX(1)));

        m_new->Bind(wxEVT_BUTTON, &ExtractorsPageWindow::OnNewExtractor, this);
        m_edit->Bind(wxEVT_BUTTON, &ExtractorsPageWindow::OnEditExtractor, this);
        m_delete->Bind(wxEVT_BUTTON, &ExtractorsPageWindow::OnDeleteExtractor, this);

        m_list->Bind(wxEVT_CHECKLISTBOX, &ExtractorsPageWindow::OnEnableExtractor, this);

        m_edit->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e) { e.Enable(m_list->GetSelection() != wxNOT_FOUND); });
        m_delete->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e) { e.Enable(m_list->GetSelection() != wxNOT_FOUND); });
        customExLabel->Bind(wxEVT_UPDATE_UI, [=](wxUpdateUIEvent& e) { e.Show(m_list->GetCount() > 0); });
    }

    void CreateBuiltinExtractorsUI(wxWindow *panel, wxSizer *topsizer)
    {
        auto sizer = new wxBoxSizer(wxHORIZONTAL);
        topsizer->Add(sizer, wxSizerFlags().Expand().Border(wxALL, PX(5)));

        sizer->Add(new wxStaticBitmap(panel, wxID_ANY, wxArtProvider::GetBitmap("ExtractorsGNUgettext")), wxSizerFlags().Top().Border(wxRIGHT, PX(5)));
        auto textSizer = new wxBoxSizer(wxVERTICAL);
        sizer->Add(textSizer, wxSizerFlags(1).Top());
        auto heading = new wxStaticText(panel, wxID_ANY, _("GNU gettext"));
#ifdef __WXOSX__
        heading->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
        heading->SetFont(heading->GetFont().Bold());
        textSizer->Add(heading, wxSizerFlags().Border(wxBOTTOM, PX(2)));
        auto desc = new ExplanationLabel(panel, _("Supports all programming languages recognized by GNU gettext tools (PHP, C/C++, C#, Perl, Python, Java, JavaScript and others)."));
        textSizer->Add(desc, wxSizerFlags(1).Expand());
        textSizer->Layout();
    }

    void InitValues(const wxConfigBase& cfg) override
    {
        m_extractors.Read(const_cast<wxConfigBase*>(&cfg));

        m_list->Clear();
        for (const auto& item: m_extractors.Data)
        {
            auto index = m_list->Append(bidi::platform_mark_direction(item.Name));
            m_list->Check(index, item.Enabled);
        }

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
    /// Called to launch dialog for editing parser properties.
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
            const LegacyExtractorSpec& nfo = m_extractors.Data[num];
            extractor_language->SetValue(bidi::platform_mark_direction(nfo.Name));
            extractor_extensions->SetValue(bidi::mark_direction(nfo.Extensions, TextDirection::LTR));
            extractor_command->SetValue(bidi::mark_direction(nfo.Command, TextDirection::LTR));
            extractor_keywords->SetValue(bidi::mark_direction(nfo.KeywordItem, TextDirection::LTR));
            extractor_files->SetValue(bidi::mark_direction(nfo.FileItem, TextDirection::LTR));
            extractor_charset->SetValue(bidi::mark_direction(nfo.CharsetItem, TextDirection::LTR));
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

        m_suppressDataTransfer++;
        dlg->ShowWindowModalThenDo([=](int retcode){
            m_suppressDataTransfer--;
            (void)dlg; // force use
            if (retcode == wxID_OK)
            {
                LegacyExtractorSpec& nfo = m_extractors.Data[num];
                nfo.Name = bidi::strip_control_chars(extractor_language->GetValue().Strip(wxString::both));
                nfo.Extensions = bidi::strip_control_chars(extractor_extensions->GetValue().Strip(wxString::both));
                nfo.Command = bidi::strip_control_chars(extractor_command->GetValue().Strip(wxString::both));
                nfo.KeywordItem = bidi::strip_control_chars(extractor_keywords->GetValue().Strip(wxString::both));
                nfo.FileItem = bidi::strip_control_chars(extractor_files->GetValue().Strip(wxString::both));
                nfo.CharsetItem = bidi::strip_control_chars(extractor_charset->GetValue().Strip(wxString::both));
                m_list->SetString(num, nfo.Name);
            }
            completionHandler(retcode == wxID_OK);
        });
    }

    void OnNewExtractor(wxCommandEvent&)
    {
        m_suppressDataTransfer++;

        LegacyExtractorSpec info;
        m_extractors.Data.push_back(info);
        auto index = m_list->Append(wxEmptyString);
        m_list->Check(index);
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

            m_suppressDataTransfer--;

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

        auto title = MSW_OR_OTHER(_("Delete extractor"), "");
        auto main = wxString::Format(_(L"Are you sure you want to delete the “%s” extractor?"), m_extractors.Data[index].Name);

        wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog(this, main, title, wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION));
        #ifdef __WXOSX__
        dlg->SetExtendedMessage(" "); // prevent wx from using the title stupidly
        #endif
        dlg->SetYesNoLabels(_("Delete"), _("Cancel"));

        dlg->ShowWindowModalThenDo([this,index,dlg](int retcode){
            if (retcode == wxID_YES)
            {
                m_extractors.Data.erase(m_extractors.Data.begin() + index);
                m_list->Delete(index);

                if (wxPreferencesEditor::ShouldApplyChangesImmediately())
                    TransferDataFromWindow();
            }
        });
    }

    void OnEnableExtractor(wxCommandEvent& e)
    {
        int index = e.GetInt();
        m_extractors.Data[index].Enabled = m_list->IsChecked(index);

        if (wxPreferencesEditor::ShouldApplyChangesImmediately())
            TransferDataFromWindow();
    }

    LegacyExtractorsDB m_extractors;

    wxCheckListBox *m_list;
    wxButton *m_new, *m_edit, *m_delete;
};

class ExtractorsPage : public wxPreferencesPage
{
public:
    wxString GetName() const override { return _("Extractors"); }
    wxBitmap GetLargeIcon() const override { return wxArtProvider::GetBitmap("Prefs-Extractors"); }
    wxWindow *CreateWindow(wxWindow *parent) override { return new ExtractorsPageWindow(parent); }
};



#ifdef HAVE_HTTP_CLIENT
class AccountsPageWindow : public PrefsPanel
{
public:
    AccountsPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
        m_login = new CrowdinLoginPanel(this);
        topsizer->Add(m_login, wxSizerFlags(1).Expand().Border(wxALL, PX(15)));
        SetSizer(topsizer);

    #ifdef __WXOSX__
        // This window was created on demand, init immediately:
        m_login->EnsureInitialized();
    #else
        // On other platforms, notebook pages are all created at once. Don't do
        // the expensive initialization until shown for the first time. This code
        // is a hack that takes advantage of wxPreferencesEditor's implementation
        // detail, but oh well:
        auto notebook = dynamic_cast<wxNotebook*>(parent);
        if (notebook)
        {
            notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [=](wxBookCtrlEvent& e){
                e.Skip();
                if (notebook->GetPage(e.GetSelection()) == this)
                    m_login->EnsureInitialized();
            });
        }
    #endif
    }

    void InitValues(const wxConfigBase&) override
    {
    }

    void SaveValues(wxConfigBase&) override
    {
    }

private:
    CrowdinLoginPanel *m_login;
};

class AccountsPage : public wxPreferencesPage
{
public:
    wxString GetName() const override { return _("Accounts"); }
    wxBitmap GetLargeIcon() const override { return wxArtProvider::GetBitmap("Prefs-Accounts"); }
    wxWindow *CreateWindow(wxWindow *parent) override { return new AccountsPageWindow(parent); }
};
#endif // HAVE_HTTP_CLIENT


#ifdef HAS_UPDATES_CHECK
class UpdatesPageWindow : public PrefsPanel
{
public:
    UpdatesPageWindow(wxWindow *parent) : PrefsPanel(parent)
    {
        wxSizer *topsizer = new wxBoxSizer(wxVERTICAL);
        topsizer->SetMinSize(PX(400), -1); // for macOS look, wouldn't fit the toolbar otherwise

        wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        topsizer->Add(sizer, wxSizerFlags().Expand().PXDoubleBorderAll());
        SetSizer(topsizer);

        m_updates = new wxCheckBox(this, wxID_ANY, _("Automatically check for updates"));
        sizer->Add(m_updates, wxSizerFlags().Expand().PXBorder(wxTOP|wxBOTTOM));

        m_beta = new wxCheckBox(this, wxID_ANY, _("Include beta versions"));
        sizer->Add(m_beta, wxSizerFlags().Expand().PXBorder(wxBOTTOM));

        sizer->Add(new ExplanationLabel(this, _("Beta versions contain the latest new features and improvements, but may be a bit less stable.")),
                   wxSizerFlags().Expand().Border(wxLEFT, PX(ExplanationLabel::CHECKBOX_INDENT)));
        sizer->AddSpacer(PX(5));

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
        topsizer->Add(sizer, wxSizerFlags(1).Expand().PXDoubleBorderAll());
        SetSizer(topsizer);

        sizer->Add(new ExplanationLabel(this, _("These settings affect internal formatting of PO files. Adjust them if you have specific requirements e.g. because of version control.")), wxSizerFlags().Expand().PXBorder(wxBOTTOM));

        auto crlfbox = new wxBoxSizer(wxHORIZONTAL);
        sizer->Add(crlfbox, wxSizerFlags().Expand().PXBorder(wxTOP));
        crlfbox->Add(new wxStaticText(this, wxID_ANY, _("Line endings:")), wxSizerFlags().Center().BORDER_WIN(wxTOP, PX(1)));
        m_crlf = new wxChoice(this, wxID_ANY);
        m_crlf->Append(_("Unix (recommended)"));
        m_crlf->Append(_("Windows"));
        crlfbox->Add(m_crlf, wxSizerFlags(1).Center().BORDER_MACOS(wxLEFT, PX(3)).BORDER_WIN(wxLEFT, PX(5)));

        /// TRANSLATORS: Followed by text control for entering number; wraps text at given width
        m_wrap = new wxCheckBox(this, wxID_ANY, _("Wrap at:"));
        crlfbox->AddSpacer(PX(10));
        crlfbox->Add(m_wrap, wxSizerFlags().Center().BORDER_WIN(wxTOP, PX(1)));
    #ifdef __WXGTK3__
        m_wrapWidth = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(PX(110),-1));
    #else
        m_wrapWidth = new wxSpinCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(PX(50),-1));
    #endif
        m_wrapWidth->SetRange(10, 999);
        crlfbox->Add(m_wrapWidth, wxSizerFlags().Center().BORDER_MACOS(wxLEFT, PX(3)));

        m_keepFmt = new wxCheckBox(this, wxID_ANY, _("Preserve formatting of existing files"));
        sizer->Add(m_keepFmt, wxSizerFlags().PXBorder(wxTOP));

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
#ifdef HAVE_HTTP_CLIENT
    p->AddPage(new AccountsPage);
#endif
#ifdef HAS_UPDATES_CHECK
    p->AddPage(new UpdatesPage);
#endif
    p->AddPage(new AdvancedPage);
    return p;
}

