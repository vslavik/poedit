/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2001-2017 Vaclav Slavik
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

#ifndef _COMMENTDLG_H_
#define _COMMENTDLG_H_

#include <wx/dialog.h>
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;

/** CommentDialog is a very simple dialog that lets the user edit
    catalog comments. Comment consists of one or more lines that
    begin with the '#' character. The user is presented with more
    user friendly representation with '#' removed. 
 */
class CommentDialog : public wxDialog
{
    public:
        /// Returns the given comment without the leading "# "
        static wxString RemoveStartHash(const wxString& comment);

        /// Returns the given comment with the leading "# " added
        static wxString AddStartHash(const wxString& comment);

        /** Ctor. 
            \param parent  Parent frame, FindFrame will float on it
            \param comment Initial value of comment
         */
        CommentDialog(wxWindow *parent, const wxString& comment);
        
        /// Returns the content of comment edit field.
        wxString GetComment() const;

    private:
        wxTextCtrl *m_text;

        void OnClear(wxCommandEvent& event);
        DECLARE_EVENT_TABLE()
};


#endif // _FINDFRAME_H_
