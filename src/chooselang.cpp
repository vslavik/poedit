/*
 *  This file is part of poEdit (http://www.poedit.org)
 *
 *  Copyright (C) 2003-2006 Vaclav Slavik
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
 *  $Id$
 *
 *  Language chooser
 *
 */

#include <wx/wxprec.h>

#include <wx/wx.h>
#include <wx/config.h>

#include "chooselang.h"

static void SaveUILanguage(wxLanguage lang)
{
    if (lang == wxLANGUAGE_UNKNOWN)
        return;
    if (lang == wxLANGUAGE_DEFAULT)
        wxConfig::Get()->Write(_T("ui_language"), _T("default"));
    else
        wxConfig::Get()->Write(_T("ui_language"),
                               wxLocale::GetLanguageInfo(lang)->CanonicalName);
}

wxLanguage GetUILanguage()
{
#if !NEED_CHOOSELANG_UI
    return wxLANGUAGE_DEFAULT;
#else
    wxLanguage lang(wxLANGUAGE_DEFAULT);
    wxString lng = wxConfig::Get()->Read(_T("ui_language"));
    if (lng.empty())
    {
        lang = ChooseLanguage();
        if (lang != wxLANGUAGE_UNKNOWN)
            SaveUILanguage(lang);
        else
            lang = wxLANGUAGE_DEFAULT;
    }
    else if (lng != _T("default"))
    {
        const wxLanguageInfo *info = wxLocale::FindLanguageInfo(lng);
        if (info != NULL)
            lang = (wxLanguage)info->Language;
        else
            wxLogError(_("Uknown locale code '%s' in registry."), lng.c_str());
    }
    return lang;
#endif
}

#if NEED_CHOOSELANG_UI
wxLanguage ChooseLanguage()
{
    struct LangInfo
    {
        const wxChar *name;
        wxLanguage code;
    };

    LangInfo langs[] =
    {
        { _("(Use default language)"), wxLANGUAGE_DEFAULT },

        { _T("Afrikaans"), wxLANGUAGE_AFRIKAANS },
        { _T("Albanian"), wxLANGUAGE_ALBANIAN },
        { _T("Amharic"), wxLANGUAGE_AMHARIC },
        { _T("Arabic"), wxLANGUAGE_ARABIC },
        { _T("Bangla"), wxLANGUAGE_BENGALI },
        { _T("Basque"), wxLANGUAGE_BASQUE },
        { _T("Belarusian"), wxLANGUAGE_BELARUSIAN },
        { _T("Breton"), wxLANGUAGE_BRETON },
        { _T("Bulgarian"), wxLANGUAGE_BULGARIAN },
        { _T("Catalan"), wxLANGUAGE_CATALAN },
        { _T("Chinese (Traditional)"), wxLANGUAGE_CHINESE_TRADITIONAL },
        { _T("Chinese (Simplified)"), wxLANGUAGE_CHINESE_SIMPLIFIED },
        { _T("Croatian"), wxLANGUAGE_CROATIAN },
        { _T("Czech"), wxLANGUAGE_CZECH },
        { _T("Danish"), wxLANGUAGE_DANISH },
        { _T("Dutch"), wxLANGUAGE_DUTCH },
        { _T("English"), wxLANGUAGE_ENGLISH },
        { _T("Estonian"), wxLANGUAGE_ESTONIAN },
        { _T("Esperanto"), wxLANGUAGE_ESPERANTO },
        { _T("Farsi"), wxLANGUAGE_FARSI },
        { _T("Finnish"), wxLANGUAGE_FINNISH },
        { _T("French"), wxLANGUAGE_FRENCH },
        { _T("Galician"), wxLANGUAGE_GALICIAN },
        { _T("Georgian"), wxLANGUAGE_GEORGIAN },
        { _T("German"), wxLANGUAGE_GERMAN },
        { _T("Greek"), wxLANGUAGE_GREEK },
        { _T("Hebrew"), wxLANGUAGE_HEBREW },
        { _T("Hindi"), wxLANGUAGE_HINDI },
        { _T("Hungarian"), wxLANGUAGE_HUNGARIAN },
        { _T("Icelandic"), wxLANGUAGE_ICELANDIC },
        { _T("Indonesian"), wxLANGUAGE_INDONESIAN },
        { _T("Italian"), wxLANGUAGE_ITALIAN },
        { _T("Japanese"), wxLANGUAGE_JAPANESE },
        { _T("Korean"), wxLANGUAGE_KOREAN },
        { _T("Kurdish"), wxLANGUAGE_KURDISH },
        { _T("Kyrgyz"), wxLANGUAGE_KIRGHIZ },
        { _T("Latvian"), wxLANGUAGE_LATVIAN },
        { _T("Lithuanian"), wxLANGUAGE_LITHUANIAN },
        { _T("Macedonian"), wxLANGUAGE_MACEDONIAN },
        { _T("Mongolian"), wxLANGUAGE_MONGOLIAN },
        { _T("Norwegian Nynorsk"), wxLANGUAGE_NORWEGIAN_NYNORSK },
        { _T("Norwegian Bokmal"), wxLANGUAGE_NORWEGIAN_BOKMAL },
        { _T("Polish"), wxLANGUAGE_POLISH },
        { _T("Portuguese"), wxLANGUAGE_PORTUGUESE },
        { _T("Portuguese (Brazilian)"), wxLANGUAGE_PORTUGUESE_BRAZILIAN },
        { _T("Punjabi"), wxLANGUAGE_PUNJABI },
        { _T("Romanian"), wxLANGUAGE_ROMANIAN },
        { _T("Russian"), wxLANGUAGE_RUSSIAN },
        { _T("Serbian"), wxLANGUAGE_SERBIAN },
        { _T("Slovak"), wxLANGUAGE_SLOVAK },
        { _T("Slovenian"), wxLANGUAGE_SLOVENIAN },
        { _T("Spanish"), wxLANGUAGE_SPANISH },
        { _T("Spanish (Puerto Rico)"), wxLANGUAGE_SPANISH_PUERTO_RICO },
        { _T("Swedish"), wxLANGUAGE_SWEDISH },
        { _T("Tamil"), wxLANGUAGE_TAMIL },
        { _T("Tatarish"), wxLANGUAGE_TATAR },
        { _T("Turkish"), wxLANGUAGE_TURKISH },
        { _T("Ukrainian"), wxLANGUAGE_UKRAINIAN },

        { NULL, wxLANGUAGE_UNKNOWN }
    };

    wxArrayString arr;
    for (int i = 0; langs[i].name; i++)
        arr.Add(langs[i].name);

    int choice = wxGetSingleChoiceIndex(
            _("Select your prefered language"),
            _("Language selection"),
            arr);
    if (choice == -1)
        return wxLANGUAGE_UNKNOWN;
    else
        return langs[choice].code;
}

void ChangeUILanguage()
{
    wxLanguage lang = ChooseLanguage();
    if (lang == wxLANGUAGE_UNKNOWN)
        return;
    SaveUILanguage(lang);
    wxMessageBox(_("You must restart poEdit for this change to take effect."),
                 _T("poEdit"),
                 wxOK | wxCENTRE | wxICON_INFORMATION);
}

#endif // NEED_CHOOSELANG_UI
