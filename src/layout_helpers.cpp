/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2024-2025 Vaclav Slavik
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

#include "layout_helpers.h"

#include <wx/button.h>
#include <wx/sizer.h>


StandardDialog::Buttons StandardDialog::CreateButtons(long flags)
{
    m_buttonsSizer = new wxStdDialogButtonSizer();
    return Buttons(this, m_buttonsSizer, flags);
}


void StandardDialog::RealizeButtons()
{
    if (!m_buttonsSizer)
        return;

    m_buttonsSizer->Realize();

    // The following codes attempts to undo wrong logic from wxWidgets and size things correctly.
    // In particular, it removes any fixed padding on left/right sides of the button sizer.

    while (true)
    {
        auto item = m_buttonsSizer->GetItem((size_t)0);
        // remove any static spacers:
        if (item->IsSpacer() && item->GetProportion() == 0)
        {
            m_buttonsSizer->Remove(0);
            continue;
        }
        // and remove any left padding otherwise:
        item->SetFlag(item->GetFlag() & ~wxLEFT);
        break;
    }

    while (true)
    {
        auto item = m_buttonsSizer->GetItem(m_buttonsSizer->GetItemCount()-1);
        // remove any static spacers:
        if (item->IsSpacer() && item->GetProportion() == 0)
        {
            m_buttonsSizer->Remove((int)m_buttonsSizer->GetItemCount()-1);
            continue;
        }
        // and remove any left padding otherwise:
        item->SetFlag(item->GetFlag() & ~wxRIGHT);
        break;
    }

    m_topSizer->Add(m_buttonsSizer, wxSizerFlags().Expand().Border(wxLEFT|wxRIGHT|wxBOTTOM, PADDING_OUTER));
}


StandardDialog::Buttons::Buttons(StandardDialog *parent, wxStdDialogButtonSizer *sizer, long flags)
    : m_parent(parent), m_sizer(sizer)
{
    if (flags & wxOK)
        Add(wxID_OK);

    if (flags & wxCANCEL)
        Add(wxID_CANCEL);
}


StandardDialog::Buttons& StandardDialog::Buttons::Add(wxButton *button)
{
    const auto buttonId = button->GetId();

    switch (buttonId)
    {
        case wxID_OK:
        case wxID_YES:
            button->SetDefault();
            button->SetFocus();
            m_sizer->SetAffirmativeButton(button);
            m_parent->SetAffirmativeId(buttonId);
            break;

        case wxID_CANCEL:
        case wxID_NO:
            m_sizer->SetCancelButton(button);
            m_parent->SetEscapeId(buttonId);
            break;

        case wxID_DELETE:
            m_sizer->SetNegativeButton(button);
            break;

        default:
            m_sizer->AddButton(button);
            break;
    }

    return *this;
}
