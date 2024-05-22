/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2021-2024 Vaclav Slavik
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

#ifndef Poedit_custom_notebook_h
#define Poedit_custom_notebook_h

#include <wx/simplebook.h>

#ifndef __WXOSX__
    #include <wx/notebook.h>
#endif


/// Possible styles of SegmentedNotebook
enum class SegmentStyle
{
    /// Inlined (e.g. within editing area) small switcher
    SmallInline,
    /// Large, covering full width of the notebook
    LargeFullWidth,
    /// Xcode-like sidebar panels switching buttons
    SidebarPanels
};

#if defined(__WXOSX__) || defined(__WXMSW__)
    #define HAS_SEGMENTED_NOTEBOOK
#endif


#ifdef HAS_SEGMENTED_NOTEBOOK

/**
    wxNotebook with nicer tabs.

    Uses NSSegmentedControl on macOS and custom, modern-looking tabs on Windows.
 */
class SegmentedNotebook : public wxSimplebook
{
public:
    SegmentedNotebook(wxWindow *parent, SegmentStyle style);

    int ChangeSelection(size_t page) override;
    bool InsertPage(size_t n, wxWindow *page, const wxString& text, bool bSelect = false, int imageId = NO_IMAGE) override;
    bool DeleteAllPages() override;

    bool SetBackgroundColour(const wxColour& clr) override;

    /// Returns sizer in the tabs portions of the control where custom controls can be added or NULL
    wxSizer *GetTabsExtensibleArea() const;

protected:
    wxWindow *DoRemovePage(size_t page) override;
    int DoSetSelection(size_t n, int flags) override;

private:
    class TabsIface;
    class SegmentedControlTabs;
    class ButtonTabs;
    class TabButton;

    TabsIface *m_tabs;
};

#else

class SegmentedNotebook : public wxNotebook
{
public:
    SegmentedNotebook(wxWindow *parent, SegmentStyle style);
    wxSizer *GetTabsExtensibleArea() const { return nullptr; }
};

#endif


#endif // Poedit_custom_notebook_h
