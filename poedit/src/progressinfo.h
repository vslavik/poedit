
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      progressinfo.h
    
      Shows progress of lengthy operation
    
      (c) Vaclav Slavik, 2000

*/


#ifdef __GNUG__
#pragma interface
#endif

#ifndef _PROGRESSINFO_H_
#define _PROGRESSINFO_H_

class wxString;
class wxDialog;
class wxWindowDisabler;

/// This class displays fancy progress dialog.
class ProgressInfo
{
    public:
            ProgressInfo();
            ~ProgressInfo();
 
            /// Sets task's title, i.e. dialog's caption.
            void SetTitle(const wxString& text);

            /// Sets gauge's values interval to <0..limit).
            void SetGaugeMax(int limit);

            /** Updates the gauge: increments it by specified delta.
                \param increment the delta
                \return false if user cancelled operation, true otherwise
             */
            void UpdateGauge(int increment);

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
