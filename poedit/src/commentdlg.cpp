/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      commentdlg.cpp
    
      A trivial dialog for editing comments
    
      (c) Vaclav Slavik, 2001

*/

#include <wx/wxprec.h>

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>

#include "catalog.h"
#include "commentdlg.h"


CommentDialog::CommentDialog(wxWindow *parent, const wxString& comment) : wxDialog()
{
    wxXmlResource::Get()->LoadDialog(this, parent, _T("comment_dlg"));
    m_text = XRCCTRL(*this, "comment", wxTextCtrl);

    m_text->SetValue(RemoveStartHash(comment));
}

wxString CommentDialog::GetComment() const
{
    // Put the start hash back
    return AddStartHash(m_text->GetValue());
}

BEGIN_EVENT_TABLE(CommentDialog, wxDialog)
   EVT_BUTTON(XRCID("clear"), CommentDialog::OnClear)
END_EVENT_TABLE()

void CommentDialog::OnClear(wxCommandEvent& event)
{
    m_text->Clear();
}


/*static*/ wxString CommentDialog::RemoveStartHash(const wxString& comment)
{
    wxString tmpComment;
    wxStringTokenizer tkn(comment, _T("\n\r"));
    while (tkn.HasMoreTokens())
        tmpComment << tkn.GetNextToken().Mid(2) << _T("\n");
    return tmpComment;
}

/*static*/ wxString CommentDialog::AddStartHash(const wxString& comment)
{
    wxString tmpComment;
    wxStringTokenizer tkn(comment, _T("\n\r"));
    while (tkn.HasMoreTokens())
        tmpComment << _T("# ") << tkn.GetNextToken() << _T("\n");
    return tmpComment;
}
