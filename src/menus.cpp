/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2020 Vaclav Slavik
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

#include "menus.h"

#ifdef __WXOSX__
#include "macos_helpers.h"
#endif

#include "recent_files.h"
#include "str_helpers.h"

#include <wx/xrc/xmlres.h>


#ifdef __WXOSX__

// TODO: move this to app delegate, once we have it (proxying to wx's one)
@interface POMenuActions : NSObject
@end

@implementation POMenuActions

- (void)showWelcomeWindow: (id)sender
{
    #pragma unused(sender)
    wxCommandEvent event(wxEVT_MENU, XRCID("menu_welcome"));
    wxTheApp->ProcessEvent(event);
}

@end

struct MenusManager::NativeMacData
{
    NSMenu *windowMenu = nullptr;
    NSMenuItem *windowMenuItem = nullptr;
    wxMenuBar *menuBar = nullptr;
    POMenuActions *actions = nullptr;
};
#endif

MenusManager::MenusManager()
{
#ifdef __WXOSX__
    m_nativeMacData.reset(new NativeMacData);
    m_nativeMacData->actions = [POMenuActions new];
    wxMenuBar::SetAutoWindowMenu(false);
#endif
}

MenusManager::~MenusManager()
{
}


wxMenuBar *MenusManager::CreateMenu(Menu purpose)
{
    wxMenuBar *bar = nullptr;

    switch (purpose)
    {
        case Menu::Global:
#ifdef __WXOSX__
            bar = wxXmlResource::Get()->LoadMenuBar("mainmenu_global");
#endif
            break;
        case Menu::WelcomeWindow:
#ifndef __WXOSX__
            bar = wxXmlResource::Get()->LoadMenuBar("mainmenu_global");
#endif
            break;
        case Menu::Editor:
            bar = wxXmlResource::Get()->LoadMenuBar("mainmenu");
            break;
    }

    wxASSERT( bar );

    RecentFiles::Get().UseMenu(bar->FindItem(XRCID("open_recent")));

#ifdef __WXOSX__
    TweakOSXMenuBar(bar);

    if (purpose == Menu::Global)
        wxMenuBar::MacSetCommonMenuBar(bar);
#endif

#ifndef HAVE_HTTP_CLIENT
    wxMenu *menu;
    wxMenuItem *item;
    item = bar->FindItem(XRCID("menu_update_from_crowdin"), &menu);
    if (item)
        menu->Destroy(item);
    item = bar->FindItem(XRCID("menu_open_crowdin"), &menu);
    if (item)
        menu->Destroy(item);
#endif

    return bar;
}


#ifdef __WXOSX__

static NSMenuItem *AddNativeItem(NSMenu *menu, int pos, const wxString& text, SEL ac, NSString *key)
{
    NSString *str = str::to_NS(text);
    if (pos == -1)
        return [menu addItemWithTitle:str action:ac keyEquivalent:key];
    else
        return [menu insertItemWithTitle:str action:ac keyEquivalent:key atIndex:pos];
}

void MenusManager::TweakOSXMenuBar(wxMenuBar *bar)
{
    wxMenu *apple = bar->OSXGetAppleMenu();
    if (apple)
    {
        apple->Insert(3, XRCID("menu_manager"), _("Catalogs Manager"));
        apple->InsertSeparator(3);
    }

#if USE_SPARKLE
    Sparkle_AddMenuItem(apple->GetHMenu(), _(L"Check for Updates…").utf8_str());
#endif

    wxMenu *fileMenu = nullptr;
    wxMenuItem *fileCloseItem = bar->FindItem(wxID_CLOSE, &fileMenu);
    if (fileMenu && fileCloseItem)
    {
        NSMenuItem *nativeCloseItem = [fileMenu->GetHMenu() itemWithTitle:str::to_NS(fileCloseItem->GetItemLabelText())];
        if (nativeCloseItem)
        {
            nativeCloseItem.target = nil;
            nativeCloseItem.action = @selector(performClose:);
        }
    }

    int editMenuPos = bar->FindMenu(_("&Edit"));
    if (editMenuPos == wxNOT_FOUND)
        editMenuPos = 1;
    wxMenu *edit = bar->GetMenu(editMenuPos);
    int pasteItem = -1;
    int findItem = -1;
    int pos = 0;
    for (auto& i : edit->GetMenuItems())
    {
        if (i->GetId() == wxID_PASTE)
            pasteItem = pos;
        else if (i->GetId() == XRCID("menu_sub_find"))
            findItem = pos;
        pos++;
    }

    NSMenu *editNS = edit->GetHMenu();

    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wundeclared-selector"
    AddNativeItem(editNS, 0, _("Undo"), @selector(undo:), @"z");
    AddNativeItem(editNS, 1, _("Redo"), @selector(redo:), @"Z");
    #pragma clang diagnostic pop
    [editNS insertItem:[NSMenuItem separatorItem] atIndex:2];
    if (pasteItem != -1) pasteItem += 3;
    if (findItem != -1) findItem += 3;

    NSMenuItem *item;
    if (pasteItem != -1)
    {
        item = AddNativeItem(editNS, pasteItem+1, _("Paste and Match Style"),
                             @selector(pasteAsPlainText:), @"V");
        [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
        item = AddNativeItem(editNS, pasteItem+2, _("Delete"),
                             @selector(delete:), @"");
        [item setKeyEquivalentModifierMask:NSCommandKeyMask];
        if (findItem != -1) findItem += 2;
    }

    #define FIND_PLUS(ofset) ((findItem != -1) ? (findItem+ofset) : -1)
    if (findItem == -1)
        [editNS addItem:[NSMenuItem separatorItem]];
    item = AddNativeItem(editNS, FIND_PLUS(1), _("Spelling and Grammar"), NULL, @"");
    NSMenu *spelling = [[NSMenu alloc] initWithTitle:@"Spelling and Grammar"];
    AddNativeItem(spelling, -1, _("Show Spelling and Grammar"), @selector(showGuessPanel:), @":");
    AddNativeItem(spelling, -1, _("Check Document Now"), @selector(checkSpelling:), @";");
    [spelling addItem:[NSMenuItem separatorItem]];
    AddNativeItem(spelling, -1, _("Check Spelling While Typing"), @selector(toggleContinuousSpellChecking:), @"");
    AddNativeItem(spelling, -1, _("Check Grammar With Spelling"), @selector(toggleGrammarChecking:), @"");
    AddNativeItem(spelling, -1, _("Correct Spelling Automatically"), @selector(toggleAutomaticSpellingCorrection:), @"");
    [editNS setSubmenu:spelling forItem:item];

    item = AddNativeItem(editNS, FIND_PLUS(2), _("Substitutions"), NULL, @"");
    NSMenu *subst = [[NSMenu alloc] initWithTitle:@"Substitutions"];
    AddNativeItem(subst, -1, _("Show Substitutions"), @selector(orderFrontSubstitutionsPanel:), @"");
    [subst addItem:[NSMenuItem separatorItem]];
    AddNativeItem(subst, -1, _("Smart Copy/Paste"), @selector(toggleSmartInsertDelete:), @"");
    AddNativeItem(subst, -1, _("Smart Quotes"), @selector(toggleAutomaticQuoteSubstitution:), @"");
    AddNativeItem(subst, -1, _("Smart Dashes"), @selector(toggleAutomaticDashSubstitution:), @"");
    AddNativeItem(subst, -1, _("Smart Links"), @selector(toggleAutomaticLinkDetection:), @"");
    AddNativeItem(subst, -1, _("Text Replacement"), @selector(toggleAutomaticTextReplacement:), @"");
    [editNS setSubmenu:subst forItem:item];

    item = AddNativeItem(editNS, FIND_PLUS(3), _("Transformations"), NULL, @"");
    NSMenu *trans = [[NSMenu alloc] initWithTitle:@"Transformations"];
    AddNativeItem(trans, -1, _("Make Upper Case"), @selector(uppercaseWord:), @"");
    AddNativeItem(trans, -1, _("Make Lower Case"), @selector(lowercaseWord:), @"");
    AddNativeItem(trans, -1, _("Capitalize"), @selector(capitalizeWord:), @"");
    [editNS setSubmenu:trans forItem:item];

    item = AddNativeItem(editNS, FIND_PLUS(4), _("Speech"), NULL, @"");
    NSMenu *speech = [[NSMenu alloc] initWithTitle:@"Speech"];
    AddNativeItem(speech, -1, _("Start Speaking"), @selector(startSpeaking:), @"");
    AddNativeItem(speech, -1, _("Stop Speaking"), @selector(stopSpeaking:), @"");
    [editNS setSubmenu:speech forItem:item];

    int viewMenuPos = bar->FindMenu(_("&View"));
    if (viewMenuPos != wxNOT_FOUND)
    {
        NSMenu *viewNS = bar->GetMenu(viewMenuPos)->GetHMenu();
        [viewNS addItem:[NSMenuItem separatorItem]];
        // TRANSLATORS: This must be the same as OS X's translation of this View menu item
        item = AddNativeItem(viewNS, -1, _("Show Toolbar"), @selector(toggleToolbarShown:), @"t");
        [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSAlternateKeyMask];
        // TRANSLATORS: This must be the same as OS X's translation of this View menu item
        AddNativeItem(viewNS, -1, _(L"Customize Toolbar…"), @selector(runToolbarCustomizationPalette:), @"");
        [viewNS addItem:[NSMenuItem separatorItem]];
        // TRANSLATORS: This must be the same as OS X's translation of this View menu item
        item = AddNativeItem(viewNS, -1, _("Enter Full Screen"), @selector(toggleFullScreen:), @"f");
        [item setKeyEquivalentModifierMask:NSCommandKeyMask | NSControlKeyMask];

    }

    if (!m_nativeMacData->windowMenu)
    {
        NSMenu *windowMenu = [[NSMenu alloc] initWithTitle:str::to_NS(_("Window"))];
        AddNativeItem(windowMenu, -1, _("Minimize"), @selector(performMiniaturize:), @"m");
        AddNativeItem(windowMenu, -1, _("Zoom"), @selector(performZoom:), @"");
        [windowMenu addItem:[NSMenuItem separatorItem]];
        item = AddNativeItem(windowMenu, -1, _("Welcome to Poedit"), @selector(showWelcomeWindow:), @"1");
        item.target = m_nativeMacData->actions;
        [item setKeyEquivalentModifierMask: NSShiftKeyMask | NSCommandKeyMask];

        [windowMenu addItem:[NSMenuItem separatorItem]];
        AddNativeItem(windowMenu, -1, _("Bring All to Front"), @selector(arrangeInFront:), @"");
        [NSApp setWindowsMenu:windowMenu];
        m_nativeMacData->windowMenu = windowMenu;
    }
}

void MenusManager::FixupMenusForMacIfNeeded()
{
    auto installed = wxMenuBar::MacGetInstalledMenuBar();
    if (m_nativeMacData->menuBar == installed)
        return;  // nothing to do

    m_nativeMacData->menuBar = nullptr;

    RecentFiles::Get().MacTransferMenuTo(installed);

    if (m_nativeMacData->windowMenuItem)
        [m_nativeMacData->windowMenuItem setSubmenu:nil];

    if (!installed)
        return;

    NSMenuItem *windowItem = [[NSApp mainMenu] itemWithTitle:str::to_NS(_("Window"))];
    if (windowItem)
    {
        [windowItem setSubmenu:m_nativeMacData->windowMenu];
        m_nativeMacData->windowMenuItem = windowItem;
    }

    m_nativeMacData->menuBar = installed;
}

#endif // __WXOSX__
