/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2000-2014 Vaclav Slavik
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
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/editlbox.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/stattext.h>
#include <wx/notebook.h>

#include <memory>

#include "propertiesdlg.h"
#include "language.h"
#include "pluralforms/pl_evaluate.h"


PropertiesDialog::PropertiesDialog(wxWindow *parent, bool fileExistsOnDisk, int initialPage)
    : m_validatedPlural(-1), m_validatedLang(-1)
{
    wxXmlResource::Get()->LoadDialog(this, parent, "properties");

    m_team = XRCCTRL(*this, "team_name", wxTextCtrl);
    m_teamEmail = XRCCTRL(*this, "team_email", wxTextCtrl);
    m_project = XRCCTRL(*this, "prj_name", wxTextCtrl);
    m_language = XRCCTRL(*this, "language", LanguageCtrl);
    m_charset = XRCCTRL(*this, "charset", wxComboBox);
    m_basePath = XRCCTRL(*this, "basepath", wxTextCtrl);
    m_sourceCodeCharset = XRCCTRL(*this, "source_code_charset", wxComboBox);

    m_pluralFormsDefault = XRCCTRL(*this, "plural_forms_default", wxRadioButton);
    m_pluralFormsCustom = XRCCTRL(*this, "plural_forms_custom", wxRadioButton);
    m_pluralFormsExpr = XRCCTRL(*this, "plural_forms_expr", wxTextCtrl);
    m_pluralFormsExpr->SetWindowVariant(wxWINDOW_VARIANT_SMALL);

    // my custom controls:
    m_keywords = new wxEditableListBox(this, -1, _("Additional keywords"));
    wxXmlResource::Get()->AttachUnknownControl("keywords", m_keywords);
    m_paths = new wxEditableListBox(this, -1, _("Paths"));
    wxXmlResource::Get()->AttachUnknownControl("paths", m_paths);

    // Controls setup:
    m_project->SetHint(_("Name of the project the translation is for"));
    m_pluralFormsExpr->SetHint(_("e.g. nplurals=2; plural=(n > 1);"));

#if defined(__WXMSW__) || defined(__WXMAC__)
    // FIXME
    SetSize(GetSize().x+1,GetSize().y+1);
#endif

    if (!fileExistsOnDisk)
        DisableSourcesControls();

    auto nb = XRCCTRL(*this, "properties_notebook", wxNotebook);
    nb->SetSelection(initialPage);

    m_language->Bind(wxEVT_TEXT, &PropertiesDialog::OnLanguageChanged, this);
    m_language->Bind(wxEVT_COMBOBOX, &PropertiesDialog::OnLanguageChanged, this);

    m_pluralFormsDefault->Bind(wxEVT_RADIOBUTTON, &PropertiesDialog::OnPluralFormsDefault, this);
    m_pluralFormsCustom->Bind(wxEVT_RADIOBUTTON, &PropertiesDialog::OnPluralFormsCustom, this);
    m_pluralFormsExpr->Bind(
        wxEVT_UPDATE_UI,
        [=](wxUpdateUIEvent& e){ e.Enable(m_pluralFormsCustom->GetValue()); });
    m_pluralFormsExpr->Bind(
        wxEVT_TEXT,
        [=](wxCommandEvent& e){ m_validatedPlural = -1; e.Skip(); });
    Bind(wxEVT_UPDATE_UI,
        [=](wxUpdateUIEvent& e){ e.Enable(Validate()); },
        wxID_OK);
    CallAfter([=]{
        m_project->SetFocus();
    });
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
        "iso-8859-1",
        "iso-8859-2",
        "iso-8859-3",
        "iso-8859-4",
        "iso-8859-5",
        "iso-8859-6",
        "iso-8859-7",
        "iso-8859-8",
        "iso-8859-9",
        "iso-8859-10",
        "iso-8859-11",
        "iso-8859-12",
        "iso-8859-13",
        "iso-8859-14",
        "iso-8859-15",
        "koi8-r",
        "windows-1250",
        "windows-1251",
        "windows-1252",
        "windows-1253",
        "windows-1254",
        "windows-1255",
        "windows-1256",
        "windows-1257"
        };

    ctrl->Clear();
    for ( int i = 0; i < (int)WXSIZEOF(all_charsets); i++ )
        ctrl->Append(all_charsets[i]);

    const wxString low = value.Lower();
    if ( low == "utf-8" || low == "utf8" )
        ctrl->SetValue(UTF_8_CHARSET);
    else
        ctrl->SetValue(value);
}

wxString GetCharsetFromCombobox(wxComboBox *ctrl)
{
    wxString c = ctrl->GetValue();
    if ( c == UTF_8_CHARSET )
        c = "UTF-8";
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
    #undef SET_VAL

    m_language->SetLang(cat->Header().Lang);
    OnLanguageValueChanged(m_language->GetValue());

    wxString pf_def = cat->Header().Lang.DefaultPluralFormsExpr();
    wxString pf_cat = cat->Header().GetHeader("Plural-Forms");
    if (pf_cat == "nplurals=INTEGER; plural=EXPRESSION;")
        pf_cat = pf_def;

    m_pluralFormsExpr->SetValue(pf_cat);
    if (!pf_cat.empty() && pf_cat == pf_def)
        m_pluralFormsDefault->SetValue(true);
    else
        m_pluralFormsCustom->SetValue(true);

    m_paths->SetStrings(cat->Header().SearchPaths);
    m_keywords->SetStrings(cat->Header().Keywords);
}


void PropertiesDialog::TransferFrom(Catalog *cat)
{
    cat->Header().Charset = GetCharsetFromCombobox(m_charset);
    cat->Header().SourceCodeCharset = GetCharsetFromCombobox(m_sourceCodeCharset);

    #define GET_VAL(what,what2) cat->Header().what = m_##what2->GetValue()
    GET_VAL(Team, team);
    GET_VAL(TeamEmail, teamEmail);
    GET_VAL(Project, project);
    GET_VAL(BasePath, basePath);
    #undef GET_VAL

    Language lang = m_language->GetLang();
    if (lang.IsValid())
        cat->Header().Lang = lang;

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
        if (!dummy.empty())
            cat->Header().SearchPaths.Add(dummy);
    }
    if (arr.GetCount() > 0 && cat->Header().BasePath.empty()) 
        cat->Header().BasePath = ".";

    m_keywords->GetStrings(arr);
    cat->Header().Keywords = arr;

    wxString pluralForms;
    if (m_pluralFormsDefault->GetValue() && cat->Header().Lang.IsValid())
    {
        pluralForms = cat->Header().Lang.DefaultPluralFormsExpr();
    }

    if (pluralForms.empty())
    {
        pluralForms = m_pluralFormsExpr->GetValue().Strip(wxString::both);
        if ( !pluralForms.empty() && !pluralForms.EndsWith(";") )
            pluralForms += ";";
    }
    cat->Header().SetHeaderNotEmpty("Plural-Forms", pluralForms);
}


void PropertiesDialog::DisableSourcesControls()
{
    m_basePath->Disable();
    m_paths->Disable();
    for (auto c: m_paths->GetChildren())
        c->Disable();

    auto label = XRCCTRL(*this, "sources_path_label", wxStaticText);
    label->SetLabel(_("Please save the file first. This section cannot be edited until then."));
    label->SetForegroundColour(*wxRED);
}


void PropertiesDialog::OnLanguageChanged(wxCommandEvent& event)
{
    m_validatedLang = -1;
    OnLanguageValueChanged(event.GetString());
    event.Skip();
}

void PropertiesDialog::OnLanguageValueChanged(const wxString& langstr)
{
    Language lang = Language::TryParse(langstr);
    wxString pluralForm = lang.DefaultPluralFormsExpr();
    if (pluralForm.empty())
    {
        m_pluralFormsDefault->Disable();
        m_pluralFormsCustom->SetValue(true);
    }
    else
    {
        m_pluralFormsDefault->Enable();
        if (m_pluralFormsExpr->GetValue().empty() ||
            m_pluralFormsExpr->GetValue() == pluralForm)
        {
            m_pluralFormsDefault->SetValue(true);
        }
    }
}

void PropertiesDialog::OnPluralFormsDefault(wxCommandEvent& event)
{
    m_rememberedPluralForm = m_pluralFormsExpr->GetValue();

    Language lang = m_language->GetLang();
    if (lang.IsValid())
    {
        wxString defaultForm = lang.DefaultPluralFormsExpr();
        if (!defaultForm.empty())
            m_pluralFormsExpr->SetValue(defaultForm);
    }

    event.Skip();
}

void PropertiesDialog::OnPluralFormsCustom(wxCommandEvent& event)
{
    if (!m_rememberedPluralForm.empty())
        m_pluralFormsExpr->SetValue(m_rememberedPluralForm);

    event.Skip();
}


bool PropertiesDialog::Validate()
{
    if (m_validatedPlural == -1)
    {
        m_validatedPlural = 1;
        if (m_pluralFormsCustom->GetValue())
        {
            wxString form = m_pluralFormsExpr->GetValue();
            if (!form.empty())
            {
                std::unique_ptr<PluralFormsCalculator> calc(PluralFormsCalculator::make(form.ToAscii()));
                if (!calc)
                    m_validatedPlural = 0;
            }
        }
    }

    if (m_validatedLang == -1)
    {
        m_validatedLang = m_language->IsValid() ? 1 : 0;
    }

    return m_validatedLang == 1 &&
           m_validatedPlural == 1 &&
           !m_project->GetValue().empty();
}
