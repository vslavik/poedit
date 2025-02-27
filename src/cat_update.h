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

#ifndef Poedit_cat_update_h
#define Poedit_cat_update_h

#include "catalog.h"
#include "gexecute.h"
#include "progress_ui.h"

#include <vector>

class WXDLLIMPEXP_FWD_CORE wxWindow;



/// Summary data about the result of merging two catalogs.
struct MergeResults
{
    // Added and removed strings (as their summary representation, not the full items)
    std::vector<wxString> added;
    std::vector<wxString> removed;

    int changes_count() const { return int(added.size() + removed.size()); }

    // Any errors/warnings that occurred during the merge
    ParsedGettextErrors errors;
};


/// Specialization of ProgressWindow that shows issues and allows viewing merge results.
class MergeProgressWindow : public ProgressWindow
{
public:
    using ProgressWindow::ProgressWindow;

protected:
    bool SetSummaryContent(const BackgroundTaskResult& data) override;

    void AddViewDetails(const MergeResults& r);
};


/**
    Update catalog from source code w/o any UI or detailed error reporting.
 */
bool PerformUpdateFromSourcesSimple(CatalogPtr catalog);

/**
    Update catalog from source code, if configured, and provide UI
    during the operation.
 */
bool PerformUpdateFromSourcesWithUI(wxWindow *parent,
                                    CatalogPtr catalog);

/**
    Similarly for updating from a reference file (i.e. POT).
 */
bool PerformUpdateFromReferenceWithUI(wxWindow *parent,
                                      CatalogPtr catalog,
                                      const wxString& reference_file);


#endif // Poedit_cat_update_h
