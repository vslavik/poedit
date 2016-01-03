/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2000-2015 Vaclav Slavik
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

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/intl.h>

#include "summarydlg.h"
#include "utility.h"


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
    for (i = 0; i < snew.GetCount(); i++)
    {
        listbox->Append(snew[i]);
    }

    listbox = XRCCTRL(*this, "obsolete_strings", wxListBox);
    for (i = 0; i < sobsolete.GetCount(); i++)
    {
        listbox->Append(sobsolete[i]);
    }
}
