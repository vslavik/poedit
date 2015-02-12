/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2001-2015 Vaclav Slavik
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

#ifndef _FINDFRAME_H_
#define _FINDFRAME_H_

#include "edlistctrl.h"

#include <wx/dialog.h>
#include <wx/weakref.h>

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxCheckBox;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;

class Catalog;

/** FindFrame is small dialog frame that contains controls for searching
    in content of EditorFrame's wxListCtrl object and associated Catalog
    instance.

    This class assumes that list control's user data contains index
    into the catalog.
 */
class FindFrame : public wxDialog
{
    public:
        /** Ctor.
            \param parent  Parent frame, FindFrame will float on it
            \param list    List control to search in
            \param catalog Catalog to search in
         */
        FindFrame(wxWindow *parent, PoeditListCtrl *list, const CatalogPtr& c,
                  wxTextCtrl *textCtrlOrig, wxTextCtrl *textCtrlTrans);
        ~FindFrame();

        /// Returns singleton instance if it exists. Updates the catalog
        /// if it differs from the current one.
        static FindFrame *Get(PoeditListCtrl *list, const CatalogPtr& forCatalog);

        static void NotifyParentDestroyed(PoeditListCtrl *list, const CatalogPtr& forCatalog);

        /** Resets the search to starting position and changes
            the catalog in use. Called by EditorFrame when the user
            reloads catalog.
         */
        void Reset(const CatalogPtr& c);

        void FocusSearchField();

        void FindPrev();
        void FindNext();

    private:
        void OnClose(wxCommandEvent &event);
        void OnPrev(wxCommandEvent &event);
        void OnNext(wxCommandEvent &event);
        void OnTextChange(wxCommandEvent &event);
        void OnCheckbox(wxCommandEvent &event);
        bool DoFind(int dir);

        wxTextCtrl *m_textField;
        wxCheckBox *m_caseSensitive, *m_fromFirst, *m_wholeWords,
                   *m_findInOrig, *m_findInTrans, *m_findInComments,
                   *m_findInAutoComments;

        wxWeakRef<PoeditListCtrl> m_listCtrl;
        CatalogPtr m_catalog;
        int m_position;
        wxButton *m_btnClose, *m_btnPrev, *m_btnNext;
        wxTextCtrl *m_textCtrlOrig, *m_textCtrlTrans;

        // NB: this is static so that last search term is remembered
        static wxString ms_text;

        static FindFrame *ms_singleton;
};


#endif // _FINDFRAME_H_
