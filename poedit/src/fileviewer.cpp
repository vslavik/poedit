
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

#define NEIGHBOUR_SIZE  40

FileViewer::FileViewer(wxWindow *parent, const wxString& filename, int linenum)
        : wxFrame(parent, -1, _("Source file: ") + filename,
                            wxPoint(
                                 wxConfig::Get()->Read("fileviewer/frame_x", -1),
                                 wxConfig::Get()->Read("fileviewer/frame_y", -1)),
                             wxSize(
                                 wxConfig::Get()->Read("fileviewer/frame_w", 600),
                                 wxConfig::Get()->Read("fileviewer/frame_h", 400)))
{
    wxPanel *panel = new wxPanel(this, -1);
	wxListBox *listbox = 
        new wxListBox(panel, -1, wxDefaultPosition, wxDefaultSize, 0, NULL, 
                      wxLB_SINGLE | wxLB_HSCROLL);
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(listbox, 1, wxEXPAND);
    panel->SetSizer(sizer);
    panel->SetAutoLayout(true);

    wxTextFile textf(filename);
    
    textf.Open();
    if (!textf.IsOpened())
    {
        listbox->Append(_("--- Error opening file! ---"));
        return;
    }
    
    int top = wxMax(0, linenum - NEIGHBOUR_SIZE),
        bottom = wxMin(linenum + NEIGHBOUR_SIZE, (int)textf.GetLineCount());

    for (int i = top; i < bottom; i++)
        listbox->Append(textf[i]);
    listbox->SetSelection(linenum - top - 1);
}


FileViewer::~FileViewer()
{
    wxSize sz = GetSize();
    wxPoint pos = GetPosition();
    wxConfigBase *cfg = wxConfig::Get();
    cfg->Write("fileviewer/frame_w", (long)sz.x);
    cfg->Write("fileviewer/frame_h", (long)sz.y);
    cfg->Write("fileviewer/frame_x", (long)pos.x);
    cfg->Write("fileviewer/frame_y", (long)pos.y);
}
