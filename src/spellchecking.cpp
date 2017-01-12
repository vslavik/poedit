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

#include "spellchecking.h"

#include "str_helpers.h"

#ifdef __WXGTK__
    #include <gtk/gtk.h>
    extern "C" {
    #include <gtkspell/gtkspell.h>
    }
#endif

#ifdef __WXMSW__
    #include <wx/msw/wrapwin.h>
    #include <Richedit.h>
    #ifndef IMF_SPELLCHECKING
        #define IMF_SPELLCHECKING 0x0800
    #endif
#endif

#include "edapp.h"


#ifdef __WXGTK__
// helper functions that finds GtkTextView of wxTextCtrl:
static GtkTextView *GetTextView(wxTextCtrl *ctrl)
{
    GtkWidget *parent = ctrl->m_widget;
    GList *child = gtk_container_get_children(GTK_CONTAINER(parent));
    while (child)
    {
        if (GTK_IS_TEXT_VIEW(child->data))
        {
            return GTK_TEXT_VIEW(child->data);
        }
        child = child->next;
    }

    wxFAIL_MSG( "couldn't find GtkTextView for text control" );
    return NULL;
}

#if GTK_CHECK_VERSION(3,0,0)

bool InitTextCtrlSpellchecker(wxTextCtrl *text, bool enable, const Language& lang)
{
    GtkTextView *textview = GetTextView(text);
    wxASSERT_MSG( textview, "wxTextCtrl is supposed to use GtkTextView" );

    GtkSpellChecker *spell = gtk_spell_checker_get_from_text_view(textview);

    if (enable)
    {
        if (!spell)
        {
            spell = gtk_spell_checker_new();
            gtk_spell_checker_attach(spell, textview);
        }

        return gtk_spell_checker_set_language(spell, lang.Code().c_str(), nullptr);
    }
    else
    {
        if (spell)
            gtk_spell_checker_detach(spell);
        return true;
    }
}

#else // GTK+ 2.x

bool InitTextCtrlSpellchecker(wxTextCtrl *text, bool enable, const Language& lang)
{
    GtkTextView *textview = GetTextView(text);
    wxASSERT_MSG( textview, "wxTextCtrl is supposed to use GtkTextView" );
    GtkSpell *spell = gtkspell_get_from_text_view(textview);

    GError *err = NULL;

    if (enable)
    {
        if (spell)
            gtkspell_set_language(spell, lang.Code().c_str(), &err);
        else
            gtkspell_new_attach(textview, lang.Code().c_str(), &err);
    }
    else // !enable
    {
        // GtkSpell when used with Zemberek Enchant module doesn't work
        // correctly if you repeatedly attach and detach a speller to text
        // view. See https://poedit.net/trac/ticket/276 for details.
        //
        // To work around this, we set the language to a non-existent one
        // instead of detaching GtkSpell -- this has the same effect as
        // detaching the speller as far as the UI is concerned.
        if (spell)
            gtkspell_set_language(spell, "unknown_language", &err);
    }

    if (err)
        g_error_free(err);

    return err == NULL;
}

#endif // GTK+ 2.x

#endif // __WXGTK__

#ifdef __WXOSX__
bool SetSpellcheckerLang(const wxString& lang)
{
    NSString *nslang = str::to_NS(lang);
    NSSpellChecker *sc = [NSSpellChecker sharedSpellChecker];
    [sc setAutomaticallyIdentifiesLanguages:NO];
    return [sc setLanguage: nslang];
}

bool InitTextCtrlSpellchecker(wxTextCtrl *text, bool enable, const Language& /*lang*/)
{
    NSScrollView *scroll = (NSScrollView*)text->GetHandle();
    NSTextView *view = [scroll documentView];

    [view setContinuousSpellCheckingEnabled:enable];
    [view setGrammarCheckingEnabled:enable];

    return true;
}
#endif // __WXOSX__

#ifdef __WXMSW__
void PrepareTextCtrlForSpellchecker(wxTextCtrl *text)
{
    // Set spellchecking-friendly style on the text control. Enabling spellchecking
    // itself is done with EM_SETLANGOPTIONS in InitTextCtrlSpellchecker()
    HWND hwnd = (HWND)text->GetHWND();
    auto editStyle = SES_USECTF | SES_CTFALLOWEMBED | SES_CTFALLOWSMARTTAG | SES_CTFALLOWPROOFING;
    ::SendMessage(hwnd, EM_SETEDITSTYLE, editStyle, editStyle);
}

bool InitTextCtrlSpellchecker(wxTextCtrl *text, bool enable, const Language& /*lang*/)
{
    HWND hwnd = (HWND) text->GetHWND();
    auto langOptions = ::SendMessage(hwnd, EM_GETLANGOPTIONS, 0, 0);
    if (enable)
        langOptions |= IMF_SPELLCHECKING;
    else
        langOptions &= ~IMF_SPELLCHECKING;
    ::SendMessage(hwnd, EM_SETLANGOPTIONS, 0, langOptions);
    return true;
}
#endif // __WXMSW__


#ifndef __WXMSW__
void ShowSpellcheckerHelp()
{
#if defined(__WXOSX__)
    #define SPELL_HELP_PAGE   "SpellcheckerMac"
#elif defined(__UNIX__)
    #define SPELL_HELP_PAGE   "SpellcheckerLinux"
#else
    #error "missing spellchecker instructions for platform"
#endif
    wxGetApp().OpenPoeditWeb("/trac/wiki/Doc/" SPELL_HELP_PAGE);
}
#endif // !__WXMSW__
