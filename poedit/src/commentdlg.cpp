/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      commentdlg.cpp
    
      A trivial dialog for editing comments
    
      (c) Vaclav Slavik, 2001

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>

#include "catalog.h"
#include "commentdlg.h"


CommentDialog::CommentDialog(wxWindow *parent, const wxString& comment) : wxDialog()
{
    wxTheXmlResource->LoadDialog(this, parent, _T("comment_dlg"));
    m_text = XMLCTRL(*this, "comment", wxTextCtrl);
    
    wxString txt;
    wxStringTokenizer tkn(comment, _T("\n\r"));
    while (tkn.HasMoreTokens())
        txt << tkn.GetNextToken().Mid(2)/* "# " */ << _T("\n");
    m_text->SetValue(txt);
}

wxString CommentDialog::GetComment() const
{
    wxString txt, txt2;
    txt = m_text->GetValue();
    wxStringTokenizer tkn(txt, _T("\n\r"));
    while (tkn.HasMoreTokens())
        txt2 << _T("# ") << tkn.GetNextToken() << _T("\n");
    return txt2;
}

BEGIN_EVENT_TABLE(CommentDialog, wxDialog)
   EVT_BUTTON(XMLID("clear"), CommentDialog::OnClear)
END_EVENT_TABLE()

void CommentDialog::OnClear(wxCommandEvent& event)
{
    m_text->Clear();
}

