/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2017 Vaclav Slavik
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

#ifndef Poedit_text_control_h
#define Poedit_text_control_h

#include <wx/textctrl.h>
#include <memory>
#include <vector>

#include "language.h"
#include "syntaxhighlighter.h"


/**
    Text control with some Poedit-specific customizations:
    
    - Allow setting text programatically, without user-input processing (macOS)
    - Disable user-usable rich text support
    - Stylistic tweaks (padding and such)
    - Generic undo/redo implementation for GTK
    - Search highlighting
 */
class CustomizedTextCtrl : public wxTextCtrl
{
public:
    static const int ALWAYS_USED_STYLE = wxTE_MULTILINE | wxTE_RICH2 | wxTE_NOHIDESEL;

    CustomizedTextCtrl(wxWindow *parent, wxWindowID winid, long style = 0);

    /// Show find result indicator at given part of the text
    void ShowFindIndicator(int from, int length);

#ifdef __WXGTK__
    // Undo/redo implementation:
    void BeginUndoGrouping();
    void EndUndoGrouping();
    void SaveSnapshot();
#endif

#ifdef __WXMSW__
    WXDWORD MSWGetStyle(long style, WXDWORD *exstyle) const override;
#endif

protected:
#ifdef __WXOSX__
    void DoSetValue(const wxString& value, int flags) override;
    wxString DoGetValue() const override;
#endif

    bool DoCopy();
    void OnCopy(wxClipboardTextEvent& event);
    void OnCut(wxClipboardTextEvent& event);
    void OnPaste(wxClipboardTextEvent& event);

    virtual wxString DoCopyText(long from, long to);
    virtual void DoPasteText(long from, long to, const wxString& s);

#ifdef __WXGTK__
    struct Snapshot
    {
        wxString text;
        long insertionPoint;
    };

    void DoSetValue(const wxString& value, int flags) override;
    void OnText(wxCommandEvent& event);
    bool CanUndo() const override;
    bool CanRedo() const override;
    void Undo() override;
    void Redo() override;

    std::vector<Snapshot> m_history;
    size_t m_historyIndex; // where in the vector to insert the next snapshot
    int m_historyLocks;
#endif // __WXGTK__
};


class AnyTranslatableTextCtrl : public CustomizedTextCtrl
{
public:
    AnyTranslatableTextCtrl(wxWindow *parent, wxWindowID winid, int style = 0);
    ~AnyTranslatableTextCtrl();

    void SetLanguage(const Language& lang);
    void SetSyntaxHighlighter(SyntaxHighlighterPtr syntax) { m_syntax = syntax; }

    // Set and get control's text as plain/raw text, with no escaping or formatting.
    // This is the "true" representation, with e.g newlines included. The version
    // displayed to the user includes syntax highlighting and escaping of some characters
    // (e.g. tabs shown as \t, newlines as \n followed by newline).
    void SetPlainText(const wxString& s);
    wxString GetPlainText() const;

    // Apply escaping as described in SetPlainText:
    static wxString EscapePlainText(const wxString& s);
    static wxString UnescapePlainText(const wxString& s);

protected:
    wxString DoCopyText(long from, long to) override;
    void DoPasteText(long from, long to, const wxString& s) override;

    void DoSetValue(const wxString& value, int flags) override;

#ifdef __WXMSW__
    void UpdateRTLStyle();
#endif // __WXMSW__

protected:
    void HighlightText();

    class Attributes;
    SyntaxHighlighterPtr m_syntax;
    std::unique_ptr<Attributes> m_attrs;
    Language m_language;
};


class SourceTextCtrl : public AnyTranslatableTextCtrl
{
public:
    SourceTextCtrl(wxWindow *parent, wxWindowID winid);

    bool AcceptsFocus() const override { return false; }
};


class TranslationTextCtrl : public AnyTranslatableTextCtrl
{
public:
    TranslationTextCtrl(wxWindow *parent, wxWindowID winid);

    /// Sets the value to something the user wrote
    void SetPlainTextUserWritten(const wxString& value);

protected:
    void OnKeyDown(wxKeyEvent& e);
    void OnText(wxCommandEvent& e);

#ifdef __WXOSX__
    void DoSetValue(const wxString& value, int flags) override;
#endif

    bool m_lastKeyWasReturn;
};

#endif // Poedit_text_control_h
