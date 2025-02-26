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
#include "errors.h"
#include "extractors/extractor.h"
#include "progress_ui.h"
#include "utility.h"

#include <wx/config.h>
#include <wx/dialog.h>
#include <wx/intl.h>
#include <wx/listbox.h>
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/numformatter.h>
#include <wx/stattext.h>
#include <wx/xrc/xmlres.h>


namespace
{

/** This class provides simple dialog that displays list
  * of changes made in the catalog.
  */
class MergeSummaryDialog : public wxDialog
{
    public:
        MergeSummaryDialog(wxWindow *parent);
        ~MergeSummaryDialog();

        /** Reads data from catalog and fill dialog's controls.
            \param snew      list of strings that are new to the catalog
            \param sobsolete list of strings that no longer appear in the
                             catalog (as compared to catalog's state before
                             parsing sources).
         */
        void TransferTo(const wxArrayString& snew, 
                        const wxArrayString& sobsolete);
};


MergeSummaryDialog::MergeSummaryDialog(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "summary");

    RestoreWindowState(this, wxDefaultSize, WinState_Size);
    CentreOnParent();
}


MergeSummaryDialog::~MergeSummaryDialog()
{
    SaveWindowState(this, WinState_Size);
}


void MergeSummaryDialog::TransferTo(const wxArrayString& snew, const wxArrayString& sobsolete)
{
    wxString sum;
    sum.Printf(_("(New: %i, obsolete: %i)"),
               (int)snew.GetCount(), (int)sobsolete.GetCount());
    XRCCTRL(*this, "items_count", wxStaticText)->SetLabel(sum);

    wxListBox *listbox;
    size_t i;
    
    listbox = XRCCTRL(*this, "new_strings", wxListBox);
#ifdef __WXOSX__
    if (@available(macOS 11.0, *))
        ((NSTableView*)[((NSScrollView*)listbox->GetHandle()) documentView]).style = NSTableViewStyleFullWidth;
#endif

    for (i = 0; i < snew.GetCount(); i++)
    {
        listbox->Append(snew[i]);
    }

    listbox = XRCCTRL(*this, "obsolete_strings", wxListBox);
#ifdef __WXOSX__
    if (@available(macOS 11.0, *))
        ((NSTableView*)[((NSScrollView*)listbox->GetHandle()) documentView]).style = NSTableViewStyleFullWidth;
#endif

    for (i = 0; i < sobsolete.GetCount(); i++)
    {
        listbox->Append(sobsolete[i]);
    }
}


inline wxString ItemMergeSummary(const CatalogItemPtr& item)
{
    wxString s = item->GetString();
    if ( item->HasPlural() )
        s += "|" + item->GetPluralString();
    if ( item->HasContext() )
        s += wxString::Format(" [%s]", item->GetContext());

    return s;
}

/** Returns list of strings that are new in reference catalog
    (compared to this one) and that are not present in \a refcat
    (i.e. are obsoleted).

    \see ShowMergeSummary
 */
void GetMergeSummary(CatalogPtr po, CatalogPtr refcat,
                     wxArrayString& snew, wxArrayString& sobsolete)
{
    wxASSERT( snew.empty() );
    wxASSERT( sobsolete.empty() );

    std::set<wxString> strsThis, strsRef;

    for (auto& i: po->items())
        strsThis.insert(ItemMergeSummary(i));
    for (auto& i: refcat->items())
        strsRef.insert(ItemMergeSummary(i));

    for (auto& i: strsThis)
    {
        if (strsRef.find(i) == strsRef.end())
            sobsolete.Add(i);
    }

    for (auto& i: strsRef)
    {
        if (strsThis.find(i) == strsThis.end())
            snew.Add(i);
    }
}

struct InterimResults
{
    CatalogPtr reference;
};


bool MergeCatalogWithReference(CatalogPtr catalog, CatalogPtr reference)
{
    auto po_catalog = std::dynamic_pointer_cast<POCatalog>(catalog);
    auto po_ref = std::dynamic_pointer_cast<POCatalog>(reference);
    if (!po_catalog || !po_ref)
        return false;

    return po_catalog->UpdateFromPOT(po_ref);
}


InterimResults ExtractPOTFromSources(CatalogPtr catalog)
{
    auto po = std::dynamic_pointer_cast<POCatalog>(catalog);
    if (!po)
        throw ExtractionException(ExtractionError::Unspecified);

    Progress progress(1);
    progress.message(_(L"Collecting source files…"));

    auto spec = catalog->GetSourceCodeSpec();
    if (!spec)
    {
        throw ExtractionException(ExtractionError::NoSourcesFound);
    }

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
            throw ExtractionException(ExtractionError::Unspecified);

        try
        {
            // FIXME: Don't do this, provide nice UI
            result.errors.log_all();

            auto pot = POCatalog::Create(result.pot_file, Catalog::CreationFlag_IgnoreHeader);

            return InterimResults{ pot };
        }
        catch (...)
        {
            wxLogError(_("Failed to load file with extracted translations."));
            throw ExtractionException(ExtractionError::Unspecified);
        }
    }
    else
    {
        throw ExtractionException(ExtractionError::NoSourcesFound);
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
                else if (@available(macOS 10.15, *))
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
bool DoPerformUpdateWithUI(wxWindow *parent,
                           CatalogPtr catalog,
                           int timeCostObtainPOT,
                           Func&& funcObtainPOT)
{
    auto cancellation = std::make_shared<dispatch::cancellation_token>();
    auto data = std::make_shared<InterimResults>();

    wxWindowPtr<ProgressWindow> progress(new ProgressWindow(parent, _("Updating translations"), cancellation));

    bool ok = progress->RunTaskModal([catalog,cancellation,funcObtainPOT,timeCostObtainPOT,data]() -> BackgroundTaskResult
    {
        Progress p(100);

        {
            Progress subtask(1, p, timeCostObtainPOT);
            *data = funcObtainPOT();
        }

        cancellation->throw_if_cancelled();

        {
            Progress subtask(1, p, 100 - timeCostObtainPOT);
            subtask.message(_(L"Merging differences…"));
            auto merged_ok = MergeCatalogWithReference(catalog, data->reference);
            if (!merged_ok)
                BOOST_THROW_EXCEPTION( BackgroundTaskException(_("Failed to load file with extracted translations.")) );
        }

        return {};
    });

    return ok;
}


} // anonymous namespace


bool PerformUpdateFromSourcesSimple(CatalogPtr catalog)
{
    Progress p(100);

    InterimResults data;
    {
        Progress subtask(1, p, 90);
        data = ExtractPOTFromSources(catalog);
        if (!data.reference)
            return false;
    }
    {
        Progress subtask(1, p, 10);
        subtask.message(_(L"Merging differences…"));
        return MergeCatalogWithReference(catalog, data.reference);
    }
}


bool PerformUpdateFromSourcesWithUI(wxWindow *parent,
                                    CatalogPtr catalog)
{
    return DoPerformUpdateWithUI(parent, catalog,
                                 80,
                                 [=]{ return ExtractPOTFromSourcesWithExplanatoryErrors(catalog); });
}


bool PerformUpdateFromReferenceWithUI(wxWindow *parent,
                                      CatalogPtr catalog,
                                      const wxString& reference_file)
{
    return DoPerformUpdateWithUI(parent, catalog,
                                 50,
                                 [=]{ return LoadReferenceFile(reference_file); });
}
