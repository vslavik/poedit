/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
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

#include "pretranslate.h"

#include "configuration.h"
#include "customcontrols.h"
#include "hidpi.h"
#include "progressinfo.h"
#include "tm/transmem.h"
#include "utility.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/windowptr.h>


template<typename T>
bool PreTranslateCatalog(wxWindow *window, CatalogPtr catalog, const T& range, int flags, int *matchesCount)
{
    if (matchesCount)
        *matchesCount = 0;

    if (range.empty())
        return false;

    if (!Config::UseTM())
        return false;

    wxBusyCursor bcur;

    TranslationMemory& tm = TranslationMemory::Get();
    auto srclang = catalog->GetSourceLanguage();
    auto lang = catalog->GetLanguage();

    // FIXME: make this window-modal
    // FIXME: and don't create it here, reuse upstream progress data
    ProgressInfo progress(window, _(L"Pre-translating…"));
    progress.UpdateMessage(_("Pre-translating…"));

    // Function to apply fetched suggestions to a catalog item:
    auto process_results = [=](CatalogItemPtr dt, const SuggestionsList& results) -> bool
        {
            if (results.empty())
                return false;
            auto& res = results.front();
            if ((flags & PreTranslate_OnlyExact) && !res.IsExactMatch())
                return false;

            if ((flags & PreTranslate_OnlyGoodQuality) && res.score < 0.80)
                return false;

            dt->SetTranslation(res.text);
            dt->SetPreTranslated(true);
            dt->SetFuzzy(!res.IsExactMatch() || (flags & PreTranslate_ExactNotFuzzy) == 0);
            return true;
        };

    std::vector<dispatch::future<bool>> operations;
    for (auto dt: range)
    {
        if (dt->HasPlural())
            continue; // can't handle yet (TODO?)
        if (dt->IsTranslated() && !dt->IsFuzzy())
            continue;

        operations.push_back(dispatch::async([=,&tm]{
            auto results = tm.Search(srclang, lang, dt->GetString().ToStdWstring());
            bool ok = process_results(dt, results);
            return ok;
        }));
    }

    progress.SetGaugeMax((int)operations.size());

    int matches = 0;
    for (auto& op: operations)
    {
        if (!progress.UpdateGauge())
            break; // TODO: cancel pending 'operations' futures

        if (op.get())
        {
            matches++;
            progress.UpdateMessage(wxString::Format(wxPLURAL("Pre-translated %u string", "Pre-translated %u strings", matches), matches));
        }
    }

    if (matchesCount)
        *matchesCount = matches;

    return matches > 0;
}


bool PreTranslateCatalog(wxWindow *window, CatalogPtr catalog, int flags, int *matchesCount)
{
    return PreTranslateCatalog(window, catalog, catalog->items(), flags, matchesCount);
}


void PreTranslateWithUI(wxWindow *window, PoeditListCtrl *list, CatalogPtr catalog, std::function<void()> onChangesMade)
{
    wxWindowPtr<wxDialog> dlg(new wxDialog(window, wxID_ANY, _("Pre-translate"), wxDefaultPosition, wxSize(PX(440), -1)));
    auto topsizer = new wxBoxSizer(wxVERTICAL);
    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto onlyExact = new wxCheckBox(dlg.get(), wxID_ANY, _("Only fill in exact matches"));
    auto onlyExactE = new ExplanationLabel(dlg.get(), _("By default, inaccurate results are filled in as well and marked as needing work. Check this option to only include accurate matches."));
    auto noFuzzy = new wxCheckBox(dlg.get(), wxID_ANY, _(L"Don’t mark exact matches as needing work"));
    auto noFuzzyE = new ExplanationLabel(dlg.get(), _("Only enable if you trust the quality of your TM. By default, all matches from the TM are marked as needing work and should be reviewed before use."));

#ifdef __WXOSX__
    sizer->Add(new HeadingLabel(dlg.get(), _("Pre-translate")), wxSizerFlags().Expand().PXBorder(wxBOTTOM));
#endif
    auto pretransE = new ExplanationLabel(dlg.get(), _("Pre-translation automatically finds exact or fuzzy matches for untranslated strings in the translation memory and fills in their translations."));
    sizer->Add(pretransE, wxSizerFlags().Expand().Border(wxBOTTOM, PX(15)));

    sizer->Add(onlyExact, wxSizerFlags().PXBorder(wxTOP));
    sizer->AddSpacer(PX(1));
    sizer->Add(onlyExactE, wxSizerFlags().Expand().Border(wxLEFT, PX(ExplanationLabel::CHECKBOX_INDENT)));
    sizer->Add(noFuzzy, wxSizerFlags().PXDoubleBorder(wxTOP));
    sizer->AddSpacer(PX(1));
    sizer->Add(noFuzzyE, wxSizerFlags().Expand().Border(wxLEFT, PX(ExplanationLabel::CHECKBOX_INDENT)));

    topsizer->Add(sizer, wxSizerFlags(1).Expand().Border(wxALL, MACOS_OR_OTHER(PX(20), PX(10))));

    auto buttons = dlg->CreateButtonSizer(wxOK | wxCANCEL);
    auto ok = static_cast<wxButton*>(dlg->FindWindow(wxID_OK));
    // TRANSLATORS: This is a somewhat common term describing the action where
    // you apply the translation memory and/or machine translation to all of the
    // strings you're translating as the first step, followed by correcting,
    // improving etc., i.e. actually translating the strings. This may be tricky
    // to express in other languages as simply as in English, but please try to
    // keep it similarly concise. Please try to avoid, if possible, describing it
    // as "auto-translation" and similar, because such terminology would mislead
    // some users into thinking it's all that needs to be done (spoken from
    // experience). "Pre-translate" nicely expresses that it's only the step done
    // *before* actual translation.
    ok->SetLabel(_("Pre-translate"));
    ok->SetDefault();
#ifdef __WXOSX__
    topsizer->Add(buttons, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT|wxBOTTOM, PX(10)));
#else
    topsizer->AddSpacer(PX(5));
    topsizer->Add(buttons, wxSizerFlags().Expand().Border(wxRIGHT, PX(12)));
    topsizer->AddSpacer(PX(12));
#endif

    dlg->SetSizer(topsizer);
    dlg->Layout();
    topsizer->SetSizeHints(dlg.get());
    dlg->CenterOnParent();

    dlg->ShowWindowModalThenDo([catalog,window,list,onlyExact,noFuzzy,onChangesMade,dlg](int retcode)
    {
        if (retcode != wxID_OK)
            return;

        int matches = 0;

        int flags = 0;
        if (onlyExact->GetValue())
            flags |= PreTranslate_OnlyExact;
        if (noFuzzy->GetValue())
            flags |= PreTranslate_ExactNotFuzzy;

        if (list->HasMultipleSelection())
        {
            if (!PreTranslateCatalog(window, catalog, list->GetSelectedCatalogItems(), flags, &matches))
                return;
        }
        else
        {
            if (!PreTranslateCatalog(window, catalog, flags, &matches))
                return;
        }

        onChangesMade();

        wxString msg, details;

        if (matches)
        {
            msg = wxString::Format(wxPLURAL("%d entry was pre-translated.",
                                            "%d entries were pre-translated.",
                                            matches), matches);
            details = _("The translations were marked as needing work, because they may be inaccurate. You should review them for correctness.");
        }
        else
        {
            msg = _("No entries could be pre-translated.");
            details = _(L"The TM doesn’t contain any strings similar to the content of this file. It is only effective for semi-automatic translations after Poedit learns enough from files that you translated manually.");
        }

        wxWindowPtr<wxMessageDialog> resultsDlg(
            new wxMessageDialog
                (
                    window,
                    msg,
                    _("Pre-translate"),
                    wxOK | wxICON_INFORMATION
                )
        );
        resultsDlg->SetExtendedMessage(details);
        resultsDlg->ShowWindowModalThenDo([resultsDlg](int){});
    });
}
