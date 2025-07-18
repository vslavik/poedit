/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2013-2025 Vaclav Slavik
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

#include "main_toolbar.h"

#include "str_helpers.h"

#import <AppKit/AppKit.h>

#include <wx/xrc/xmlres.h>
#include <wx/translation.h>


@interface POToolbarItem : NSToolbarItem
@end

@implementation POToolbarItem

- (void)validate
{
    [self setEnabled:[self.target validateToolbarItem:self]];
}

@end


@interface POToolbarController : NSObject<NSToolbarDelegate> {
    wxFrame *m_parent;
    int id_validate, id_pretranslate, id_update, id_sidebar;
    NSImage *m_imgUpdate, *m_imgSync;
}

@property IBOutlet NSToolbarItem *updateItem;
@property IBOutlet NSToolbar *toolbar;

- (id)initWithParent:(wxFrame*)parent;

@end


@implementation POToolbarController

- (id)initWithParent:(wxFrame*)parent
{
    self = [super init];
    if (self)
    {
        m_parent = parent;
        id_validate = XRCID("menu_validate");
        id_pretranslate = XRCID("menu_pretranslate");
        id_update = XRCID("toolbar_update");
        id_sidebar = XRCID("show_sidebar");
        m_imgUpdate = [NSImage imageNamed:@"UpdateTemplate"];
        m_imgSync = [NSImage imageNamed:@"SyncTemplate"];
    }
    return self;
}

-(void)installToolbar:(wxFrame*)win
{
    NSWindow *window = win->GetWXWindow();
    [window setToolbar:self.toolbar];
}

-(void)uninstallToolbar:(wxFrame*)win
{
    NSWindow *window = win->GetWXWindow();
    [window setToolbar:nil];
}

- (void)enableSyncWithCrowdin:(BOOL)on
{
    NSToolbarItem *tool = self.updateItem;
    if (on)
    {
        [tool setLabel:str::to_NS(_("Sync"))];
        [tool setToolTip:str::to_NS(_("Synchronize the translation with Crowdin"))];
        [tool setImage:m_imgSync];
    }
    else
    {
        [tool setLabel:str::to_NS(_("Update from Code"))];
        [tool setToolTip:str::to_NS(_("Update from source code"))];
        [tool setImage:m_imgUpdate];
    }
}

- (BOOL)validateToolbarItem:(NSToolbarItem *)theItem
{
    SEL action = theItem.action;
    int wxid = -1;
    if (action == @selector(onValidate:))
        wxid = id_validate;
    else if (action == @selector(onPreTranslate:))
        wxid = id_pretranslate;
    else if (action == @selector(onUpdate:))
        wxid = id_update;
    else if (action == @selector(onSidebar:))
        wxid = id_sidebar;
    else
        return YES;

    wxUpdateUIEvent event(wxid);
    m_parent->ProcessWindowEvent(event);
    return !event.GetSetEnabled() || event.GetEnabled();
}

- (void)wxSendEvent:(int)wxid
{
    wxCommandEvent event(wxEVT_TOOL, wxid);
    m_parent->ProcessWindowEvent(event);
}

- (IBAction)onValidate:(id)sender
{
    #pragma unused(sender)
    [self wxSendEvent:id_validate];
}

- (IBAction)onPreTranslate:(id)sender
{
    #pragma unused(sender)
    [self wxSendEvent:id_pretranslate];
}

- (IBAction)onUpdate:(id)sender
{
    #pragma unused(sender)
    [self wxSendEvent:id_update];
}

- (IBAction)onSidebar:(id)sender
{
    #pragma unused(sender)
    [self wxSendEvent:id_sidebar];
}

@end


class CocoaMainToolbar : public MainToolbar
{
public:
    CocoaMainToolbar(wxFrame *parent) : m_parent(parent)
    {
        m_controller = [[POToolbarController new] initWithParent:parent];

        NSNib *nib = [[NSNib alloc] initWithNibNamed:@"MainToolbar" bundle:nil];
        if (![nib instantiateWithOwner:m_controller topLevelObjects:nil])
            return;

        [m_controller installToolbar:m_parent];
    }

    ~CocoaMainToolbar()
    {
        [m_controller uninstallToolbar:m_parent];
    }

    void EnableSyncWithCrowdin(bool on) override
    {
        [m_controller enableSyncWithCrowdin:on];
    }

private:
    wxFrame *m_parent;
    POToolbarController *m_controller;
};


std::unique_ptr<MainToolbar> MainToolbar::Create(wxFrame *parent)
{
    return std::unique_ptr<MainToolbar>(new CocoaMainToolbar(parent));
}
