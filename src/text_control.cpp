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

#include "text_control.h"

#include <wx/clipbrd.h>
#include <wx/wupdlock.h>

#ifdef __WXOSX__
  #import <AppKit/NSTextView.h>
  #import <Foundation/NSUndoManager.h>
#endif

#ifdef __WXMSW__
  #include <windows.h>
  #include <richedit.h>
  #ifndef BOE_UNICODEBIDI
    #define BOE_UNICODEBIDI 0x0080
  #endif
  #ifndef BOM_UNICODEBIDI
    #define BOM_UNICODEBIDI 0x0080
  #endif

  #include <comdef.h>
  #include <tom.h>
  _COM_SMARTPTR_TYPEDEF(ITextDocument, __uuidof(ITextDocument));
#endif

#include "colorscheme.h"
#include "spellchecking.h"
#include "str_helpers.h"
#include "unicode_helpers.h"


namespace
{

#ifdef __WXOSX__

inline NSTextView *TextView(const wxTextCtrl *ctrl)
{
    NSScrollView *scroll = (NSScrollView*)ctrl->GetHandle();
    return [scroll documentView];
}

class DisableAutomaticSubstitutions
{
public:
    DisableAutomaticSubstitutions(wxTextCtrl *ctrl) : m_view(TextView(ctrl))
    {
        m_dash = m_view.automaticDashSubstitutionEnabled;
        m_quote = m_view.automaticQuoteSubstitutionEnabled;
        m_text = m_view.automaticTextReplacementEnabled;
        m_spelling = m_view.automaticSpellingCorrectionEnabled;

        m_view.automaticDashSubstitutionEnabled = NO;
        m_view.automaticQuoteSubstitutionEnabled = NO;
        m_view.automaticTextReplacementEnabled = NO;
        m_view.automaticSpellingCorrectionEnabled = NO;
    }

    ~DisableAutomaticSubstitutions()
    {
        m_view.automaticDashSubstitutionEnabled = m_dash;
        m_view.automaticQuoteSubstitutionEnabled = m_quote;
        m_view.automaticTextReplacementEnabled = m_text;
        m_view.automaticSpellingCorrectionEnabled = m_spelling;
    }

private:
    NSTextView *m_view;
    BOOL m_quote, m_dash, m_text, m_spelling;
};

#endif // __WXOSX__


#ifdef __WXMSW__

inline ITextDocumentPtr TextDocument(wxTextCtrl *ctrl)
{
    IUnknown *ole_raw;
    ::SendMessage((HWND) ctrl->GetHWND(), EM_GETOLEINTERFACE, 0, (LPARAM) &ole_raw);
    IUnknownPtr ole(ole_raw, /*addRef=*/false);
    ITextDocumentPtr doc;
    if (ole)
        ole->QueryInterface<ITextDocument>(&doc);
    return doc;
}

// Temporarily suppresses recording of changes for Undo/Redo functionality
// See http://stackoverflow.com/questions/4138981/temporaily-disabling-the-c-sharp-rich-edit-undo-buffer-while-performing-syntax-h
// and http://forums.codeguru.com/showthread.php?325068-Realizing-Undo-Redo-functionality-for-RichEdit-Syntax-Highlighter
class UndoSuppressor
{
public:
    UndoSuppressor(CustomizedTextCtrl *ctrl) : m_doc(TextDocument(ctrl))
    {
        if (m_doc)
            m_doc->Undo(tomSuspend, NULL);
    }

    ~UndoSuppressor()
    {
        if (m_doc)
            m_doc->Undo(tomResume, NULL);
    }

private:
    ITextDocumentPtr m_doc;
};
#endif


#if defined(__WXOSX__)

// Group undo operations into a single group
class UndoGroup
{
public:
    UndoGroup(TranslationTextCtrl *ctrl)
    {
        m_undo = [TextView(ctrl) undoManager];
        [m_undo beginUndoGrouping];
    }

    ~UndoGroup()
    {
        [m_undo endUndoGrouping];
    }

private:
    NSUndoManager *m_undo;
};

#elif defined(__WXMSW__)

class UndoGroup
{
public:
    UndoGroup(TranslationTextCtrl *ctrl) : m_doc(TextDocument(ctrl))
    {
        if (m_doc)
            m_doc->BeginEditCollection();
    }

    ~UndoGroup()
    {
        if (m_doc)
            m_doc->EndEditCollection();
    }

private:
    ITextDocumentPtr m_doc;
};

#elif defined(__WXGTK__)

class UndoGroup
{
public:
    UndoGroup(TranslationTextCtrl *ctrl) : m_ctrl(ctrl)
    {
        m_ctrl->BeginUndoGrouping();
    }

    ~UndoGroup()
    {
        m_ctrl->EndUndoGrouping();
    }

private:
    TranslationTextCtrl *m_ctrl;
};

#endif


} // anonymous namespace


#ifdef __WXOSX__

// wxTextCtrl implementation on macOS uses insertText:, which is intended for
// user input and performs some user input processing, such as autocorrections.
// We need to avoid this, because Poedit's text control is filled with data
// when moving in the list control: https://github.com/vslavik/poedit/issues/81
// Solve this by using a customized control with overridden DoSetValue().

CustomizedTextCtrl::CustomizedTextCtrl(wxWindow *parent, wxWindowID winid, long style)
    : wxTextCtrl(parent, winid, "", wxDefaultPosition, wxDefaultSize, style | ALWAYS_USED_STYLE)
{
    auto text = TextView(this);

    [text setTextContainerInset:NSMakeSize(0,3)];
    [text setRichText:NO];

    Bind(wxEVT_TEXT_COPY, &CustomizedTextCtrl::OnCopy, this);
    Bind(wxEVT_TEXT_CUT, &CustomizedTextCtrl::OnCut, this);
    Bind(wxEVT_TEXT_PASTE, &CustomizedTextCtrl::OnPaste, this);
}

void CustomizedTextCtrl::DoSetValue(const wxString& value, int flags)
{
    wxEventBlocker block(this, (flags & SetValue_SendEvent) ? 0 : wxEVT_ANY);

    [TextView(this) setString:str::to_NS(value)];

    SendTextUpdatedEventIfAllowed();
}

wxString CustomizedTextCtrl::DoGetValue() const
{
    // wx's implementation is not sufficient and neither is [NSTextView string]
    // (which wx uses): they ignore formatting, which would be desirable, but
    // they also include embedded Unicode marks such as U+202A (Left-to-Right Embedding)
    // or U+202C (Pop Directional Format) that are essential for correct
    // handling of BiDi text.
    //
    // Instead, export the internal storage into plain-text, UTF-8 data and
    // load that into wxString. That shouldn't be too inefficient (wx does
    // UTF-8 roundtrip anyway) and preserves the marks; it is what TextEdit.app
    // does when saving text files.
    auto ctrl = TextView(this);
    NSTextStorage *text = [ctrl textStorage];
    NSDictionary *attrs = @{
                             NSDocumentTypeDocumentAttribute: NSPlainTextDocumentType,
                             NSCharacterEncodingDocumentAttribute: @(NSUTF8StringEncoding)
                           };
    NSData *data = [text dataFromRange:NSMakeRange(0, [text length]) documentAttributes:attrs error:nil];
    if (data && [data length] > 0)
        return wxString::FromUTF8((const char*)[data bytes], [data length]);
    else
        return wxTextCtrl::DoGetValue();
}

#else // !__WXOSX__

CustomizedTextCtrl::CustomizedTextCtrl(wxWindow *parent, wxWindowID winid, long style)
{
    wxTextCtrl::Create(parent, winid, "", wxDefaultPosition, wxDefaultSize, style | ALWAYS_USED_STYLE);

    wxTextAttr padding;
    padding.SetLeftIndent(5);
    padding.SetRightIndent(5);
    SetDefaultStyle(padding);

    Bind(wxEVT_TEXT_COPY, &CustomizedTextCtrl::OnCopy, this);
    Bind(wxEVT_TEXT_CUT, &CustomizedTextCtrl::OnCut, this);
    Bind(wxEVT_TEXT_PASTE, &CustomizedTextCtrl::OnPaste, this);

#ifdef __WXGTK__
    m_historyLocks = 0;
    if (!(style & wxTE_READONLY))
        Bind(wxEVT_TEXT, &CustomizedTextCtrl::OnText, this);
#endif
}

#endif // !__WXOSX__


#ifdef __WXMSW__

WXDWORD CustomizedTextCtrl::MSWGetStyle(long style, WXDWORD *exstyle) const
{
    auto msStyle = wxTextCtrl::MSWGetStyle(style, exstyle);
    // Disable always-shown scrollbars. The reason wx does this doesn't seem to
    // affect Poedit, so it should be safe:
    msStyle &= ~ES_DISABLENOSCROLL;
    return msStyle;
}

#endif // __WXMSW__


// We use wxTE_RICH2 style, which allows for pasting rich-formatted
// text into the control. We want to allow only plain text (all the
// formatting done is Poedit's syntax highlighting), so we need to
// override copy/cut/paste commands. Plus, the richedit control
// (or wx's use of it) has a bug in it that causes it to copy wrong
// data when copying from the same text control to itself after its
// content was programatically changed:
// https://sourceforge.net/tracker/index.php?func=detail&aid=1910234&group_id=27043&atid=389153

// Note that GTK has a very similar problem with pasting rich text,
// which is why this code is enabled for GTK too.

bool CustomizedTextCtrl::DoCopy()
{
    long from, to;
    GetSelection(&from, &to);
    if ( from == to )
        return false;

    wxClipboardLocker lock;
    wxCHECK_MSG( !!lock, false, "failed to lock clipboard" );

    auto text = DoCopyText(from, to);
    wxClipboard::Get()->SetData(new wxTextDataObject(text));
    return true;
}

void CustomizedTextCtrl::OnCopy(wxClipboardTextEvent&)
{
    if (!CanCopy())
        return;

    DoCopy();
}

void CustomizedTextCtrl::OnCut(wxClipboardTextEvent&)
{
    if (!CanCut())
        return;

    if (!DoCopy())
        return;

    long from, to;
    GetSelection(&from, &to);
    Remove(from, to);
}

void CustomizedTextCtrl::OnPaste(wxClipboardTextEvent&)
{
    if (!CanPaste())
        return;

    wxClipboardLocker lock;
    wxCHECK_RET( !!lock, "failed to lock clipboard" );

    wxTextDataObject d;
    wxClipboard::Get()->GetData(d);

    long from, to;
    GetSelection(&from, &to);
    DoPasteText(from, to, d.GetText());
}

wxString CustomizedTextCtrl::DoCopyText(long from, long to)
{
    return GetRange(from, to);
}

void CustomizedTextCtrl::DoPasteText(long from, long to, const wxString& s)
{
    Replace(from, to, s);
}

#ifdef __WXGTK__
void CustomizedTextCtrl::BeginUndoGrouping()
{
    m_historyLocks++;
}

void CustomizedTextCtrl::EndUndoGrouping()
{
    if (--m_historyLocks == 0)
        SaveSnapshot();
}

void CustomizedTextCtrl::SaveSnapshot()
{
    // if we saved the snapshot in DoSetValue, OnText might still call this function again
    // therefore, we make sure to filter out duplicate entries
    if (m_historyIndex && m_history[m_historyIndex - 1].text == GetValue())
        return;

    m_history.resize(m_historyIndex); // truncate the list
    m_history.push_back({GetValue(), GetInsertionPoint()});
    m_historyIndex++;
}

void CustomizedTextCtrl::DoSetValue(const wxString& value, int flags)
{
    // SetValue_SendEvent is set if this function was called from SetValue
    // SetValue_SendEvent is NOT set if this function was called from ChangeValue
    if (flags & SetValue_SendEvent)
    {
        // clear the history
        // m_history itself will be cleared when SaveSnapshot is called
        m_historyIndex = 0;

        // set the new value
        wxTextCtrl::DoSetValue(value, flags);

        // make sure to save a snapshot even if EVT_TEXT is blocked
        SaveSnapshot();
    }
    else
    {
        // just set the new value, don't save a snapshot
        // this is what happens when you click Undo or Redo
        wxTextCtrl::DoSetValue(value, flags);
    }
}

void CustomizedTextCtrl::OnText(wxCommandEvent& event)
{
    SaveSnapshot();
    event.Skip();
}

bool CustomizedTextCtrl::CanUndo() const
{
    return (m_historyIndex > 1);
}

bool CustomizedTextCtrl::CanRedo() const
{
    return (m_historyIndex < m_history.size());
}

void CustomizedTextCtrl::Undo()
{
    // ChangeValue calls AnyTranslatableTextCtrl::DoSetValue, which calls CustomizedTextCtrl::DoSetValue
    ChangeValue(m_history[m_historyIndex - 2].text);
    SetInsertionPoint(m_history[m_historyIndex - 2].insertionPoint);
    m_historyIndex--;
}

void CustomizedTextCtrl::Redo()
{
    // ChangeValue calls AnyTranslatableTextCtrl::DoSetValue, which calls CustomizedTextCtrl::DoSetValue
    ChangeValue(m_history[m_historyIndex].text);
    SetInsertionPoint(m_history[m_historyIndex].insertionPoint);
    m_historyIndex++;
}
#endif // __WXGTK__

void CustomizedTextCtrl::ShowFindIndicator(int from, int length)
{
    ShowPosition(from);
#ifdef __WXOSX__
    [TextView(this) showFindIndicatorForRange:NSMakeRange(from, length)];
#else
    SetSelection(from, from + length);
#endif
}



class AnyTranslatableTextCtrl::Attributes
{
public:
#ifdef __WXOSX__
    NSDictionary *m_attrSpace, *m_attrEscape, *m_attrMarkup, *m_attrFormat;
    typedef NSDictionary* AttrType;

    Attributes()
    {
        m_attrSpace  = @{NSBackgroundColorAttributeName: ColorScheme::Get(Color::SyntaxLeadingWhitespaceBg).OSXGetNSColor()};
        m_attrEscape = @{NSBackgroundColorAttributeName: ColorScheme::Get(Color::SyntaxEscapeBg).OSXGetNSColor(),
                         NSForegroundColorAttributeName: ColorScheme::Get(Color::SyntaxEscapeFg).OSXGetNSColor()};
        m_attrMarkup = @{NSForegroundColorAttributeName: ColorScheme::Get(Color::SyntaxMarkup).OSXGetNSColor()};
        m_attrFormat = @{NSForegroundColorAttributeName: ColorScheme::Get(Color::SyntaxFormat).OSXGetNSColor()};
    }
#else // !__WXOSX__
    wxTextAttr m_attrDefault, m_attrSpace, m_attrEscape, m_attrMarkup, m_attrFormat;
    typedef wxTextAttr AttrType;

    Attributes()
    {
        m_attrDefault.SetBackgroundColour(*wxWHITE);
        m_attrDefault.SetTextColour(*wxBLACK);

        m_attrSpace.SetBackgroundColour(ColorScheme::Get(Color::SyntaxLeadingWhitespaceBg));

        m_attrEscape.SetBackgroundColour(ColorScheme::Get(Color::SyntaxEscapeBg));
        m_attrEscape.SetTextColour(ColorScheme::Get(Color::SyntaxEscapeFg));

        m_attrMarkup.SetTextColour(ColorScheme::Get(Color::SyntaxMarkup));

        m_attrFormat.SetTextColour(ColorScheme::Get(Color::SyntaxFormat));
    }

    const AttrType& Default() const {  return m_attrDefault; }
#endif

    const AttrType& For(SyntaxHighlighter::TextKind kind) const
    {
        switch (kind)
        {
            case SyntaxHighlighter::LeadingWhitespace:  return m_attrSpace;
            case SyntaxHighlighter::Escape:             return m_attrEscape;
            case SyntaxHighlighter::Markup:             return m_attrMarkup;
            case SyntaxHighlighter::Format:             return m_attrFormat;
        }
        return m_attrSpace; // silence bogus warning
    }
};


AnyTranslatableTextCtrl::AnyTranslatableTextCtrl(wxWindow *parent, wxWindowID winid, int style)
   : CustomizedTextCtrl(parent, winid, style),
     m_attrs(new Attributes)
{
    Bind(wxEVT_TEXT, [=](wxCommandEvent& e){
        e.Skip();
        HighlightText();
    });

    m_language = Language::English();
}

AnyTranslatableTextCtrl::~AnyTranslatableTextCtrl()
{
}

void AnyTranslatableTextCtrl::SetLanguage(const Language& lang)
{
    m_language = lang;

    wxEventBlocker block(this, wxEVT_TEXT);

#ifdef __WXOSX__
    NSTextView *text = TextView(this);
    if (lang.IsRTL())
    {
        [text setBaseWritingDirection:NSWritingDirectionRightToLeft];
        if ([NSApp userInterfaceLayoutDirection] == NSUserInterfaceLayoutDirectionLeftToRight)
        {
            // extra nudge to make typing behave as expected:
            [text makeTextWritingDirectionRightToLeft:nil];
        }
    }
    else
    {
        [text setBaseWritingDirection:NSWritingDirectionLeftToRight];
    }
#endif

#ifdef __WXMSW__
    BIDIOPTIONS bidi;
    ::ZeroMemory(&bidi, sizeof(bidi));
    bidi.cbSize = sizeof(bidi);
    bidi.wMask = BOM_UNICODEBIDI;
    bidi.wEffects = lang.IsRTL() ? BOE_UNICODEBIDI : 0;
    ::SendMessage((HWND)GetHWND(), EM_SETBIDIOPTIONS, 0, (LPARAM) &bidi);

    ::SendMessage((HWND)GetHWND(), EM_SETEDITSTYLE, lang.IsRTL() ? SES_BIDI : 0, SES_BIDI);

    UpdateRTLStyle();
#endif
}


void AnyTranslatableTextCtrl::SetPlainText(const wxString& s)
{
    SetValue(EscapePlainText(s));
}

wxString AnyTranslatableTextCtrl::GetPlainText() const
{
    return UnescapePlainText(bidi::strip_pointless_control_chars(GetValue(), m_language.Direction()));
}


wxString AnyTranslatableTextCtrl::EscapePlainText(const wxString& s)
{
    // Note: the escapes used here should match with
    //       BasicSyntaxHighlighter::Highlight() ones
    wxString s2;
    s2.reserve(s.length());
    for (auto i = s.begin(); i != s.end(); ++i)
    {
        wchar_t c = *i;
        switch (c)
        {
            case '\n':
                s2 += "\\n\n";
                break;
            case '\r':
                s2 += "\\r";
                break;
            case '\t':
                s2 += "\\t";
                break;
            case '\a':
                s2 += "\\a";
                break;
            case '\0':
                s2 += "\\0";
                break;
            case '\\':
            {
                s2 += c;
                auto peek = i + 1;
                if ( peek != s.end() )
                {
                    switch ((wchar_t)*peek)
                    {
                        case 'n': case '\n':
                        case 'r': case '\r':
                        case 't': case '\t':
                        case '0': case '\0':
                        case '\\':
                            s2 += c; // escape problematic backslash
                            break;
                    }
                }
                break;
            }
            default:
                s2 += c;
                break;
        }
    }
    return s2;
}

wxString AnyTranslatableTextCtrl::UnescapePlainText(const wxString& s)
{
    wxString s2;
    s2.reserve(s.length());
    for (auto i = s.begin(); i != s.end(); ++i)
    {
        wchar_t c0 = *i;
        if (c0 == '\\')
        {
            if ( ++i == s.end() )
            {
                s2 += '\\';
                return s2;
            }
            wchar_t c = *i;
            switch (c)
            {
                case 'r':
                    s2 += '\r';
                    break;
                case 't':
                    s2 += '\t';
                    break;
                case 'a':
                    s2 += '\a';
                    break;
                case '0':
                    s2 += '\0';
                    break;
                case 'n':
                {
                    s2 += '\n';
                    auto peek = i + 1;
                    if ( peek != s.end() && *peek == '\n' )
                    {
                        // "\\n\n" should be treated as single newline
                        i = peek;
                    }
                    break;
                }
                case '\\':
                    s2 += '\\';
                    break;
                default:
                    s2 += '\\';
                    s2 += c;
                    break;
            }
        }
        else
        {
            s2 += c0;
        }
    }
    return s2;
}

wxString AnyTranslatableTextCtrl::DoCopyText(long from, long to)
{
    return UnescapePlainText(GetRange(from, to));
}

void AnyTranslatableTextCtrl::DoPasteText(long from, long to, const wxString& s)
{
    Replace(from, to, EscapePlainText(s));
}

void AnyTranslatableTextCtrl::DoSetValue(const wxString& value, int flags)
{
#ifdef __WXMSW__
    wxWindowUpdateLocker dis(this);
#endif
    CustomizedTextCtrl::DoSetValue(value, flags);
#ifdef __WXMSW__
    UpdateRTLStyle();
#endif
    HighlightText();
}

#ifdef __WXMSW__
void AnyTranslatableTextCtrl::UpdateRTLStyle()
{
    wxEventBlocker block(this, wxEVT_TEXT);
    UndoSuppressor blockUndo(this);

    PARAFORMAT2 pf;
    ::ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);
    pf.dwMask |= PFM_RTLPARA;
    if (m_language.IsRTL())
        pf.wEffects |= PFE_RTLPARA;

    long start, end;
    GetSelection(&start, &end);
    SetSelection(-1, -1);
    ::SendMessage((HWND) GetHWND(), EM_SETPARAFORMAT, 0, (LPARAM) &pf);
    SetSelection(start, end);
}
#endif // !__WXMSW__

void AnyTranslatableTextCtrl::HighlightText()
{
    auto text = GetValue().ToStdWstring();

#ifdef __WXOSX__

    NSRange fullRange = NSMakeRange(0, text.length());
    NSLayoutManager *layout = [TextView(this) layoutManager];
    [layout removeTemporaryAttribute:NSForegroundColorAttributeName forCharacterRange:fullRange];
    [layout removeTemporaryAttribute:NSBackgroundColorAttributeName forCharacterRange:fullRange];

    if (m_syntax)
    {
        m_syntax->Highlight(text, [=](int a, int b, SyntaxHighlighter::TextKind kind){
            [layout addTemporaryAttributes:m_attrs->For(kind) forCharacterRange:NSMakeRange(a, b-a)];
        });
    }

#else // !__WXOSX__

#ifndef __WXGTK__
    // Freezing (and more to the point, thawing) the window from inside wxEVT_TEXT
    // handler breaks pasting under GTK+ (selection is not replaced).
    // See https://github.com/vslavik/poedit/issues/139
    wxWindowUpdateLocker noupd(this);
#endif

    wxEventBlocker block(this, wxEVT_TEXT);
  #ifdef __WXMSW__
    UndoSuppressor blockUndo(this);
  #endif

    auto deflt = m_attrs->Default();
    deflt.SetFont(GetFont());
    SetStyle(0, text.length(), deflt);

    if (m_syntax)
    {
        m_syntax->Highlight(text, [=](int a, int b, SyntaxHighlighter::TextKind kind){
            SetStyle(a, b, m_attrs->For(kind));
        });
    }
#endif // __WXOSX__/!__WXOSX__
}



SourceTextCtrl::SourceTextCtrl(wxWindow *parent, wxWindowID winid)
    : AnyTranslatableTextCtrl(parent, winid, wxTE_READONLY | wxNO_BORDER)
{
    SetLanguage(Language::English());
}


TranslationTextCtrl::TranslationTextCtrl(wxWindow *parent, wxWindowID winid)
    : AnyTranslatableTextCtrl(parent, winid, wxNO_BORDER),
      m_lastKeyWasReturn(false)
{
#ifdef __WXMSW__
    PrepareTextCtrlForSpellchecker(this);
#endif

#ifdef __WXOSX__
    [TextView(this) setAllowsUndo:YES];
#endif

    Bind(wxEVT_KEY_DOWN, &TranslationTextCtrl::OnKeyDown, this);
    Bind(wxEVT_TEXT, &TranslationTextCtrl::OnText, this);
}

void TranslationTextCtrl::OnKeyDown(wxKeyEvent& e)
{
    m_lastKeyWasReturn = (e.GetUnicodeKey() == WXK_RETURN);
    e.Skip();
}

void TranslationTextCtrl::OnText(wxCommandEvent& e)
{
    if (m_lastKeyWasReturn)
    {
        // Insert \n markup in front of newlines:
        m_lastKeyWasReturn = false;
        long pos = GetInsertionPoint();
        auto range = GetRange(std::max(0l, pos - 3), pos);
        if (range.empty() || (range.Last() == '\n' && range != "\\n\n"))
        {
          #ifdef __WXGTK__
            // GTK+ doesn't like modifying the content in the "changed" signal:
            CallAfter([=]{
                Replace(pos - 1, pos, "\\n\n");
                HighlightText();
            });
          #else
            Replace(pos - 1, pos, "\\n\n");
          #endif
        }
    }

    e.Skip();
}

#ifdef __WXOSX__
void TranslationTextCtrl::DoSetValue(const wxString& value, int flags)
{
    AnyTranslatableTextCtrl::DoSetValue(value, flags);

    NSUndoManager *undo = [TextView(this) undoManager];
    [undo removeAllActions];
}
#endif

void TranslationTextCtrl::SetPlainTextUserWritten(const wxString& value)
{
    UndoGroup undo(this);

#ifdef __WXOSX__
    DisableAutomaticSubstitutions disableAuto(this);
#endif

    SelectAll();
    WriteText(EscapePlainText(value));
    SetInsertionPointEnd();

    HighlightText();
}
