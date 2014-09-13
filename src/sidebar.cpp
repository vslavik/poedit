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

#include "catalog.h"
#include "customcontrols.h"
#include "commentdlg.h"

#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#define SIDEBAR_BACKGROUND      wxColour("#EDF0F4")
#define GRAY_LINES_COLOR        wxColour(220,220,220)
#define GRAY_LINES_COLOR_DARK   wxColour(180,180,180)


class SidebarSeparator : public wxWindow
{
public:
    SidebarSeparator(wxWindow *parent)
        : wxWindow(parent, wxID_ANY),
          m_sides(SIDEBAR_BACKGROUND),
          m_center(GRAY_LINES_COLOR_DARK)
    {
        Bind(wxEVT_PAINT, &SidebarSeparator::OnPaint, this);
    }

    virtual wxSize DoGetBestSize() const
    {
        return wxSize(-1, 1);
    }

private:
    void OnPaint(wxPaintEvent&)
    {
        wxPaintDC dc(this);
        auto w = dc.GetSize().x;
        dc.GradientFillLinear(wxRect(0,0,15,1), m_sides, m_center);
        dc.GradientFillLinear(wxRect(15,0,w,1), m_center, m_sides);
    }

    wxColour m_sides, m_center;
};


class SidebarBlock
{
public:
    SidebarBlock(wxWindow *parent, const wxString& label)
    {
        m_sizer = new wxBoxSizer(wxVERTICAL);
        m_sizer->AddSpacer(15);
        if (!label.empty())
        {
            m_sizer->Add(new SidebarSeparator(parent),
                         wxSizerFlags().Expand().Border(wxBOTTOM|wxLEFT, 2));
            m_sizer->Add(new HeadingLabel(parent, label),
                         wxSizerFlags().Expand().DoubleBorder(wxLEFT|wxRIGHT));
        }
        m_innerSizer = new wxBoxSizer(wxVERTICAL);
        m_sizer->Add(m_innerSizer, wxSizerFlags(1).Expand().DoubleBorder(wxLEFT|wxRIGHT));
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

    virtual bool IsGrowable() const { return false; }

protected:
    wxSizer *m_innerSizer;
private:
    wxSizer *m_sizer;
};



class OldMsgidSidebarBlock : public SidebarBlock
{
public:
    OldMsgidSidebarBlock(wxWindow *parent)
          /// TRANSLATORS: "Previous" as in used in the past, now replaced with newer.
        : SidebarBlock(parent, _("Previous source text:"))
    {
        m_innerSizer->AddSpacer(2);
        m_innerSizer->Add(new ExplanationLabel(parent, _("The old source text (before it changed during an update) that the fuzzy translation corresponds to.")),
                     wxSizerFlags().Expand());
        m_innerSizer->AddSpacer(5);
        m_text = new AutoWrappingText(parent, "");
        m_innerSizer->Add(m_text, wxSizerFlags().Expand());
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
    AutoCommentSidebarBlock(wxWindow *parent)
        : SidebarBlock(parent, _("Notes for translators:"))
    {
        m_innerSizer->AddSpacer(5);
        m_comment = new AutoWrappingText(parent, "");
        m_innerSizer->Add(m_comment, wxSizerFlags().Expand());
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


class CommentSidebarBlock : public SidebarBlock
{
public:
    CommentSidebarBlock(wxWindow *parent)
        : SidebarBlock(parent, _("Comment:"))
    {
        m_innerSizer->AddSpacer(5);
        m_comment = new AutoWrappingText(parent, "");
        m_innerSizer->Add(m_comment, wxSizerFlags().Expand());
    }

    virtual bool ShouldShowForItem(CatalogItem *item) const
    {
        return item->HasComment();
    }

    void Update(CatalogItem *item) override
    {
        auto text = CommentDialog::RemoveStartHash(item->GetComment());
        text.Trim();
        m_comment->SetAndWrapLabel(text);
    }

private:
    AutoWrappingText *m_comment;
};


class AddCommentSidebarBlock : public SidebarBlock
{
public:
    AddCommentSidebarBlock(wxWindow *parent) : SidebarBlock(parent, "")
    {
    #ifdef __WXMSW__
        auto label = _("Add comment");
    #else
        auto label = _("Add Comment");
    #endif
        m_btn = new wxButton(parent, XRCID("menu_comment"), _("Add Comment"));
        m_innerSizer->AddStretchSpacer();
        m_innerSizer->Add(m_btn, wxSizerFlags().Right());
    }

    virtual bool IsGrowable() const { return true; }

    virtual bool ShouldShowForItem(CatalogItem*) const { return true; }

    void Update(CatalogItem *item) override
    {
    #ifdef __WXMSW__
        auto add = _("Add comment");
        auto edit = _("Edit comment");
    #else
        auto add = _("Add Comment");
        auto edit = _("Edit Comment");
    #endif
        m_btn->SetLabel(item->HasComment() ? edit : add);
    }

private:
    wxButton *m_btn;
};



Sidebar::Sidebar(wxWindow *parent)
    : wxPanel(parent, wxID_ANY),
      m_selectedItem(nullptr)
{
    SetBackgroundColour(SIDEBAR_BACKGROUND);
    Bind(wxEVT_PAINT, &Sidebar::OnPaint, this);
#ifdef __WXOSX__
    SetWindowVariant(wxWINDOW_VARIANT_SMALL);
#endif

    auto *topSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->SetMinSize(wxSize(300, -1));

    m_blocksSizer = new wxBoxSizer(wxVERTICAL);
    topSizer->Add(m_blocksSizer, wxSizerFlags(1).Expand().DoubleBorder(wxTOP|wxBOTTOM));

    m_topBlocksSizer = new wxBoxSizer(wxVERTICAL);
    m_bottomBlocksSizer = new wxBoxSizer(wxVERTICAL);

    m_blocksSizer->Add(m_topBlocksSizer, wxSizerFlags().Expand());
    m_blocksSizer->AddStretchSpacer();
    m_blocksSizer->Add(m_bottomBlocksSizer, wxSizerFlags().Expand());

    AddBlock(new OldMsgidSidebarBlock(this), Bottom);
    AddBlock(new AutoCommentSidebarBlock(this), Bottom);
    AddBlock(new CommentSidebarBlock(this), Bottom);
    AddBlock(new AddCommentSidebarBlock(this), Bottom);

    SetSizerAndFit(topSizer);

    SetSelectedItem(nullptr);
}


void Sidebar::AddBlock(SidebarBlock *block, BlockPos pos)
{
    m_blocks.emplace_back(block);

    auto sizer = (pos == Top) ? m_topBlocksSizer : m_bottomBlocksSizer;
    auto grow = (block->IsGrowable()) ? 1 : 0;
    sizer->Add(block->GetSizer(), wxSizerFlags(grow).Expand());
}


Sidebar::~Sidebar()
{
}


void Sidebar::SetSelectedItem(CatalogItem *item)
{
    m_selectedItem = item;
    RefreshContent();
}

void Sidebar::SetMultipleSelection()
{
    SetSelectedItem(nullptr);
}

void Sidebar::RefreshContent()
{
    for (auto& b: m_blocks)
        b->SetItem(m_selectedItem);

    Layout();
}

void Sidebar::SetUpperHeight(int size)
{
    int pos = GetSize().y - size;
#ifdef __WXOSX__
    pos += 4;
#else
    pos += 6;
#endif
    m_bottomBlocksSizer->SetMinSize(wxSize(-1, pos));
    Layout();
}


void Sidebar::OnPaint(wxPaintEvent&)
{
    wxPaintDC dc(this);

    dc.SetPen(wxPen(GRAY_LINES_COLOR));
#ifndef __WXMSW__
    dc.DrawLine(0, 0, 0, dc.GetSize().y-1);
#endif
#ifndef __WXOSX__
    dc.DrawLine(0, 0, dc.GetSize().x - 1, 0);
#endif
}
