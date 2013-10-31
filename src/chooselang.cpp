/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2003-2013 Vaclav Slavik
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

#include "chooselang.h"

#if NEED_CHOOSELANG_UI

#include "language.h"

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/translation.h>

static void SaveUILanguage(const wxString& lang)
{
    if (lang.empty())
        wxConfig::Get()->Write("ui_language", "default");
    else
        wxConfig::Get()->Write("ui_language", lang);
}

wxString GetUILanguage()
{
    wxString lng = wxConfig::Get()->Read("ui_language");
    if (!lng.empty() && lng != "default")
        return lng;
    else
        return "";
}

static bool ChooseLanguage(wxString *value)
{
    wxArrayString langs;
    wxArrayString arr;

    {
        wxBusyCursor bcur;
        langs = wxTranslations::Get()->GetAvailableTranslations("poedit");

        arr.push_back(_("(Use default language)"));
        for (auto i : langs)
            arr.push_back(Language::TryParse(i).DisplayNameInItself());
    }
    int choice = wxGetSingleChoiceIndex(
            _("Select your prefered language"),
            _("Language selection"),
            arr);
    if ( choice == -1 )
        return false;

    if ( choice == 0 )
        *value = "";
    else
        *value = langs[choice-1];
    return true;
}

void ChangeUILanguage()
{
    wxString lang;
    if ( !ChooseLanguage(&lang) )
        return;
    SaveUILanguage(lang);
    wxMessageBox(_("You must restart Poedit for this change to take effect."),
                 "Poedit",
                 wxOK | wxCENTRE | wxICON_INFORMATION);
}

#endif // NEED_CHOOSELANG_UI
