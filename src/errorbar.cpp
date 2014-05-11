/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2012-2014 Vaclav Slavik
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

#include "errorbar.h"

#include "utility.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/dcclient.h>

namespace
{

const wxColour gs_ErrorColor("#ff5050");
const wxColour gs_WarningColor("#ffff50");

} // anonymous namespace


ErrorBar::ErrorBar(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxTAB_TRAVERSAL | wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE)
{
    Bind(wxEVT_PAINT, &ErrorBar::OnPaint, this);

#ifdef __WXMAC__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    m_label = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_label->SetBackgroundColour(gs_ErrorColor);

    wxSizer *sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
    sizer->Add(m_label, wxSizerFlags(1).Center().Border(wxTOP | wxBOTTOM | wxRIGHT, 3));

    SetSizer(sizer);

    // the bar should be initially hidden
    Show(false);
}


void ErrorBar::ShowError(const wxString& error, Severity severity)
{
    const wxString prefix = severity == Sev_Error ? _("Error:") : _("Warning:");
    m_label->SetBackgroundColour(severity == Sev_Error ? gs_ErrorColor : gs_WarningColor);
    m_label->SetLabelMarkup(
        wxString::Format("<b>%s</b> %s", prefix, EscapeMarkup(error)));

    GetContainingSizer()->Show(this);
}


void ErrorBar::HideError()
{
    GetContainingSizer()->Hide(this);
}


void ErrorBar::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    dc.SetBrush(m_label->GetBackgroundColour());
    dc.SetPen(m_label->GetBackgroundColour());
    dc.DrawRoundedRectangle(wxPoint(0,0), dc.GetSize(), 2.0);
}
