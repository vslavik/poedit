
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      fileviewer.cpp
    
      Shows part of file around specified line
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include "fileviewer.h"
#include <wx/wx.h>
#include <wx/textfile.h>
#include <wx/config.h>
#include <wx/sizer.h>
#include <wx/listctrl.h>
#include <wx/xrc/xmlres.h>

#define NEIGHBOUR_SIZE  40

FileViewer::FileViewer(wxWindow *parent, 
                       const wxString& basePath,
                       const wxArrayString& references,
                       size_t startAt)
        : wxFrame(parent, -1, _("Source file"),
                            wxPoint(
                                 wxConfig::Get()->Read(_T("fileviewer/frame_x"), -1),
                                 wxConfig::Get()->Read(_T("fileviewer/frame_y"), -1)),
                             wxSize(
                                 wxConfig::Get()->Read(_T("fileviewer/frame_w"), 600),
                                 wxConfig::Get()->Read(_T("fileviewer/frame_h"), 400))),
          m_references(references)
{
    m_basePath = basePath;
    m_basePath << _T('/');
    SetToolBar(wxTheXmlResource->LoadToolBar(this, _T("fileview_toolbar")));

    wxPanel *panel = new wxPanel(this, -1);
	m_list = new wxListCtrl(panel, -1, wxDefaultPosition, wxDefaultSize, 
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER |
                            wxSUNKEN_BORDER);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_list, 1, wxEXPAND);
    panel->SetSizer(sizer);
    panel->SetAutoLayout(true);
    
    wxChoice *choice = XMLCTRL(*GetToolBar(), "references", wxChoice);
    for (size_t i = 0; i < references.Count(); i++)
        choice->Append(references[i]);
    choice->SetSelection(startAt);

    ShowReference(m_references[startAt]);
}

void FileViewer::ShowReference(const wxString& ref)
{
    wxString filename = m_basePath + ref.BeforeLast(_T(':'));
    long linenum;
    if (!ref.AfterLast(_T(':')).ToLong(&linenum))
        linenum = 0;

    wxTextFile textf(filename);

    textf.Open();
    if (!textf.IsOpened())
    {
        wxLogError(_("Error opening file %s!"), filename.c_str());
        return;
    }
    
    int top = wxMax(0, linenum - NEIGHBOUR_SIZE),
        bottom = wxMin(linenum + NEIGHBOUR_SIZE, (int)textf.GetLineCount());

    m_list->ClearAll();
    m_list->InsertColumn(0, wxEmptyString, wxLIST_FORMAT_RIGHT);
    m_list->InsertColumn(1, wxEmptyString);

    wxString linestr;
    for (int i = top; i < bottom; i++)
    {
        linestr.Printf(_T("%i  "), i+1);
        m_list->InsertItem(i - top, linestr);
        m_list->SetItem(i - top, 1, textf[i]);
    }

    m_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_list->SetColumnWidth(1, wxLIST_AUTOSIZE);

    m_list->SetItemState(linenum-1 - top,
                         wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    m_list->EnsureVisible(wxMax(0, linenum-1 - top - 5));
    m_list->EnsureVisible(wxMin(linenum-1 - top + 5, bottom - top - 1));
    m_list->EnsureVisible(linenum-1 - top);

    m_current = ref;
}

FileViewer::~FileViewer()
{
    wxSize sz = GetSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write(_T("fileviewer/frame_w"), (long)sz.x);
    cfg->Write(_T("fileviewer/frame_h"), (long)sz.y);
    cfg->Write(_T("fileviewer/frame_x"), (long)pos.x);
    cfg->Write(_T("fileviewer/frame_y"), (long)pos.y);
}


BEGIN_EVENT_TABLE(FileViewer, wxFrame)
    EVT_CHOICE(XMLID("references"), FileViewer::OnChoice)
    EVT_MENU(XMLID("edit_file"), FileViewer::OnEditFile)
END_EVENT_TABLE()

void FileViewer::OnChoice(wxCommandEvent &event)
{
    ShowReference(m_references[event.GetSelection()]);
}

void FileViewer::OnEditFile(wxCommandEvent &event)
{
    wxString editor = wxConfig::Get()->Read(_T("ext_editor"), wxEmptyString);
    if (!editor)
    {
        wxLogError(_("No editor specified. Please set it in Preferences dialog."));
        return;
    }
    editor.Replace(_T("%f"), m_basePath + m_current.BeforeLast(_T(':')));
    editor.Replace(_T("%l"), m_current.AfterLast(_T(':')));
    wxExecute(editor);
}
