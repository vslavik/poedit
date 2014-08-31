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

#ifndef Poedit_customcontrols_h
#define Poedit_customcontrols_h

#include <wx/stattext.h>
#include <wx/hyperlink.h>
#include <wx/xrc/xmlres.h>


// Longer, often multiline, explanation label used to provide more information
// about the effects of some less obvious settings. Typeset using smaller font
// on OS X and grey appearence. Auto-wraps itself to fit surrounding control's
// width.
class ExplanationLabel : public wxStaticText
{
public:
    ExplanationLabel(wxWindow *parent, const wxString& label);

private:
    void OnSize(wxSizeEvent& e);

    wxString m_text;
    int m_wrapWidth;
};


// "Learn more" hyperlink for dialogs.
class LearnMoreLink : public wxHyperlinkCtrl
{
public:
    LearnMoreLink(wxWindow *parent, const wxString& url, wxString label = wxString());

#ifdef __WXOSX__
    static const int EXTRA_INDENT = 2;
#else
    static const int EXTRA_INDENT = 0;
#endif
};


class LearnMoreLinkXmlHandler : public wxXmlResourceHandler
{
public:
    LearnMoreLinkXmlHandler() {}
    wxObject *DoCreateResource() override;
    bool CanHandle(wxXmlNode *node) override;
};

#endif // Poedit_customcontrols_h
