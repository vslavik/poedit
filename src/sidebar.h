/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2014 Vaclav Slavik
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

#ifndef Poedit_sidebar_h
#define Poedit_sidebar_h

#include <memory>

#include <wx/panel.h>

class WXDLLIMPEXP_CORE wxSizer;
class WXDLLIMPEXP_CORE wxStaticText;

class CatalogItem;
class SidebarBlock;

/**
    Control showing Poedit's assistance sidebar.
    
    Contains TM suggestions, comments and possibly other auxiliary stuff.
 */
class Sidebar : public wxPanel
{
public:
    Sidebar(wxWindow *parent);
    ~Sidebar();

    /// Update selected item, if there's a single one. May be nullptr.
    void SetSelectedItem(CatalogItem *item);

    /// Tell the sidebar there's multiple selection.
    void SetMultipleSelection();

    /// Refreshes displayed content
    void RefreshContent();

private:
    CatalogItem *m_selectedItem;

    std::unique_ptr<SidebarBlock> m_oldMsgid;
    std::unique_ptr<SidebarBlock> m_autoComments;
    std::unique_ptr<SidebarBlock> m_comment;

    wxSizer *m_blocksSizer;
};

#endif // Poedit_sidebar_h
