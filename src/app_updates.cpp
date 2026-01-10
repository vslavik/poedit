/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2024-2026 Vaclav Slavik
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
#include "concurrency.h"

#include <wx/menu.h>

#ifdef __WXOSX__
#import <Sparkle/Sparkle.h>
#endif

#ifdef __WXMSW__
#include <winsparkle.h>
#endif


#ifdef __WXOSX__

@interface PoeditSparkleDelegate : NSObject <SPUUpdaterDelegate>
@end

@implementation PoeditSparkleDelegate

- (NSArray<NSDictionary<NSString *, NSString *> *> *)feedParametersForUpdater:(SPUUpdater *)updater sendingSystemProfile:(BOOL)sendingProfile;
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

        m_delegate = [PoeditSparkleDelegate new];
        m_controller = [[SPUStandardUpdaterController alloc] initWithUpdaterDelegate:m_delegate userDriverDelegate:nil];
    }

    void EnableAutomaticChecks(bool enable)
    {
        SetBoolValue("SUEnableAutomaticChecks", enable);
    }

    bool AutomaticChecksEnabled() const
    {
        return GetBoolValue("SUEnableAutomaticChecks");
    }

    bool CanCheckForUpdates() const
    {
        return m_controller.updater.canCheckForUpdates;
    }

    void CheckForUpdatesWithUI()
    {
        [m_controller checkForUpdates:nil];
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
    SPUStandardUpdaterController *m_controller = nil;
    NSObject<SPUUpdaterDelegate> *m_delegate = nil;
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

    bool CanCheckForUpdates() const
    {
        return true;
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
        wxASSERT( !wxThread::IsMain() );

        return dispatch::on_main([]() {
            return !PoeditFrame::AnyWindowIsModified();
        }).get();
    }

    static void WinSparkle_Shutdown()
    {
        // Register for Application Restart so that Restart Manager (used by Inno Setup)
        // can restart Poedit after it closes us during installation.
        // Use empty command line ("") and avoid restarts after crash/hang/reboot.
        RegisterApplicationRestart(L"", RESTART_NO_CRASH | RESTART_NO_HANG | RESTART_NO_REBOOT);

        // Do NOT shut down here! The installer will close us via Restart Manager and
        // restart after installation completes.
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

bool AppUpdates::CanCheckForUpdates() const
{
    return m_impl->CanCheckForUpdates();
}

void AppUpdates::CheckForUpdatesWithUI()
{
    m_impl->CheckForUpdatesWithUI();
}

#ifdef __WXMSW__
void AppUpdates::SetLanguage(const std::string& lang)
{
    m_impl->SetLanguage(lang);
}
#endif // __WXMSW__

#endif // HAS_UPDATES_CHECK
