/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2001-2008 Vaclav Slavik
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
 *  Search frame
 *
 */

#ifndef _FINDFRAME_H_
#define _FINDFRAME_H_

#include <wx/frame.h>

class PoeditListCtrl;
class WXDLLEXPORT wxButton;
class Catalog;

/** FindFrame is small dialog frame that contains controls for searching
    in content of EditorFrame's wxListCtrl object and associated Catalog
    instance.

    This class assumes that list control's user data contains index
    into the catalog.
 */
class FindFrame : public wxFrame
{
    public:
        /** Ctor.
            \param parent  Parent frame, FindFrame will float on it
            \param list    List control to search in
            \param catalog Catalog to search in
         */
        FindFrame(wxWindow *parent, PoeditListCtrl *list, Catalog *c,
                  wxTextCtrl *textCtrlOrig, wxTextCtrl *textCtrlTrans,
                  wxTextCtrl *textCtrlComments, wxTextCtrl *textCtrlAutoComments);
        ~FindFrame();

        /** Resets the search to starting position and changes
            the catalog in use. Called by EditorFrame when the user
            reloads catalog.
         */
        void Reset(Catalog *c);

    private:
        void OnCancel(wxCommandEvent &event);
        void OnClose(wxCloseEvent &event);
        void OnPrev(wxCommandEvent &event);
        void OnNext(wxCommandEvent &event);
        void OnTextChange(wxCommandEvent &event);
        void OnCheckbox(wxCommandEvent &event);
        bool DoFind(int dir);
        DECLARE_EVENT_TABLE()

        PoeditListCtrl *m_listCtrl;
        Catalog *m_catalog;
        int m_position;
        wxButton *m_btnPrev, *m_btnNext;
        wxTextCtrl *m_textCtrlOrig, *m_textCtrlTrans;
        wxTextCtrl *m_textCtrlComments, *m_textCtrlAutoComments;

        // NB: this is static so that last search term is remembered
        static wxString ms_text;
};


#endif // _FINDFRAME_H_
