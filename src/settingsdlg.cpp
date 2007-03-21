/*
 *  This file is part of poEdit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2005 Vaclav Slavik
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
 *  Catalog settings dialog
 *
 */

#include <wx/wxprec.h>

#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include "editlbox/editlbox.h"
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

#include "isocodes.h"
#include "settingsdlg.h"


SettingsDialog::SettingsDialog(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, _T("settings"));

    m_team = XRCCTRL(*this, "team_name", wxTextCtrl);
    m_teamEmail = XRCCTRL(*this, "team_email", wxTextCtrl);
    m_project = XRCCTRL(*this, "prj_name", wxTextCtrl);
    m_language = XRCCTRL(*this, "language", wxComboBox);
    m_country = XRCCTRL(*this, "country", wxComboBox);
    m_charset = XRCCTRL(*this, "charset", wxComboBox);
    m_basePath = XRCCTRL(*this, "basepath", wxTextCtrl);
    m_sourceCodeCharset = XRCCTRL(*this, "source_code_charset", wxComboBox);
    m_pluralForms = XRCCTRL(*this, "plural_forms", wxTextCtrl);

    const LanguageStruct *lang = isoLanguages; /*from isocodes.h*/ 
    m_language->Append(wxEmptyString);
    while (lang->lang != NULL)
        m_language->Append((lang++)->lang);
    
    lang = isoCountries;
    m_country->Append(wxEmptyString);
    while (lang->lang != NULL)
        m_country->Append((lang++)->lang);
        
    // my custom controls:
    m_keywords = new wxEditableListBox(this, -1, _("Keywords"));
    wxXmlResource::Get()->AttachUnknownControl(_T("keywords"), m_keywords);
    m_paths = new wxEditableListBox(this, -1, _("Paths"));
    wxXmlResource::Get()->AttachUnknownControl(_T("paths"), m_paths);
}


#define DEFAULT_CHARSETS \
    _T(":utf-8:iso-8859-1:iso-8859-2:iso-8859-3:iso-8859-4:iso-8859-5")\
    _T(":iso-8859-6:iso-8859-7:iso-8859-8:iso-8859-9:iso-8859-10")\
    _T(":iso-8859-11:iso-8859-12:iso-8859-13:iso-8859-14:iso-8859-15:")\
    _T(":koi8-r:windows-1250:windows-1251:windows-1252:windows-1253:")\
    _T(":windows-1254:windows-1255:windows-1256:windows-1257:")

void SettingsDialog::TransferTo(Catalog *cat)
{
    wxStringTokenizer tkn(wxConfig::Get()->Read(_T("used_charsets"), 
                          DEFAULT_CHARSETS), _T(":"), wxTOKEN_STRTOK);

    while (tkn.HasMoreTokens())
    {
        wxString strToken = tkn.GetNextToken();
        m_charset->Append(strToken);

        // check if the charset is supported by iconv()
        if(strToken.compare(0, 3, _T("utf")) == 0 ||
                           strToken.compare(0, 3, _T("iso")) == 0)
            m_sourceCodeCharset->Append(strToken);
    }

    #define SET_VAL(what,what2) m_##what2->SetValue(cat->Header().what)
    SET_VAL(Team, team);
    SET_VAL(TeamEmail, teamEmail);
    SET_VAL(Project, project);
    SET_VAL(BasePath, basePath);
    SET_VAL(Language, language);
    SET_VAL(Country, country);
    SET_VAL(Charset, charset);
    SET_VAL(SourceCodeCharset, sourceCodeCharset);
    #undef SET_VAL

    if (cat->Header().HasHeader(_T("Plural-Forms")))
        m_pluralForms->SetValue(cat->Header().GetHeader(_T("Plural-Forms")));
    
    m_paths->SetStrings(cat->Header().SearchPaths);
    m_keywords->SetStrings(cat->Header().Keywords);
}

 
            
void SettingsDialog::TransferFrom(Catalog *cat)
{
    #define GET_VAL(what,what2) cat->Header().what = m_##what2->GetValue()
    GET_VAL(Language, language);
    GET_VAL(Country, country);
    GET_VAL(Charset, charset);
    GET_VAL(Team, team);
    GET_VAL(TeamEmail, teamEmail);
    GET_VAL(Project, project);
    GET_VAL(BasePath, basePath);
    GET_VAL(SourceCodeCharset, sourceCodeCharset);
    #undef GET_VAL

    wxString dummy;
    wxArrayString arr;   
    
    cat->Header().SearchPaths.Clear();
    cat->Header().Keywords.Clear();

    m_paths->GetStrings(arr);
    for (size_t i = 0; i < arr.GetCount(); i++)
    {
        dummy = arr[i];
        if (dummy[dummy.Length() - 1] == _T('/') || 
                dummy[dummy.Length() - 1] == _T('\\')) 
            dummy.RemoveLast();
        cat->Header().SearchPaths.Add(dummy);
    }
    if (arr.GetCount() > 0 && cat->Header().BasePath.IsEmpty()) 
        cat->Header().BasePath = _T(".");

    m_keywords->GetStrings(arr);
    cat->Header().Keywords = arr;

    wxString charsets = wxConfig::Get()->Read(_T("used_charsets"), DEFAULT_CHARSETS);
    wxString charset = m_charset->GetValue().Lower();
    if (!charsets.Contains(_T(":")+charset+_T(":")))
    {
        charsets = _T(":") + charset + charsets;
        wxConfig::Get()->Write(_T("used_charsets"), charsets);
    }
   
    cat->Header().SetHeaderNotEmpty(_T("Plural-Forms"),
                                    m_pluralForms->GetValue());
}
