/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2017 Vaclav Slavik
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

#ifndef Poedit_languagectrl_h
#define Poedit_languagectrl_h

#include <wx/combobox.h>
#include <wx/dialog.h>

#include "language.h"

/// Control for editing languages nicely
class LanguageCtrl : public wxComboBox
{
public:
    LanguageCtrl();
    LanguageCtrl(wxWindow *parent, wxWindowID winid = wxID_ANY, Language lang = Language());

    void SetLang(const Language& lang);
    Language GetLang() const;

    bool IsValid() const
    {
        return GetLang().IsValid();
    }

#ifdef __WXOSX__
    int FindString(const wxString& s, bool bCase) const override;
    wxString GetString(unsigned int n) const override;
#endif

#ifdef __WXMSW__
    wxSize DoGetBestSize() const;
#endif

private:
    void Init(Language lang);

    bool m_inited;

#ifdef __WXOSX__
    struct impl;
    std::unique_ptr<impl> m_impl;
#endif

    DECLARE_DYNAMIC_CLASS(LanguageCtrl)
};


/// A dialog for choosing language for a (new) catalog.
class LanguageDialog : public wxDialog
{
public:
    LanguageDialog(wxWindow *parent);

    void SetLang(const Language& lang);

    Language GetLang() const { return m_language->GetLang(); }

    virtual bool Validate();
    virtual void EndModal(int retval);

    static Language GetLastChosen();
    static void SetLastChosen(Language lang);

private:
    LanguageCtrl *m_language;
    int m_validatedLang;
};

#endif // Poedit_languagectrl_h
