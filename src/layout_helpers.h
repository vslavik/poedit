/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2025-2026 Vaclav Slavik
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

#ifndef Poedit_layout_helpers_h
#define Poedit_layout_helpers_h

#include "hidpi.h"

#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/sizer.h>


inline int AboveChoicePadding()
{
#ifdef __WXOSX__
    if (__builtin_available(macOS 26.0, *))
        return 0;
    else
        return 2;
#else
    return 0;
#endif
}

inline int UnderCheckboxIndent()
{
#if defined(__WXOSX__)
    if (__builtin_available(macOS 26.0, *))
        return 22;
    else
        return 20;
#elif defined(__WXMSW__)
    return PX(17);
#elif defined(__WXGTK__)
    return PX(25);
#else
    #error "Implement UnderCheckboxIndent() for your platform"
#endif
}


// Padding constants

#if defined(__WXOSX__)
    #define PADDING_OUTER          PX(20)
#elif defined(__WXGTK__)
    #define PADDING_OUTER          PX(12)
#else
    #define PADDING_OUTER          PX(12)
#endif


// Base class for windows that needs to use layout helpers.
template<typename Base>
class StandardLayout : public Base
{
protected:
    // generic forwarding ctor that adds initialization
    template <typename... Args>
    StandardLayout(Args&&... args) : Base(std::forward<Args>(args)...)
    {
        InitLayout();
    }

    wxBoxSizer *TopSizer() { return m_topSizer; }
    wxBoxSizer *ContentSizer() { return m_contentSizer; }

    void InitLayout()
    {
        m_topSizer = new wxBoxSizer(wxVERTICAL);
        m_contentSizer = new wxBoxSizer(wxVERTICAL);

        m_topSizer->Add(m_contentSizer, wxSizerFlags(1).Expand().Border(wxALL, PADDING_OUTER));
        this->SetSizer(m_topSizer);
    }

    void FitSizer()
    {
        this->SetSizerAndFit(m_topSizer);
    }

protected:
    // Outer sizer set for the entire window, with standard padding.
    wxBoxSizer *m_topSizer = nullptr;
    // Sizer to put the content into, inside the padding.
    wxBoxSizer *m_contentSizer = nullptr;
};


/**
    Common dialog for Poedit.

    Constructor of derived class must call PostInit().
 */
class StandardDialog : public StandardLayout<wxDialog>
{
protected:
    StandardDialog(wxWindow *parent, const wxString& title, int style = wxDEFAULT_DIALOG_STYLE)
        : StandardLayout<wxDialog>(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, style)
    {
    }

    // Helper for creating standard buttons in dialogs
    class Buttons
    {
    private:
        Buttons(StandardDialog *parent, wxStdDialogButtonSizer *sizer, long flags);

    public:
        Buttons& Add(wxButton *button);

        Buttons& Add(wxWindowID buttonId)
        {
            Add(new wxButton(m_parent, buttonId));
            return *this;
        }


        ~Buttons() { m_parent->RealizeButtons(); }

        Buttons(const Buttons&) = delete;
        Buttons& operator=(const Buttons&) = delete;

    private:
        wxStdDialogButtonSizer *m_sizer;
        StandardDialog *m_parent;

        friend class StandardDialog;
    };

    /**
        Create sizer with specified buttons.

        @a flags is OR-combination of wxOK, wxCANCEL etc. No buttons are created if 0.
     */
    Buttons CreateButtons(long flags = 0);

private:
    void RealizeButtons();

protected:
    wxStdDialogButtonSizer *m_buttonsSizer = nullptr;

    friend class Buttons;
};

#endif // Poedit_layout_helpers_h
