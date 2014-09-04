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

#include "sidebar.h"

#include "customcontrols.h"
#include "catalog.h"

#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>


class SidebarBlock
{
public:
    SidebarBlock(wxWindow *parent, const wxString& label)
    {
        m_sizer = new wxBoxSizer(wxVERTICAL);
        m_sizer->AddSpacer(15);
        m_sizer->Add(new HeadingLabel(parent, label),
                     wxSizerFlags().Expand());
    }
    virtual ~SidebarBlock() {}

    wxSizer *GetSizer() const { return m_sizer; }

    void Show(bool show)
    {
        m_sizer->ShowItems(show);
    }

    void SetItem(CatalogItem *item)
    {
        if (!item)
        {
            Show(false);
            return;
        }

        bool use = ShouldShowForItem(item);
        if (use)
            Update(item);
        Show(use);
    }

    virtual bool ShouldShowForItem(CatalogItem *item) const = 0;

    virtual void Update(CatalogItem *item) = 0;

protected:
    wxSizer *m_sizer;
};



class OldMsgidSidebarBlock : public SidebarBlock
{
public:
    OldMsgidSidebarBlock(wxWindow *parent, const wxString& label)
        : SidebarBlock(parent, label)
    {
        m_sizer->AddSpacer(2);
        m_sizer->Add(new ExplanationLabel(parent, _("The old source text (before it changed during an update) that the fuzzy translation corresponds to.")),
                     wxSizerFlags().Expand());
        m_sizer->AddSpacer(5);
        m_text = new AutoWrappingText(parent, "");
        m_sizer->Add(m_text, wxSizerFlags().Expand());
    }

    virtual bool ShouldShowForItem(CatalogItem *item) const
    {
        return item->HasOldMsgid();
    }

    void Update(CatalogItem *item) override
    {
        auto txt = wxJoin(item->GetOldMsgid(), ' ', '\0');
        m_text->SetAndWrapLabel(txt);
    }

private:
    AutoWrappingText *m_text;
};


class AutoCommentSidebarBlock : public SidebarBlock
{
public:
    AutoCommentSidebarBlock(wxWindow *parent, const wxString& label)
        : SidebarBlock(parent, label)
    {
        m_sizer->AddSpacer(5);
        m_comment = new AutoWrappingText(parent, "");
        m_sizer->Add(m_comment, wxSizerFlags().Expand());
    }

    virtual bool ShouldShowForItem(CatalogItem *item) const
    {
        return item->HasAutoComments();
    }

    void Update(CatalogItem *item) override
    {
        auto comment = wxJoin(item->GetAutoComments(), ' ', '\0');
        if (comment.StartsWith("TRANSLATORS:") || comment.StartsWith("translators:"))
        {
            comment.Remove(0, 12);
            if (!comment.empty() && comment[0] == ' ')
                comment.Remove(0, 1);
        }
        m_comment->SetAndWrapLabel(comment);
    }

private:
    AutoWrappingText *m_comment;
};



Sidebar::Sidebar(wxWindow *parent)
    : wxPanel(parent, wxID_ANY),
      m_selectedItem(nullptr)
{
    auto *topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize(wxSize(300, -1));

    m_blocksSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_blocksSizer, wxSizerFlags(1).Expand().DoubleBorder());

    m_blocksSizer->AddStretchSpacer();

    /// TRANSLATORS: "Previous" as in used in the past, now replaced with newer.
    m_oldMsgid.reset(new OldMsgidSidebarBlock(this, _("Previous source text:")));
    m_blocksSizer->Add(m_oldMsgid->GetSizer(), wxSizerFlags().Expand());

    m_autoComments.reset(new AutoCommentSidebarBlock(this, _("Notes for translators:")));
    m_blocksSizer->Add(m_autoComments->GetSizer(), wxSizerFlags().Expand());

    SetSizerAndFit(topSizer);

    SetSelectedItem(nullptr);
}

Sidebar::~Sidebar()
{
}


void Sidebar::SetSelectedItem(CatalogItem *item)
{
    m_selectedItem = item;

    m_oldMsgid->SetItem(m_selectedItem);
    m_autoComments->SetItem(m_selectedItem);

    Layout();
}

void Sidebar::SetMultipleSelection()
{
    SetSelectedItem(nullptr);
}
