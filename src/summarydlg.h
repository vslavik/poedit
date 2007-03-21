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
 *  Catalog update summary dialog
 *
 */

#ifndef _SUMMARYDLG_H_
#define _SUMMARYDLG_H_

#include <wx/string.h>
#include <wx/dialog.h>


/** This class provides simple dialog that displays list
  * of changes made in the catalog.
  */
class MergeSummaryDialog : public wxDialog
{
    public:
        MergeSummaryDialog(wxWindow *parent = NULL);
        ~MergeSummaryDialog();

        /** Reads data from catalog and fill dialog's controls.
            \param snew      list of strings that are new to the catalog
            \param sobsolete list of strings that no longer appear in the
                             catalog (as compared to catalog's state before
                             parsing sources).
         */
        void TransferTo(const wxArrayString& snew, 
                        const wxArrayString& sobsolete);
};



#endif // _SUMMARYDLG_H_
