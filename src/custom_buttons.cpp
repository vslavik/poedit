/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2026 Vaclav Slavik
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

#include <wx/artprov.h>

#ifdef __WXMSW__
#include <wx/dc.h>
#include <wx/graphics.h>
#include <wx/scopedptr.h>
#include <wx/msw/dcclient.h>
#include <wx/msw/wrapgdip.h>
#include <wx/msw/uxtheme.h>
#endif

#ifdef __WXGTK3__
#include <gtk/gtk.h>
#endif

#ifdef __WXOSX__

#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

#include "StyleKit.h"

// ---------------------------------------------------------------------
// ActionButton
// ---------------------------------------------------------------------

@interface POActionButton : NSButton

@property wxWindow *parent;
@property NSString *heading;
@property BOOL mouseHover;

@end

@implementation POActionButton

- (id)initWithLabel:(NSString*)label heading:(NSString*)heading
{
    self = [super init];
    if (self)
    {
        self.title = label;
        self.heading = heading;
        self.buttonType = NSButtonTypeMomentaryPushIn;
        self.bezelStyle = NSBezelStyleTexturedRounded;
        self.showsBorderOnlyWhileMouseInside = YES;
        self.mouseHover = NO;
    }
    return self;
}

- (void)sizeToFit
{
    [super sizeToFit];
    NSSize size = self.frame.size;
    size.height = 48;
    if (self.image)
        size.width += 32;
    [self setFrameSize:size];
}

- (void)drawRect:(NSRect)dirtyRect
{
    #pragma unused(dirtyRect)
    NSColor *bg = NSColor.clearColor;
    if (self.mouseHover)
    {
        NSColor *highlight = [self.window.backgroundColor colorWithSystemEffect:NSColorSystemEffectRollover];
        // use only lighter version of the highlight by blending with the background
        bg = [highlight colorWithAlphaComponent:0.2];
    }
    [StyleKit drawActionButtonWithFrame:self.bounds
                            buttonColor:bg
                                hasIcon:self.image != nil
                                  label:self.heading
                            description:self.title];

    // unlike normal drawing methods, NSButtonCell's drawImage supports template images
    if (self.image)
        [self.cell drawImage:self.image withFrame:NSMakeRect(NSMinX(self.bounds) + 18, NSMinY(self.bounds) + 8, 32, 32) inView:self];
}

- (void)mouseEntered:(NSEvent *)event
{
    [super mouseEntered:event];
    self.mouseHover = YES;
    [self setNeedsDisplay:YES];
}

- (void)mouseExited:(NSEvent *)event
{
    [super mouseExited:event];
    self.mouseHover = NO;
    [self setNeedsDisplay:YES];
}

- (void)controlAction:(id)sender
{
    #pragma unused(sender)
    wxCommandEvent event(wxEVT_MENU, _parent->GetId());
    event.SetEventObject(_parent);
    _parent->ProcessWindowEvent(event);
}

@end

ActionButton::ActionButton(wxWindow *parent, wxWindowID winid, const wxString& symbolicName, const wxString& label, const wxString& note)
{
    POActionButton *view = [[POActionButton alloc] initWithLabel:str::to_NS(note) heading:str::to_NS(label)];
    if (!symbolicName.empty())
        view.image = [NSImage imageNamed:str::to_NS("AB_" + symbolicName + "Template")];
    view.parent = this;
    wxNativeWindow::Create(parent, winid, view);
}


// ---------------------------------------------------------------------
// SwitchButton
// ---------------------------------------------------------------------

@interface POSwitchButton : NSButton

@property NSColor* onColor;
@property NSColor* labelOffColor;
@property SwitchButton *parent;
@property (nonatomic) double animationPosition;

@end

@implementation POSwitchButton

- (id)initWithLabel:(NSString*)label
{
    self = [super init];
    if (self)
    {
        self.title = label;
        self.bezelStyle = NSBezelStyleSmallSquare;
        self.buttonType = NSButtonTypeOnOff;
        self.bordered = NO;
        self.font = [NSFont boldSystemFontOfSize:[NSFont smallSystemFontSize]];
        self.animationPosition = 0.0;
        self.onColor = [NSColor colorWithCalibratedRed:0.302 green:0.847 blue:0.396 alpha:1.0];
        self.labelOffColor = [NSColor colorWithCalibratedRed:0 green:0 blue:0 alpha:1.0];
    }
    return self;
}

- (void)viewWillMoveToWindow:(NSWindow *)newWindow {
    NSNotificationCenter *nc = NSNotificationCenter.defaultCenter;
    if (self.window)
    {
        [nc removeObserver:self name:NSWindowDidBecomeKeyNotification object:self.window];
        [nc removeObserver:self name:NSWindowDidResignKeyNotification object:self.window];
    }
    if (newWindow)
    {
        [nc addObserver:self selector:@selector(onIsKeyWindowChanged:) name:NSWindowDidBecomeKeyNotification object:newWindow];
        [nc addObserver:self selector:@selector(onIsKeyWindowChanged:) name:NSWindowDidResignKeyNotification object:newWindow];
    }
    [super viewWillMoveToWindow:newWindow];
}

- (void)onIsKeyWindowChanged:(NSNotification *)n
{
    #pragma unused(n)
    [self setNeedsDisplay:YES];
}

- (void)sizeToFit
{
    [super sizeToFit];
    NSSize size = self.frame.size;
    size.width += 32 + 8;
    if (@available(macOS 26, *))
        size.width += 4;
    size.height = 18;
    [self setFrameSize:size];
}

- (void)drawRect:(NSRect)dirtyRect
{
    #pragma unused(dirtyRect)

    // Input parameters:
    CGFloat t = fmin(fmax(self.animationPosition, 0.0), 1.0); // 0..1

    BOOL isDarkMode = [self.effectiveAppearance.name isEqualToString:NSAppearanceNameDarkAqua];
    BOOL isToggledOn = t > 0.5;
    BOOL isInKeyWindow = self.window.isKeyWindow;
    BOOL isLiquidGlass = NO;
    if (@available(macOS 26, *))
        isLiquidGlass = YES;

    // Geometry:
    CGFloat trackW = 32;
    CGFloat trackH = 18;
    CGFloat knobInset = 1.3; // NB: native is 1.0, but this looks a bit better with our color
    CGFloat knobExtraW = 0;

    if (isLiquidGlass)
    {
        // Unlike on previous versions, much not native NSSwitch, but its SwiftUI version, because
        // it is more widely used in the OS and NSSwitch feels a bit too large there.
        trackW = 36;
        trackH = 16;
        knobInset = 1.5; // native
        knobExtraW = 8.0;
    }

    CGFloat knobD_y = trackH - 2.0 * knobInset;
    CGFloat knobD_x = knobD_y + knobExtraW;
    CGFloat radius = trackH / 2.0;

    // Layout: label on the left, switch on the right
    NSRect track = NSMakeRect(NSMaxX(self.bounds) - trackW,
                              NSMidY(self.bounds) - trackH / 2,
                              trackW, trackH);
    NSRect labelRect = self.bounds;
    labelRect.size.width = NSMinX(track) - 8.0; // 8pt gap before the switch

    // Colors:
    NSColor *offTrack = NSColor.quaternaryLabelColor;
    NSColor *onColor = self.onColor;
    if (!isInKeyWindow)
        onColor = NSColor.tertiaryLabelColor;

    NSColor *trackColor = isToggledOn ? onColor : offTrack;
    NSColor *trackStroke = isDarkMode? [NSColor colorWithWhite:1 alpha:0.15] : [NSColor colorWithWhite:0 alpha:0.05];

    NSColor *knobFill = nil;
    if (isLiquidGlass)
        knobFill = isDarkMode ? [NSColor colorWithWhite:1.0 alpha:0.9] : NSColor.whiteColor;
    else
        knobFill = isDarkMode ? [NSColor colorWithWhite:0.79 alpha:1.0] : NSColor.whiteColor;

    NSColor *knobStroke = [NSColor colorWithWhite:0 alpha:(isDarkMode ? 0.2 : 0.05)];

    NSColor *textColor = (isToggledOn && isInKeyWindow) ? onColor : self.labelOffColor;

    NSShadow *shadow = [[NSShadow alloc] init];
    shadow.shadowOffset = NSMakeSize(1, -1);
    shadow.shadowBlurRadius = 1.0;
    shadow.shadowColor = [NSColor colorWithWhite:0 alpha:isDarkMode ? 0.2 : 0.05];

    // Track (pill)
    NSBezierPath *pill = [NSBezierPath bezierPathWithRoundedRect:track xRadius:radius yRadius:radius];
    [trackColor setFill];
    [pill fill];
    if (!isLiquidGlass)
    {
        [trackStroke setStroke];
        pill.lineWidth = 1.0;
        [pill stroke];
    }

    // Knob position: lerp between left and right
    CGFloat x0 = NSMinX(track) + knobInset;
    CGFloat x1 = NSMaxX(track) - knobInset - knobD_x;
    CGFloat kx = x0 + (x1 - x0) * t;
    NSRect knob = NSMakeRect(kx, NSMidY(track) - knobD_y * 0.5, knobD_x, knobD_y);

    NSBezierPath *knobPath = [NSBezierPath bezierPathWithRoundedRect:knob xRadius:knobD_y/2 yRadius:knobD_y/2];
    [knobFill setFill];
    [NSGraphicsContext saveGraphicsState];
    [shadow set];
    [knobPath fill];
    [NSGraphicsContext restoreGraphicsState];
    if (!isLiquidGlass)
    {
        [knobStroke setStroke];
        knobPath.lineWidth = 1.0;
        [knobPath stroke];
    }

    // Label (right-aligned to the space before the switch)
    NSDictionary *attrs = @{
        NSFontAttributeName: [NSFont boldSystemFontOfSize:NSFont.smallSystemFontSize],
        NSForegroundColorAttributeName: textColor,
    };
    [self.title drawAtPoint:NSMakePoint(0, 2) withAttributes:attrs];
}

- (void)setAnimationPosition:(double)animationPosition
{
    _animationPosition = animationPosition;
    self.needsDisplay = YES;
}

+ (id)defaultAnimationForKey:(NSString *)key
{
    if ([key isEqualToString:@"animationPosition"])
        return [CABasicAnimation animation];

    return [super defaultAnimationForKey:key];
}

- (void)setState:(NSInteger)state
{
    if (state == self.state)
        return;
    [super setState:state];
    self.animationPosition = (state == NSControlStateValueOn) ? 1.0 : 0.0;
}

- (void)controlAction:(id)sender
{
    #pragma unused(sender)
    double target = (self.state == NSControlStateValueOn) ? 1.0 : 0.0;

    [NSAnimationContext runAnimationGroup:^(NSAnimationContext *context)
    {
        context.duration = 0.2;
        self.animator.animationPosition = target;
    }];

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
        m_view.state = value ? NSControlStateValueOn : NSControlStateValueOff;
    }

    bool GetValue() const
    {
        return m_view.state == NSControlStateValueOn;
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

#ifdef __WXGTK__
ActionButton::ActionButton(wxWindow *parent, wxWindowID winid, const wxString& /*symbolicName*/, const wxString& label, const wxString& note)
    : wxButton(parent, winid, label, wxDefaultPosition, wxSize(-1, 50), wxBU_LEFT)
{
    SetLabelMarkup(wxString::Format("<b>%s</b>\n<small>%s</small>", label, note));
    Bind(wxEVT_BUTTON, &ActionButton::OnPressed, this);
}
#endif // __WXGTK__

#ifdef __WXMSW__
ActionButton::ActionButton(wxWindow *parent, wxWindowID winid, const wxString& symbolicName, const wxString& label, const wxString& note) 
    : wxCommandLinkButton(parent, winid, label, note, wxDefaultPosition, wxSize(-1, PX(48)))
{
    m_title = label;
    m_note = note;
    m_titleFont = GetFont().MakeLarger();

    MakeOwnerDrawn();

    if (!symbolicName.empty())
    {
        auto bmp = wxArtProvider::GetBitmap("AB_" + symbolicName + "Template@opaque");
        SetBitmap(bmp);
    }

    Bind(wxEVT_BUTTON, &ActionButton::OnPressed, this);
}

bool ActionButton::MSWOnDraw(WXDRAWITEMSTRUCT* wxdis)
{
    LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)wxdis;
    HDC hdc = lpDIS->hDC;

    UINT state = lpDIS->itemState;
    const bool highlighted = IsMouseInWindow();

    wxRect rect(lpDIS->rcItem.left, lpDIS->rcItem.top, lpDIS->rcItem.right - lpDIS->rcItem.left, lpDIS->rcItem.bottom - lpDIS->rcItem.top);
    wxRect textRect(rect);
    textRect.SetLeft(rect.GetLeft() + PX(8));

    wxPaintDCEx dc(this, hdc);

    if (highlighted)
    {
        wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));

        gc->EnableOffset(false);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(GetBackgroundColour().ChangeLightness(95));
        gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, PX(3));
    }

    auto bmp = GetBitmap();
    if (bmp.IsOk())
    {
        dc.DrawBitmap(bmp, PX(16), PX(8));
        textRect.SetLeft(textRect.GetLeft() + PX(48));
        textRect.SetRight(rect.GetRight());
    };

    dc.SetFont(m_titleFont);
    dc.SetTextForeground(ColorScheme::Get(::Color::Label, this));
    int theight;
    dc.GetTextExtent(m_title, nullptr, &theight);
    dc.DrawText(m_title, textRect.x, PX(24) - theight);
    dc.SetFont(GetFont());
    dc.SetTextForeground(ColorScheme::Get(::Color::SecondaryLabel, this));
    dc.DrawText(m_note, textRect.x, PX(24));

    // draw the focus rectangle if we need it
    if ((state & ODS_FOCUS) && !(state & ODS_NOFOCUSRECT))
    {
        RECT r = { rect.x, rect.y, rect.width, rect.height };
        DrawFocusRect(hdc, &r);
    }

    return true;
}

#endif // __WXMSW__

void ActionButton::OnPressed(wxCommandEvent&)
{
    wxCommandEvent event(wxEVT_MENU, GetId());
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}


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
#if wxUSE_ACCESSIBILITY
    Bind(wxEVT_TOGGLEBUTTON, [=](wxCommandEvent& e) {
        wxAccessible::NotifyEvent(wxACC_EVENT_OBJECT_STATECHANGE, this, wxOBJID_CLIENT, wxACC_SELF);
        e.Skip();
    });
#endif // wxUSE_ACCESSIBILITY
#endif // __WXMSW__
}

#ifdef __WXMSW__
#if wxUSE_ACCESSIBILITY
wxAccessible* SwitchButton::CreateAccessible() 
{
    return new accessible(this);
}
#endif // wxUSE_ACCESSIBILITY
#endif // __WXMSW__

void SwitchButton::SetColors(const wxColour& on, const wxColour& offLabel)
{
    (void)on;
    (void)offLabel;
#ifdef __WXMSW__
    m_clrOn = on;
    m_clrOffLabel = offLabel;
#endif

#ifdef __WXGTK3__
    static const char *css_style = R"(
        * {
            padding: 0;
            margin: 0;
            font-weight: bold;
            font-size: 80%;
            color: %s;
        }

        *:checked {
            color: %s;
        }
    )";
    auto css_on = on.GetAsString(wxC2S_CSS_SYNTAX);
    auto css_off = offLabel.GetAsString(wxC2S_CSS_SYNTAX);
    GTKApplyCssStyle(wxString::Format(css_style, css_off, css_on).utf8_str());
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
    size.x += PX(42);
    size.y = PX(22);
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
    const bool isRtl = ::GetLayout(hdc) & LAYOUT_RTL;

    wxRect rect(lpDIS->rcItem.left, lpDIS->rcItem.top, lpDIS->rcItem.right - lpDIS->rcItem.left, lpDIS->rcItem.bottom - lpDIS->rcItem.top);

    wxScopedPtr<wxGraphicsContext> gc(wxGraphicsContext::CreateFromNativeHDC(hdc));
    gc->EnableOffset(false);

    if (isRtl)
    {
        gc->Translate(rect.width, 0);
        gc->Scale(-1.0, 1.0);
    }

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

    wxRect switchRect(rect.GetRight() - PX(42), 0, PX(42), wxMin(PX(22), rect.GetHeight()));
    switchRect.CenterIn(rect, wxVERTICAL);
    switchRect.Deflate(PX(2));

    double radius = (switchRect.height - 1) / 2.0;
    gc->DrawRoundedRectangle(switchRect.x + 0.5, switchRect.y + 0.5, switchRect.width - 1, switchRect.height - 1, radius);

    if (toggled)
    {
        gc->SetPen(*wxWHITE);
        gc->SetBrush(*wxWHITE);
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
    gc->PushState();
    if (isRtl)
    {
        gc->Translate(textw, 0);
        gc->Scale(-1.0, 1.0);
    }
    gc->DrawText(GetLabel(), rect.x, textpos);
    gc->PopState();

    // draw the focus rectangle if we need it
    if ((state & ODS_FOCUS) && !(state & ODS_NOFOCUSRECT))
    {
        RECT r = {rect.x, textpos, rect.x + (int)textw, textpos + (int)texth};
        DrawFocusRect(hdc, &r);
    }

    return true;
}

#if wxUSE_ACCESSIBILITY
wxAccStatus SwitchButton::accessible::GetRole(int childId, wxAccRole* role)
{
    if (childId != wxACC_SELF)
    {
        return wxAccessible::GetRole(childId, role);
    }
    *role = wxROLE_SYSTEM_CHECKBUTTON;
    return wxACC_OK;
}

wxAccStatus SwitchButton::accessible::GetState(int childId, long* state)
{
    if (childId != wxACC_SELF)
    {
        return wxAccessible::GetState(childId, state);
    }
    auto window = dynamic_cast<SwitchButton*>(this->GetWindow());
    if (window->IsFocusable())
    {
        *state |= wxACC_STATE_SYSTEM_FOCUSABLE;
    }
    if (!window->IsShown())
    {
        *state |= wxACC_STATE_SYSTEM_INVISIBLE;
    }
    if (window->GetValue())
    {
        *state |= wxACC_STATE_SYSTEM_CHECKED;
    }
    if (!window->IsEnabled())
    {
        *state |= wxACC_STATE_SYSTEM_UNAVAILABLE;
    }
    if (window->HasFocus())
    {
        *state |= wxACC_STATE_SYSTEM_FOCUSED;
    }
    return wxACC_OK;
}

#endif // wxUSE_ACCESSIBILITY

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
        self.bezelStyle = NSBezelStyleRoundRect;
        self.buttonType = NSButtonTypeMomentaryPushIn;
        self.font = [NSFont systemFontOfSize:[NSFont smallSystemFontSize]];
    }
    return self;
}

- (void)sizeToFit
{
    NSSize size = self.attributedTitle.size;
    size.width += 28;
    size.height = 26;
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
    ColorScheme::SetupWindowColors(this, [=]
    {
        if (ColorScheme::GetAppMode() == ColorScheme::Light)
            SetBackgroundColour(ColorScheme::GetBlendedOn(::Color::TranslucentButton, parent));
        else
            SetBackgroundColour(GetDefaultAttributes().colBg);
    });
#endif

#ifdef __WXGTK3__
    GTKApplyCssStyle(R"(
        * {
            background-image: none;
            background-color: rgba(255,255,255,0.5);
            color: rgba(0,0,0,0.7);
            text-shadow: none;
            border-color: rgba(0,0,0,0.3);
            border-image: none;
            border-radius: 2;
        }
        *:hover {
            background-color: rgba(255,255,255,0.7);
        }
    )");
#endif
}

#endif // !__WXOSX__
