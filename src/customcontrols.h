/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2017 Vaclav Slavik
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

#include "concurrency.h"
#include "language.h"

#include <wx/stattext.h>
#include <wx/hyperlink.h>
#include <wx/xrc/xmlres.h>

class WXDLLIMPEXP_ADV wxActivityIndicator;

#include <exception>
#include <functional>


// Label marking a subsection of a dialog:
class HeadingLabel : public wxStaticText
{
public:
    HeadingLabel(wxWindow *parent, const wxString& label);
};

// Label that auto-wraps itself to fit surrounding control's width.
class AutoWrappingText : public wxStaticText
{
public:
    AutoWrappingText(wxWindow *parent, const wxString& label);

    void SetLanguage(Language lang);
    void SetAlignment(TextDirection dir);

    void SetAndWrapLabel(const wxString& label);

protected:
    void OnSize(wxSizeEvent& e);

    wxString m_text;
    int m_wrapWidth;
    Language m_language;
};

/// Like AutoWrappingText, but allows selecting (macOS) or at least copying (Windows)
/// the text too.
class SelectableAutoWrappingText : public AutoWrappingText
{
public:
    SelectableAutoWrappingText(wxWindow *parent, const wxString& label);
};


/** 
    Longer, often multiline, explanation label used to provide more information
    about the effects of some less obvious settings. Typeset using smaller font
    on macOS and grey appearance. Auto-wraps itself to fit surrounding control's
    width.
 */
class ExplanationLabel : public AutoWrappingText
{
public:
    ExplanationLabel(wxWindow *parent, const wxString& label);

#if defined(__WXOSX__)
    static const int CHECKBOX_INDENT = 19;
#elif defined(__WXMSW__)
    static const int CHECKBOX_INDENT = 17;
#elif defined(__WXGTK__)
    static const int CHECKBOX_INDENT = 25;
#endif

    static wxColour GetTextColor();
};

/// Like ExplanationLabel, but nonwrapping
class SecondaryLabel : public wxStaticText
{
public:
    SecondaryLabel(wxWindow *parent, const wxString& label);

    static wxColour GetTextColor() { return ExplanationLabel::GetTextColor(); }
};


/// "Learn more" hyperlink for dialogs.
class LearnMoreLink : public wxHyperlinkCtrl
{
public:
    LearnMoreLink(wxWindow *parent, const wxString& url, wxString label = wxString(), wxWindowID winid = wxID_ANY);

#if defined(__WXOSX__)
    static const int EXTRA_INDENT = 2;
#elif defined(__WXMSW__) || defined(__WXGTK__)
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


/// Indicator of background activity, using spinners where appropriate.
class ActivityIndicator : public wxWindow
{
public:
    enum Flags
    {
        Centered = 1,
    };

    ActivityIndicator(wxWindow *parent, int flags = 0);

    /// Start indicating, with optional progress label.
    void Start(const wxString& msg = "");

    /// Stop the indicator.
    void Stop();

    /// Stop the indicator and report error in its place.
    void StopWithError(const wxString& msg);

    /// Is between Start() and Stop() calls?
    bool IsRunning() const { return m_running; }

    /// Convenience function for showing error message in the indicator
    std::function<void(dispatch::exception_ptr)> HandleError;

    bool HasTransparentBackground() override { return true; }

private:
    void UpdateLayoutAfterTextChange();

    bool m_running;
    wxActivityIndicator *m_spinner;
    wxStaticText *m_label, *m_error;
};

#endif // Poedit_customcontrols_h
