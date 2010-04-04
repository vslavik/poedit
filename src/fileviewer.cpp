/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 1999-2005 Vaclav Slavik
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
        : wxFrame(parent, -1, _("Source file")),
          m_references(references)
{
    m_basePath = basePath;
    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, _T("fileview_toolbar")));

    wxPanel *panel = new wxPanel(this, -1);
	m_list = new wxListCtrl(panel, -1, wxDefaultPosition, wxDefaultSize, 
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER |
                            wxSUNKEN_BORDER);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_list, 1, wxEXPAND);
    panel->SetSizer(sizer);
    panel->SetAutoLayout(true);
	panel->Layout();
    
    wxChoice *choice = XRCCTRL(*GetToolBar(), "references", wxChoice);
    for (size_t i = 0; i < references.Count(); i++)
        choice->Append(references[i]);
    choice->SetSelection(startAt);

    ShowReference(m_references[startAt]);

    wxConfigBase *cfg = wxConfig::Get();
    int width = cfg->Read(_T("fileviewer/frame_w"), 600);
    int height = cfg->Read(_T("fileviewer/frame_h"), 400);
    SetClientSize(width, height);

#ifndef __WXGTK__
    int posx = cfg->Read(_T("fileviewer/frame_x"), -1);
    int posy = cfg->Read(_T("fileviewer/frame_y"), -1);
    Move(posx, posy);
#endif
}

void FileViewer::ShowReference(const wxString& ref)
{
    wxPathFormat pathfmt = ref.Contains(_T('\\')) ? wxPATH_WIN : wxPATH_UNIX;
    wxFileName filename(ref.BeforeLast(_T(':')), pathfmt);
    filename.MakeAbsolute(m_basePath);

    // support GNOME's xml2po's extension to references in the form of
    // filename:line(xml_node):
    wxString linenumStr = ref.AfterLast(_T(':')).BeforeFirst(_T('('));

    long linenum;
    if (!linenumStr.ToLong(&linenum))
        linenum = 0;

    wxTextFile textf(filename.GetFullPath());

    textf.Open();
    if (!textf.IsOpened())
    {
        wxLogError(_("Error opening file %s!"), filename.GetFullPath().c_str());
        return;
    }
    
    int top    = wxMax(1, linenum - NEIGHBOUR_SIZE),
        bottom = wxMin(linenum + NEIGHBOUR_SIZE, (int)textf.GetLineCount());

    m_list->ClearAll();
    m_list->InsertColumn(0, _T("#"), wxLIST_FORMAT_RIGHT);
    m_list->InsertColumn(1, _("Line"));

    for (int i = top; i < bottom; i++)
    {
        wxString linenum(wxString::Format(_T("%i  "), i));
        wxString linetxt = textf[i-1];
        // FIXME: this is not correct
        linetxt.Replace(_T("\t"), _T("    "));
        m_list->InsertItem(i - top, linenum);
        m_list->SetItem(i - top, 1, linetxt);
    }

    m_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
    m_list->SetColumnWidth(1, wxLIST_AUTOSIZE);

    m_list->SetItemState(linenum - top,
                         wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    m_list->EnsureVisible(wxMax(0, linenum - top - 5));
    m_list->EnsureVisible(wxMin(linenum - top + 5, bottom - top - 1));
    m_list->EnsureVisible(linenum - top);

    m_current = ref;
}

FileViewer::~FileViewer()
{
    wxSize sz = GetClientSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write(_T("fileviewer/frame_w"), (long)sz.x);
    cfg->Write(_T("fileviewer/frame_h"), (long)sz.y);
    cfg->Write(_T("fileviewer/frame_x"), (long)pos.x);
    cfg->Write(_T("fileviewer/frame_y"), (long)pos.y);
}


BEGIN_EVENT_TABLE(FileViewer, wxFrame)
    EVT_CHOICE(XRCID("references"), FileViewer::OnChoice)
END_EVENT_TABLE()

void FileViewer::OnChoice(wxCommandEvent &event)
{
    ShowReference(m_references[event.GetSelection()]);
}
