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

#ifndef Poedit_app_updates_h
#define Poedit_app_updates_h

#if defined(__WXMSW__) || defined(__WXOSX__)
    #define HAS_UPDATES_CHECK
#endif

#include <wx/defs.h>

class WXDLLIMPEXP_FWD_CORE wxMenu;

#include <memory>

#ifdef HAS_UPDATES_CHECK

/// Management of app updates.
class AppUpdates
{
public:
    /// Return singleton instance of the manager.
    static AppUpdates& Get();

    // InitAndStart and start checking for updates (if allowed by the user).
    void InitAndStart();

    /// Destroys the singleton, must be called (only) on app shutdown.
    static void CleanUp();

    void EnableAutomaticChecks(bool enable);
    bool AutomaticChecksEnabled() const;

    bool CanCheckForUpdates() const;
    void CheckForUpdatesWithUI();

#ifdef __WXMSW__
    void SetLanguage(const std::string& lang);
#endif

private:
    AppUpdates();
    ~AppUpdates();

    class impl;
    std::unique_ptr<impl> m_impl;
};

#endif // HAS_UPDATES_CHECK

#endif // Poedit_app_updates_h
