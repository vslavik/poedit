/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2017 Vaclav Slavik
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

#ifndef _PROGRESSINFO_H_
#define _PROGRESSINFO_H_

#include <wx/string.h>

class WXDLLIMPEXP_FWD_CORE wxWindow;
class WXDLLIMPEXP_FWD_CORE wxDialog;
class WXDLLIMPEXP_FWD_CORE wxWindowDisabler;

/// This class displays fancy progress dialog.
class ProgressInfo
{
    public:
            ProgressInfo(wxWindow *parent, const wxString& title);
            ~ProgressInfo();

            /// Hides temporarily
            void Hide();

            /// Shows again after having been hidden with Hide()
            void Show();

            /// Hides for good, called when all is done
            void Done();

            /// Sets gauge's values interval to <0..limit).
            void SetGaugeMax(int limit);

            /** Updates the gauge: increments it by specified delta.
                \param increment the delta
                \return false if user cancelled operation, true otherwise
             */
            bool UpdateGauge(int increment = +1);

            /// Resets the gauge to given \a value.
            void ResetGauge(int value = 0);

            /// Indicate indeterminate progress
            void PulseGauge();

            /// Updates informative message.
            void UpdateMessage(const wxString& text);
            
            /// Returns whether the user cancelled operation.
            bool Cancelled() const { return m_cancelled; }
            
    private:
            wxDialog *m_dlg;
            bool m_cancelled;
            wxWindowDisabler *m_disabler;
};



#endif // _PROGRESSINFO_H_
