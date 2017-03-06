/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2001-2017 Vaclav Slavik
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
#include <wx/textctrl.h>
#include <wx/tokenzr.h>

#include "catalog.h"
#include "commentdlg.h"


CommentDialog::CommentDialog(wxWindow *parent, const wxString& comment) : wxDialog()
{
    wxXmlResource::Get()->LoadDialog(this, parent, "comment_dlg");
#ifndef __WXOSX__
    CenterOnParent();
#endif
    m_text = XRCCTRL(*this, "comment", wxTextCtrl);

    m_text->SetValue(RemoveStartHash(comment));

    wxAcceleratorEntry entries[] = {
        { wxACCEL_CMD, WXK_RETURN, wxID_OK }
    };
    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
}

wxString CommentDialog::GetComment() const
{
    // Put the start hash back
    return AddStartHash(m_text->GetValue());
}

BEGIN_EVENT_TABLE(CommentDialog, wxDialog)
   EVT_BUTTON(XRCID("clear"), CommentDialog::OnClear)
END_EVENT_TABLE()

void CommentDialog::OnClear(wxCommandEvent&)
{
    m_text->Clear();
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
