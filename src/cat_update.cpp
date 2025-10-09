/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2025 Vaclav Slavik
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

#include "cat_update.h"

#include "catalog_po.h"
#include "colorscheme.h"
#include "custom_notebook.h"
#include "errors.h"
#include "extractors/extractor.h"
#include "hidpi.h"
#include "progress_ui.h"
#include "utility.h"

#include <wx/artprov.h>
#include <wx/button.h>
#include <wx/config.h>
#include <wx/dataview.h>
#include <wx/dialog.h>
#include <wx/intl.h>
#include <wx/listbox.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/numformatter.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/stdpaths.h>
#include <wx/wupdlock.h>


namespace
{

inline auto variant_vector(std::initializer_list<wxVariant>&& init)
{
    return wxVector<wxVariant>(init.begin(), init.end());
}



class SummaryList : public wxDataViewListCtrl
{
public:
    SummaryList(wxWindow *parent, int flags = 0)
        : wxDataViewListCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                             flags | wxDV_ROW_LINES | wxDV_VARIABLE_LINE_HEIGHT | wxBORDER_NONE)
    {
        SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#ifdef __WXOSX__
        ((NSTableView*)[((NSScrollView*)GetHandle()) documentView]).style = NSTableViewStyleFullWidth;
#endif
    }
};


/// This class provides simple dialog that displays list of changes made in the catalog.
class MergeSummaryDialog : public wxDialog
{
public:
    MergeSummaryDialog(wxWindow *parent);
    ~MergeSummaryDialog();

    /// Reads data from catalog and fill dialog's controls.
    void TransferTo(const MergeStats& r);

private:
    SegmentedNotebookBase *m_notebook;
};


MergeSummaryDialog::MergeSummaryDialog(wxWindow *parent)
    // TRANSLATORS: Title of window showing summary (added/removed strings, issues) of updating translations from sources or POT file
    : wxDialog(parent, wxID_ANY, MSW_OR_OTHER(_("Update summary"), _("Update Summary")),
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    SetName("summary");

#ifdef __WXMSW__
    SetIcons(wxIconBundle(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\Poedit.ico"));
#endif

    auto topsizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(topsizer);

    auto panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | MSW_OR_OTHER(wxBORDER_SIMPLE, wxBORDER_SUNKEN));

    ColorScheme::SetupWindowColors(panel, [=]
    {
        if (ColorScheme::GetWindowMode(panel) == ColorScheme::Light)
            panel->SetBackgroundColour(*wxWHITE);
        else
            panel->SetBackgroundColour(GetDefaultAttributes().colBg);
    });
    topsizer->Add(panel, wxSizerFlags(1).Expand().Border(wxLEFT|wxRIGHT|wxTOP, PX(20)));

    auto sizer = new wxBoxSizer(wxVERTICAL);
    panel->SetSizer(sizer);

    m_notebook = SegmentedNotebook::Create(panel, SegmentStyle::SidebarPanels);
    sizer->Add(m_notebook, wxSizerFlags(1).Expand().Border(wxTOP, PX(1)));

    auto buttons = CreateButtonSizer(wxOK);
    auto ok = static_cast<wxButton*>(FindWindow(wxID_OK));
    ok->SetLabel(_("Close"));
    ok->SetDefault();

#ifdef __WXOSX__
    topsizer->AddSpacer(PX(5));
    topsizer->Add(buttons, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT|wxBOTTOM, PX(10)));
#else
    topsizer->AddSpacer(PX(10));
    topsizer->Add(buttons, wxSizerFlags().Expand().Border(wxRIGHT, PX(15)));
    topsizer->AddSpacer(PX(15));
#endif

    RestoreWindowState(this, wxSize(PX(700), PX(500)), WinState_Size);
    CentreOnParent();
}


MergeSummaryDialog::~MergeSummaryDialog()
{
    SaveWindowState(this, WinState_Size);
}


void MergeSummaryDialog::TransferTo(const MergeStats& r)
{
    wxWindowUpdateLocker lock(this);

    if (r.errors)
    {
        auto list = new SummaryList(m_notebook);
        m_notebook->AddPage(list, _("Issues"));

        // TRANSLATORS: Column header in the list of issues where rows are filename:line:text of issue
        list->AppendTextColumn(_("File"), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);
        // TRANSLATORS: Column header in the list of issues where rows are filename:line:text of issue
        list->AppendTextColumn(_("Line"), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_RIGHT);
        // TRANSLATORS: Column header in the list of issues where rows are filename:line:text of issue
        list->AppendTextColumn(_("Issue"), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

        for (auto& i: r.errors.items)
        {
            if (i.has_location())
                list->AppendItem(variant_vector({i.file, wxString::Format("%d", i.line), i.text}));
            else
                list->AppendItem(variant_vector({"", "", i.text}));
        }
    }

    if (!r.added.empty())
    {
        auto list = new SummaryList(m_notebook, wxDV_NO_HEADER);
        m_notebook->AddPage(list, MSW_OR_OTHER(_("New strings"), _("New Strings")));

        list->AppendTextColumn(MSW_OR_OTHER(_("New strings"), _("New Strings")), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

        for (auto& s: r.added)
        {
            list->AppendItem(variant_vector({s.to_string()}));
        }
    }

    if (!r.removed.empty())
    {
        auto list = new SummaryList(m_notebook, wxDV_NO_HEADER);
        m_notebook->AddPage(list, MSW_OR_OTHER(_("Removed strings"), _("Removed Strings")));

        list->AppendTextColumn(MSW_OR_OTHER(_("Removed strings"), _("Removed Strings")), wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE);

        for (auto& s: r.removed)
        {
            list->AppendItem(variant_vector({s.to_string()}));
        }
    }
}


struct InterimResults
{
    CatalogPtr reference;
    ParsedGettextErrors errors;
};


InterimResults ExtractPOTFromSources(CatalogPtr catalog)
{
    auto po = std::dynamic_pointer_cast<POCatalog>(catalog);
    if (!po)
        BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::Unspecified));

    Progress progress(1);
    progress.message(_(L"Collecting source files…"));

    auto spec = catalog->GetSourceCodeSpec();
    if (!spec)
    {
        BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::NoSourcesFound));
    }

    InterimResults output;
    auto files = Extractor::CollectAllFiles(*spec);

    progress.message
    (
        // TRANSLATORS: %s is the number of files
        wxString::Format
        (
            wxPLURAL(L"Extracting translatable strings from %s file…",
                     L"Extracting translatable strings from %s files…",
                     (int)files.size()),
            wxNumberFormatter::ToString((long)files.size())
        )
    );

    if (!files.empty())
    {
        TempDirectory tmpdir;
        auto result = Extractor::ExtractWithAll(tmpdir, *spec, files);
        if (!result)
            BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::Unspecified));

        try
        {
            output.errors = result.errors;
            output.reference = POCatalog::Create(result.pot_file, Catalog::CreationFlag_IgnoreHeader);
            return output;
        }
        catch (...)
        {
            wxLogError(_("Failed to load file with extracted translations."));
            BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::Unspecified));
        }
    }
    else
    {
        BOOST_THROW_EXCEPTION(ExtractionException(ExtractionError::NoSourcesFound));
    }


}


InterimResults ExtractPOTFromSourcesWithExplanatoryErrors(CatalogPtr catalog)
{
    try
    {
        return ExtractPOTFromSources(catalog);
    }
    catch (ExtractionException& e)
    {
        // FIXME: nicer UI than this; log inside summary as multiple errors
        wxString msgSuffix;
        if (!e.file.empty() && e.file != ".")
            msgSuffix += "\n\n" + wxString::Format(_("In: %s"), e.file);

        wxString msg, explain;

        switch (e.error)
        {
            case ExtractionError::NoSourcesFound:
                msg = _("Source code not available.");
                explain = _(L"Translations couldn’t be updated from the source code, because no code was found in the location specified in the file’s Properties.");
                break;

            case ExtractionError::PermissionDenied:
                msg = _("Permission denied.");
                explain = _(L"You don’t have permission to read source code files from the location specified in the file’s Properties.");
#ifdef __WXOSX__
                if (@available(macOS 13.0, *))
                {
                    // TRANSLATORS: The System Settings etc. references macOS 13 Ventura or newer system settings and should be translated EXACTLY as in macOS. If you don't use macOS and can't check, please leave it untranslated.
                    explain += "\n\n" + _("If you previously denied access to your files, you can allow it in System Settings > Privacy & Security > Files & Folders.");
                }
                else
                {
                    // TRANSLATORS: The System Preferences etc. references macOS system settings and should be translated EXACTLY as in macOS. If you don't use macOS and can't check, please leave it untranslated.
                    explain += "\n\n" + _("If you previously denied access to your files, you can allow it in System Preferences > Security & Privacy > Privacy > Files & Folders.");
                }
#endif
                break;

            case ExtractionError::Unspecified:
                msg = _("Failed to extract strings from source code.");
                explain.clear(); // will be taken from wxLogError output, if any
                break;
        }

        explain += msgSuffix;
        BOOST_THROW_EXCEPTION( BackgroundTaskException(msg, explain) );
    }
}


InterimResults LoadReferenceFile(const wxString& ref_file)
{
    try
    {
        InterimResults data;
        data.reference = Catalog::Create(ref_file, Catalog::CreationFlag_IgnoreTranslations);

        // Silently fix duplicates because they are common in WordPress world:
        auto pot = std::dynamic_pointer_cast<POCatalog>(data.reference);
        if (pot)
        {
            if (pot->HasDuplicateItems())
                pot->FixDuplicateItems();
        }

        return data;
    }
    catch (...)
    {
        BOOST_THROW_EXCEPTION
        (
              BackgroundTaskException
              (
                  wxString::Format(_(L"The file “%s” couldn’t be opened."), wxFileName(ref_file).GetFullName()),
                  DescribeCurrentException()
              )
        );
    }
}


template<typename Func>
dispatch::future<CatalogPtr> DoPerformUpdateWithUI(wxWindow *parent,
                                                   CatalogPtr catalog,
                                                   int timeCostObtainPOT,
                                                   Func&& funcObtainPOT)
{
    auto promise = std::make_shared<dispatch::promise<CatalogPtr>>();
    auto merge_result = std::make_shared<MergeResult>();

    auto cancellation = std::make_shared<dispatch::cancellation_token>();
    wxWindowPtr<ProgressWindow> progress(new MergeProgressWindow(parent, _("Updating translations"), cancellation));

    progress->RunTaskThenDo([merge_result,catalog,cancellation,funcObtainPOT,timeCostObtainPOT]() -> BackgroundTaskResult
    {
        InterimResults data;

        Progress p(100);

        {
            Progress subtask(1, p, timeCostObtainPOT);
            data = funcObtainPOT();
        }

        cancellation->throw_if_cancelled();

        MergeStats stats;
        stats.errors = data.errors;

        {
            Progress subtask(1, p, (100 - timeCostObtainPOT) / 2);
            subtask.message(_(L"Determining differences…"));
            ComputeMergeStats(stats, catalog, data.reference);
        }

        cancellation->throw_if_cancelled();

        {
            Progress subtask(1, p, (100 - timeCostObtainPOT) / 2);
            subtask.message(_(L"Merging differences…"));
            *merge_result = MergeCatalogWithReference(catalog, data.reference);
            if (!(*merge_result))
                BOOST_THROW_EXCEPTION( BackgroundTaskException(_("Failed to load file with extracted translations.")) );

            stats.errors += merge_result->errors;
        }

        BackgroundTaskResult bg;
        bg.user_data = stats;

        if (stats.changes_count() == 0)
        {
            bg.summary = _("Translation file is already up to date, no changes to strings were made.");
        }
        else
        {
            bg.summary = wxString::Format
                         (
                             wxPLURAL("Translation file was updated with %s change.", "Translation file was updated with %s changes.", stats.changes_count()),
                             wxNumberFormatter::ToString((long)stats.changes_count())
                          );
            bg.details.emplace_back(_("New strings to translate:"), wxNumberFormatter::ToString((long)stats.added.size()));
            bg.details.emplace_back(_("Removed strings (no longer used):"), wxNumberFormatter::ToString((long)stats.removed.size()));
        }

        return bg;
    },
    [progress,promise,merge_result](bool ok)
    {
        promise->set_value(ok && merge_result ? merge_result->updated_catalog : nullptr);
    });

    return promise->get_future();
}


} // anonymous namespace


MergeResult PerformUpdateFromSourcesSimple(CatalogPtr catalog)
{
    Progress p(100);

    InterimResults data;
    {
        Progress subtask(1, p, 90);
        data = ExtractPOTFromSources(catalog);
        if (!data.reference)
            return {};
    }
    {
        Progress subtask(1, p, 10);
        subtask.message(_(L"Merging differences…"));
        auto merged = MergeCatalogWithReference(catalog, data.reference);
        merged.errors += data.errors;
        return merged;
    }
}


dispatch::future<CatalogPtr>
PerformUpdateFromSourcesWithUI(wxWindow *parent, CatalogPtr catalog)
{
    return DoPerformUpdateWithUI(parent, catalog,
                                 90,
                                 [=]{ return ExtractPOTFromSourcesWithExplanatoryErrors(catalog); });
}


dispatch::future<CatalogPtr>
PerformUpdateFromReferenceWithUI(wxWindow *parent, CatalogPtr catalog, const wxString& reference_file)
{
    return DoPerformUpdateWithUI(parent, catalog,
                                 50,
                                 [=]{ return LoadReferenceFile(reference_file); });
}


bool MergeProgressWindow::SetSummaryContent(const BackgroundTaskResult& data)
{
    if (!ProgressWindow::SetSummaryContent(data))
        return false;

    if (!data.user_data.has_value())
        return true;  // nothing more to show

    if (auto r = std::any_cast<MergeStats>(&data.user_data))
    {
        AddViewDetails(*r);
    }
    else if (auto e = std::any_cast<ParsedGettextErrors>(&data.user_data))
    {
        MergeStats ms;
        ms.errors = *e;
        AddViewDetails(ms);
    }

    return true;
}


void MergeProgressWindow::AddViewDetails(const MergeStats& r)
{
    if (!r.errors && r.added.empty() && r.removed.empty())
        return;  // nothing at all to show

    if (r.errors)
    {
        auto msg = wxString::Format
        (
         wxPLURAL("%d issue with the source strings was detected.",
                  "%d issues with the source strings were detected.",
                  (int)r.errors.items.size()),
         (int)r.errors.items.size()
         );

        auto sizer = GetDetailsSizer();
        auto line = new wxBoxSizer(wxHORIZONTAL);
        sizer->Insert(0, line, wxSizerFlags().Expand().Border(wxBOTTOM, PX(6)));

        line->Add(new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap("StatusWarning")), wxSizerFlags().Center().Border(wxTOP|wxBOTTOM, PX(2)));
        line->AddSpacer(PX(6));

        auto label = new wxStaticText(this, wxID_ANY, msg);
#if defined(__WXOSX__) || defined(__WXGTK__)
        label->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif
#ifndef __WXGTK__
        ColorScheme::SetupWindowColors(this, [=]
                                       {
            label->SetForegroundColour(ColorScheme::Get(Color::ItemFuzzy));
        });
#endif
        line->Add(label, wxSizerFlags().Center());
    }

    auto sizer = GetButtonSizer();
    auto button = new wxButton(this, wxID_ANY, MSW_OR_OTHER(_(L"View details…"), _(L"View Details…")));
    sizer->Insert(0, button);
    sizer->InsertStretchSpacer(1);

    button->Bind(wxEVT_BUTTON, [=](wxCommandEvent&)
    {
        auto dlg = new MergeSummaryDialog(this);
        dlg->TransferTo(r);
        dlg->ShowModal();
    });
}
