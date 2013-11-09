/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2013 Vaclav Slavik
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

#include "languagectrl.h"

#include <wx/config.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

IMPLEMENT_DYNAMIC_CLASS(LanguageCtrl, wxComboBox)

LanguageCtrl::LanguageCtrl(wxWindow *parent, wxWindowID winid)
    : wxComboBox(parent, winid)
{
    Init();
}

void LanguageCtrl::Init()
{
    SetHint(_("Language Code or Name (e.g. en_GB)"));

    for (auto x: Language::AllFormattedNames())
        Append(x);

    m_inited = true;
}

void LanguageCtrl::SetLang(const Language& lang)
{
    if (!m_inited)
        Init();

    SetValue(lang.FormatForRoundtrip());
}

Language LanguageCtrl::GetLang() const
{
    return Language::TryParse(GetValue());
}

#ifdef __WXMSW__
wxSize LanguageCtrl::DoGetBestSize() const
{
    // wxComboBox's implementation is insanely slow, at least on MSW.
    // Hardcode a value instead, it doesn't matter for Poedit's use anyway,
    // this control's best size is not the determining factor.
    return GetSizeFromTextSize(100);
}
#endif



LanguageDialog::LanguageDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, _("Translation Language")),
      m_validatedLang(-1)
{
    auto sizer = new wxBoxSizer(wxVERTICAL);

    auto label = new wxStaticText(this, wxID_ANY, _("Language of the translation:"));
    m_language = new LanguageCtrl(this);
    m_language->SetMinSize(wxSize(300,-1));
    auto buttons = CreateButtonSizer(wxOK | wxCANCEL);

#ifdef __WXOSX__
    sizer->AddSpacer(10);
    sizer->Add(label, wxSizerFlags().Border());
    sizer->Add(m_language, wxSizerFlags().Expand().DoubleBorder(wxLEFT|wxRIGHT));
    sizer->Add(buttons, wxSizerFlags().Expand());
#else
    sizer->AddSpacer(10);
    sizer->Add(label, wxSizerFlags().DoubleBorder(wxLEFT|wxRIGHT));
    sizer->Add(m_language, wxSizerFlags().Expand().DoubleBorder(wxLEFT|wxRIGHT));
    sizer->Add(buttons, wxSizerFlags().Expand().Border());
#endif

    m_language->Bind(wxEVT_TEXT,     [=](wxCommandEvent&){ m_validatedLang = -1; });
    m_language->Bind(wxEVT_COMBOBOX, [=](wxCommandEvent&){ m_validatedLang = -1; });

    wxString lang = wxConfigBase::Get()->Read("/last_translation_lang", "");
    if (!lang.empty())
        SetLang(Language::TryParse(lang));

    Bind(wxEVT_UPDATE_UI,
        [=](wxUpdateUIEvent& e){ e.Enable(Validate()); },
        wxID_OK);

    SetSizerAndFit(sizer);
    CenterOnParent();
}

bool LanguageDialog::Validate()
{
    if (m_validatedLang == -1)
    {
        m_validatedLang = m_language->IsValid() ? 1 : 0;
    }

    return m_validatedLang == 1;
}

void LanguageDialog::EndModal(int retval)
{
    if (retval == wxID_OK)
    {
        wxConfigBase::Get()->Write("/last_translation_lang", GetLang().Code().c_str());
    }
    wxDialog::EndModal(retval);
}


void LanguageDialog::SetLang(const Language& lang)
{
    m_validatedLang = -1;
    m_language->SetLang(lang);
}
