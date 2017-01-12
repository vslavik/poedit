/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2017 Vaclav Slavik
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

#include "custom_buttons.h"

#include "colorscheme.h"
#include "hidpi.h"
#include "str_helpers.h"
#include "utility.h"

#ifdef __WXMSW__
#include <wx/dc.h>
#include <wx/graphics.h>
#include <wx/scopedptr.h>
#include <wx/msw/wrapgdip.h>
#include <wx/msw/uxtheme.h>
#endif


#ifdef __WXOSX__

#import <AppKit/AppKit.h>
#include "StyleKit.h"

// ---------------------------------------------------------------------
// SwitchButton
// ---------------------------------------------------------------------

@interface POSwitchButton : NSButton

@property NSColor* onColor;
@property NSColor* labelOffColor;
@property SwitchButton *parent;
@property double animationPosition;

@end

@implementation POSwitchButton

- (id)initWithLabel:(NSString*)label
{
    self = [super init];
    if (self)
    {
        self.title = label;
        self.bezelStyle = NSSmallSquareBezelStyle;
        self.buttonType = NSOnOffButton;
        self.font = [NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]];
        self.animationPosition = 0.0;
        self.onColor = [NSColor colorWithCalibratedRed:0.302 green:0.847 blue:0.396 alpha:1.0];
        self.labelOffColor = [NSColor colorWithCalibratedRed:0 green:0 blue:0 alpha:1.0];
    }
    return self;
}

- (void)sizeToFit
{
    [super sizeToFit];
    NSSize size = self.frame.size;
    size.width += 32;
    [self setFrameSize:size];
}

- (void)drawRect:(NSRect)dirtyRect
{
    #pragma unused(dirtyRect)
    [StyleKit drawSwitchButtonWithFrame:self.bounds
                                onColor:self.onColor
                          labelOffColor:self.labelOffColor
                                  label:self.title
                         togglePosition:self.animationPosition];
}

- (void)setState:(NSInteger)state
{
    [super setState:state];
    self.animationPosition = (state == NSOnState) ? 1.0 : 0.0;
}

- (void)controlAction:(id)sender
{
    #pragma unused(sender)
    self.animationPosition = (self.state == NSOnState) ? 1.0 : 0.0;
    _parent->SendToggleEvent();
}

@end


class SwitchButton::impl
{
public:
    impl(SwitchButton *parent, const wxString& label)
    {
        m_view = [[POSwitchButton alloc] initWithLabel:str::to_NS(label)];
        m_view.parent = parent;
    }

    NSView *View() const { return m_view; }

    void SetColors(const wxColour& on, const wxColour& offLabel)
    {
        m_view.onColor = on.OSXGetNSColor();
        m_view.labelOffColor = offLabel.OSXGetNSColor();
    }

    void SetValue(bool value)
    {
        m_view.state = value ? NSOnState : NSOffState;
    }

    bool GetValue() const
    {
        return m_view.state == NSOnState;
    }

private:
    POSwitchButton *m_view;
};


SwitchButton::SwitchButton(wxWindow *parent, wxWindowID winid, const wxString& label)
{
    m_impl.reset(new impl(this, label));

    wxNativeWindow::Create(parent, winid, m_impl->View());
}

SwitchButton::~SwitchButton()
{
}

void SwitchButton::SetColors(const wxColour& on, const wxColour& offLabel)
{
    m_impl->SetColors(on, offLabel);
}

void SwitchButton::SetValue(bool value)
{
    m_impl->SetValue(value);
}

bool SwitchButton::GetValue() const
{
    return m_impl->GetValue();
}

void SwitchButton::SendToggleEvent()
{
    wxCommandEvent event(wxEVT_TOGGLEBUTTON, m_windowId);
    event.SetInt(GetValue());
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}


#else // !__WXOSX__


SwitchButton::SwitchButton(wxWindow *parent, wxWindowID winid, const wxString& label)
{
    long style = wxBU_EXACTFIT;
#ifdef __WXMSW__
    style |= wxNO_BORDER;
#endif

    wxToggleButton::Create(parent, winid, label, wxDefaultPosition, wxDefaultSize, style);

#ifdef __WXMSW__
    SetFont(GetFont().Bold());
    SetBackgroundColour(parent->GetBackgroundColour());
    MakeOwnerDrawn();
    Bind(wxEVT_LEFT_DOWN, &SwitchButton::OnMouseClick, this);
#endif
}

void SwitchButton::SetColors(const wxColour& on, const wxColour& offLabel)
{
    (void)on;
    (void)offLabel;
#ifdef __WXMSW__
    m_clrOn = on;
    m_clrOffLabel = offLabel;
#endif
}

#ifdef __WXMSW__

void SwitchButton::OnMouseClick(wxMouseEvent& e)
{
    // normal click handling moves focus to the switch (which is a button
    // underneath), which we'd rather not do
    SetValue(!GetValue());

    // we need to send the event, because SetValue() doesn't
    wxCommandEvent event(wxEVT_TOGGLEBUTTON, GetId());
    event.SetInt(GetValue());
    event.SetEventObject(this);
    ProcessCommand(event);
}


wxSize SwitchButton::DoGetBestSize() const
{
    auto size = wxToggleButton::DoGetBestSize();
    size.x += PX(38);
    size.y = PX(20);
    return size;
}


bool SwitchButton::MSWOnDraw(WXDRAWITEMSTRUCT *wxdis)
{
    LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT) wxdis;
    HDC hdc = lpDIS->hDC;

    UINT state = lpDIS->itemState;
    if (GetNormalState() == State_Pressed)
        state |= ODS_SELECTED;
    const bool toggled = state & ODS_SELECTED;

    wxRect rect(lpDIS->rcItem.left, lpDIS->rcItem.top, lpDIS->rcItem.right - lpDIS->rcItem.left, lpDIS->rcItem.bottom - lpDIS->rcItem.top);

    wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromNativeHDC(hdc));
    gc->EnableOffset(false);

    if (toggled)
    {
        gc->SetBrush(m_clrOn);
        gc->SetPen(wxPen(m_clrOn.ChangeLightness(95), PX(2)));
    }
    else
    {
        gc->SetBrush(GetBackgroundColour());
        gc->SetPen(wxPen(m_clrOffLabel, PX(2)));
    }

    wxRect switchRect(rect.GetRight() - PX(44), 0, PX(44), wxMin(PX(20), rect.GetHeight()));
    switchRect.CenterIn(rect, wxVERTICAL);
    switchRect.Deflate(PX(2));

    double radius = switchRect.height / 2.0;
    gc->DrawRoundedRectangle(switchRect.x, switchRect.y, switchRect.width, switchRect.height, radius);

    if (toggled)
    {
        gc->SetPen(GetBackgroundColour());
        gc->SetBrush(GetBackgroundColour());
    }
    else
    {
        gc->SetPen(wxPen(m_clrOffLabel, PX(1)));
        gc->SetBrush(m_clrOffLabel.ChangeLightness(105));
    }

    double position = toggled ? 1.0 : 0.0;
    wxRect dotRect(switchRect);
    dotRect.Deflate(PX(4));
    dotRect.SetLeft(dotRect.GetLeft() + position * (dotRect.GetWidth() - dotRect.GetHeight()));
    dotRect.SetWidth(dotRect.GetHeight());
    gc->DrawEllipse(dotRect.x, dotRect.y, dotRect.width, dotRect.height);

    gc->SetFont(GetFont(), toggled ? m_clrOn : m_clrOffLabel);
    double textw, texth, descent;
    gc->GetTextExtent(GetLabel(), &textw, &texth, &descent);
    texth += descent;
    wxCoord textpos = switchRect.y + (switchRect.height - texth) / 2 + PX(0.5);
    gc->DrawText(GetLabel(), rect.x, textpos);

    // draw the focus rectangle if we need it
    if ((state & ODS_FOCUS) && !(state & ODS_NOFOCUSRECT))
    {
        RECT r = {rect.x, textpos, rect.x + (int)textw, textpos + (int)texth};
        DrawFocusRect(hdc, &r);
    }

    return true;
}

#endif // __WXMSW__

#endif // !__WXOSX__

// ---------------------------------------------------------------------
// TranslucentButton
// ---------------------------------------------------------------------

#ifdef __WXOSX__

@interface POTranslucentButton : NSButton

@property TranslucentButton *parent;

@end

@implementation POTranslucentButton

- (id)initWithLabel:(NSString*)label
{
    self = [super init];
    if (self)
    {
        self.title = label;
        self.bezelStyle = NSSmallSquareBezelStyle;
        self.buttonType = NSMomentaryPushInButton;
        self.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    }
    return self;
}

- (void)sizeToFit
{
    NSSize size = self.attributedTitle.size;
    size.width += 28;
    size.height = 24;
    [self setFrameSize:size];
}

- (void)drawRect:(NSRect)dirtyRect
{
    #pragma unused(dirtyRect)
    [StyleKit drawTranslucentButtonWithFrame:self.bounds label:[self title] pressed:[self isHighlighted]];
}

- (void)controlAction:(id)sender
{
    #pragma unused(sender)
    wxCommandEvent event(wxEVT_BUTTON, _parent->GetId());
    event.SetEventObject(_parent);
    _parent->ProcessWindowEvent(event);
}

@end

TranslucentButton::TranslucentButton(wxWindow *parent, wxWindowID winid, const wxString& label)
{
    POTranslucentButton *view = [[POTranslucentButton alloc] initWithLabel:str::to_NS(label)];
    view.parent = this;
    wxNativeWindow::Create(parent, winid, view);
}


#else // !__WXOSX__


TranslucentButton::TranslucentButton(wxWindow *parent, wxWindowID winid, const wxString& label)
{
    wxButton::Create(parent, winid, label);
#ifdef __WXMSW__
    if (IsWindows10OrGreater())
        SetBackgroundColour(ColorScheme::GetBlendedOn(::Color::TranslucentButton, parent));
#endif
}

#endif // !__WXOSX__
