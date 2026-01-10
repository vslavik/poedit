/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2001-2026 Vaclav Slavik
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

#include "commentdlg.h"

#include <wx/config.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>

#include "layout_helpers.h"


CommentDialog::CommentDialog(wxWindow *parent, const wxString& comment)
    : StandardDialog(parent, _("Edit comment"), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto sizer = ContentSizer();

    auto label = new wxStaticText(this, wxID_ANY, _("Comment:"));
    sizer->Add(label, wxSizerFlags().Left().Border(wxBOTTOM, PX(6)));

    m_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(PX(400), PX(160)), wxTE_MULTILINE);
    sizer->Add(m_text, wxSizerFlags(1).Expand());

    auto okButton = new wxButton(this, wxID_OK, _("Update"));
    auto deleteButton = new wxButton(this, wxID_DELETE);
    deleteButton->SetToolTip(_("Delete the comment"));

    CreateButtons()
        .Add(okButton)
        .Add(deleteButton)
        .Add(wxID_CANCEL);

    m_text->SetValue(RemoveStartHash(comment).Strip(wxString::both));
    m_text->SetFocus();

    if (comment.empty())
    {
        deleteButton->Disable();
        okButton->SetLabel(_("Add"));
    }
    // else: button is labeled "Update"

    // Bind events instead of using event table
    deleteButton->Bind(wxEVT_BUTTON, &CommentDialog::OnDelete, this);

    FitSizer();

#ifndef __WXOSX__
    CenterOnParent();
#endif
}

wxString CommentDialog::GetComment() const
{
    // Put the start hash back
    return AddStartHash(m_text->GetValue().Strip(wxString::both));
}

void CommentDialog::OnDelete(wxCommandEvent&)
{
    m_text->Clear();
    EndModal(wxID_OK);
}


/*static*/ wxString CommentDialog::RemoveStartHash(const wxString& comment)
{
    wxString tmpComment;
    wxStringTokenizer tkn(comment, "\n\r");
    while (tkn.HasMoreTokens())
        tmpComment << tkn.GetNextToken().Mid(2) << "\n";
    return tmpComment;
}

/*static*/ wxString CommentDialog::AddStartHash(const wxString& comment)
{
    wxString tmpComment;
    wxStringTokenizer tkn(comment, "\n\r");
    while (tkn.HasMoreTokens())
        tmpComment << "# " << tkn.GetNextToken() << "\n";
    return tmpComment;
}
