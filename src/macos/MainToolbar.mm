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
    int id_validate, id_pretranslate, id_update, id_sync, id_sidebar;
}

@property IBOutlet NSToolbarItem *syncItem;
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
        id_sync = XRCID("menu_cloud_sync");
        id_sidebar = XRCID("show_sidebar");
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

- (void)enableCloudSync:(std::shared_ptr<CloudSyncDestination>)dest isCrowdin:(BOOL)isCrowdin
{
    NSToolbarItem *tool = self.syncItem;

    if (!dest || isCrowdin)
    {
        [tool setImage:[NSImage imageNamed:@"SyncTemplate"]];
        [tool setLabel:str::to_NS(_("Sync"))];
        [tool setToolTip:str::to_NS(_("Synchronize translations with Crowdin"))];
    }
    else
    {
        [tool setImage:[NSImage imageNamed:@"UploadTemplate"]];
        [tool setLabel:str::to_NS(_("Upload"))];
        // TRANSLATORS: this is the tooltip for the "Upload" button in the toolbar, %s is hostname or service (Crowdin, ftp.foo.com etc.)
        auto tooltip = wxString::Format(_("Upload translations to %s"), dest->GetName());
        [tool setToolTip:str::to_NS(tooltip)];
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
    else if (action == @selector(onSync:))
        wxid = id_sync;
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

- (IBAction)onSync:(id)sender
{
    #pragma unused(sender)
    [self wxSendEvent:id_sync];
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

    void EnableCloudSync(std::shared_ptr<CloudSyncDestination> sync, bool isCrowdin) override
    {
        [m_controller enableCloudSync:sync isCrowdin:isCrowdin];
    }

private:
    wxFrame *m_parent;
    POToolbarController *m_controller;
};


std::unique_ptr<MainToolbar> MainToolbar::Create(wxFrame *parent)
{
    return std::unique_ptr<MainToolbar>(new CocoaMainToolbar(parent));
}
