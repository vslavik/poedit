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
 *  Preferences settings dialog
 *
 */

#ifndef _PREFSDLG_H_
#define _PREFSDLG_H_

#include <wx/dialog.h>

#include "parser.h"
#include "chooselang.h"

class WXDLLEXPORT wxConfigBase;

/** Preferences dialog for setting user's identity, parsers and other
    global, catalog-independent settings.
 */
class PreferencesDialog : public wxDialog
{
    public:
        /// Ctor.
        PreferencesDialog(wxWindow *parent = NULL);

        /// Reads data from config/registry and fills dialog's controls.
        void TransferTo(wxConfigBase *cfg);

        /// Saves data from the dialog to config/registry.
        void TransferFrom(wxConfigBase *cfg);
            
    private:
        ParsersDB m_parsers;

    private:
        DECLARE_EVENT_TABLE()

#ifdef USE_TRANSMEM
        void OnTMAddLang(wxCommandEvent& event);
        void OnTMBrowseDbPath(wxCommandEvent& event);
        void OnTMGenerate(wxCommandEvent& event);
#endif

#if NEED_CHOOSELANG_UI
        void OnUILanguage(wxCommandEvent& event);
#endif
        void OnNewParser(wxCommandEvent& event);
        void OnEditParser(wxCommandEvent& event);
        void OnDeleteParser(wxCommandEvent& event);
        /// Called to launch dialog for editting parser properties.
        bool EditParser(int num);

        void OnChooseListFont(wxCommandEvent& event);
        void OnChooseTextFont(wxCommandEvent& event);
        void DoChooseFont(wxTextCtrl *nameField);
};


#endif // _PREFSDLG_H_
