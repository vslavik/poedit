/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2026 Vaclav Slavik
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

#include <memory>
#include <string>
#include <variant>

class Catalog;


/**
    Base class for a cloud account client (e.g. Crowdin).
 */
class CloudAccountClient
{
public:
    /// Return singleton instance of specific client.
    static CloudAccountClient& Get(const std::string& service_name);

    /// Get singleton instance for given metadata object.
    template<typename T>
    static CloudAccountClient& GetFor(const T& obj) { return Get(obj.service); }

    /// Destroys all singletons, must be called (only) on app shutdown.
    static void CleanUp();

    virtual ~CloudAccountClient() {}

    /// Returns identifier of the account's service
    virtual const char *GetServiceName() const = 0; // { return SERVICE_NAME; }
    // Informal protocol: should be present in every derived class
    // static constexpr const char* SERVICE_NAME = "Something";

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

    /// Information about a specific file within a project.
    struct ProjectFile
    {
        std::wstring title;
        std::wstring description;

        /// Implementation-specific internal data
        struct Internal
        {
            virtual ~Internal() {}
        };
        std::shared_ptr<Internal> internal;
    };

    /// Project detailed information not included in ProjectInfo
    struct ProjectDetails
    {
        /// Languages supported by the project
        std::vector<Language> languages;
        /// All files in the project.
        std::vector<ProjectFile> files;
    };

    /// Retrieve details about given project
    virtual dispatch::future<ProjectDetails> GetProjectDetails(const ProjectInfo& project) = 0;

    /// Metadata needed for uploading/downloading files
    struct FileSyncMetadata
    {
        virtual ~FileSyncMetadata() {}

        /// Service (Crowdin etc.) the account is for
        std::string service;
    };

    /// Create filename on local filesystem suitable for the remote file
    virtual std::wstring CreateLocalFilename(const ProjectInfo& project, const ProjectFile& file, const Language& lang) const = 0;

    /**
        Extract sync metadata from a file if present.

        Returns nullptr if @a catalog is not from this cloud account or is missing metadata.
      */
    virtual std::shared_ptr<FileSyncMetadata> ExtractSyncMetadata(Catalog& catalog) = 0;

    /// Extract sync metadata from a file if present, trying all cloud accounts.
    static std::shared_ptr<FileSyncMetadata> ExtractSyncMetadataIfAny(Catalog& catalog);

    /// Asynchronously download specific file into @a output_file.
    virtual dispatch::future<void> DownloadFile(const std::wstring& output_file, const ProjectInfo& project, const ProjectFile& file, const Language& lang) = 0;

    /// Asynchronously download specific file into @a output_file, using data from ExtractSyncMetadata().
    virtual dispatch::future<void> DownloadFile(const std::wstring& output_file, std::shared_ptr<FileSyncMetadata> meta) = 0;

    /**
        Asynchronously upload a file.

        The file is stored in a memory buffer and the destination information is provided by ExtractSyncMetadata().
     */
    virtual dispatch::future<void> UploadFile(const std::string& file_buffer, std::shared_ptr<FileSyncMetadata> meta) = 0;

protected:
    CloudAccountClient() {}
};

#endif // !HAVE_HTTP_CLIENT

#endif // Poedit_cloud_accounts_h
