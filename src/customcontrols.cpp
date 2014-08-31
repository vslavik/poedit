/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2014 Vaclav Slavik
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

#include "customcontrols.h"

#include <wx/textwrapper.h>

namespace
{

class LabelWrapper : public wxTextWrapper
{
public:
    void WrapLabel(wxWindow *window, const wxString& text, int widthMax)
    {
        m_text.clear();
        Wrap(window, text, widthMax);
        window->SetLabel(m_text);
    }

    void OnOutputLine(const wxString& line) override
        { m_text += line; }
    void OnNewLine() override
        { m_text += '\n'; }

    wxString m_text;
};

}

ExplanationLabel::ExplanationLabel(wxWindow *parent, const wxString& label)
    : wxStaticText(parent, wxID_ANY, ""),
      m_text(label),
      m_wrapWidth(-1)
{
    m_text.Replace("\n", " ");
    SetLabel(m_text);

#ifdef __WXOSX__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    SetForegroundColour(wxColour("#888888"));
#else
    SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
#endif
    
    SetInitialSize(wxSize(10,10));
    Bind(wxEVT_SIZE, &ExplanationLabel::OnSize, this);
}

void ExplanationLabel::OnSize(wxSizeEvent& e)
{
    e.Skip();
    int w = e.GetSize().x;
    if (w == m_wrapWidth)
        return;

    m_wrapWidth = w;
    LabelWrapper().WrapLabel(this, m_text, w);
    
    InvalidateBestSize();
    Fit();
    SetMinSize(GetBestSize());
}



LearnMoreLink::LearnMoreLink(wxWindow *parent, const wxString& url, wxString label)
{
    if (label.empty())
    {
#ifdef __WXMSW__
        label = _("Learn more");
#else
        label = _("Learn More");
#endif
    }

    wxHyperlinkCtrl::Create(parent, wxID_ANY, label, url);
    SetNormalColour("#2F79BE");
    SetVisitedColour("#2F79BE");
    SetHoverColour("#3D8DD5");

#ifdef __WXOSX__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
    SetFont(GetFont().Underlined());
#endif
}


wxObject *LearnMoreLinkXmlHandler::DoCreateResource()
{
    auto w = new LearnMoreLink(m_parentAsWindow, GetText("url"), GetText("label"));
    SetupWindow(w);
    return w;
}

bool LearnMoreLinkXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, "LearnMoreLink");
}
