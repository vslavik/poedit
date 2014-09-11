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

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/fs_mem.h>
#include <wx/datetime.h>
#include <wx/tokenzr.h>
#include <wx/xrc/xmlres.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/statusbr.h>
#include <wx/stdpaths.h>
#include <wx/splitter.h>
#include <wx/fontutil.h>
#include <wx/textfile.h>
#include <wx/wupdlock.h>
#include <wx/iconbndl.h>
#include <wx/clipbrd.h>
#include <wx/dnd.h>
#include <wx/windowptr.h>

#ifdef __WXOSX__
#include <wx/cocoa/string.h>
#import <AppKit/NSDocumentController.h>
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

#include <map>
#include <algorithm>
#include <future>

#include "catalog.h"
#include "edapp.h"
#include "edframe.h"
#include "propertiesdlg.h"
#include "prefsdlg.h"
#include "fileviewer.h"
#include "findframe.h"
#include "tm/transmem.h"
#include "language.h"
#include "progressinfo.h"
#include "commentdlg.h"
#include "manager.h"
#include "pluralforms/pl_evaluate.h"
#include "attentionbar.h"
#include "errorbar.h"
#include "utility.h"
#include "languagectrl.h"
#include "welcomescreen.h"
#include "errors.h"
#include "sidebar.h"
#include "spellchecking.h"
#include "syntaxhighlighter.h"


// this should be high enough to not conflict with any wxNewId-allocated value,
PoeditFrame::PoeditFramesList PoeditFrame::ms_instances;

// but there's a check for this in the PoeditFrame ctor, too
const wxWindowID ID_POEDIT_FIRST = wxID_HIGHEST + 10000;
const unsigned   ID_POEDIT_STEP  = 1000;

const wxWindowID ID_POPUP_REFS   = ID_POEDIT_FIRST + 1*ID_POEDIT_STEP;
const wxWindowID ID_POPUP_TRANS  = ID_POEDIT_FIRST + 2*ID_POEDIT_STEP;
const wxWindowID ID_POPUP_DUMMY  = ID_POEDIT_FIRST + 3*ID_POEDIT_STEP;
const wxWindowID ID_BOOKMARK_GO  = ID_POEDIT_FIRST + 4*ID_POEDIT_STEP;
const wxWindowID ID_BOOKMARK_SET = ID_POEDIT_FIRST + 5*ID_POEDIT_STEP;

const wxWindowID ID_POEDIT_LAST  = ID_POEDIT_FIRST + 6*ID_POEDIT_STEP;

const wxWindowID ID_LIST = wxNewId();
const wxWindowID ID_TEXTORIG = wxNewId();
const wxWindowID ID_TEXTORIGPLURAL = wxNewId();
const wxWindowID ID_TEXTTRANS = wxNewId();



#ifdef __VISUALC__
// Disabling the useless and annoying MSVC++'s
// warning C4800: 'long' : forcing value to bool 'true' or 'false'
// (performance warning):
#pragma warning ( disable : 4800 )
#endif


// I don't like this global flag, but all PoeditFrame instances must share it
bool g_focusToText = false;

/*static*/ PoeditFrame *PoeditFrame::Find(const wxString& filename)
{
    wxFileName fn(filename);

    for (PoeditFramesList::const_iterator n = ms_instances.begin();
         n != ms_instances.end(); ++n)
    {
        if (wxFileName((*n)->m_fileName) == fn)
            return *n;
    }
    return NULL;
}

/*static*/ PoeditFrame *PoeditFrame::UnusedActiveWindow()
{
    for (PoeditFramesList::const_iterator n = ms_instances.begin();
         n != ms_instances.end(); ++n)
    {
        PoeditFrame *win = *n;
        if (win->IsActive() && win->m_catalog == NULL)
            return win;
    }
    return NULL;
}

/*static*/ bool PoeditFrame::AnyWindowIsModified()
{
    for (PoeditFramesList::const_iterator n = ms_instances.begin();
         n != ms_instances.end(); ++n)
    {
        if ((*n)->IsModified())
            return true;
    }
    return false;
}

/*static*/ PoeditFrame *PoeditFrame::Create(const wxString& filename)
{
    PoeditFrame *f = PoeditFrame::Find(filename);
    if (f)
    {
        f->Raise();
    }
    else
    {
        // NB: duplicated in ReadCatalog()
        Catalog *cat = new Catalog(filename);
        if (!cat->IsOk())
        {
            wxMessageDialog dlg
            (
                nullptr,
                _("The file cannot be opened."),
                _("Invalid file"),
                wxOK | wxICON_ERROR
            );
            dlg.SetExtendedMessage(
                _("The file may be either corrupted or in a format not recognized by Poedit.")
            );
            dlg.ShowModal();
            delete cat;
            return nullptr;
        }

        f = new PoeditFrame();
        f->Show(true);
        f->ReadCatalog(cat);
    }

    f->Show(true);

    if (g_focusToText && f->m_textTrans)
        ((wxTextCtrl*)f->m_textTrans)->SetFocus();
    else if (f->m_list)
        f->m_list->SetFocus();

    return f;
}

/*static*/ PoeditFrame *PoeditFrame::CreateEmpty()
{
    PoeditFrame *f = new PoeditFrame;
    f->Show(true);

    return f;
}

/*static*/ PoeditFrame *PoeditFrame::CreateWelcome()
{
    PoeditFrame *f = new PoeditFrame;
    f->EnsureContentView(Content::Welcome);
    f->Show(true);

    return f;
}


class ListHandler;
class TextctrlHandler : public wxEvtHandler
{
    public:
        TextctrlHandler(PoeditFrame*) {}

    private:
#ifdef __WXMSW__
        // We use wxTE_RICH2 style, which allows for pasting rich-formatted
        // text into the control. We want to allow only plain text (all the
        // formatting done is Poedit's syntax highlighting), so we need to
        // override copy/cut/paste command.s Plus, the richedit control
        // (or wx's use of it) has a bug in it that causes it to copy wrong
        // data when copying from the same text control to itself after its
        // content was programatically changed:
        // https://sourceforge.net/tracker/index.php?func=detail&aid=1910234&group_id=27043&atid=389153

        bool DoCopy(wxTextCtrl *textctrl)
        {
            long from, to;
            textctrl->GetSelection(&from, &to);
            if ( from == to )
                return false;

            const wxString sel = textctrl->GetRange(from, to);

            wxClipboardLocker lock;
            wxCHECK_MSG( !!lock, false, "failed to lock clipboard" );

            wxClipboard::Get()->SetData(new wxTextDataObject(sel));
            return true;
        }

        void OnCopy(wxClipboardTextEvent& event)
        {
            wxTextCtrl *textctrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
            wxCHECK_RET( textctrl, "wrong use of event handler" );

            DoCopy(textctrl);
        }

        void OnCut(wxClipboardTextEvent& event)
        {
            wxTextCtrl *textctrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
            wxCHECK_RET( textctrl, "wrong use of event handler" );

            if ( !DoCopy(textctrl) )
                return;

            long from, to;
            textctrl->GetSelection(&from, &to);
            textctrl->Remove(from, to);
        }

        void OnPaste(wxClipboardTextEvent& event)
        {
            wxTextCtrl *textctrl = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
            wxCHECK_RET( textctrl, "wrong use of event handler" );

            wxClipboardLocker lock;
            wxCHECK_RET( !!lock, "failed to lock clipboard" );

            wxTextDataObject d;
            wxClipboard::Get()->GetData(d);

            long from, to;
            textctrl->GetSelection(&from, &to);
            textctrl->Replace(from, to, d.GetText());
        }
#endif // __WXMSW__

        DECLARE_EVENT_TABLE()

        friend class ListHandler;
};

BEGIN_EVENT_TABLE(TextctrlHandler, wxEvtHandler)
#ifdef __WXMSW__
    EVT_TEXT_COPY(-1, TextctrlHandler::OnCopy)
    EVT_TEXT_CUT(-1, TextctrlHandler::OnCut)
    EVT_TEXT_PASTE(-1, TextctrlHandler::OnPaste)
#endif
END_EVENT_TABLE()


class TransTextctrlHandler : public TextctrlHandler
{
    public:
        TransTextctrlHandler(PoeditFrame* frame)
            : TextctrlHandler(frame), m_frame(frame) {}

    private:
        void OnText(wxCommandEvent& event)
        {
            m_frame->UpdateFromTextCtrl();
            event.Skip();
        }

        PoeditFrame *m_frame;

        DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(TransTextctrlHandler, TextctrlHandler)
    EVT_TEXT(-1, TransTextctrlHandler::OnText)
END_EVENT_TABLE()


// special handling of events in listctrl
class ListHandler : public wxEvtHandler
{
    public:
        ListHandler(PoeditFrame *frame) :
                 wxEvtHandler(), m_frame(frame) {}

    private:
        void OnSel(wxListEvent& event) { m_frame->OnListSel(event); }
        void OnRightClick(wxMouseEvent& event) { m_frame->OnListRightClick(event); }
        void OnFocus(wxFocusEvent& event) { m_frame->OnListFocus(event); }

        DECLARE_EVENT_TABLE()

        PoeditFrame *m_frame;
};

BEGIN_EVENT_TABLE(ListHandler, wxEvtHandler)
   EVT_LIST_ITEM_SELECTED  (ID_LIST, ListHandler::OnSel)
   EVT_RIGHT_DOWN          (          ListHandler::OnRightClick)
   EVT_SET_FOCUS           (          ListHandler::OnFocus)
END_EVENT_TABLE()

#ifdef __WXOSX__
// wxTextCtrl implementation on OS X uses insertText:, which is intended for
// user input and performs some user input processing, such as autocorrections.
// We need to avoid this, because Poedit's text control is filled with data
// when moving in the list control: https://github.com/vslavik/poedit/issues/81
// Solve this by using a customized control with overriden DoSetValue().
class CustomizedTextCtrl : public wxTextCtrl
{
public:
    CustomizedTextCtrl(wxWindow *parent,
                       wxWindowID winid,
                       const wxString &value = wxEmptyString,
                       const wxPoint &pos = wxDefaultPosition,
                       const wxSize &size = wxDefaultSize,
                       long style = 0,
                       const wxValidator& validator = wxDefaultValidator,
                       const wxString &name = wxTextCtrlNameStr)
       : wxTextCtrl(parent, winid, value, pos, size, style, validator, name)
    {
        NSTextView *text = TextView();

        [text setRichText:NO];

        [text setAutomaticQuoteSubstitutionEnabled:NO];
        [text setAutomaticDashSubstitutionEnabled:NO];

        [text setAllowsUndo:YES];
    }

protected:
    void DoSetValue(const wxString& value, int flags) override
    {
        wxEventBlocker block(this, (flags & SetValue_SendEvent) ? 0 : wxEVT_ANY);

        NSTextView *text = TextView();
        [text setString:wxNSStringWithWxString(value)];
        NSUndoManager *undo = [text undoManager];
        [undo removeAllActions];

        SendTextUpdatedEventIfAllowed();
    }

    wxString DoGetValue() const override
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
        NSTextView *ctrl = TextView();
        NSTextStorage *text = [ctrl textStorage];
        NSDictionary *attrs = @{
                                 NSDocumentTypeDocumentAttribute: NSPlainTextDocumentType,
                                 NSCharacterEncodingDocumentAttribute: @(NSUTF8StringEncoding)
                               };
        NSData *data = [text dataFromRange:NSMakeRange(0, [text length]) documentAttributes:attrs error:nil];
        if (data)
            return wxString::FromUTF8Unchecked((const char*)[data bytes], [data length]);
        else
            return wxTextCtrl::DoGetValue();
    }

    NSTextView *TextView() const
    {
        NSScrollView *scroll = (NSScrollView*)GetHandle();
        return [scroll documentView];
    }
};
#else // !__WXOSX__
typedef wxTextCtrl CustomizedTextCtrl;
#endif

class UnfocusableTextCtrl : public CustomizedTextCtrl
{
    public:
        UnfocusableTextCtrl(wxWindow *parent,
                            wxWindowID winid,
                            const wxString &value = wxEmptyString,
                            const wxPoint &pos = wxDefaultPosition,
                            const wxSize &size = wxDefaultSize,
                            long style = 0,
                            const wxValidator& validator = wxDefaultValidator,
                            const wxString &name = wxTextCtrlNameStr)
           : CustomizedTextCtrl(parent, winid, value, pos, size, style, validator, name)
        {
        }

        virtual bool AcceptsFocus() const { return false; }
};


class AnyTranslatableTextCtrl : public CustomizedTextCtrl
{
    public:
        AnyTranslatableTextCtrl(wxWindow *parent, wxWindowID winid, int style = 0)
           : CustomizedTextCtrl(parent, winid, wxEmptyString,
                                wxDefaultPosition, wxDefaultSize,
                                wxTE_MULTILINE | wxTE_RICH2 | style)
        {
            InitColors();
            Bind(wxEVT_TEXT, [=](wxCommandEvent& e){
                e.Skip();
                HighlightText();
            });

        #ifdef __WXMSW__
            m_isRTL = false;
        #endif
        }

        void SetLanguageRTL(bool isRTL)
        {
        #ifdef __WXOSX__
            NSTextView *text = TextView();
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
    protected:
        virtual void DoSetValue(const wxString& value, int flags) override
        {
            wxWindowUpdateLocker dis(this);
            CustomizedTextCtrl::DoSetValue(value, flags);
            UpdateRTLStyle();
        }

        void UpdateRTLStyle()
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

        bool m_isRTL;
#endif // __WXMSW__

    private:
#ifdef __WXOSX__
        void InitColors()
        {
            m_attrSpace  = @{NSBackgroundColorAttributeName: [NSColor colorWithSRGBRed:0.89 green:0.96 blue:0.68 alpha:1]};
            m_attrEscape = @{NSBackgroundColorAttributeName: [NSColor colorWithSRGBRed:1 green:0.95 blue:1 alpha:1],
                             NSForegroundColorAttributeName: [NSColor colorWithSRGBRed:0.46 green:0 blue:0.01 alpha:1]};
        }

        void HighlightText()
        {
            auto text = GetValue().ToStdWstring();

            NSRange fullRange = NSMakeRange(0, text.length());
            NSLayoutManager *layout = [TextView() layoutManager];
            [layout removeTemporaryAttribute:NSForegroundColorAttributeName forCharacterRange:fullRange];
            [layout removeTemporaryAttribute:NSBackgroundColorAttributeName forCharacterRange:fullRange];

            m_syntax.Highlight(text, [=](int a, int b, SyntaxHighlighter::TextKind kind){
                [layout addTemporaryAttributes:AttrFor(kind) forCharacterRange:NSMakeRange(a, b-a)];
            });
        }

        typedef NSDictionary* AttrType;
        NSDictionary *m_attrSpace, *m_attrEscape;

#else // !__WXOSX__

        void InitColors()
        {
            m_attrDefault.SetBackgroundColour(*wxWHITE);
            m_attrDefault.SetTextColour(*wxBLACK);

            m_attrSpace.SetBackgroundColour("#E4F6AE");

            m_attrEscape.SetBackgroundColour("#FFF1FF");
            m_attrEscape.SetTextColour("#760003");
        }

        void HighlightText()
        {
            auto text = GetValue().ToStdWstring();

            wxWindowUpdateLocker noupd(this);
            wxEventBlocker block(this, wxEVT_TEXT);
            SetStyle(0, text.length(), m_attrDefault);

            m_syntax.Highlight(text, [=](int a, int b, SyntaxHighlighter::TextKind kind){
                SetStyle(a, b, AttrFor(kind));
            });
        }

        typedef wxTextAttr AttrType;
        wxTextAttr m_attrDefault, m_attrSpace, m_attrEscape;
#endif // __WXOSX__/!__WXOSX__

        const AttrType& AttrFor(SyntaxHighlighter::TextKind kind) const
        {
            switch (kind)
            {
                case SyntaxHighlighter::LeadingWhitespace:  return m_attrSpace;
                case SyntaxHighlighter::Escape:             return m_attrEscape;
            }
            return m_attrSpace; // silence bogus warning
        }

        SyntaxHighlighter m_syntax;
};


class SourceTextCtrl : public AnyTranslatableTextCtrl
{
    public:
        SourceTextCtrl(wxWindow *parent, wxWindowID winid)
            : AnyTranslatableTextCtrl(parent, winid, wxTE_READONLY)
        {
            SetLanguageRTL(false); // English is LTR
        }
};


class TranslationTextCtrl : public AnyTranslatableTextCtrl
{
    public:
        TranslationTextCtrl(wxWindow *parent, wxWindowID winid)
            : AnyTranslatableTextCtrl(parent, winid)
        {
#ifdef __WXMSW__
            PrepareTextCtrlForSpellchecker(this);
#endif
        }
};


BEGIN_EVENT_TABLE(PoeditFrame, wxFrame)
// OS X and GNOME apps should open new documents in a new window. On Windows,
// however, the usual thing to do is to open the new document in the already
// open window and replace the current document.
#ifdef __WXMSW__
   EVT_MENU           (wxID_NEW,                  PoeditFrame::OnNew)
   EVT_MENU           (XRCID("menu_new_from_pot"),PoeditFrame::OnNew)
   EVT_MENU           (wxID_OPEN,                 PoeditFrame::OnOpen)
 #ifndef __WXOSX__
   EVT_MENU_RANGE     (wxID_FILE1, wxID_FILE9,    PoeditFrame::OnOpenHist)
 #endif
#endif // __WXMSW__
   EVT_MENU           (wxID_CLOSE,                PoeditFrame::OnCloseCmd)
   EVT_MENU           (wxID_SAVE,                 PoeditFrame::OnSave)
   EVT_MENU           (wxID_SAVEAS,               PoeditFrame::OnSaveAs)
   EVT_MENU           (XRCID("menu_export"),      PoeditFrame::OnExport)
   EVT_MENU           (XRCID("menu_catproperties"), PoeditFrame::OnProperties)
   EVT_MENU           (XRCID("menu_update"),      PoeditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_update_from_pot"),PoeditFrame::OnUpdate)
   EVT_MENU           (XRCID("menu_validate"),    PoeditFrame::OnValidate)
   EVT_MENU           (XRCID("menu_purge_deleted"), PoeditFrame::OnPurgeDeleted)
   EVT_MENU           (XRCID("menu_fuzzy"),       PoeditFrame::OnFuzzyFlag)
   EVT_MENU           (XRCID("menu_ids"),         PoeditFrame::OnIDsFlag)
   EVT_MENU           (XRCID("sort_by_order"),    PoeditFrame::OnSortByFileOrder)
   EVT_MENU           (XRCID("sort_by_source"),    PoeditFrame::OnSortBySource)
   EVT_MENU           (XRCID("sort_by_translation"), PoeditFrame::OnSortByTranslation)
   EVT_MENU           (XRCID("sort_group_by_context"), PoeditFrame::OnSortGroupByContext)
   EVT_MENU           (XRCID("sort_untrans_first"), PoeditFrame::OnSortUntranslatedFirst)
   EVT_MENU           (XRCID("menu_copy_from_src"), PoeditFrame::OnCopyFromSource)
   EVT_MENU           (XRCID("menu_clear"),       PoeditFrame::OnClearTranslation)
   EVT_MENU           (XRCID("menu_references"),  PoeditFrame::OnReferencesMenu)
   EVT_MENU           (wxID_FIND,                 PoeditFrame::OnFind)
   EVT_MENU           (XRCID("menu_find_next"),   PoeditFrame::OnFindNext)
   EVT_MENU           (XRCID("menu_find_prev"),   PoeditFrame::OnFindPrev)
   EVT_MENU           (XRCID("menu_comment"),     PoeditFrame::OnEditComment)
   EVT_MENU           (XRCID("go_done_and_next"),   PoeditFrame::OnDoneAndNext)
   EVT_MENU           (XRCID("go_prev"),            PoeditFrame::OnPrev)
   EVT_MENU           (XRCID("go_next"),            PoeditFrame::OnNext)
   EVT_MENU           (XRCID("go_prev_page"),       PoeditFrame::OnPrevPage)
   EVT_MENU           (XRCID("go_next_page"),       PoeditFrame::OnNextPage)
   EVT_MENU           (XRCID("go_prev_unfinished"), PoeditFrame::OnPrevUnfinished)
   EVT_MENU           (XRCID("go_next_unfinished"), PoeditFrame::OnNextUnfinished)
   EVT_MENU_RANGE     (ID_POPUP_REFS, ID_POPUP_REFS + 999, PoeditFrame::OnReference)
   EVT_MENU_RANGE     (ID_POPUP_TRANS, ID_POPUP_TRANS + 999,
                       PoeditFrame::OnAutoTranslate)
   EVT_MENU           (XRCID("menu_auto_translate"), PoeditFrame::OnAutoTranslateAll)
   EVT_MENU_RANGE     (ID_BOOKMARK_GO, ID_BOOKMARK_GO + 9,
                       PoeditFrame::OnGoToBookmark)
   EVT_MENU_RANGE     (ID_BOOKMARK_SET, ID_BOOKMARK_SET + 9,
                       PoeditFrame::OnSetBookmark)
   EVT_CLOSE          (                PoeditFrame::OnCloseWindow)
   EVT_SIZE           (PoeditFrame::OnSize)

   // handling of selection:
   EVT_UPDATE_UI(XRCID("menu_references"), PoeditFrame::OnSingleSelectionUpdate)
   EVT_UPDATE_UI_RANGE(ID_BOOKMARK_SET, ID_BOOKMARK_SET + 9, PoeditFrame::OnSingleSelectionUpdate)

   EVT_UPDATE_UI(XRCID("menu_fuzzy"), PoeditFrame::OnSelectionUpdate)
   EVT_UPDATE_UI(XRCID("menu_copy_from_src"), PoeditFrame::OnSelectionUpdate)
   EVT_UPDATE_UI(XRCID("menu_clear"), PoeditFrame::OnSelectionUpdate)
   EVT_UPDATE_UI(XRCID("menu_comment"), PoeditFrame::OnSelectionUpdate)

#if defined(__WXMSW__) || defined(__WXGTK__)
   EVT_MENU(wxID_UNDO,      PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_REDO,      PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_CUT,       PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_COPY,      PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_PASTE,     PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_DELETE,    PoeditFrame::OnTextEditingCommand)
   EVT_MENU(wxID_SELECTALL, PoeditFrame::OnTextEditingCommand)
   EVT_UPDATE_UI(wxID_UNDO,      PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_REDO,      PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_CUT,       PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_COPY,      PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_PASTE,     PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_DELETE,    PoeditFrame::OnTextEditingCommandUpdate)
   EVT_UPDATE_UI(wxID_SELECTALL, PoeditFrame::OnTextEditingCommandUpdate)
  #if 0
    // These translations are provided by wxWidgets. Force the strings here,
    // even though unused, because Poedit is translated into many more languages
    // than wx is.
    _("&Undo"),               _("Undo")
    _("&Redo"),               _("Redo")
    _("Cu&t"),                _("Cut")
    _("&Copy"),               _("Copy")
    _("&Paste"),              _("Paste")
    _("&Delete"),             _("Delete")
    _("Select &All"),         _("Select All")
  #endif
#endif
END_EVENT_TABLE()



class PoeditDropTarget : public wxFileDropTarget
{
public:
    PoeditDropTarget(PoeditFrame *win) : m_win(win) {}

    virtual bool OnDropFiles(wxCoord /*x*/, wxCoord /*y*/,
                             const wxArrayString& files)
    {
        if ( files.size() != 1 )
        {
            wxLogError(_("You can't drop more than one file on Poedit window."));
            return false;
        }

        wxFileName f(files[0]);

        if ( f.GetExt().Lower() != "po" )
        {
            wxLogError(_("File '%s' is not a message catalog."),
                       f.GetFullPath().c_str());
            return false;
        }

        if ( !f.FileExists() )
        {
            wxLogError(_("File '%s' doesn't exist."), f.GetFullPath().c_str());
            return false;
        }

        m_win->OpenFile(f.GetFullPath());

        return true;
    }

private:
    PoeditFrame *m_win;
};


// Frame class:

PoeditFrame::PoeditFrame() :
    wxFrame(NULL, -1, _("Poedit"),
            wxDefaultPosition, wxDefaultSize,
            wxDEFAULT_FRAME_STYLE | wxNO_FULL_REPAINT_ON_RESIZE,
            "mainwin"),
    m_contentType(Content::Invalid),
    m_contentView(nullptr),
    m_catalog(nullptr),
    m_fileExistsOnDisk(false),
    m_list(nullptr),
    m_modified(false),
    m_hasObsoleteItems(false),
    m_dontAutoclearFuzzyStatus(false),
    m_setSashPositionsWhenMaximized(false)
{
    m_list = nullptr;
    m_textTrans = nullptr;
    m_textOrig = nullptr;
    m_textOrigPlural = nullptr;
    m_splitter = nullptr;
    m_sidebar = nullptr;

    // make sure that the [ID_POEDIT_FIRST,ID_POEDIT_LAST] range of IDs is not
    // used for anything else:
    wxASSERT_MSG( wxGetCurrentId() < ID_POEDIT_FIRST ||
                  wxGetCurrentId() > ID_POEDIT_LAST,
                  "detected ID values conflict!" );
    wxRegisterId(ID_POEDIT_LAST);

    wxConfigBase *cfg = wxConfig::Get();

    m_displayIDs = (bool)cfg->Read("display_lines", (long)false);
    g_focusToText = (bool)cfg->Read("focus_to_text", (long)false);

#if defined(__WXGTK__)
    wxIconBundle appicons;
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(16,16)));
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(32,32)));
    appicons.AddIcon(wxArtProvider::GetIcon("poedit", wxART_FRAME_ICON, wxSize(48,48)));
    SetIcons(appicons);
#elif defined(__WXMSW__)
    SetIcons(wxIconBundle(wxStandardPaths::Get().GetResourcesDir() + "\\Resources\\Poedit.ico"));
#endif

    // This is different from the default, because it's a bit smaller on OS X
    m_normalGuiFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    m_boldGuiFont = m_normalGuiFont;
    m_boldGuiFont.SetWeight(wxFONTWEIGHT_BOLD);

    wxMenuBar *MenuBar = wxXmlResource::Get()->LoadMenuBar("mainmenu");
    if (MenuBar)
    {
#ifndef __WXOSX__
        wxString menuName(_("&File"));
        menuName.Replace(wxT("&"), wxEmptyString);
        m_menuForHistory = MenuBar->GetMenu(MenuBar->FindMenu(menuName));
        FileHistory().UseMenu(m_menuForHistory);
        FileHistory().AddFilesToMenu(m_menuForHistory);
#endif
        SetMenuBar(MenuBar);
        AddBookmarksMenu(MenuBar->GetMenu(MenuBar->FindMenu(_("&Go"))));
#ifdef __WXOSX__
        wxGetApp().TweakOSXMenuBar(MenuBar);
#endif
    }
    else
    {
        wxLogError("Cannot load main menu from resource, something must have went terribly wrong.");
        wxLog::FlushActive();
        return;
    }

    wxXmlResource::Get()->LoadToolBar(this, "toolbar");

    GetMenuBar()->Check(XRCID("menu_ids"), m_displayIDs);

    CreateStatusBar(1, wxST_SIZEGRIP);

    m_contentWrappingSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_contentWrappingSizer);

    m_attentionBar = new AttentionBar(this);
    m_contentWrappingSizer->Add(m_attentionBar, wxSizerFlags().Expand());

    SetAccelerators();
    RestoreWindowState(this, wxSize(980, 700), WinState_Size | WinState_Pos);

    UpdateMenu();

    ms_instances.insert(this);

    SetDropTarget(new PoeditDropTarget(this));

#ifdef __WXOSX__
    NSWindow *wnd = (NSWindow*)GetWXWindow();
    [wnd setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
#endif
}


void PoeditFrame::EnsureContentView(Content type)
{
    if (m_contentType == type)
        return;

#ifdef __WXMSW__
    wxWindowUpdateLocker no_updates(this);
#endif

    if (m_contentView)
        DestroyContentView();

    switch (type)
    {
        case Content::Invalid:
            m_contentType = Content::Invalid;
            return; // nothing to do

        case Content::Welcome:
            m_contentView = CreateContentViewWelcome();
            break;

        case Content::Empty_PO:
            m_contentView = CreateContentViewEmptyPO();
            break;

        case Content::PO:
            m_contentView = CreateContentViewPO();
            break;
    }

    m_contentType = type;
    m_contentWrappingSizer->Add(m_contentView, wxSizerFlags(1).Expand());
    Layout();
#ifdef __WXMSW__
    m_contentView->Show();
    Layout();
#endif
}


wxWindow* PoeditFrame::CreateContentViewPO()
{
    auto main = new wxPanel(this, wxID_ANY);
    auto mainSizer = new wxBoxSizer(wxHORIZONTAL);
    main->SetSizer(mainSizer);

#ifdef __WXMSW__
    // don't create the window as shown, avoid flicker
    main->Hide();
#endif

#if defined(__WXMSW__)
    const int SPLITTER_FLAGS = wxSP_NOBORDER;
#elif defined(__WXMAC__)
    // wxMac doesn't show XORed line:
    const int SPLITTER_FLAGS = wxSP_LIVE_UPDATE;
#else
    const int SPLITTER_FLAGS = wxSP_3DBORDER;
#endif

    m_splitter = new wxSplitterWindow(main, -1,
                                      wxDefaultPosition, wxDefaultSize,
                                      SPLITTER_FLAGS);

    mainSizer->Add(m_splitter, wxSizerFlags(1).Expand());

    // make only the upper part grow when resizing
    m_splitter->SetSashGravity(1.0);

    m_list = new PoeditListCtrl(m_splitter,
                                ID_LIST,
                                wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT,
                                m_displayIDs);

    m_bottomPanel = new wxPanel(m_splitter);
    m_bottomPanel->Bind(wxEVT_UPDATE_UI, &PoeditFrame::OnSingleSelectionUpdate, this);

    wxStaticText *labelSource =
        new wxStaticText(m_bottomPanel, -1, _("Source text:"));
    labelSource->SetFont(m_boldGuiFont);

    m_labelContext = new wxStaticText(m_bottomPanel, -1, wxEmptyString);
    m_labelContext->SetFont(m_normalGuiFont);
    m_labelContext->Hide();

    m_labelSingular = new wxStaticText(m_bottomPanel, -1, _("Singular:"));
    m_labelSingular->SetFont(m_normalGuiFont);
    m_textOrig = new SourceTextCtrl(m_bottomPanel, ID_TEXTORIG);
    m_labelPlural = new wxStaticText(m_bottomPanel, -1, _("Plural:"));
    m_labelPlural->SetFont(m_normalGuiFont);
    m_textOrigPlural = new SourceTextCtrl(m_bottomPanel, ID_TEXTORIGPLURAL);

    wxStaticText *labelTrans = new wxStaticText(m_bottomPanel, -1, _("Translation:"));
    labelTrans->SetFont(m_boldGuiFont);

    m_textTrans = new TranslationTextCtrl(m_bottomPanel, ID_TEXTTRANS);
    m_textTrans->PushEventHandler(new TransTextctrlHandler(this));

    // in case of plurals form, this is the control for n=1:
    m_textTransSingularForm = NULL;

    m_pluralNotebook = new wxNotebook(m_bottomPanel, -1);

    m_errorBar = new ErrorBar(m_bottomPanel);

    SetCustomFonts();

    wxSizer *panelSizer = new wxBoxSizer(wxVERTICAL);

    wxFlexGridSizer *gridSizer = new wxFlexGridSizer(2);
    gridSizer->AddGrowableCol(1);
    gridSizer->AddGrowableRow(0);
    gridSizer->AddGrowableRow(1);
    gridSizer->Add(m_labelSingular, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    gridSizer->Add(m_textOrig, 1, wxEXPAND);
    gridSizer->Add(m_labelPlural, 0, wxALIGN_CENTER_VERTICAL | wxALL, 3);
    gridSizer->Add(m_textOrigPlural, 1, wxEXPAND);
    gridSizer->SetItemMinSize(m_textOrig, 1, 1);
    gridSizer->SetItemMinSize(m_textOrigPlural, 1, 1);

    panelSizer->Add(m_labelContext, 0, wxEXPAND | wxALL, 3);
    panelSizer->Add(labelSource, 0, wxEXPAND | wxALL, 3);
    panelSizer->Add(gridSizer, 1, wxEXPAND);
    panelSizer->Add(labelTrans, 0, wxEXPAND | wxALL, 3);
    panelSizer->Add(m_textTrans, 1, wxEXPAND);
    panelSizer->Add(m_pluralNotebook, 1, wxEXPAND);
    panelSizer->Add(m_errorBar, 0, wxEXPAND | wxALL, 2);

    m_bottomPanel->SetAutoLayout(true);
    m_bottomPanel->SetSizer(panelSizer);

    m_splitter->SetMinimumPaneSize(200);

    m_list->PushEventHandler(new ListHandler(this));


    m_sidebar = new Sidebar(main);
    mainSizer->Add(m_sidebar, wxSizerFlags().Expand().Border(wxLEFT, 1));


    ShowPluralFormUI(false);
    UpdateMenu();

    switch ( m_list->sortOrder.by )
    {
        case SortOrder::By_FileOrder:
            GetMenuBar()->Check(XRCID("sort_by_order"), true);
            break;
        case SortOrder::By_Source:
            GetMenuBar()->Check(XRCID("sort_by_source"), true);
            break;
        case SortOrder::By_Translation:
            GetMenuBar()->Check(XRCID("sort_by_translation"), true);
            break;
    }
    GetMenuBar()->Check(XRCID("sort_group_by_context"), m_list->sortOrder.groupByContext);
    GetMenuBar()->Check(XRCID("sort_untrans_first"), m_list->sortOrder.untransFirst);

    // Call splitter splitting later, when the window is layed out, otherwise
    // the sizes would get truncated immediately:
    CallAfter([=]{
        // This is a hack -- windows are not maximized immediately and so we can't
        // set correct sash position in ctor (unmaximized window may be too small
        // for sash position when maximized -- see bug #2120600)
        if ( wxConfigBase::Get()->Read(WindowStatePath(this) + "maximized", long(0)) )
            m_setSashPositionsWhenMaximized = true;

        m_splitter->SplitHorizontally(m_list, m_bottomPanel, (int)wxConfigBase::Get()->Read("splitter", -250L));
    });

    return main;
}


wxWindow* PoeditFrame::CreateContentViewWelcome()
{
    return new WelcomeScreenPanel(this);
}


wxWindow* PoeditFrame::CreateContentViewEmptyPO()
{
    return new EmptyPOScreenPanel(this);
}


void PoeditFrame::DestroyContentView()
{
    if (!m_contentView)
        return;

    if (m_list)
        m_list->PopEventHandler(true/*delete*/);
    if (m_textTrans)
        m_textTrans->PopEventHandler(true/*delete*/);
    for (auto tp : m_textTransPlural)
    {
        tp->PopEventHandler(true/*delete*/);
    }
    m_textTransPlural.clear();

    if (m_list)
        m_list->CatalogChanged(NULL);

    if (m_splitter)
        wxConfigBase::Get()->Write("/splitter", (long)m_splitter->GetSashPosition());

    m_contentWrappingSizer->Detach(m_contentView);
    m_contentView->Destroy();
    m_contentView = nullptr;

    m_list = nullptr;
    m_textTrans = nullptr;
    m_textOrig = nullptr;
    m_textOrigPlural = nullptr;
    m_splitter = nullptr;
}


PoeditFrame::~PoeditFrame()
{
    ms_instances.erase(this);

    FindFrame::NotifyParentDestroyed(m_list, m_catalog);

    DestroyContentView();

    wxConfigBase *cfg = wxConfig::Get();
    cfg->SetPath("/");

    cfg->Write("display_lines", m_displayIDs);

    SaveWindowState(this);

#ifndef __WXOSX__
    FileHistory().RemoveMenu(m_menuForHistory);
    FileHistory().Save(*cfg);
#endif

    // write all changes:
    cfg->Flush();

    delete m_catalog;
    m_catalog = NULL;

    // shutdown the spellchecker:
    InitSpellchecker();
}


void PoeditFrame::SetAccelerators()
{
    wxAcceleratorEntry entries[] = {
#ifdef __WXMSW__
        { wxACCEL_CTRL, WXK_F3,                 XRCID("menu_find_next") },
        { wxACCEL_CTRL | wxACCEL_SHIFT, WXK_F3, XRCID("menu_find_prev") },
#endif

        { wxACCEL_CTRL, WXK_PAGEUP,             XRCID("go_prev_page") },
        { wxACCEL_CTRL, WXK_NUMPAD_PAGEUP,      XRCID("go_prev_page") },
        { wxACCEL_CTRL, WXK_PAGEDOWN,           XRCID("go_next_page") },
        { wxACCEL_CTRL, WXK_NUMPAD_PAGEDOWN,    XRCID("go_next_page") },

        { wxACCEL_CTRL, WXK_UP,                 XRCID("go_prev") },
        { wxACCEL_CTRL, WXK_NUMPAD_UP,          XRCID("go_prev") },
        { wxACCEL_CTRL, WXK_DOWN,               XRCID("go_next") },
        { wxACCEL_CTRL, WXK_NUMPAD_DOWN,        XRCID("go_next") },

        { wxACCEL_CTRL, WXK_RETURN,             XRCID("go_done_and_next") },
        { wxACCEL_CTRL, WXK_NUMPAD_ENTER,       XRCID("go_done_and_next") }
    };

    wxAcceleratorTable accel(WXSIZEOF(entries), entries);
    SetAcceleratorTable(accel);
}


void PoeditFrame::InitSpellchecker()
{
    if (!IsSpellcheckingAvailable())
        return;

    if (!m_catalog || !m_textTrans)
        return;

    Language lang = m_catalog->GetLanguage();

    bool report_problem = false;
    bool enabled = m_catalog &&
                #ifndef __WXMSW__ // language choice is automatic, per-keyboard on Windows
                   lang.IsValid() &&
                #endif
                   wxConfig::Get()->Read("enable_spellchecking",
                                         (long)true);
    const bool enabledInitially = enabled;

#ifdef __WXOSX__
    if (enabled)
    {
        if ( !SetSpellcheckerLang(lang.LangAndCountry()) )
        {
            enabled = false;
            report_problem = true;
        }
    }
#endif

    if ( !InitTextCtrlSpellchecker(m_textTrans, enabled, lang) )
        report_problem = true;

    for (size_t i = 0; i < m_textTransPlural.size(); i++)
    {
        if ( !InitTextCtrlSpellchecker(m_textTransPlural[i], enabled, lang) )
            report_problem = true;
    }

#ifndef __WXMSW__ // language choice is automatic, per-keyboard on Windows, can't fail
    if ( enabledInitially && report_problem )
    {
        AttentionMessage msg
        (
            "missing-spell-dict",
            AttentionMessage::Warning,
            wxString::Format
            (
                // TRANSLATORS: %s is language name in its basic form (as you
                // would see e.g. in a list of supported languages). You may need
                // to rephrase it, e.g. to an equivalent of "for language %s".
                _("Spellchecker dictionary for %s isn't available, you need to install it."),
                lang.DisplayName()
            )
        );
        msg.AddAction(_("Learn more"), std::bind(ShowSpellcheckerHelp));
        msg.AddDontShowAgain();
        m_attentionBar->ShowMessage(msg);
    }
#endif // !__WXMSW__
}


void PoeditFrame::UpdateTextLanguage()
{
    if (!m_catalog || !m_textTrans)
        return;

    InitSpellchecker();

    auto isRTL = m_catalog->GetLanguage().IsRTL();
    m_textTrans->SetLanguageRTL(isRTL);
    for (auto tp : m_textTransPlural)
        tp->SetLanguageRTL(isRTL);
}


void PoeditFrame::OnCloseCmd(wxCommandEvent&)
{
    Close();
}


void PoeditFrame::OpenFile(const wxString& filename)
{
    DoIfCanDiscardCurrentDoc([=]{
        DoOpenFile(filename);
    });
}


void PoeditFrame::DoOpenFile(const wxString& filename)
{
    ReadCatalog(filename);

    if (m_textTrans && m_list)
    {
        if (g_focusToText)
            m_textTrans->SetFocus();
        else
            m_list->SetFocus();
    }
}


bool PoeditFrame::NeedsToAskIfCanDiscardCurrentDoc() const
{
    return m_catalog && m_modified;
}

template<typename TFunctor>
void PoeditFrame::DoIfCanDiscardCurrentDoc(TFunctor completionHandler)
{
    if ( !NeedsToAskIfCanDiscardCurrentDoc() )
    {
        completionHandler();
        return;
    }

    wxWindowPtr<wxMessageDialog> dlg = CreateAskAboutSavingDialog();

    dlg->ShowWindowModalThenDo([this,dlg,completionHandler](int retval) {
        // hide the dialog asap, WriteCatalog() may show another modal sheet
        dlg->Hide();
#ifdef __WXOSX__
        // Hide() alone is not sufficient on OS X, we need to destroy dlg
        // shared_ptr and only then continue. Because this code is called
        // from event loop (and not this functions' caller) at an unspecified
        // time anyway, we can just as well defer it into the next idle time
        // iteration.
        CallAfter([this,retval,completionHandler]() {
#endif

        if (retval == wxID_YES)
        {
            if (!m_fileExistsOnDisk || m_fileName.empty())
                m_fileName = GetSaveAsFilename(m_catalog, m_fileName);

            WriteCatalog(m_fileName, [=](bool saved){
                if (saved)
                    completionHandler();
            });
        }
        else if (retval == wxID_NO)
        {
            // call completion without saving the document
            completionHandler();
        }
        else if (retval == wxID_CANCEL)
        {
            // do not call -- not OK
        }

#ifdef __WXOSX__
        });
#endif
    });
}

wxWindowPtr<wxMessageDialog> PoeditFrame::CreateAskAboutSavingDialog()
{
    wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog
                    (
                        this,
                        _("Catalog modified. Do you want to save changes?"),
                        _("Save changes"),
                        wxYES_NO | wxCANCEL | wxICON_QUESTION
                    ));
    dlg->SetExtendedMessage(_("Your changes will be lost if you don't save them."));
    dlg->SetYesNoLabels
         (
            _("Save"),
        #ifdef __WXMSW__
            _("Don't save")
        #else
            _("Don't Save")
        #endif
         );
    return dlg;
}



void PoeditFrame::OnCloseWindow(wxCloseEvent& event)
{
    if (event.CanVeto() && NeedsToAskIfCanDiscardCurrentDoc())
    {
#ifdef __WXMAC__
        // Veto the event by default, the window-modally ask for permission.
        // If it turns out that the window can be closed, the completion handler
        // will do it:
        event.Veto();
#endif
        DoIfCanDiscardCurrentDoc([=]{
            Destroy();
        });
    }
    else // can't veto
    {
        Destroy();
    }
}


void PoeditFrame::OnOpen(wxCommandEvent&)
{
    DoIfCanDiscardCurrentDoc([=]{

        wxString path = wxPathOnly(m_fileName);
        if (path.empty())
            path = wxConfig::Get()->Read("last_file_path", wxEmptyString);

        wxString name = wxFileSelector(_("Open catalog"),
                        path, wxEmptyString, wxEmptyString,
                        wxString::Format("%s (*.po)|*.po|%s (*.*)|*.*",
                            _("PO Translation Files"), _("All Files")),
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);

        if (!name.empty())
        {
            wxConfig::Get()->Write("last_file_path", wxPathOnly(name));

            DoOpenFile(name);
        }
    });
}



#ifndef __WXOSX__
void PoeditFrame::OnOpenHist(wxCommandEvent& event)
{
    wxString f(FileHistory().GetHistoryFile(event.GetId() - wxID_FILE1));
    if ( !wxFileExists(f) )
    {
        wxLogError(_("File '%s' doesn't exist."), f.c_str());
        return;
    }

    OpenFile(f);
}
#endif // !__WXOSX__


void PoeditFrame::OnSave(wxCommandEvent& event)
{
    try
    {
        if (!m_fileExistsOnDisk || m_fileName.empty())
            OnSaveAs(event);
        else
            WriteCatalog(m_fileName);
    }
    catch (Exception& e)
    {
        wxLogError("%s", e.what());
    }
}


static wxString SuggestFileName(const Catalog *catalog)
{
    wxString name;
    if (catalog)
        name = catalog->GetLanguage().Code();

    if (name.empty())
        return "default";
    else
        return name;
}

wxString PoeditFrame::GetSaveAsFilename(Catalog *cat, const wxString& current)
{
    wxString name(wxFileNameFromPath(current));
    wxString path(wxPathOnly(current));

    if (name.empty())
    {
        path = wxConfig::Get()->Read("last_file_path", wxEmptyString);
        name = SuggestFileName(cat) + ".po";
    }

    name = wxFileSelector(_("Save as..."), path, name, wxEmptyString,
	                      wxString::Format("%s (*.po)|*.po|%s (*.*)|*.*",
                              _("PO Translation Files"), _("All Files")),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (!name.empty())
    {
        wxConfig::Get()->Write("last_file_path", wxPathOnly(name));
    }

    return name;
}

void PoeditFrame::DoSaveAs(const wxString& filename)
{
    if (filename.empty())
        return;

    m_fileName = filename;
    WriteCatalog(filename);
}

void PoeditFrame::OnSaveAs(wxCommandEvent&)
{
    DoSaveAs(GetSaveAsFilename(m_catalog, m_fileName));
}


void PoeditFrame::OnExport(wxCommandEvent&)
{
    wxString name;
    wxFileName::SplitPath(m_fileName, nullptr, &name, nullptr);

    if (name.empty())
    {
        name = SuggestFileName(m_catalog) + ".html";
    }
    else
        name += ".html";

    name = wxFileSelector(_("Export as..."),
                          wxPathOnly(m_fileName), name, wxEmptyString,
                          wxString::Format("%s (*.html)|*.html", _("HTML Files")),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT, this);
    if (!name.empty())
    {
        wxConfig::Get()->Write("last_file_path", wxPathOnly(name));
        ExportCatalog(name);
    }
}

bool PoeditFrame::ExportCatalog(const wxString& filename)
{
    wxBusyCursor bcur;
    bool ok = m_catalog->ExportToHTML(filename);
    return ok;
}



void PoeditFrame::OnNew(wxCommandEvent& event)
{
    DoIfCanDiscardCurrentDoc([=]{
        bool isFromPOT = event.GetId() == XRCID("menu_new_from_pot");
        if (isFromPOT)
            NewFromPOT();
        else
            NewFromScratch();
    });
}


void PoeditFrame::NewFromPOT()
{
    Catalog *catalog = new Catalog;

    wxString path = wxPathOnly(m_fileName);
    if (path.empty())
        path = wxConfig::Get()->Read("last_file_path", wxEmptyString);
    wxString pot_file =
        wxFileSelector(_("Open catalog template"),
             path, wxEmptyString, wxEmptyString,
             wxString::Format
             (
                 "%s (*.pot)|*.pot|%s (*.po)|*.po|%s (*.*)|*.*",
                 _("POT Translation Templates"),
                 _("PO Translation Files"),
                 _("All Files")
             ),
             wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
    bool ok = false;
    if (!pot_file.empty())
    {
        wxConfig::Get()->Write("last_file_path", wxPathOnly(pot_file));
        ok = catalog->UpdateFromPOT(pot_file,
                                    /*summary=*/false,
                                    /*cancelledByUser=*/nullptr,
                                    /*replace_header=*/true);
    }
    if (!ok)
    {
        delete catalog;
        return;
    }

    delete m_catalog;
    m_catalog = catalog;

    m_fileName.clear();
    m_fileExistsOnDisk = false;
    m_modified = true;

    if (m_catalog->empty())
    {
        EnsureContentView(Content::Empty_PO);
    }
    else
    {
        EnsureContentView(Content::PO);
        m_list->CatalogChanged(m_catalog);
    }

    UpdateTitle();
    UpdateMenu();
    UpdateStatusBar();
    UpdateTextLanguage();

    // Choose the language:
    wxWindowPtr<LanguageDialog> dlg(new LanguageDialog(this));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            Language lang = dlg->GetLang();
            catalog->Header().Lang = lang;
            catalog->Header().SetHeaderNotEmpty("Plural-Forms", lang.DefaultPluralFormsExpr());

            // Derive save location for the file from the location of the POT
            // file (same directory, language-based name). This doesn't always
            // work, e.g. WordPress plugins use different naming, so don't actually
            // save the file just yet and let the user confirm the location when saving.
            wxFileName pot_fn(pot_file);
            pot_fn.SetFullName(lang.Code() + ".po");

            m_fileName = pot_fn.GetFullPath();
            m_fileExistsOnDisk = false;
            m_modified = true;
        }
        else
        {
            // default to English style plural
            if (catalog->HasPluralItems())
                catalog->Header().SetHeaderNotEmpty("Plural-Forms", "nplurals=2; plural=(n != 1);");
        }

        RecreatePluralTextCtrls();
        UpdateTitle();
        UpdateMenu();
        UpdateStatusBar();
        UpdateTextLanguage();
        if (m_list)
            m_list->CatalogChanged(m_catalog); // refresh language column
    });
}


void PoeditFrame::NewFromScratch()
{
    Catalog *catalog = new Catalog;
    catalog->CreateNewHeader();

    delete m_catalog;
    m_catalog = catalog;

    m_fileName.clear();
    m_fileExistsOnDisk = false;
    m_modified = true;

    EnsureContentView(Content::Empty_PO);

    UpdateTitle();
    UpdateMenu();
    UpdateStatusBar();

    // Choose the language:
    wxWindowPtr<LanguageDialog> dlg(new LanguageDialog(this));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            Language lang = dlg->GetLang();
            catalog->Header().Lang = lang;
            catalog->Header().SetHeaderNotEmpty("Plural-Forms", lang.DefaultPluralFormsExpr());
        }
    });
}


void PoeditFrame::OnProperties(wxCommandEvent&)
{
    EditCatalogProperties();
}

void PoeditFrame::EditCatalogProperties()
{
    wxWindowPtr<PropertiesDialog> dlg(new PropertiesDialog(this, m_fileExistsOnDisk));

    const Language prevLang = m_catalog->GetLanguage();
    dlg->TransferTo(m_catalog);
    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            dlg->TransferFrom(m_catalog);
            m_modified = true;
            RecreatePluralTextCtrls();
            UpdateTitle();
            UpdateMenu();
            if (prevLang != m_catalog->GetLanguage())
            {
                UpdateTextLanguage();
                // trigger resorting and language header update:
                if (m_list)
                    m_list->CatalogChanged(m_catalog);
            }
        }
    });
}

void PoeditFrame::EditCatalogPropertiesAndUpdateFromSources()
{
    // TODO: share code with EditCatalogProperties()

    wxWindowPtr<PropertiesDialog> dlg(new PropertiesDialog(this, m_fileExistsOnDisk, 1));

    const Language prevLang = m_catalog->GetLanguage();
    dlg->TransferTo(m_catalog);
    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            dlg->TransferFrom(m_catalog);
            m_modified = true;
            if (m_list)
                RecreatePluralTextCtrls();
            UpdateTitle();
            UpdateMenu();
            if (prevLang != m_catalog->GetLanguage())
            {
                UpdateTextLanguage();
                // trigger resorting and language header update:
                if (m_list)
                    m_list->CatalogChanged(m_catalog);
            }

            if (!m_catalog->Header().SearchPaths.empty())
            {
                EnsureContentView(Content::PO);
                UpdateCatalog();
            }
        }
    });
}


void PoeditFrame::UpdateAfterPreferencesChange()
{
    g_focusToText = (bool)wxConfig::Get()->Read("focus_to_text",
                                                 (long)false);

    if (m_list)
    {
        SetCustomFonts();
        m_list->Refresh(); // if font changed
        UpdateTextLanguage();
    }
}

/*static*/ void PoeditFrame::UpdateAllAfterPreferencesChange()
{
    for (PoeditFramesList::const_iterator n = ms_instances.begin();
         n != ms_instances.end(); ++n)
    {
        (*n)->UpdateAfterPreferencesChange();
    }
}


bool PoeditFrame::UpdateCatalog(const wxString& pot_file)
{
    // This ensures that the list control won't be redrawn during Update()
    // call when a dialog box is hidden; another alternative would be to call
    // m_list->CatalogChanged(NULL) here
    std::unique_ptr<wxWindowUpdateLocker> locker;
    if (m_list)
        locker.reset(new wxWindowUpdateLocker(m_list));

    ProgressInfo progress(this, _("Updating catalog"));

    bool cancelledByUser;
    bool succ;
    if (pot_file.empty())
        succ = m_catalog->Update(&progress, true, &cancelledByUser);
    else
        succ = m_catalog->UpdateFromPOT(pot_file, true, &cancelledByUser);

    EnsureContentView(Content::PO);
    m_list->CatalogChanged(m_catalog);

    m_modified = succ || m_modified;
    if (!succ && !cancelledByUser)
    {
        wxLogWarning(_("Entries in the catalog are probably incorrect."));
        wxLogError(
           _("Updating the catalog failed. Click on 'Details >>' for details."));
    }

    return succ;
}

void PoeditFrame::OnUpdate(wxCommandEvent& event)
{
    DoIfCanDiscardCurrentDoc([=]{

        wxString pot_file;

        if (event.GetId() == XRCID("menu_update_from_pot"))
        {
            wxString path = wxPathOnly(m_fileName);
            if (path.empty())
                path = wxConfig::Get()->Read("last_file_path", wxEmptyString);
            pot_file =
                wxFileSelector(_("Open catalog template"),
                     path, wxEmptyString, wxEmptyString,
                     wxString::Format
                     (
                         "%s (*.pot)|*.pot|%s (*.po)|*.po|%s (*.*)|*.*",
                         _("POT Translation Templates"),
                         _("PO Translation Files"),
                         _("All Files")
                     ),
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST, this);
            if (pot_file.empty())
                return;
            wxConfig::Get()->Write("last_file_path", wxPathOnly(pot_file));
        }

        try
        {
            if (UpdateCatalog(pot_file))
            {
                if (wxConfig::Get()->ReadBool("use_tm", true) &&
                    wxConfig::Get()->ReadBool("use_tm_when_updating", false))
                {
                    AutoTranslateCatalog();
                }
            }
        }
        catch (Exception& e)
        {
            wxLogError("%s", e.what());
        }

        RefreshControls();

    });
}


void PoeditFrame::OnValidate(wxCommandEvent&)
{
    try
    {
        wxBusyCursor bcur;
        ReportValidationErrors(m_catalog->Validate(),
                               /*mo_compilation_failed=*/Catalog::CompilationStatus::NotDone,
                               /*from_save=*/false, []{});
    }
    catch (Exception& e)
    {
        wxLogError("%s", e.what());
    }
}


template<typename TFunctor>
void PoeditFrame::ReportValidationErrors(int errors,
                                         Catalog::CompilationStatus mo_compilation_status,
                                         bool from_save, TFunctor completionHandler)
{
    wxWindowPtr<wxMessageDialog> dlg;

    if ( errors )
    {
        if (m_list && m_catalog->GetCount())
            m_list->RefreshItems(0, m_catalog->GetCount()-1);
        RefreshControls();

        dlg.reset(new wxMessageDialog
        (
            this,
            wxString::Format
            (
                wxPLURAL("%d issue with the translation found.",
                         "%d issues with the translation found.",
                         errors),
                errors
            ),
            _("Validation results"),
            wxOK | wxICON_ERROR
        ));
        wxString details = _("Entries with errors were marked in red in the list. Details of the error will be shown when you select such an entry.");
        if ( from_save )
        {
            details += "\n\n";
            switch ( mo_compilation_status )
            {
                case Catalog::CompilationStatus::NotDone:
                    details += _("The file was saved safely.");
                    break;
                case Catalog::CompilationStatus::Success:
                    details += _("The file was saved safely and compiled into the MO format, but it will probably not work correctly.");
                    break;
                case Catalog::CompilationStatus::Error:
                    details += _("The file was saved safely, but it cannot be compiled into the MO format and used.");
                    break;
            }
        }
        dlg->SetExtendedMessage(details);
    }
    else
    {
        wxASSERT( !from_save );

        dlg.reset(new wxMessageDialog
        (
            this,
            _("No problems with the translation found."),
            _("Validation results"),
            wxOK | wxICON_INFORMATION
        ));

        wxString details;
        int unfinished = 0;
        m_catalog->GetStatistics(nullptr, nullptr, nullptr, nullptr, &unfinished);
        if (unfinished)
        {
            details = wxString::Format(wxPLURAL("The translation is ready for use, but %d entry is not translated yet.",
                                                "The translation is ready for use, but %d entries are not translated yet.", unfinished), unfinished);
        }
        else
        {
            details = _("The translation is ready for use.");
        }
        dlg->SetExtendedMessage(details);
    }

    dlg->ShowWindowModalThenDo([dlg,completionHandler](int){
        completionHandler();
    });
}


void PoeditFrame::OnListSel(wxListEvent& event)
{
    wxWindow *focus = wxWindow::FindFocus();
    bool hasFocus = (focus == m_textTrans) ||
                    (focus && focus->GetParent() == m_pluralNotebook);

    event.Skip();

    UpdateToTextCtrl();

    if (m_sidebar && m_list)
    {
        if (m_list->HasMultipleSelection())
            m_sidebar->SetMultipleSelection();
        else
            m_sidebar->SetSelectedItem(GetCurrentItem()); // may be nullptr
    }

    if (hasFocus)
    {
        if (m_textTrans->IsShown())
            m_textTrans->SetFocus();
        else if (!m_textTransPlural.empty())
            m_textTransPlural[0]->SetFocus();
    }
}



void PoeditFrame::OnReferencesMenu(wxCommandEvent&)
{
    CatalogItem *entry = GetCurrentItem();
    if ( !entry )
        return;

    const wxArrayString& refs = entry->GetReferences();

    if (refs.GetCount() == 0)
        wxMessageBox(_("No references to this string found."));
    else if (refs.GetCount() == 1)
        ShowReference(0);
    else
    {
        wxString *table = new wxString[refs.GetCount()];
        for (unsigned i = 0; i < refs.GetCount(); i++)
            table[i] = refs[i];
        int result = wxGetSingleChoiceIndex(_("Please choose the reference you want to show:"), _("References"),
                          (int)refs.GetCount(), table);
        delete[] table;
        if (result != -1)
            ShowReference(result);
    }
}


void PoeditFrame::OnReference(wxCommandEvent& event)
{
    ShowReference(event.GetId() - ID_POPUP_REFS);
}



void PoeditFrame::ShowReference(int num)
{
    CatalogItem *entry = GetCurrentItem();
    wxCHECK_RET( entry, "no entry selected" );

    wxBusyCursor bcur;

    wxString basepath;
    wxString cwd = wxGetCwd();

    if (!!m_fileName)
    {
        wxString path;

        if (wxIsAbsolutePath(m_catalog->Header().BasePath))
            path = m_catalog->Header().BasePath;
        else
            path = wxPathOnly(m_fileName) + "/" + m_catalog->Header().BasePath;

        if (path.Last() == _T('/') || path.Last() == _T('\\'))
            path.RemoveLast();

        if (wxIsAbsolutePath(path))
            basepath = path;
        else
            basepath = cwd + "/" + path;
    }

    FileViewer *w = new FileViewer(this, basepath,
                                   entry->GetReferences(),
                                   num);
    if (w->FileOk())
        w->Show(true);
    else
        w->Close();
}



void PoeditFrame::OnFuzzyFlag(wxCommandEvent& event)
{
    if (event.GetEventObject() == GetToolBar())
    {
        GetMenuBar()->Check(XRCID("menu_fuzzy"),
                            GetToolBar()->GetToolState(XRCID("menu_fuzzy")));
    }
    else
    {
        GetToolBar()->ToggleTool(XRCID("menu_fuzzy"),
                                 GetMenuBar()->IsChecked(XRCID("menu_fuzzy")));
    }

    bool setFuzzy = !GetCurrentItem()->IsFuzzy();

    bool modified = false;

    m_list->ForSelectedCatalogItemsDo([=,&modified](CatalogItem& item){
        if (item.IsFuzzy() != setFuzzy)
        {
            item.SetFuzzy(setFuzzy);
            item.SetModified(true);
            modified = true;
        }
    });

    if (modified && !IsModified())
    {
        m_modified = true;
        UpdateTitle();
    }
    UpdateStatusBar();

    UpdateToTextCtrl();

    if (m_list->HasSingleSelection())
    {
        // The user explicitly changed fuzzy status (e.g. to on). Normally, if the
        // user edits an entry, it's fuzzy flag is cleared, but if the user sets
        // fuzzy on to indicate the translation is problematic and then continues
        // editing the entry, we do not want to annoy him by changing fuzzy back on
        // every keystroke.
        m_dontAutoclearFuzzyStatus = true;
    }
}



void PoeditFrame::OnIDsFlag(wxCommandEvent&)
{
    m_displayIDs = GetMenuBar()->IsChecked(XRCID("menu_ids"));
    m_list->SetDisplayLines(m_displayIDs);
}


void PoeditFrame::OnCopyFromSource(wxCommandEvent&)
{
    bool modified = false;

    m_list->ForSelectedCatalogItemsDo([&modified](CatalogItem& item){
        item.SetTranslationFromSource();
        if (item.IsModified())
            modified = true;
    });

    if (modified && !IsModified())
    {
        m_modified = true;
        UpdateTitle();
    }
    UpdateStatusBar();

    UpdateToTextCtrl();
}

void PoeditFrame::OnClearTranslation(wxCommandEvent&)
{
    bool modified = false;

    m_list->ForSelectedCatalogItemsDo([&modified](CatalogItem& item){
        item.ClearTranslation();
        if (item.IsModified())
            modified = true;
    });

    if (modified && !IsModified())
    {
        m_modified = true;
        UpdateTitle();
    }
    UpdateStatusBar();

    UpdateToTextCtrl();
}


void PoeditFrame::OnFind(wxCommandEvent&)
{
    FindFrame *f = FindFrame::Get(m_list, m_catalog);

    if (!f)
        f = new FindFrame(this, m_list, m_catalog, m_textOrig, m_textTrans);
    f->Show(true);
    f->Raise();
    f->FocusSearchField();
}

void PoeditFrame::OnFindNext(wxCommandEvent&)
{
    FindFrame *f = FindFrame::Get(m_list, m_catalog);
    if ( f )
        f->FindNext();
}

void PoeditFrame::OnFindPrev(wxCommandEvent&)
{
    FindFrame *f = FindFrame::Get(m_list, m_catalog);
    if ( f )
        f->FindPrev();
}


CatalogItem *PoeditFrame::GetCurrentItem() const
{
    if ( !m_catalog || !m_list )
        return NULL;

    int item = m_list->GetFirstSelectedCatalogItem();
    if ( item == -1 )
        return NULL;

    wxASSERT( item >= 0 && item < (int)m_catalog->GetCount() );

    return &(*m_catalog)[item];
}


static inline bool IsAnyQuote(wchar_t c)
{
    switch (c)
    {
        case 0x00BB: // »
        case 0x00AB: // «
        case 0x201C: // “
        case 0x201D: // ”
        case 0x201E: // „
        case 0x201F: // ‟
        case '"':
            return true;
        default:
            return false;
    }
}

static wxString TransformNewval(const wxString& val)
{
    wxString newval(val);

#ifdef __WXOSX__
    // Fix occurrences of smart quotes after \, e.g. \“ -- they shouldn't happen
    // because we disabled smart quotes, but apparently some people either
    // enable them back or something else interfered. So play it safe and filter
    // them out:
    for (wxString::iterator i = newval.begin(); i != newval.end(); ++i)
    {
        if (*i == '\\')
        {
            ++i;
            if (i == newval.end())
                break;
            if (IsAnyQuote(*i))
                *i = '"';
        }
    }
#endif // __WXOSX__

    newval.Replace("\n", "");

    if (!newval.empty() && newval[0u] == _T('"'))
        newval.Prepend("\\");
    for (unsigned i = 1; i < newval.Len(); i++)
        if (newval[i] == _T('"') && newval[i-1] != _T('\\'))
        {
            newval = newval.Mid(0, i) + _T("\\\"") + newval.Mid(i+1);
            i++;
        }

    // string ending with [^\]\ is invalid:
    if (newval.length() > 1 &&
        newval[newval.length()-1] == _T('\\') &&
        newval[newval.length()-2] != _T('\\'))
    {
        newval.RemoveLast();
    }

    return newval;
}

void PoeditFrame::UpdateFromTextCtrl()
{
    CatalogItem *entry = GetCurrentItem();
    if ( !entry )
        return;

    wxString key = entry->GetString();
    bool newfuzzy = GetToolBar()->GetToolState(XRCID("menu_fuzzy"));

    const bool oldIsTranslated = entry->IsTranslated();
    bool allTranslated = true; // will be updated later
    bool anyTransChanged = false; // ditto

    if (entry->HasPlural())
    {
        wxArrayString str;
        for (unsigned i = 0; i < m_textTransPlural.size(); i++)
        {
            wxString val = TransformNewval(m_textTransPlural[i]->GetValue());
            str.Add(val);
            if ( val.empty() )
                allTranslated = false;
        }

        if ( str != entry->GetTranslations() )
        {
            anyTransChanged = true;
            entry->SetTranslations(str);
        }
    }
    else
    {
        wxString newval =
            TransformNewval(m_textTrans->GetValue());

        if ( newval.empty() )
            allTranslated = false;

        if ( newval != entry->GetTranslation() )
        {
            anyTransChanged = true;
            entry->SetTranslation(newval);
        }
    }

    if (entry->IsFuzzy() == newfuzzy && !anyTransChanged)
    {
        return; // not even fuzzy status changed, so return
    }

    // did something affecting statistics change?
    bool statisticsChanged = false;

    if (newfuzzy == entry->IsFuzzy() && !m_dontAutoclearFuzzyStatus)
        newfuzzy = false;


    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), newfuzzy);
    GetMenuBar()->Check(XRCID("menu_fuzzy"), newfuzzy);

    if ( entry->IsFuzzy() != newfuzzy )
    {
        entry->SetFuzzy(newfuzzy);
        statisticsChanged = true;
    }
    if ( oldIsTranslated != allTranslated )
    {
        entry->SetTranslated(allTranslated);
        statisticsChanged = true;
    }
    entry->SetModified(true);
    entry->SetAutomatic(false);

    m_list->RefreshSelectedItems();

    if ( statisticsChanged )
    {
        UpdateStatusBar();
    }
    // else: no point in recomputing stats

    if ( !IsModified() )
    {
        m_modified = true;
        UpdateTitle();
    }
}

namespace
{

struct EventHandlerDisabler
{
    EventHandlerDisabler(wxEvtHandler *h) : m_hnd(h)
        { m_hnd->SetEvtHandlerEnabled(false); }
    ~EventHandlerDisabler()
        { m_hnd->SetEvtHandlerEnabled(true); }

    wxEvtHandler *m_hnd;
};

void SetTranslationValue(wxTextCtrl *txt, const wxString& value)
{
    // disable EVT_TEXT forwarding -- the event is generated by
    // programmatic changes to text controls' content and we *don't*
    // want UpdateFromTextCtrl() to be called from here
    EventHandlerDisabler disabler(txt->GetEventHandler());
    txt->SetValue(value);
}

} // anonymous namespace

void PoeditFrame::UpdateToTextCtrl()
{
    CatalogItem *entry = GetCurrentItem();
    if ( !entry )
        return;

    wxString t_o, t_t, t_c, t_ac;
    t_o = entry->GetString();
    t_o.Replace("\\n", "\\n\n");
    t_c = entry->GetComment();
    t_c.Replace("\\n", "\\n\n");

    for (unsigned i=0; i < entry->GetAutoComments().GetCount(); i++)
      t_ac += entry->GetAutoComments()[i] + "\n";
    t_ac.Replace("\\n", "\\n\n");

    // remove "# " in front of every comment line
    t_c = CommentDialog::RemoveStartHash(t_c);

    m_textOrig->SetValue(t_o);

    if (entry->HasPlural())
    {
        wxString t_op = entry->GetPluralString();
        t_op.Replace("\\n", "\\n\n");
        m_textOrigPlural->SetValue(t_op);

        unsigned formsCnt = (unsigned)m_textTransPlural.size();
        for (unsigned j = 0; j < formsCnt; j++)
            SetTranslationValue(m_textTransPlural[j], wxEmptyString);

        unsigned i = 0;
        for (i = 0; i < std::min(formsCnt, entry->GetNumberOfTranslations()); i++)
        {
            t_t = entry->GetTranslation(i);
            t_t.Replace("\\n", "\\n\n");
            SetTranslationValue(m_textTransPlural[i], t_t);
        }
    }
    else
    {
        t_t = entry->GetTranslation();
        t_t.Replace("\\n", "\\n\n");
        SetTranslationValue(m_textTrans, t_t);
    }

    if ( entry->HasContext() )
    {
        const wxString prefix = _("Context:");
        const wxString ctxt = entry->GetContext();
        m_labelContext->SetLabelMarkup(
            wxString::Format("<b>%s</b> %s", prefix, EscapeMarkup(ctxt)));
    }
    m_labelContext->GetContainingSizer()->Show(m_labelContext, entry->HasContext());

    if( entry->GetValidity() == CatalogItem::Val_Invalid )
        m_errorBar->ShowError(entry->GetErrorString());
    else
        m_errorBar->HideError();

    // by default, editing fuzzy item unfuzzies it
    m_dontAutoclearFuzzyStatus = false;

    GetToolBar()->ToggleTool(XRCID("menu_fuzzy"), entry->IsFuzzy());
    GetMenuBar()->Check(XRCID("menu_fuzzy"), entry->IsFuzzy());

    ShowPluralFormUI(entry->HasPlural());
}



void PoeditFrame::ReadCatalog(const wxString& catalog)
{
    wxBusyCursor bcur;

    // NB: duplicated in PoeditFrame::Create()
    Catalog *cat = new Catalog(catalog);
    if (cat->IsOk())
    {
        ReadCatalog(cat);
    }
    else
    {
        wxMessageDialog dlg
        (
            this,
            _("The file cannot be opened."),
            _("Invalid file"),
            wxOK | wxICON_ERROR
        );
        dlg.SetExtendedMessage(
            _("The file may be either corrupted or in a format not recognized by Poedit.")
        );
        dlg.ShowModal();
        delete cat;
    }
}


void PoeditFrame::ReadCatalog(Catalog *cat)
{
    wxASSERT( cat && cat->IsOk() );

#ifdef __WXMSW__
    wxWindowUpdateLocker no_updates(this);
#endif

    delete m_catalog;
    m_catalog = cat;

    if (m_catalog->empty())
    {
        EnsureContentView(Content::Empty_PO);
    }
    else
    {
        EnsureContentView(Content::PO);
        // This must be done as soon as possible, otherwise the list would be
        // confused. GetCurrentItem() could return nullptr or something invalid,
        // causing crash in UpdateToTextCtrl() called from
        // RecreatePluralTextCtrls() just few lines below.
        m_list->CatalogChanged(m_catalog);
    }

    m_fileName = cat->GetFileName();
    m_fileExistsOnDisk = true;
    m_modified = false;

    RecreatePluralTextCtrls();
    RefreshControls();
    UpdateTitle();
    UpdateTextLanguage();

    // FIXME: do this for Gettext PO files only
    if (wxConfig::Get()->Read("translator_name", "").empty() ||
        wxConfig::Get()->Read("translator_email", "").empty())
    {
        AttentionMessage msg
            (
                "no-translator-info",
                AttentionMessage::Info,
                _("You should set your email address in Preferences so that it can be used for Last-Translator header in GNU gettext files.")
            );
        msg.AddAction(_("Set email"),
                      std::bind(&PoeditApp::EditPreferences, &wxGetApp()));
        msg.AddDontShowAgain();

        m_attentionBar->ShowMessage(msg);
    }

    if (m_catalog->Header().Project.empty())
    {
        AttentionMessage msg
            (
                "missing-project-id",
                AttentionMessage::Error,
                _("Translation's project name isn't set.")
            );
        msg.AddAction(_("Set name"),
                      std::bind(&PoeditFrame::EditCatalogProperties, this));

        m_attentionBar->ShowMessage(msg);
    }

    Language language = m_catalog->GetLanguage();
    if (!language.IsValid())
    {
        AttentionMessage msg
            (
                "missing-language",
                AttentionMessage::Error,
                _("Language of the translation isn't set.")
            );
        msg.AddAction(_("Set language"),
                      std::bind(&PoeditFrame::EditCatalogProperties, this));

        m_attentionBar->ShowMessage(msg);
    }

    // check if plural forms header is correct:
    if ( m_catalog->HasPluralItems() )
    {
        wxString err;

        if ( m_catalog->Header().GetHeader("Plural-Forms").empty() )
        {
            err = _("This catalog has entries with plural forms, but doesn't have Plural-Forms header configured.");
        }
        else if ( m_catalog->HasWrongPluralFormsCount() )
        {
            err = _("Entries in this catalog have different plural forms count from what catalog's Plural-Forms header says");
        }

        // FIXME: make this part of global error checking
        wxString plForms = m_catalog->Header().GetHeader("Plural-Forms");
        PluralFormsCalculator *plCalc =
                PluralFormsCalculator::make(plForms.ToAscii());
        if ( !plCalc )
        {
            if ( plForms.empty() )
            {
                err = _("Required header Plural-Forms is missing.");
            }
            else
            {
                err = wxString::Format(
                            _("Syntax error in Plural-Forms header (\"%s\")."),
                            plForms.c_str());
            }
        }
        delete plCalc;

        if ( !err.empty() )
        {
            AttentionMessage msg
                (
                    "malformed-plural-forms",
                    AttentionMessage::Error,
                    err
                );
            msg.AddAction(_("Fix the header"),
                          std::bind(&PoeditFrame::EditCatalogProperties, this));

            m_attentionBar->ShowMessage(msg);
        }
        else // no error, check for warning-worthy stuff
        {
            if ( language.IsValid() )
            {
                // Check for unusual plural forms. Do some normalization to avoid unnecessary
                // complains when the only differences are in whitespace for example.
                wxString pl1 = plForms;
                wxString pl2 = language.DefaultPluralFormsExpr();
                if (!pl2.empty())
                {
                    pl1.Replace(" ", "");
                    pl2.Replace(" ", "");
                    if ( pl1 != pl2 )
                    {
                        if (pl1.Find(";plural=(") == wxNOT_FOUND && pl1.Last() == ';')
                        {
                            pl1.Replace(";plural=", ";plural=(");
                            pl1.RemoveLast();
                            pl1 += ");";
                        }
                    }

                    if ( pl1 != pl2 )
                    {
                        AttentionMessage msg
                            (
                                "unusual-plural-forms",
                                AttentionMessage::Warning,
                                wxString::Format
                                (
                                    // TRANSLATORS: %s is language name in its basic form (as you
                                    // would see e.g. in a list of supported languages). You may need
                                    // to rephrase it, e.g. to an equivalent of "for language %s".
                                    _("Plural forms expression used by the catalog is unusual for %s."),
                                    language.DisplayName()
                                )
                            );
                        msg.AddAction(_("Review"),
                                      std::bind(&PoeditFrame::EditCatalogProperties, this));
                        msg.AddDontShowAgain();

                        m_attentionBar->ShowMessage(msg);
                    }
                }
            }
        }
    }

    NoteAsRecentFile();
}


void PoeditFrame::NoteAsRecentFile()
{
    wxFileName fn(m_fileName);
    fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
#ifdef __WXOSX__
    [[NSDocumentController sharedDocumentController] noteNewRecentDocumentURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String: fn.GetFullPath().utf8_str()]]];
#else
    FileHistory().AddFileToHistory(fn.GetFullPath());
#endif
}


void PoeditFrame::RefreshControls()
{
    if (!m_catalog)
        return;

    m_hasObsoleteItems = false;
    if (!m_catalog->IsOk())
    {
        wxLogError(_("Error loading message catalog file '%s'."), m_fileName.c_str());
        m_fileName.clear();
        m_fileExistsOnDisk = false;
        UpdateMenu();
        UpdateTitle();
        delete m_catalog;
        m_catalog = NULL;
        if (m_list)
            m_list->CatalogChanged(NULL);
        return;
    }

    wxBusyCursor bcur;
    UpdateMenu();

    if (m_list)
    {
        // update catalog view, this may involve reordering the items...
        m_list->CatalogChanged(m_catalog);

        FindFrame *f = FindFrame::Get(m_list, m_catalog);
        if (f)
            f->Reset(m_catalog);
    }

    UpdateTitle();
    UpdateStatusBar();
    Refresh();
}



void PoeditFrame::UpdateStatusBar()
{
    int all, fuzzy, untranslated, badtokens, unfinished;
    if (m_catalog)
    {
        wxString txt;

        m_catalog->GetStatistics(&all, &fuzzy, &badtokens, &untranslated, &unfinished);

        int percent = (all == 0 ) ? 0 : (100 * (all - unfinished) / all);

        wxString details;
        if ( fuzzy > 0 )
        {
            details += wxString::Format(wxPLURAL("%i fuzzy", "%i fuzzy", fuzzy), fuzzy);
        }
        if ( badtokens > 0 )
        {
            if ( !details.empty() )
                details += ", ";
            details += wxString::Format(wxPLURAL("%i bad token", "%i bad tokens", badtokens), badtokens);
        }
        if ( untranslated > 0 )
        {
            if ( !details.empty() )
                details += ", ";
            details += wxString::Format(wxPLURAL("%i not translated", "%i not translated", untranslated), untranslated);
        }
        if ( details.empty() )
        {
            txt.Printf(wxPLURAL("%i %% translated, %i string", "%i %% translated, %i strings", all),
                       percent, all);
        }
        else
        {
            txt.Printf(wxPLURAL("%i %% translated, %i string (%s)", "%i %% translated, %i strings (%s)", all),
                       percent, all, details.c_str());
        }

        GetStatusBar()->SetStatusText(txt);
    }
}

void PoeditFrame::DoGiveHelp(const wxString& text, bool show)
{
    if (show || !text.empty())
        wxFrame::DoGiveHelp(text, show);
    else
        UpdateStatusBar();
}


void PoeditFrame::UpdateTitle()
{
#ifdef __WXMAC__
    OSXSetModified(IsModified());
#endif

    wxString title;
    if ( !m_fileName.empty() )
    {
        wxFileName fn(m_fileName);
        wxString fpath = fn.GetFullName();

        if (m_fileExistsOnDisk)
            SetRepresentedFilename(m_fileName);
        else
            fpath += _(" (unsaved)");

        if ( !m_catalog->Header().Project.empty() )
        {
            title.Printf(
#ifdef __WXOSX__
                L"%s — %s",
#else
                L"%s • %s",
#endif
                fpath, m_catalog->Header().Project);
        }
        else
        {
            title = fn.GetFullName();
        }
#ifndef __WXOSX__
        if ( IsModified() )
            title += _(" (modified)");
        title += " - Poedit";
#endif
    }
    else
    {
        title = "Poedit";
    }

    SetTitle(title);
}



void PoeditFrame::UpdateMenu()
{
    wxMenuBar *menubar = GetMenuBar();
    wxToolBar *toolbar = GetToolBar();

    const bool hasCatalog = m_catalog != nullptr;
    const bool editable = hasCatalog && !m_catalog->empty();

    menubar->Enable(wxID_SAVE, hasCatalog);
    menubar->Enable(wxID_SAVEAS, hasCatalog);
    menubar->Enable(XRCID("menu_export"), hasCatalog);

#ifndef __WXOSX__
    toolbar->EnableTool(wxID_SAVE, hasCatalog);
#endif
    toolbar->EnableTool(XRCID("menu_update"), editable);
    toolbar->EnableTool(XRCID("menu_validate"), editable);
    toolbar->EnableTool(XRCID("menu_fuzzy"), editable);
    toolbar->EnableTool(XRCID("menu_comment"), editable);

    menubar->Enable(XRCID("menu_update"), editable);
    menubar->Enable(XRCID("menu_validate"), editable);
    menubar->Enable(XRCID("menu_fuzzy"), editable);
    menubar->Enable(XRCID("menu_comment"), editable);
    menubar->Enable(XRCID("menu_copy_from_src"), editable);
    menubar->Enable(XRCID("menu_clear"), editable);
    menubar->Enable(XRCID("menu_references"), editable);
    menubar->Enable(wxID_FIND, editable);
    menubar->Enable(XRCID("menu_find_next"), editable);
    menubar->Enable(XRCID("menu_find_prev"), editable);

    menubar->Enable(XRCID("menu_update"), editable);
    menubar->Enable(XRCID("menu_update_from_pot"), hasCatalog);
    menubar->Enable(XRCID("menu_auto_translate"), editable);
    menubar->Enable(XRCID("menu_purge_deleted"), editable);
    menubar->Enable(XRCID("menu_validate"), editable);
    menubar->Enable(XRCID("menu_catproperties"), hasCatalog);

    menubar->Enable(XRCID("menu_ids"), editable);

    menubar->Enable(XRCID("sort_by_order"), editable);
    menubar->Enable(XRCID("sort_by_source"), editable);
    menubar->Enable(XRCID("sort_by_translation"), editable);
    menubar->Enable(XRCID("sort_group_by_context"), editable);
    menubar->Enable(XRCID("sort_untrans_first"), editable);

    if (m_textTrans)
        m_textTrans->Enable(editable);
    if (m_list)
        m_list->Enable(editable);

    menubar->Enable(XRCID("menu_purge_deleted"),
                    editable && m_catalog->HasDeletedItems());

    const bool doupdate = hasCatalog &&
                          !m_catalog->Header().SearchPaths.empty();
    toolbar->EnableTool(XRCID("menu_update"), doupdate);
    menubar->Enable(XRCID("menu_update"), doupdate);

#ifdef __WXGTK__
    if (!editable)
    {
        // work around a wxGTK bug: enabling wxTextCtrl makes it editable too
        // in wxGTK <= 2.8:
        if (m_textOrig)
            m_textOrig->SetEditable(false);
        if (m_textOrigPlural)
            m_textOrigPlural->SetEditable(false);
    }
#endif

    menubar->EnableTop(3, editable);
    for (int i = 0; i < 10; i++)
    {
        menubar->Enable(ID_BOOKMARK_SET + i, editable);
        menubar->Enable(ID_BOOKMARK_GO + i,
                        editable &&
                        m_catalog->GetBookmarkIndex(Bookmark(i)) != -1);
    }
}


void PoeditFrame::WriteCatalog(const wxString& catalog)
{
    WriteCatalog(catalog, [](bool){});
}

template<typename TFunctor>
void PoeditFrame::WriteCatalog(const wxString& catalog, TFunctor completionHandler)
{
    wxBusyCursor bcur;

    std::future<void> tmUpdateThread;
    if (wxConfig::Get()->ReadBool("use_tm", true))
    {
        tmUpdateThread = std::async(std::launch::async, [=](){
            try
            {
                auto tm = TranslationMemory::Get().CreateWriter();
                tm->Insert(*m_catalog);
                tm->Commit();
            }
            catch ( const Exception& e )
            {
                wxLogWarning(_("Failed to update translation memory: %s"), e.what());
            }
            catch ( ... )
            {
                wxLogWarning(_("Failed to update translation memory: %s"), "unknown error");
            }
        });
    }

    Catalog::HeaderData& dt = m_catalog->Header();
    dt.Translator = wxConfig::Get()->Read("translator_name", dt.Translator);
    dt.TranslatorEmail = wxConfig::Get()->Read("translator_email", dt.TranslatorEmail);

    int validation_errors = 0;
    Catalog::CompilationStatus mo_compilation_status = Catalog::CompilationStatus::NotDone;
    if ( !m_catalog->Save(catalog, true, validation_errors, mo_compilation_status) )
    {
        completionHandler(false);
        return;
    }

    m_fileName = catalog;
    m_modified = false;
    m_fileExistsOnDisk = true;

#ifndef __WXOSX__
    FileHistory().AddFileToHistory(m_fileName);
#endif

    UpdateTitle();

    RefreshControls();

    NoteAsRecentFile();

    if (ManagerFrame::Get())
        ManagerFrame::Get()->NotifyFileChanged(m_fileName);

    if ( validation_errors )
    {
        // Note: this may show window-modal window and because we may
        //       be called from such window too, run this in the next
        //       event loop iteration.
        CallAfter([=]{
            ReportValidationErrors(validation_errors, mo_compilation_status, /*from_save=*/true, [=]{
                completionHandler(true);
            });
        });
    }
    else
    {
        completionHandler(true);
    }
}


void PoeditFrame::OnEditComment(wxCommandEvent&)
{
    CatalogItem *firstItem = GetCurrentItem();
    wxCHECK_RET( firstItem, "no entry selected" );

    wxWindowPtr<CommentDialog> dlg(new CommentDialog(this, firstItem->GetComment()));

    dlg->ShowWindowModalThenDo([=](int retcode){
        if (retcode == wxID_OK)
        {
            m_modified = true;
            UpdateTitle();
            wxString comment = dlg->GetComment();

            bool modified = false;
            m_list->ForSelectedCatalogItemsDo([&modified,comment](CatalogItem& item){
                if (item.GetComment() != comment)
                {
                    item.SetComment(comment);
                    item.SetModified(true);
                    modified = true;
                }
            });
            if (modified && !IsModified())
            {
                m_modified = true;
                UpdateTitle();
            }

            // update comment window
            if (m_sidebar)
                m_sidebar->RefreshContent();
        }
    });
}

void PoeditFrame::OnPurgeDeleted(wxCommandEvent& WXUNUSED(event))
{
    const wxString title =
        _("Purge deleted translations");
    const wxString main =
        _("Do you want to remove all translations that are no longer used?");
    const wxString details =
        _("If you continue with purging, all translations marked as deleted will be permanently removed. You will have to translate them again if they are added back in the future.");

    wxWindowPtr<wxMessageDialog> dlg(new wxMessageDialog(this, main, title, wxYES_NO | wxICON_QUESTION));
    dlg->SetExtendedMessage(details);
    dlg->SetYesNoLabels(_("Purge"), _("Keep"));

    dlg->ShowWindowModalThenDo([this,dlg](int retcode){
        if (retcode == wxID_YES) {
            m_catalog->RemoveDeletedItems();
            UpdateMenu();
        }
    });
}


void PoeditFrame::OnAutoTranslate(wxCommandEvent& event)
{
    CatalogItem *entry = GetCurrentItem();
    wxCHECK_RET( entry, "no entry selected" );

    int ind = event.GetId() - ID_POPUP_TRANS;

    entry->SetTranslation(m_autoTranslations[ind]);
    entry->SetFuzzy(false);
    entry->SetModified(true);

    // FIXME: instead of this mess, use notifications of catalog change
    m_modified = true;
    UpdateTitle();

    UpdateToTextCtrl();
    m_list->RefreshSelectedItems();
}

void PoeditFrame::OnAutoTranslateAll(wxCommandEvent&)
{
    int matches = 0;
    if (!AutoTranslateCatalog(&matches))
        return;

    wxString msg, details;

    if (matches)
    {
        msg = wxString::Format(wxPLURAL("%d entry was filled from the translation memory.",
                                        "%d entries were filled from the translation memory.",
                                        matches), matches);
        details = _("The translations were marked as fuzzy, because they may be inaccurate. You should review them for correctness.");
    }
    else
    {
        msg = _("No entries could be filled from the translation memory.");
        details = _("The TM doesn’t contain any strings similar to the content of this file. "
                    "It is only effective for semi-automatic translations after Poedit learns enough from files that you translated manually.");
    }

    wxWindowPtr<wxMessageDialog> dlg(
        new wxMessageDialog
            (
                this,
                msg,
                _("Filling missing translations from TM..."),
                wxOK | wxICON_INFORMATION
            )
    );
    dlg->SetExtendedMessage(details);
    dlg->ShowWindowModalThenDo([dlg](int){});
}

bool PoeditFrame::AutoTranslateCatalog(int *matchesCount)
{
    if (matchesCount)
        *matchesCount = 0;

    if (!wxConfig::Get()->ReadBool("use_tm", true))
        return false;

    wxBusyCursor bcur;

    TranslationMemory& tm = TranslationMemory::Get();
    std::string langcode(m_catalog->GetLanguage().Code());

    int cnt = m_catalog->GetCount();
    if (cnt)
    {
        int matches = 0;
        wxString msg;

        // TODO: make this window-modal
        ProgressInfo progress(this, _("Translating"));
        progress.UpdateMessage(_("Filling missing translations from TM..."));
        progress.SetGaugeMax(cnt);
        for (int i = 0; i < cnt; i++)
        {
            progress.UpdateGauge();

            CatalogItem& dt = (*m_catalog)[i];
            if (dt.HasPlural())
                continue; // can't handle yet (TODO?)
            if (dt.IsFuzzy() || !dt.IsTranslated())
            {
                TranslationMemory::Results results;
                if (tm.Search(langcode, dt.GetString().ToStdWstring(), results, 1))
                {
                    dt.SetTranslation(results[0]);
                    dt.SetAutomatic(true);
                    dt.SetFuzzy(true);
                    matches++;
                    msg.Printf(wxPLURAL("Translated %u string", "Translated %u strings", matches), matches);
                    progress.UpdateMessage(msg);

                    if (m_modified == false)
                    {
                        m_modified = true;
                        UpdateTitle();
                    }
                }
            }
        }

        if (matchesCount)
            *matchesCount = matches;
    }

    RefreshControls();

    return true;
}


wxMenu *PoeditFrame::GetPopupMenu(int item)
{
    if (!m_catalog) return NULL;
    if (item < 0 || item >= (int)m_list->GetItemCount()) return NULL;

    const wxArrayString& refs = (*m_catalog)[item].GetReferences();
    wxMenu *menu = new wxMenu;

    menu->Append(XRCID("menu_copy_from_src"),
                 #ifdef __WXMSW__
                 wxString(_("Copy from source text"))
                 #else
                 wxString(_("Copy from Source Text"))
                 #endif
                   + "\tCtrl+B");
    menu->Append(XRCID("menu_clear"),
                 #ifdef __WXMSW__
                 wxString(_("Clear translation"))
                 #else
                 wxString(_("Clear Translation"))
                 #endif
                   + "\tCtrl+K");
   menu->Append(XRCID("menu_comment"),
                 #ifdef __WXMSW__
                 wxString(_("Edit comment"))
                 #else
                 wxString(_("Edit Comment"))
                 #endif
                   + "\tCtrl+M");

    if (wxConfig::Get()->ReadBool("use_tm", true))
    {
        wxBusyCursor bcur;
        CatalogItem& dt = (*m_catalog)[item];
        std::string langcode(m_catalog->GetLanguage().Code());
        if (TranslationMemory::Get().Search(langcode, dt.GetString().ToStdWstring(), m_autoTranslations))
        {
            menu->AppendSeparator();
            wxMenuItem *it2 = new wxMenuItem
                                  (
                                      menu,
                                      ID_POPUP_DUMMY+1,
                                      #ifdef __WXMSW__
                                      _("Translation suggestions:")
                                      #else
                                      _("Translation Suggestions:")
                                      #endif
                                  );
#ifdef __WXMSW__
            it2->SetFont(m_boldGuiFont);
            menu->Append(it2);
#else
            menu->Append(it2);
            it2->Enable(false);
#endif

            for (size_t i = 0; i < m_autoTranslations.size(); i++)
            {
                wxString s;
                // TRANSLATORS: Quoted text with leading 4 spaces
                s.Printf(_(L"    “%s”"), m_autoTranslations[i]);
                s.Replace("&", "&&");
                menu->Append(ID_POPUP_TRANS + int(i), s);
            }
        }
    }

    if ( !refs.empty() )
    {
        menu->AppendSeparator();

        wxMenuItem *it1 = new wxMenuItem(menu, ID_POPUP_DUMMY+0, _("References:"));
#ifdef __WXMSW__
        it1->SetFont(m_boldGuiFont);
        menu->Append(it1);
#else
        menu->Append(it1);
        it1->Enable(false);
#endif

        for (int i = 0; i < (int)refs.GetCount(); i++)
            menu->Append(ID_POPUP_REFS + i, "    " + refs[i]);
    }

    return menu;
}


static inline void SetCtrlFont(wxWindow *win, const wxFont& font)
{
#ifdef __WXMSW__
    // Native wxMSW text control sends EN_CHANGE when the font changes,
    // producing a wxEVT_TEXT event as if the user changed the value.
    // Unfortunately the event seems to be used internally for sizing,
    // so we can't just filter it out completely. What we can do, however,
    // is to disable *our* handling of the event.
    EventHandlerDisabler disabler(win->GetEventHandler());
#endif
    win->SetFont(font);
}

void PoeditFrame::SetCustomFonts()
{
    if (!m_list)
        return;
    wxConfigBase *cfg = wxConfig::Get();

    static bool prevUseFontText = false;

    bool useFontList = (bool)cfg->Read("custom_font_list_use", (long)false);
    bool useFontText = (bool)cfg->Read("custom_font_text_use", (long)false);

    if (useFontList)
    {
        wxString name = cfg->Read("custom_font_list_name", wxEmptyString);
        if (!name.empty())
        {
            wxNativeFontInfo fi;
            fi.FromString(name);
            wxFont font;
            font.SetNativeFontInfo(fi);
            m_list->SetCustomFont(font);
        }
    }
    else
    {
        m_list->SetCustomFont(wxNullFont);
    }

    if (useFontText)
    {
        wxString name = cfg->Read("custom_font_text_name", wxEmptyString);
        if (!name.empty())
        {
            wxNativeFontInfo fi;
            fi.FromString(name);
            wxFont font;
            font.SetNativeFontInfo(fi);
            SetCtrlFont(m_textOrig, font);
            SetCtrlFont(m_textOrigPlural, font);
            SetCtrlFont(m_textTrans, font);
            for (size_t i = 0; i < m_textTransPlural.size(); i++)
                SetCtrlFont(m_textTransPlural[i], font);
            prevUseFontText = true;
        }
    }
    else if (prevUseFontText)
    {
        wxFont font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        SetCtrlFont(m_textOrig, font);
        SetCtrlFont(m_textOrigPlural, font);
        SetCtrlFont(m_textTrans, font);
        for (size_t i = 0; i < m_textTransPlural.size(); i++)
            SetCtrlFont(m_textTransPlural[i], font);
        prevUseFontText = false;
    }
}

void PoeditFrame::OnSize(wxSizeEvent& event)
{
    event.Skip();

    // see the comment in PoeditFrame ctor
    if ( m_setSashPositionsWhenMaximized && IsMaximized() )
    {
        m_setSashPositionsWhenMaximized = false;

        // update sizes of child windows first:
        Layout();

        // then set sash positions
        m_splitter->SetSashPosition((int)wxConfig::Get()->Read("splitter", 240L));
    }
}

void PoeditFrame::ShowPluralFormUI(bool show)
{
    if (show && (!m_catalog || m_catalog->GetPluralFormsCount() == 0))
        show = false;

    wxSizer *origSizer = m_textOrig->GetContainingSizer();
    origSizer->Show(m_labelSingular, show);
    origSizer->Show(m_labelPlural, show);
    origSizer->Show(m_textOrigPlural, show);
    origSizer->Layout();

    wxSizer *textSizer = m_textTrans->GetContainingSizer();
    textSizer->Show(m_textTrans, !show);
    textSizer->Show(m_pluralNotebook, show);
    textSizer->Layout();
}


void PoeditFrame::RecreatePluralTextCtrls()
{
    if (!m_catalog || !m_list)
        return;

    for (size_t i = 0; i < m_textTransPlural.size(); i++)
       m_textTransPlural[i]->PopEventHandler(true/*delete*/);
    m_textTransPlural.clear();
    m_pluralNotebook->DeleteAllPages();
    m_textTransSingularForm = NULL;

    PluralFormsCalculator *calc = PluralFormsCalculator::make(
                m_catalog->Header().GetHeader("Plural-Forms").ToAscii());

    int formsCount = m_catalog->GetPluralFormsCount();
    for (int form = 0; form < formsCount; form++)
    {
        // find example number that would use this plural form:
        static const int maxExamplesCnt = 5;
        wxString examples;
        int firstExample = -1;
        int examplesCnt = 0;

        if (calc && formsCount > 1)
        {
            for (int example = 0; example < 1000; example++)
            {
                if (calc->evaluate(example) == form)
                {
                    if (++examplesCnt == 1)
                        firstExample = example;
                    if (examplesCnt == maxExamplesCnt)
                    {
                        examples += L'…';
                        break;
                    }
                    else if (examplesCnt == 1)
                        examples += wxString::Format("%d", example);
                    else
                        examples += wxString::Format(", %d", example);
                }
            }
        }

        wxString desc;
        if (formsCount == 1)
            desc = _("Everything");
        else if (examplesCnt == 0)
            desc.Printf(_("Form %i"), form);
        else if (examplesCnt == 1)
        {
            if (firstExample == 0)
                desc = _("Zero");
            else if (firstExample == 1)
                desc = _("One");
            else if (firstExample == 2)
                desc = _("Two");
            else
                desc.Printf(L"n = %s", examples);
        }
        else if (formsCount == 2 && firstExample != 1 && examplesCnt == maxExamplesCnt)
            desc = _("Other");
        else
            desc.Printf(L"n → %s", examples);

        // create text control and notebook page for it:
        auto txt = new TranslationTextCtrl(m_pluralNotebook, wxID_ANY);
        txt->PushEventHandler(new TransTextctrlHandler(this));
        m_textTransPlural.push_back(txt);
        m_pluralNotebook->AddPage(txt, desc);

        if (examplesCnt == 1 && firstExample == 1) // == singular
            m_textTransSingularForm = txt;
    }

    // as a fallback, assume 1st form for plural entries is the singular
    // (like in English and most real-life uses):
    if (!m_textTransSingularForm && !m_textTransPlural.empty())
        m_textTransSingularForm = m_textTransPlural[0];

    delete calc;

    SetCustomFonts();
    UpdateTextLanguage();
    UpdateToTextCtrl();
}

void PoeditFrame::OnListRightClick(wxMouseEvent& event)
{
    long item;
    int flags = wxLIST_HITTEST_ONITEM;
    auto list = static_cast<PoeditListCtrl*>(event.GetEventObject());

    item = list->HitTest(event.GetPosition(), flags);
    if (item != -1 && (flags & wxLIST_HITTEST_ONITEM))
    {
        list->SelectAndFocus(item);
    }

    wxMenu *menu = GetPopupMenu(m_list->ListIndexToCatalog(int(item)));
    if (menu)
    {
        list->PopupMenu(menu, event.GetPosition());
        delete menu;
    }
    else event.Skip();
}

void PoeditFrame::OnListFocus(wxFocusEvent& event)
{
    if (g_focusToText)
    {
        if (m_textTrans->IsShown())
            m_textTrans->SetFocus();
        else if (!m_textTransPlural.empty())
            (m_textTransPlural)[0]->SetFocus();
    }
    else
        event.Skip();
}

void PoeditFrame::AddBookmarksMenu(wxMenu *parent)
{
    wxMenu *menu = new wxMenu();

    parent->AppendSeparator();
    parent->AppendSubMenu(menu, _("&Bookmarks"));

#if defined(__WXMAC__)
    // on Mac, Alt+something is used during normal typing, so we shouldn't
    // use it as shortcuts:
    #define LABEL_BOOKMARK_SET   _("Set Bookmark %i\tCtrl+%i")
    #define LABEL_BOOKMARK_GO    _("Go to Bookmark %i\tCtrl+Alt+%i")
#elif defined(__WXMSW__)
    #define LABEL_BOOKMARK_SET   _("Set bookmark %i\tAlt+%i")
    #define LABEL_BOOKMARK_GO    _("Go to bookmark %i\tCtrl+%i")
#elif defined(__WXGTK__)
    #define LABEL_BOOKMARK_SET   _("Set Bookmark %i\tAlt+%i")
    #define LABEL_BOOKMARK_GO    _("Go to Bookmark %i\tCtrl+%i")
#else
    #error "what is correct capitalization for this toolkit?"
#endif

    for (int i = 0; i < 10; i++)
    {
        menu->Append(ID_BOOKMARK_SET + i,
                     wxString::Format(LABEL_BOOKMARK_SET, i, i));
    }
    menu->AppendSeparator();
    for (int i = 0; i < 10; i++)
    {
        menu->Append(ID_BOOKMARK_GO + i,
                     wxString::Format(LABEL_BOOKMARK_GO, i, i));
    }
}

void PoeditFrame::OnGoToBookmark(wxCommandEvent& event)
{
    // Go to bookmark, if there is an item for it
    Bookmark bk = static_cast<Bookmark>(event.GetId() - ID_BOOKMARK_GO);
    int bkIndex = m_catalog->GetBookmarkIndex(bk);
    if (bkIndex != -1)
    {
        int listIndex = m_list->CatalogIndexToList(bkIndex);
        if (listIndex >= 0 && listIndex < m_list->GetItemCount())
        {
            m_list->EnsureVisible(listIndex);
            m_list->SelectOnly(listIndex);
        }
    }
}

void PoeditFrame::OnSetBookmark(wxCommandEvent& event)
{
    // Set bookmark if different from the current value for the item,
    // else unset it
    int bkIndex = -1;
    int selItemIndex = m_list->GetFirstSelectedCatalogItem();
    if (selItemIndex == -1)
        return;

    Bookmark bk = static_cast<Bookmark>(event.GetId() - ID_BOOKMARK_SET);
    if (m_catalog->GetBookmarkIndex(bk) == selItemIndex)
    {
        m_catalog->SetBookmark(selItemIndex, NO_BOOKMARK);
    }
    else
    {
        bkIndex = m_catalog->SetBookmark(selItemIndex, bk);
    }

    // Refresh items
    m_list->RefreshSelectedItems();
    if (bkIndex != -1)
        m_list->RefreshItem(m_list->CatalogIndexToList(bkIndex));

    // Catalog has been modified
    m_modified = true;
    UpdateTitle();
    UpdateMenu();
}


void PoeditFrame::OnSortByFileOrder(wxCommandEvent&)
{
    m_list->sortOrder.by = SortOrder::By_FileOrder;
    m_list->Sort();
}


void PoeditFrame::OnSortBySource(wxCommandEvent&)
{
    m_list->sortOrder.by = SortOrder::By_Source;
    m_list->Sort();
}


void PoeditFrame::OnSortByTranslation(wxCommandEvent&)
{
    m_list->sortOrder.by = SortOrder::By_Translation;
    m_list->Sort();
}


void PoeditFrame::OnSortGroupByContext(wxCommandEvent& event)
{
    m_list->sortOrder.groupByContext = event.IsChecked();
    m_list->Sort();
}


void PoeditFrame::OnSortUntranslatedFirst(wxCommandEvent& event)
{
    m_list->sortOrder.untransFirst = event.IsChecked();
    m_list->Sort();
}

void PoeditFrame::OnSelectionUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_list && m_list->HasSelection());
}

void PoeditFrame::OnSingleSelectionUpdate(wxUpdateUIEvent& event)
{
    event.Enable(m_list && m_list->HasSingleSelection());
}

#if defined(__WXMSW__) || defined(__WXGTK__)
// Emulate something like OS X's first responder: pass text editing commands to
// the focused text control.
void PoeditFrame::OnTextEditingCommand(wxCommandEvent& event)
{
#ifdef __WXGTK__
    wxEventBlocker block(this);
#endif
    wxWindow *w = wxWindow::FindFocus();
    if (!w || w == this || !w->ProcessWindowEventLocally(event))
        event.Skip();
}

void PoeditFrame::OnTextEditingCommandUpdate(wxUpdateUIEvent& event)
{
#ifdef __WXGTK__
    wxEventBlocker block(this);
#endif
    wxWindow *w = wxWindow::FindFocus();
    if (!w || w == this || !w->ProcessWindowEventLocally(event))
        event.Enable(false);
}
#endif // __WXMSW__ || __WXGTK__

// ------------------------------------------------------------------
//  catalog navigation
// ------------------------------------------------------------------

namespace
{

bool Pred_AnyItem(const CatalogItem&)
{
    return true;
}

bool Pred_UnfinishedItem(const CatalogItem& item)
{
    return !item.IsTranslated() || item.IsFuzzy();
}

} // anonymous namespace


void PoeditFrame::Navigate(int step, NavigatePredicate predicate, bool wrap)
{
    const int count = m_list ? m_list->GetItemCount() : 0;
    if ( !count )
        return;

    const long start = m_list->GetFirstSelected();

    long i = start;

    for ( ;; )
    {
        i += step;

        if ( i < 0 )
        {
            if ( wrap )
                i += count;
            else
                return; // nowhere to go
        }
        else if ( i >= count )
        {
            if ( wrap )
                i -= count;
            else
                return; // nowhere to go
        }

        if ( i == start )
            return; // nowhere to go

        const CatalogItem& item = m_list->ListIndexToCatalogItem(i);
        if ( predicate(item) )
        {
            m_list->SelectOnly(i);
            return;
        }
    }
}

void PoeditFrame::OnPrev(wxCommandEvent&)
{
    Navigate(-1, Pred_AnyItem, /*wrap=*/false);
}

void PoeditFrame::OnNext(wxCommandEvent&)
{
    Navigate(+1, Pred_AnyItem, /*wrap=*/false);
}

void PoeditFrame::OnPrevUnfinished(wxCommandEvent&)
{
    Navigate(-1, Pred_UnfinishedItem, /*wrap=*/false);
}

void PoeditFrame::OnNextUnfinished(wxCommandEvent&)
{
    Navigate(+1, Pred_UnfinishedItem, /*wrap=*/false);
}

void PoeditFrame::OnDoneAndNext(wxCommandEvent&)
{
    // like "next unfinished", but wraps
    Navigate(+1, Pred_UnfinishedItem, /*wrap=*/true);
}

void PoeditFrame::OnPrevPage(wxCommandEvent&)
{
    if (!m_list)
        return;
    auto pos = std::max(m_list->GetFirstSelected()-10, 0L);
    m_list->SelectOnly(pos);
}

void PoeditFrame::OnNextPage(wxCommandEvent&)
{
    if (!m_list)
        return;
    auto pos = std::min(m_list->GetFirstSelected()+10, long(m_list->GetItemCount())-1);
    m_list->SelectOnly(pos);
}
