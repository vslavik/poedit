/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2025 Vaclav Slavik
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
#include "progress_ui.h"
#include "str_helpers.h"
#include "tm/transmem.h"
#include "utility.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>
#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/numformatter.h>
#include <wx/sizer.h>
#include <wx/windowptr.h>


namespace
{

enum class ResType
{
    None = 0,  // no matches
    Rejected,  // found matches, but rejected by settings
    Fuzzy,     // approximate match
    Exact      // exact match
};

inline bool translated(ResType r) { return r >= ResType::Fuzzy; }


struct Stats
{
    int input_strings_count = 0;
    int total = 0;
    int matched = 0;
    int exact = 0;
    int fuzzy = 0;

    explicit operator bool() const { return matched > 0; }

    void add(ResType r)
    {
        total++;
        if (translated(r))
            matched++;
        if (r == ResType::Exact)
            exact++;
        else if (r == ResType::Fuzzy)
            fuzzy++;
    }
};


template<typename T>
Stats PreTranslateCatalogImpl(CatalogPtr catalog, const T& range, PreTranslateOptions options, dispatch::cancellation_token_ptr cancellation_token)
{
    if (range.empty())
        return {};

    if (!Config::UseTM())
        return {};

    TranslationMemory& tm = TranslationMemory::Get();
    auto srclang = catalog->GetSourceLanguage();
    auto lang = catalog->GetLanguage();
    const auto flags = options.flags;

    Progress top_progress(1);
    top_progress.message(_(L"Preparing strings…"));

    // Function to apply fetched suggestions to a catalog item:
    auto process_results = [=](CatalogItemPtr dt, unsigned index, const SuggestionsList& results) -> ResType
    {
        if (results.empty())
            return ResType::None;
        auto& res = results.front();
        if ((flags & PreTranslate_OnlyExact) && !res.IsExactMatch())
            return ResType::Rejected;

        if ((flags & PreTranslate_OnlyGoodQuality) && res.score < 0.80)
            return ResType::None;

        dt->SetTranslation(res.text, index);
        dt->SetPreTranslated(true);

        bool isFuzzy = true;
        if (res.IsExactMatch() && (flags & PreTranslate_ExactNotFuzzy))
        {
            if (results.size() > 1 && results[1].IsExactMatch())
            {
                // more than one exact match is ambiguous, so keep it flagged for review
            }
            else
            {
                isFuzzy = false;
            }
        }
        dt->SetFuzzy(isFuzzy);

        return res.IsExactMatch() ? ResType::Exact : ResType::Fuzzy;
    };

    Stats stats;

    std::vector<dispatch::future<ResType>> operations;
    for (auto dt: range)
    {
        if (dt->IsTranslated() && !dt->IsFuzzy())
            continue;

        stats.input_strings_count++;

        operations.push_back(dispatch::async([=,&tm]() -> ResType
        {
            if (cancellation_token->is_cancelled())
                return {};

            auto results = tm.Search(srclang, lang, str::to_wstring(dt->GetString()));
            auto rt = process_results(dt, 0, results);

            if (translated(rt) && dt->HasPlural())
            {
                switch (lang.nplurals())
                {
                    case 2:  // "simple" English-like plurals
                    {
                        auto results_plural = tm.Search(srclang, lang, str::to_wstring(dt->GetPluralString()));
                        process_results(dt, 1, results_plural);
                    }
                    case 1:  // nothing else to do
                    default: // not supported
                        break;
                }
            }

            return rt;
        }));
    }

    Progress progress((int)operations.size());
    progress.message(_(L"Pre-translating from translation memory…"));

    for (auto& op: operations)
    {
        if (cancellation_token->is_cancelled())
            break;

        auto rt = op.get();
        stats.add(rt);
        if (translated(rt))
            progress.message(wxString::Format(wxPLURAL("Pre-translated %u string", "Pre-translated %u strings", stats.matched), stats.matched));

        progress.increment();
    }

    return stats;
}


template<typename T, typename TCompletion>
void PreTranslateCatalog(wxWindow *window,
                         CatalogPtr catalog, const T& range,
                         const PreTranslateOptions& options,
                         TCompletion&& completionHandler)
{
    auto changesMade = std::make_shared<bool>(false);

    auto cancellation = std::make_shared<dispatch::cancellation_token>();
    wxWindowPtr<ProgressWindow> progress(new ProgressWindow(window, _(L"Pre-translating…"), cancellation));
    progress->RunTaskThenDo([=]()
    {
        auto stats = PreTranslateCatalogImpl(catalog, range, options, cancellation);
        *changesMade = stats.matched > 0;

        BackgroundTaskResult bg;
        if (stats.matched)
        {
            bg.summary = wxString::Format(wxPLURAL("%d entry was pre-translated.",
                                                   "%d entries were pre-translated.",
                                                   stats.matched), stats.matched);

            if (stats.exact < stats.matched || !(options.flags & PreTranslate_ExactNotFuzzy))
            {
                bg.details.emplace_back(_("The translations were marked as needing work, because they may be inaccurate. You should review them for correctness."), "");
            }

            bg.details.emplace_back(_("Exact matches from TM"), wxNumberFormatter::ToString((long)stats.exact));
            bg.details.emplace_back(_("Approximate matches from TM"), wxNumberFormatter::ToString((long)stats.fuzzy));
        }
        else
        {
            bg.summary = _("No entries could be pre-translated.");
            if (stats.input_strings_count == 0)
            {
                bg.details.emplace_back(_("All strings were already translated."), "");
            }
            else
            {
                bg.details.emplace_back(_(L"The TM doesn’t contain any strings similar to the content of this file. It is only effective for semi-automatic translations after Poedit learns enough from files that you translated manually."), "");
            }
        }

        return bg;
    },
    [changesMade,progress,completionHandler=std::move(completionHandler)](bool success)
    {
        if (success && *changesMade)
            completionHandler();
    });
}

} // anonymous namespace


void PreTranslateCatalogAuto(wxWindow *window, CatalogPtr catalog, const PreTranslateOptions& options, std::function<void()> onChangesMade)
{
    PreTranslateCatalog(window, catalog, catalog->items(), options, std::move(onChangesMade));
}


void PreTranslateWithUI(wxWindow *window, PoeditListCtrl *list, CatalogPtr catalog, std::function<void()> onChangesMade)
{
    if (catalog->UsesSymbolicIDsForSource())
    {
        wxWindowPtr<wxMessageDialog> resultsDlg(
            new wxMessageDialog
                (
                    window,
                    _("Cannot pre-translate without source text."),
                    _("Pre-translate"),
                    wxOK | wxICON_ERROR
                )
        );
        resultsDlg->SetExtendedMessage(_(L"Pre-translation requires that source text is available. It doesn’t work if only IDs without the actual text are used."));
        resultsDlg->ShowWindowModalThenDo([resultsDlg](int){});
        return;
    }
    else if (!catalog->GetSourceLanguage().IsValid())
    {
        wxWindowPtr<wxMessageDialog> resultsDlg(
            new wxMessageDialog
                (
                    window,
                    _("Cannot pre-translate from unknown language."),
                    _("Pre-translate"),
                    wxOK | wxICON_ERROR
                )
        );
        resultsDlg->SetExtendedMessage(_(L"Pre-translation requires that source text’s language is known. Poedit couldn’t detect it in this file."));
        resultsDlg->ShowWindowModalThenDo([resultsDlg](int){});
        return;
    }

    wxWindowPtr<wxDialog> dlg(new wxDialog(window, wxID_ANY, _("Pre-translate"), wxDefaultPosition, wxSize(PX(440), -1)));
    auto topsizer = new wxBoxSizer(wxVERTICAL);
    auto sizer = new wxBoxSizer(wxVERTICAL);
    auto onlyExact = new wxCheckBox(dlg.get(), wxID_ANY, _("Only fill in exact matches"));
    auto onlyExactE = new ExplanationLabel(dlg.get(), _("By default, inaccurate results are also included, but marked as needing work. Check this option to only include perfect matches."));
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

    {
        PretranslateSettings settings = Config::PretranslateSettings();
        onlyExact->SetValue(settings.onlyExact);
        noFuzzy->SetValue(settings.exactNotFuzzy);
    }

    dlg->ShowWindowModalThenDo([catalog,window,list,onlyExact,noFuzzy,onChangesMade,dlg](int retcode)
    {
        if (retcode != wxID_OK)
            return;

        PretranslateSettings settings;
        settings.onlyExact = onlyExact->GetValue();
        settings.exactNotFuzzy = noFuzzy->GetValue();
        Config::PretranslateSettings(settings);

        PreTranslateOptions options;
        if (settings.onlyExact)
            options.flags |= PreTranslate_OnlyExact;
        if (settings.exactNotFuzzy)
            options.flags |= PreTranslate_ExactNotFuzzy;

        if (list->HasMultipleSelection())
        {
            PreTranslateCatalog(window, catalog, list->GetSelectedCatalogItems(), options, std::move(onChangesMade));
        }
        else
        {
            PreTranslateCatalog(window, catalog, catalog->items(), options, std::move(onChangesMade));
        }
    });
}
