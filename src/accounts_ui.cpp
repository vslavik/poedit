/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023 Vaclav Slavik
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

#include "accounts_ui.h"

#ifdef HAVE_HTTP_CLIENT

#include "customcontrols.h"
#include "crowdin_gui.h"

#include <wx/sizer.h>


AccountsPanel::AccountsPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY)
{
    wxSizer *sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);

    m_crowdin = new CrowdinLoginPanel(this);
    sizer->Add(m_crowdin, wxSizerFlags(1).Expand());
}


void AccountsPanel::InitializeAfterShown()
{
    m_crowdin->EnsureInitialized();
}

#endif // #ifdef HAVE_HTTP_CLIENT
