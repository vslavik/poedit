/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2024 Vaclav Slavik
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

#include "app_updates.h"

#ifdef HAS_UPDATES_CHECK

#include "edapp.h"
#include "edframe.h"

#include <wx/menu.h>

#ifdef __WXOSX__
#import <Sparkle/Sparkle.h>
#endif

#ifdef __WXMSW__
#include <winsparkle.h>
#endif


#ifdef __WXOSX__

@interface PoeditSparkleDelegate : NSObject <SUUpdaterDelegate>
@end

@implementation PoeditSparkleDelegate

- (NSArray *)feedParametersForUpdater:(SUUpdater *)updater sendingSystemProfile:(BOOL)sendingProfile
{
    #pragma unused(updater, sendingProfile)
    if (Config::CheckForBetaUpdates())
    {
        return @[ @{
            @"key":           @"beta",
            @"value":         @"1",
            @"displayKey":    @"Beta Versions",
            @"displayValue":  @"Yes"
        } ];
    }
    else
    {
        return @[];
    }
}

@end


class AppUpdates::impl
{
public:
    impl() {}

    ~impl()
    {
        //  Make sure that Sparkle's updates to plist preferences are saved:
        [[NSUserDefaults standardUserDefaults] synchronize];
    }

    void InitAndStart()
    {
        // Poedit < 2.0 stored this in preferences, which was wrong - it overrode
        // changes to Info.plist. Undo the damage:
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"SUFeedURL"];

        // For Preferences window, have default in sync with Info.plist:
        NSDictionary *sparkleDefaults = @{ @"SUEnableAutomaticChecks": @YES };
        [[NSUserDefaults standardUserDefaults] registerDefaults:sparkleDefaults];

        m_sparkleDelegate = [PoeditSparkleDelegate new];
        SUUpdater.sharedUpdater.delegate = m_sparkleDelegate;
    }

    void EnableAutomaticChecks(bool enable)
    {
        SetBoolValue("SUEnableAutomaticChecks", enable);
    }

    bool AutomaticChecksEnabled() const
    {
        return GetBoolValue("SUEnableAutomaticChecks");
    }

    void AddMenuItem(wxMenu *appleMenu)
    {
        NSString *nstitle = [NSString stringWithUTF8String: _(L"Check for Updates…").utf8_str()];
        NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:nstitle
                                               action:@selector(checkForUpdates:)
                                               keyEquivalent:@""];
        SUUpdater *updater = [SUUpdater sharedUpdater];
        [item setEnabled:YES];
        [item setTarget:updater];
        [appleMenu->GetHMenu() insertItem:item atIndex:1];
    }

private:
    bool GetBoolValue(const char *key) const
    {
        NSString *nskey = [NSString stringWithUTF8String: key];
        return [[NSUserDefaults standardUserDefaults] boolForKey:nskey];
    }

    void SetBoolValue(const char *key, int value)
    {
        NSString *nskey = [NSString stringWithUTF8String: key];

        [[NSUserDefaults standardUserDefaults] setBool:value forKey:nskey];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }

private:
    NSObject<SUUpdaterDelegate> *m_sparkleDelegate = nullptr;
};

#endif // __WXOSX__


#ifdef __WXMSW__

class AppUpdates::impl
{
public:
    impl() {}

    ~impl()
    {
        win_sparkle_cleanup();
    }

    void InitAndStart()
    {
        if (Config::CheckForBetaUpdates())
            win_sparkle_set_appcast_url("https://poedit.net/updates_v2/win/appcast/beta");
        else
            win_sparkle_set_appcast_url("https://poedit.net/updates_v2/win/appcast");

        win_sparkle_set_can_shutdown_callback(&impl::WinSparkle_CanShutdown);
        win_sparkle_set_shutdown_request_callback(&impl::WinSparkle_Shutdown);
        auto buildnum = wxGetApp().GetAppBuildNumber();
        if (!buildnum.empty())
            win_sparkle_set_app_build_version(buildnum.wc_str());
        win_sparkle_init();
    }

    void SetLanguage(const std::string& lang)
	{
		win_sparkle_set_lang(lang.c_str());
	}

    void CheckForUpdatesWithUI()
    {
		win_sparkle_check_update_with_ui();
    }

    void EnableAutomaticChecks(bool enable)
    {
        if (enable)
            SetupAppcastURL();
        win_sparkle_set_automatic_check_for_updates(enable);
    }

    bool AutomaticChecksEnabled() const
    {
        return win_sparkle_get_automatic_check_for_updates();
    }

private:
    void SetupAppcastURL()
    {
        if (Config::CheckForBetaUpdates())
            win_sparkle_set_appcast_url("https://poedit.net/updates_v2/win/appcast/beta");
        else
            win_sparkle_set_appcast_url("https://poedit.net/updates_v2/win/appcast");
    }

    // WinSparkle callbacks:
    static int WinSparkle_CanShutdown()
    {
        return !PoeditFrame::AnyWindowIsModified();
    }

    static void WinSparkle_Shutdown()
    {
        wxCommandEvent evt(wxEVT_COMMAND_MENU_SELECTED, wxID_EXIT);
        wxGetApp().AddPendingEvent(evt);
    }
};

#endif // __WXMSW__


// Boilerplate:

namespace
{
    static std::once_flag initializationFlag;
    AppUpdates* gs_instance = nullptr;
}

AppUpdates& AppUpdates::Get()
{
    std::call_once(initializationFlag, []() {
        gs_instance = new AppUpdates;
    });
    return *gs_instance;
}

void AppUpdates::CleanUp()
{
    if (gs_instance)
    {
        delete gs_instance;
        gs_instance = nullptr;
    }
}

AppUpdates::AppUpdates() : m_impl(new impl) {}
AppUpdates::~AppUpdates() {}

void AppUpdates::InitAndStart()
{
    m_impl->InitAndStart();
}

void AppUpdates::EnableAutomaticChecks(bool enable)
{
    m_impl->EnableAutomaticChecks(enable);
}

bool AppUpdates::AutomaticChecksEnabled() const
{
    return m_impl->AutomaticChecksEnabled();
}

#ifdef __WXMSW__
void AppUpdates::SetLanguage(const std::string& lang)
{
    m_impl->SetLanguage(lang);
}

void AppUpdates::CheckForUpdatesWithUI()
{
    m_impl->CheckForUpdatesWithUI();
}
#endif // __WXMSW__

#ifdef __WXOSX__
void AppUpdates::AddMenuItem(wxMenu *appleMenu)
{
    m_impl->AddMenuItem(appleMenu);
}
#endif

#endif // HAS_UPDATES_CHECK
