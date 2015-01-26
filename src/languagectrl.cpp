/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2013-2015 Vaclav Slavik
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

#include "hidpi.h"

#include <wx/config.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#ifdef __WXOSX__
extern "C" {
#import "NSObject+REResponder.h"
}
#endif

IMPLEMENT_DYNAMIC_CLASS(LanguageCtrl, wxComboBox)

LanguageCtrl::LanguageCtrl(wxWindow *parent, wxWindowID winid, Language lang)
    : wxComboBox(parent, winid)
{
    Init(lang);
}

void LanguageCtrl::Init(Language lang)
{
    SetHint(_("Language Code or Name (e.g. en_GB)"));

    // wxGTK must have the value set before autocompletion to avoid annoying
    // popups in some (hard to determine) cases.
#ifdef __WXGTK__
    if (lang.IsValid())
        SetValue(lang.FormatForRoundtrip());
#endif

#ifdef __WXOSX__
    for (auto x: Language::AllFormattedNames())
        Append(x);
    NSComboBox *cb = (NSComboBox*) GetHandle();
    [cb setCompletes:YES];
    if (floor(NSAppKitVersionNumber) > NSAppKitVersionNumber10_7)
    {
        // default completion is case-sensitive, we'd rather be case-insensitive, so plug in
        // customized completedString: implementation
        RESetBlock(cb.cell, @selector(completedString:), NO, nil, ^(NSComboBox *receiver, NSString *string) {
            for (NSString *item in receiver.objectValues) {
                if ([item compare:string
                          options:NSCaseInsensitiveSearch|NSDiacriticInsensitiveSearch
                            range:NSMakeRange(0, std::min([item length], [string length]))
                           locale:[NSLocale currentLocale]] == NSOrderedSame)
                    return item;
            }
            return (NSString*)nil;
        });
    }
#else
    static wxArrayString choices;
    if (choices.empty())
    {
        for (auto x: Language::AllFormattedNames())
            choices.push_back(x);
    }
    Set(choices);
    AutoComplete(choices);
#endif

    m_inited = true;

    // ...but wxMSW requires the opposite, otherwise the text wouldn't appear.
#ifndef __WXGTK__
    if (lang.IsValid())
        SetValue(lang.FormatForRoundtrip());
#endif
}

void LanguageCtrl::SetLang(const Language& lang)
{
    if (!m_inited)
        Init(lang);
    else
        SetValue(lang.FormatForRoundtrip());
}

Language LanguageCtrl::GetLang() const
{
    return Language::TryParse(GetValue().ToStdWstring());
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
    wxString langcode = wxConfigBase::Get()->Read("/last_translation_lang", "");
    Language lang;
    if (!langcode.empty())
        lang = Language::TryParse(langcode.ToStdWstring());

    auto sizer = new wxBoxSizer(wxVERTICAL);

    auto label = new wxStaticText(this, wxID_ANY, _("Language of the translation:"));
    m_language = new LanguageCtrl(this, wxID_ANY, lang);
    m_language->SetMinSize(wxSize(PX(300),-1));
    auto buttons = CreateButtonSizer(wxOK | wxCANCEL);

#ifdef __WXOSX__
    sizer->AddSpacer(PX(10));
    sizer->Add(label, wxSizerFlags().PXBorderAll());
    sizer->Add(m_language, wxSizerFlags().Expand().PXDoubleBorder(wxLEFT|wxRIGHT));
    sizer->Add(buttons, wxSizerFlags().Expand());
#else
    sizer->AddSpacer(PX(10));
    sizer->Add(label, wxSizerFlags().PXDoubleBorder(wxLEFT|wxRIGHT));
    sizer->Add(m_language, wxSizerFlags().Expand().PXDoubleBorder(wxLEFT|wxRIGHT));
    sizer->Add(buttons, wxSizerFlags().Expand().PXBorderAll());
#endif

    m_language->Bind(wxEVT_TEXT,     [=](wxCommandEvent& e){ m_validatedLang = -1; e.Skip(); });
    m_language->Bind(wxEVT_COMBOBOX, [=](wxCommandEvent& e){ m_validatedLang = -1; e.Skip(); });

    Bind(wxEVT_UPDATE_UI,
        [=](wxUpdateUIEvent& e){ e.Enable(Validate()); },
        wxID_OK);

    SetSizerAndFit(sizer);
    CenterOnParent();

    m_language->SetFocus();
#ifdef __WXOSX__
    // Workaround wx bug: http://trac.wxwidgets.org/ticket/9521
    m_language->SelectAll();
#endif
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
