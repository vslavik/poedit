/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2012 Vaclav Slavik
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

#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/combobox.h>
#include <wx/textctrl.h>
#include "wxeditablelistbox.h"
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

#include "isocodes.h"
#include "propertiesdlg.h"


PropertiesDialog::PropertiesDialog(wxWindow *parent)
{
    wxXmlResource::Get()->LoadDialog(this, parent, _T("properties"));

#ifdef __WXMAC__
    XRCCTRL(*this, "plural_forms_help", wxControl)->SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    m_team = XRCCTRL(*this, "team_name", wxTextCtrl);
    m_teamEmail = XRCCTRL(*this, "team_email", wxTextCtrl);
    m_project = XRCCTRL(*this, "prj_name", wxTextCtrl);
    m_language = XRCCTRL(*this, "language", wxTextCtrl);
    m_charset = XRCCTRL(*this, "charset", wxComboBox);
    m_basePath = XRCCTRL(*this, "basepath", wxTextCtrl);
    m_sourceCodeCharset = XRCCTRL(*this, "source_code_charset", wxComboBox);
    m_pluralForms = XRCCTRL(*this, "plural_forms", wxTextCtrl);

    // my custom controls:
    m_keywords = new wxEditableListBox(this, -1, _("Keywords"));
    wxXmlResource::Get()->AttachUnknownControl(_T("keywords"), m_keywords);
    m_paths = new wxEditableListBox(this, -1, _("Paths"));
    wxXmlResource::Get()->AttachUnknownControl(_T("paths"), m_paths);

#if defined(__WXMSW__) || defined(__WXMAC__)
    // FIXME
    SetSize(GetSize().x+1,GetSize().y+1);
#endif
}


namespace
{

#define UTF_8_CHARSET  _("UTF-8 (recommended)")

void SetCharsetToCombobox(wxComboBox *ctrl, const wxString& value)
{
    static const wxString all_charsets[] =
        {
        UTF_8_CHARSET,
        // and legacy ones
        _T("iso-8859-1"),
        _T("iso-8859-2"),
        _T("iso-8859-3"),
        _T("iso-8859-4"),
        _T("iso-8859-5"),
        _T("iso-8859-6"),
        _T("iso-8859-7"),
        _T("iso-8859-8"),
        _T("iso-8859-9"),
        _T("iso-8859-10"),
        _T("iso-8859-11"),
        _T("iso-8859-12"),
        _T("iso-8859-13"),
        _T("iso-8859-14"),
        _T("iso-8859-15"),
        _T("koi8-r"),
        _T("windows-1250"),
        _T("windows-1251"),
        _T("windows-1252"),
        _T("windows-1253"),
        _T("windows-1254"),
        _T("windows-1255"),
        _T("windows-1256"),
        _T("windows-1257")
        };

    ctrl->Clear();
    for ( int i = 0; i < (int)WXSIZEOF(all_charsets); i++ )
        ctrl->Append(all_charsets[i]);

    const wxString low = value.Lower();
    if ( low == _T("utf-8") || low == _T("utf8") )
        ctrl->SetValue(UTF_8_CHARSET);
    else
        ctrl->SetValue(value);
}

wxString GetCharsetFromCombobox(wxComboBox *ctrl)
{
    wxString c = ctrl->GetValue();
    if ( c == UTF_8_CHARSET )
        c = _T("UTF-8");
    return c;
}

} // anonymous namespace


void PropertiesDialog::TransferTo(Catalog *cat)
{
    SetCharsetToCombobox(m_charset, cat->Header().Charset);
    SetCharsetToCombobox(m_sourceCodeCharset, cat->Header().SourceCodeCharset);

    #define SET_VAL(what,what2) m_##what2->SetValue(cat->Header().what)
    SET_VAL(Team, team);
    SET_VAL(TeamEmail, teamEmail);
    SET_VAL(Project, project);
    SET_VAL(BasePath, basePath);
    SET_VAL(LanguageCode, language);
    #undef SET_VAL

    if (cat->Header().HasHeader(_T("Plural-Forms")))
        m_pluralForms->SetValue(cat->Header().GetHeader(_T("Plural-Forms")));

    m_paths->SetStrings(cat->Header().SearchPaths);
    m_keywords->SetStrings(cat->Header().Keywords);
}


void PropertiesDialog::TransferFrom(Catalog *cat)
{
    cat->Header().Charset = GetCharsetFromCombobox(m_charset);
    cat->Header().SourceCodeCharset = GetCharsetFromCombobox(m_sourceCodeCharset);

    #define GET_VAL(what,what2) cat->Header().what = m_##what2->GetValue()
    GET_VAL(LanguageCode, language);
    GET_VAL(Team, team);
    GET_VAL(TeamEmail, teamEmail);
    GET_VAL(Project, project);
    GET_VAL(BasePath, basePath);
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
    if (arr.GetCount() > 0 && cat->Header().BasePath.empty()) 
        cat->Header().BasePath = _T(".");

    m_keywords->GetStrings(arr);
    cat->Header().Keywords = arr;

    wxString pluralForms = m_pluralForms->GetValue().Strip(wxString::both);
    if ( !pluralForms.empty() && !pluralForms.EndsWith(_T(";")) )
        pluralForms += _T(";");
    cat->Header().SetHeaderNotEmpty(_T("Plural-Forms"), pluralForms);
}
