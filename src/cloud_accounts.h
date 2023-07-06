/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2023 Vaclav Slavik
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

#ifndef Poedit_cloud_accounts_h
#define Poedit_cloud_accounts_h

#ifdef HAVE_HTTP_CLIENT

#include "concurrency.h"
#include "language.h"

#include <string>
#include <variant>


/**
    Base class for a cloud account client (e.g. Crowdin).
 */
class CloudAccountClient
{
public:
    virtual ~CloudAccountClient() {}

    /// Returns identifier of the account's service
    virtual const char *GetServiceID() const = 0; // { return SERVICE_ID; }
    // Informal protocol: should be present in every derived class
    // static constexpr const char* SERVICE_ID = "something";

    /// Is the user logged into this account?
    virtual bool IsSignedIn() const = 0;

    /// Sign out of the account, forget any stored credentials
    virtual void SignOut() = 0;

    /// Information about logged-in user
    struct UserInfo
    {
        /// Service (Crowdin etc.) the account is for
        std::string service;
        /// Human-readable name of the user
        std::wstring name;
        /// User's login name
        std::string login;
        /// URL of the user's avatar image (optional)
        std::string avatarUrl;
    };

    /// Retrieve information about the current user asynchronously
    virtual dispatch::future<UserInfo> GetUserInfo() = 0;

    /// Project listing info
    struct ProjectInfo
    {
        /// Service (Crowdin etc.) the account is for
        std::string service;
        /// Service's internal ID of the project, if used
        std::variant<int, std::string> internalID;

        /// Human-readable name of the service
        std::wstring name;
        /// Slug, i.e. symbolic name of the project; typically included in URLs
        std::string slug;
        /// URL of the projects's avatar/logo image (optional)
        std::string avatarUrl;
    };

    /// Retrieve listing of projects accessible to the user
    virtual dispatch::future<std::vector<ProjectInfo>> GetUserProjects() = 0;

protected:
    CloudAccountClient() {}
};

#endif // !HAVE_HTTP_CLIENT

#endif // Poedit_cloud_accounts_h
