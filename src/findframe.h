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

#ifndef _FINDFRAME_H_
#define _FINDFRAME_H_

#include "edlistctrl.h"

#include <wx/frame.h>
#include <wx/weakref.h>

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxCheckBox;
class WXDLLIMPEXP_FWD_CORE wxChoice;
class WXDLLIMPEXP_FWD_CORE wxTextCtrl;

class Catalog;
class EditingArea;
class PoeditFrame;

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
            \param owner   Parent frame, FindFrame will float on it
            \param list    List control to search in
            \param catalog Catalog to search in
         */
        FindFrame(PoeditFrame *owner,
                  PoeditListCtrl *list, EditingArea *editingArea,
                  const CatalogPtr& c);
        ~FindFrame();

        /** Resets the search to starting position and changes
            the catalog in use. Called by EditorFrame when the user
            reloads catalog.
         */
        void Reset(const CatalogPtr& c);

        void FindPrev();
        void FindNext();

        void ShowForFind();
        void ShowForReplace();

        bool HasText() const { return !ms_text.empty(); }

    private:
        void UpdateButtons();
        void DoShowFor(int mode);
        void OnClose(wxCommandEvent &event);
        void OnModeChanged();
        void OnPrev(wxCommandEvent &event);
        void OnNext(wxCommandEvent &event);
        void OnTextChange(wxCommandEvent &event);
        void OnCheckbox(wxCommandEvent &event);
        void OnReplace(wxCommandEvent &event);
        void OnReplaceAll(wxCommandEvent &event);
        bool DoFind(int dir);
        bool DoReplaceInItem(CatalogItemPtr item);

        PoeditFrame *m_owner;
        wxChoice *m_mode;
        wxTextCtrl *m_searchField, *m_replaceField;
        wxCheckBox *m_ignoreCase, *m_wrapAround, *m_wholeWords,
                   *m_findInOrig, *m_findInTrans, *m_findInComments;

        wxWeakRef<PoeditListCtrl> m_listCtrl;
        wxWeakRef<EditingArea> m_editingArea;
        CatalogPtr m_catalog;
        int m_position;
        CatalogItemPtr m_lastItem;
        wxButton *m_btnClose, *m_btnReplaceAll, *m_btnReplace, *m_btnPrev, *m_btnNext;

        // NB: this is static so that last search term is remembered
        static wxString ms_text;
};


#endif // _FINDFRAME_H_
