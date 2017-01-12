/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2008-2017 Vaclav Slavik
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

#ifndef _ATTENTIONBAR_H_
#define _ATTENTIONBAR_H_

#include <wx/panel.h>

#include <vector>
#include <map>
#include <functional>

class WXDLLIMPEXP_FWD_CORE wxCheckBox;
class WXDLLIMPEXP_FWD_CORE wxStaticText;
class WXDLLIMPEXP_FWD_CORE wxStaticBitmap;
class WXDLLIMPEXP_FWD_CORE wxSizer;
class AutoWrappingText;

/// Message to be displayed in AttentionBar
class AttentionMessage
{
public:
    /// Kind of the message
    enum Kind
    {
        Warning,
        Question,
        Error
    };

    /// Information passed to the action callback
    struct ActionInfo
    {
        /// State of the (optional) checkbox
        bool checkbox;
    };

    /**
        Ctor.

        @param id   ID of the message. This ID must be globally unique within
                    the application. It is used to record this message's
                    status, i.e. if the user marked it as "don't show again"
                    etc.
        @param kind Kind of the message.
        @param text Text of the message; this should be kept reasonably short.

        @see AddToBlacklist
     */
    AttentionMessage(const wxString& id, Kind kind, const wxString& text)
        : m_id(id), m_kind(kind), m_text(text), m_explanation("") {}

    typedef std::function<void(ActionInfo info)> Callback;
    typedef std::function<void()> CallbackNoArgs;

    /**
        Adds an action button to the bar. By default, a close button is shown,
        this makes it possible to add custom buttons.

        @param label    Short label for the action.
        @param callback Function to call when the button is clicked.
     */
    void AddAction(const wxString& label, CallbackNoArgs callback)
        { AddActionWithInfo(label, [=](ActionInfo){ callback(); }); }

    /// Similarly to AddAction(), but uses callback that is passed ActionInfo
    void AddActionWithInfo(const wxString& label, Callback callback)
        { m_actions.push_back(std::make_pair(label, callback)); }

    /// Adds "Don't show again" action.
    void AddDontShowAgain();

    /// Set additional explanatory text
    void SetExplanation(const wxString& txt) { m_explanation = txt; }

    /// Add checkbox to the message
    void AddCheckbox(const wxString& label) { m_checkbox = label; }

    wxString m_id;
    Kind m_kind;
    wxString m_text;
    wxString m_explanation;
    wxString m_checkbox;

    typedef std::pair<wxString, Callback> Action;
    typedef std::vector<Action> Actions;
    Actions m_actions;

    /**
        Adds message with given ID to blacklist, i.e. it won't be shown
        ever again.
     */
    static void AddToBlacklist(const wxString& id);

    /// Returns true if @a id is on the blacklist
    static bool IsBlacklisted(const wxString& id);

    /// Returns true if this message is on the blacklist
    bool IsBlacklisted() const { return IsBlacklisted(m_id); }
};


/**
    Attention bar is a tooltip-colored bar displayed on top of the main
    window (a la Firefox or other browsers).
 */
class AttentionBar : public wxPanel
{
public:
    AttentionBar(wxWindow *parent);

    /**
        Shows the message
        (unless the user disallowed showing this particular message.
     */
    void ShowMessage(const AttentionMessage& msg);

    /// Hides currently shown message
    void HideMessage();

private:
    void OnClose(wxCommandEvent& event);
    void OnAction(wxCommandEvent& event);
    void OnPaint(wxPaintEvent& event);

private:
#ifndef __WXGTK__
    wxStaticBitmap *m_icon;
#endif
    AutoWrappingText *m_label;
    AutoWrappingText *m_explanation;
    wxCheckBox *m_checkbox;
    wxSizer *m_buttons;
    typedef std::map<wxObject*, AttentionMessage::Callback> ActionsMap;
    ActionsMap m_actions;

    DECLARE_EVENT_TABLE()
};

#endif // _ATTENTIONBAR_H_
