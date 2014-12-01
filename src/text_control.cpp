/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 1999-2014 Vaclav Slavik
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
  #include <wx/cocoa/string.h>
  #import <AppKit/NSTextView.h>
  #import <Foundation/NSUndoManager.h>
#endif

#ifdef __WXMSW__
  #include <richedit.h>
  #ifndef BOE_UNICODEBIDI
    #define BOE_UNICODEBIDI 0x0080
  #endif
  #ifndef BOM_UNICODEBIDI
    #define BOM_UNICODEBIDI 0x0080
  #endif
#endif

#include "spellchecking.h"


namespace
{

#ifdef __WXOSX__

inline NSTextView *TextView(const wxTextCtrl *ctrl)
{
    NSScrollView *scroll = (NSScrollView*)ctrl->GetHandle();
    return [scroll documentView];
}

#endif // __WXOSX__

} // anonymous namespace


#ifdef __WXOSX__

// wxTextCtrl implementation on OS X uses insertText:, which is intended for
// user input and performs some user input processing, such as autocorrections.
// We need to avoid this, because Poedit's text control is filled with data
// when moving in the list control: https://github.com/vslavik/poedit/issues/81
// Solve this by using a customized control with overridden DoSetValue().

CustomizedTextCtrl::CustomizedTextCtrl(wxWindow *parent, wxWindowID winid, long style)
    : wxTextCtrl(parent, winid, "", wxDefaultPosition, wxDefaultSize, style | ALWAYS_USED_STYLE)
{
    auto text = TextView(this);

    [text setTextContainerInset:NSMakeSize(1,3)];
    [text setRichText:NO];

    [text setAutomaticQuoteSubstitutionEnabled:NO];
    [text setAutomaticDashSubstitutionEnabled:NO];

    [text setAllowsUndo:YES];
}

void CustomizedTextCtrl::DoSetValue(const wxString& value, int flags)
{
    wxEventBlocker block(this, (flags & SetValue_SendEvent) ? 0 : wxEVT_ANY);

    auto text = TextView(this);
    [text setString:wxNSStringWithWxString(value)];
    NSUndoManager *undo = [text undoManager];
    [undo removeAllActions];

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
   : wxTextCtrl(parent, winid, "", wxDefaultPosition, wxDefaultSize, style | ALWAYS_USED_STYLE)
{
    wxTextAttr padding;
    padding.SetLeftIndent(5);
    padding.SetRightIndent(5);
    SetDefaultStyle(padding);

#ifdef __WXMSW__
    Bind(wxEVT_TEXT_COPY, &CustomizedTextCtrl::OnCopy, this);
    Bind(wxEVT_TEXT_CUT, &CustomizedTextCtrl::OnCut, this);
    Bind(wxEVT_TEXT_PASTE, &CustomizedTextCtrl::OnPaste, this);
#endif
}

#endif // !__WXOSX__

#ifdef __WXMSW__
// We use wxTE_RICH2 style, which allows for pasting rich-formatted
// text into the control. We want to allow only plain text (all the
// formatting done is Poedit's syntax highlighting), so we need to
// override copy/cut/paste command.s Plus, the richedit control
// (or wx's use of it) has a bug in it that causes it to copy wrong
// data when copying from the same text control to itself after its
// content was programatically changed:
// https://sourceforge.net/tracker/index.php?func=detail&aid=1910234&group_id=27043&atid=389153

bool CustomizedTextCtrl::DoCopy()
{
    long from, to;
    GetSelection(&from, &to);
    if ( from == to )
        return false;

    const wxString sel = GetRange(from, to);

    wxClipboardLocker lock;
    wxCHECK_MSG( !!lock, false, "failed to lock clipboard" );

    wxClipboard::Get()->SetData(new wxTextDataObject(sel));
    return true;
}

void CustomizedTextCtrl::OnCopy(wxClipboardTextEvent& event)
{
    DoCopy();
}

void CustomizedTextCtrl::OnCut(wxClipboardTextEvent& event)
{
    if ( !DoCopy() )
        return;

    long from, to;
    GetSelection(&from, &to);
    Remove(from, to);
}

void CustomizedTextCtrl::OnPaste(wxClipboardTextEvent& event)
{
    wxClipboardLocker lock;
    wxCHECK_RET( !!lock, "failed to lock clipboard" );

    wxTextDataObject d;
    wxClipboard::Get()->GetData(d);

    long from, to;
    GetSelection(&from, &to);
    Replace(from, to, d.GetText());
}
#endif // __WXMSW__



class AnyTranslatableTextCtrl::Attributes
{
public:
#ifdef __WXOSX__
    NSDictionary *m_attrSpace, *m_attrEscape;
    typedef NSDictionary* AttrType;

    Attributes()
    {
        m_attrSpace  = @{NSBackgroundColorAttributeName: [NSColor colorWithSRGBRed:0.89 green:0.96 blue:0.68 alpha:1]};
        m_attrEscape = @{NSBackgroundColorAttributeName: [NSColor colorWithSRGBRed:1 green:0.95 blue:1 alpha:1],
                         NSForegroundColorAttributeName: [NSColor colorWithSRGBRed:0.46 green:0 blue:0.01 alpha:1]};
    }
#else // !__WXOSX__
    wxTextAttr m_attrDefault, m_attrSpace, m_attrEscape;
    typedef wxTextAttr AttrType;

    Attributes()
    {
        m_attrDefault.SetBackgroundColour(*wxWHITE);
        m_attrDefault.SetTextColour(*wxBLACK);

        m_attrSpace.SetBackgroundColour("#E4F6AE");

        m_attrEscape.SetBackgroundColour("#FFF1FF");
        m_attrEscape.SetTextColour("#760003");
    }

    const AttrType& Default() const {  return m_attrDefault; }
#endif

    const AttrType& For(SyntaxHighlighter::TextKind kind) const
    {
        switch (kind)
        {
            case SyntaxHighlighter::LeadingWhitespace:  return m_attrSpace;
            case SyntaxHighlighter::Escape:             return m_attrEscape;
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

#ifdef __WXMSW__
    m_isRTL = false;
#endif
}

AnyTranslatableTextCtrl::~AnyTranslatableTextCtrl()
{
}

void AnyTranslatableTextCtrl::SetLanguageRTL(bool isRTL)
{
#ifdef __WXOSX__
    NSTextView *text = TextView(this);
    [text setBaseWritingDirection:isRTL ? NSWritingDirectionRightToLeft : NSWritingDirectionLeftToRight];
#endif
#ifdef __WXMSW__
    m_isRTL = isRTL;

    BIDIOPTIONS bidi;
    ::ZeroMemory(&bidi, sizeof(bidi));
    bidi.cbSize = sizeof(bidi);
    bidi.wMask = BOM_UNICODEBIDI;
    bidi.wEffects = isRTL ? BOE_UNICODEBIDI : 0;
    ::SendMessage((HWND)GetHWND(), EM_SETBIDIOPTIONS, 0, (LPARAM) &bidi);

    ::SendMessage((HWND)GetHWND(), EM_SETEDITSTYLE, isRTL ? SES_BIDI : 0, SES_BIDI);

    UpdateRTLStyle();
#endif
}

#ifdef __WXMSW__
void AnyTranslatableTextCtrl::DoSetValue(const wxString& value, int flags)
{
    wxWindowUpdateLocker dis(this);
    CustomizedTextCtrl::DoSetValue(value, flags);
    UpdateRTLStyle();
}

void AnyTranslatableTextCtrl::UpdateRTLStyle()
{
    wxEventBlocker block(this, wxEVT_TEXT);

    PARAFORMAT2 pf;
    ::ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);
    pf.dwMask |= PFM_RTLPARA;
    if (m_isRTL)
        pf.wEffects |= PFE_RTLPARA;

    long start, end;
    GetSelection(&start, &end);
    SetSelection(-1, -1);
    ::SendMessage((HWND) GetHWND(), EM_SETPARAFORMAT, 0, (LPARAM) &pf);
    SetSelection(start, end);
}
#endif // __WXMSW__

#ifdef __WXGTK__
void AnyTranslatableTextCtrl::DoSetValue(const wxString& value, int flags)
{
    CustomizedTextCtrl::DoSetValue(value, flags);
    HighlightText();
}
#endif

void AnyTranslatableTextCtrl::HighlightText()
{
    auto text = GetValue().ToStdWstring();

#ifdef __WXOSX__

    NSRange fullRange = NSMakeRange(0, text.length());
    NSLayoutManager *layout = [TextView(this) layoutManager];
    [layout removeTemporaryAttribute:NSForegroundColorAttributeName forCharacterRange:fullRange];
    [layout removeTemporaryAttribute:NSBackgroundColorAttributeName forCharacterRange:fullRange];

    m_syntax.Highlight(text, [=](int a, int b, SyntaxHighlighter::TextKind kind){
        [layout addTemporaryAttributes:m_attrs->For(kind) forCharacterRange:NSMakeRange(a, b-a)];
    });

#else // !__WXOSX__

    wxWindowUpdateLocker noupd(this);
    wxEventBlocker block(this, wxEVT_TEXT);
    SetStyle(0, text.length(), m_attrs->Default());

    m_syntax.Highlight(text, [=](int a, int b, SyntaxHighlighter::TextKind kind){
        SetStyle(a, b, m_attrs->For(kind));
    });

#endif // __WXOSX__/!__WXOSX__
}



SourceTextCtrl::SourceTextCtrl(wxWindow *parent, wxWindowID winid)
    : AnyTranslatableTextCtrl(parent, winid, wxTE_READONLY)
{
    SetLanguageRTL(false); // English is LTR
}


TranslationTextCtrl::TranslationTextCtrl(wxWindow *parent, wxWindowID winid)
    : AnyTranslatableTextCtrl(parent, winid)
{
#ifdef __WXMSW__
    PrepareTextCtrlForSpellchecker(this);
#endif
}
