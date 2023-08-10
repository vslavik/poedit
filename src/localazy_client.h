/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2023 Vaclav Slavik
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

#ifndef Poedit_localazy_client_h
#define Poedit_localazy_client_h

#ifdef HAVE_HTTP_CLIENT

#include <map>
#include <memory>
#include <mutex>

#include "cloud_accounts.h"


/**
    Client to the Localazy platform.
 */
class LocalazyClient : public CloudAccountClient
{
public:
    /// Return singleton instance of the client.
    static LocalazyClient& Get();

    /// Destroys the singleton, must be called (only) on app shutdown.
    static void CleanUp();

    static constexpr const char* SERVICE_ID = "localazy";
    const char *GetServiceID() const override { return SERVICE_ID; }

    /// Is the user currently signed into Localazy?
    bool IsSignedIn() const override;

    /// Wrap relative Localazy URL to absolute URL with attribution
    static std::string AttributeLink(std::string page);

    /**
        Authenticate with Localazy.
        
        This opens the browser to authenticate the app. The app must handle
        poedit:// URL and call HandleAuthCallback. @a callback will be
        called after receiving the auth token.
     */
    dispatch::future<void> Authenticate();
    void HandleAuthCallback(const std::string& uri);
    static bool IsAuthCallback(const std::string& uri);

    /// Sign out of Localazy, forget the tokens
    void SignOut() override;

    /// Retrieve information about the current user asynchronously
    dispatch::future<UserInfo> GetUserInfo() override;

    /// Retrieve listing of projects accessible to the user
    dispatch::future<std::vector<ProjectInfo>> GetUserProjects() override;

    /// Retrieve details about given project
    dispatch::future<ProjectDetails> GetProjectDetails(const ProjectInfo& project) override;

    /// Create filename on local filesystem suitable for the remote file
    std::wstring CreateLocalFilename(const ProjectInfo& project, const ProjectFile& file, const Language& lang) const override;

    /// Asynchronously download specific file into @a output_file.
    dispatch::future<void> DownloadFile(const std::wstring& output_file, const ProjectInfo& project, const ProjectFile& file, const Language& lang) override;

private:
    /**
        Exchanges temporary token for per-project token.

        After exchange, updates stored tokens and project metadata and saves them.
     */
    dispatch::future<void> ExchangeTemporaryToken(const std::string& token);

    /// Get authorization header for given project
    std::string GetAuthorization(const std::string& project_id) const;

    struct FileInternal : public ProjectFile::Internal
    {
        // Localazy uses non-standard language codes that we need to remap
        std::map<std::string, std::string> tagToLocale;
    };

private:
    class localazy_http_client;
    class project_tokens;
    class metadata;

    LocalazyClient();
    ~LocalazyClient();

    void InitMetadataAndTokens();
    void SaveMetadataAndTokens();

    std::unique_ptr<project_tokens> m_tokens;
    std::unique_ptr<metadata> m_metadata;
    mutable std::mutex m_mutex; // guards m_tokens and m_metadata

    std::unique_ptr<localazy_http_client> m_api;

    std::shared_ptr<dispatch::promise<void>> m_authCallback;

    static LocalazyClient *ms_instance;
};

#endif // HAVE_HTTP_CLIENT

#endif // Poedit_localazy_client_h
