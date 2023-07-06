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


#include "crowdin_client.h"

#include "catalog.h"
#include "errors.h"
#include "http_client.h"
#include "keychain/keytar.h"
#include "str_helpers.h"

#include <functional>
#include <mutex>
#include <stack>
#include <iostream>
#include <ctime>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <wx/translation.h>
#include <wx/utils.h>

#include <iostream>

// ----------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------

namespace
{

// see https://support.crowdin.com/enterprise/creating-oauth-app/
#define OAUTH_BASE_URL                  "https://accounts.crowdin.com/oauth/authorize"
#define OAUTH_CLIENT_ID                 "6Xsr0OCnsRdALYSHQlvs"
#define OAUTH_SCOPE                     "project"
#define OAUTH_CALLBACK_URL_PREFIX       "poedit://auth/crowdin/"


std::string base64_decode_json_part(const std::string &in)
{
    std::string out;
    std::vector<int> t(256, -1);
    {
        static const char* b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        for (int i = 0; i < 64; i ++)
            t[b[i]] = i;
    }

    int val = 0,
        valb = -8;

    for (uint8_t c : in)
    {
        if (t[c] == -1)
            break;
        val = (val << 6) + t[c];
        valb += 6;
        if (valb >= 0)
        {
            out.push_back(char(val >> valb & 0xFF));
            valb -= 8;
        }
    }

    return out;
}

} // anonymous namespace


std::string CrowdinClient::AttributeLink(std::string page)
{
    static const char *base_url = "https://crowdin.com";
    static const char *utm = "utm_source=poedit.net&utm_medium=referral&utm_campaign=poedit";

    if (!boost::starts_with(page, "http"))
        page = base_url + page;

    page += page.find('?') == std::string::npos ? '?' : '&';
    page += utm;

    return page;
}


class CrowdinClient::crowdin_http_client : public http_client
{
public:
    crowdin_http_client(CrowdinClient& owner, const std::string& url_prefix)
        : http_client(url_prefix), m_owner(owner)
    {}

protected:
    std::string parse_json_error(const json& response) const override
    {
        wxLogTrace("poedit.crowdin", "JSON error: %s", response.dump().c_str());

        try
        {
            // Like e.g. on 400 here https://support.crowdin.com/api/v2/#operation/api.projects.getMany
            // where "key" is usually "error" as figured out while looking in responses with above code
            // on most API requests used here
            return response.at("errors").at(0).at("error").at("errors").at(0).at("message").get<std::string>();
        }
        catch (...)
        {
            try
            {
                // Like e.g. on 401 here https://support.crowdin.com/api/v2/#operation/api.user.get
                // as well as in most other requests on above error code
                return response.at("error").at("message").get<std::string>();
            }
            catch (...)
            {
                std::string msg;
                msg = _("Unknown Crowdin error.").utf8_str();
                return msg;
            }
        }
    }

    void on_error_response(int& statusCode, std::string& message) override
    {
        if (statusCode == 401/*Unauthorized*/)
        {
            // message is e.g. "The access token provided is invalid"
            message = _("Not authorized, please sign in again.").utf8_str();
        }
        wxLogTrace("poedit.crowdin", "JSON error: %s", message.c_str());
    }

    CrowdinClient& m_owner;
};


class CrowdinClient::crowdin_token
{
public:
    explicit crowdin_token(const std::string& jwt_token)
    {
        try
        {
            if (jwt_token.empty())
                return;

            auto token_json = json::parse(
                base64_decode_json_part(std::string(
                    wxString(jwt_token).AfterFirst('.').BeforeFirst('.').utf8_str()
            )));
            domain = get_value(token_json, "domain", "");
            if (!domain.empty())
                domain += '.';

            time_t expiration = (time_t)get_value(token_json, "exp", 0.0);
            m_valid = (expiration > time(NULL));
            if (m_valid)
                encoded = jwt_token;
        }
        catch (...)
        {
            wxLogTrace("poedit.crowdin", "Failed to decode token. Most probable invalid/corrupted or unknown/unsupported type", jwt_token.c_str());
        }
    }

    bool is_valid() const { return m_valid; }

    std::string domain;
    std::string encoded;

private:
    bool m_valid = false;
};


CrowdinClient::CrowdinClient()
{
    SignInIfAuthorized();
}

CrowdinClient::~CrowdinClient() {}


dispatch::future<void> CrowdinClient::Authenticate()
{
    m_authCallback.reset(new dispatch::promise<void>);
    m_authCallbackExpectedState = boost::uuids::to_string(boost::uuids::random_generator()());

    auto url = OAUTH_BASE_URL
               "?response_type=token"
               "&scope=" OAUTH_SCOPE
               "&client_id=" OAUTH_CLIENT_ID
               "&redirect_uri=" OAUTH_CALLBACK_URL_PREFIX
               "&state=" + m_authCallbackExpectedState;

    wxLaunchDefaultBrowser(AttributeLink(url));
    return m_authCallback->get_future();
}


void CrowdinClient::HandleOAuthCallback(const std::string& uri)
{
    wxLogTrace("poedit.crowdin", "Callback URI %s", uri.c_str());

    std::smatch m;

    if (!(std::regex_search(uri, m, std::regex("state=([^&]+)"))
            && m.size() > 1
            && m.str(1) == m_authCallbackExpectedState))
        return;

    if (!(std::regex_search(uri, m, std::regex("access_token=([^&]+)"))
            && m.size() > 1))
        return;

    if (!m_authCallback)
        return;
    SaveAndSetToken(m.str(1));
    m_authCallback->set_value();
    m_authCallback.reset();
}

bool CrowdinClient::IsOAuthCallback(const std::string& uri)
{
    return boost::starts_with(uri, OAUTH_CALLBACK_URL_PREFIX);
}

//TODO: validate JSON schema in all API responses
//      and handle errors since now Poedit just crashes
//      in case of some of expected keys missing

dispatch::future<CloudAccountClient::UserInfo> CrowdinClient::GetUserInfo()
{
    return m_api->get("user")
        .then([](json r)
        {
            wxLogTrace("poedit.crowdin", "Got user info: %s", r.dump().c_str());
            const json& d = r["data"];
            UserInfo u;
            u.service = SERVICE_ID;
            d.at("username").get_to(u.login);
            u.avatarUrl = d.value("avatarUrl", "");
            std::string fullName;
            try
            {
                d.at("fullName").get_to(fullName);
                // if individual (not enterprise) account
            }
            catch (...) // or enterpise otherwise
            {
                try
                {
                    d.at("firstName").get_to(fullName);
                }
                catch (...) {}
                try
                {
                    auto lastName = d.at("lastName").get<std::string>();
                    if (!lastName.empty())
                    {
                        if (!fullName.empty())
                            fullName += " ";
                        fullName += lastName;
                    }
                }
                catch (...) {}
            }
            if (fullName.empty())
                u.name = str::to_wstring(u.login);
            else
                u.name = str::to_wstring(fullName);
            return u;
        });
}


dispatch::future<std::vector<CloudAccountClient::ProjectInfo>> CrowdinClient::GetUserProjects()
{
    // TODO: handle pagination if projects more than 500
    //       (what is quite rare case I guess)
    return m_api->get("projects?limit=500")
        .then([](json r)
        {
            wxLogTrace("poedit.crowdin", "Got projects: %s", r.dump().c_str());
            std::vector<ProjectInfo> all;
            for (const auto& d : r["data"])
            {
                const json& i = d["data"];
                all.push_back(
                {
                    SERVICE_ID,
                    i.at("id").get<int>(),
                    i.at("name").get<std::wstring>(),
                    i.at("identifier").get<std::string>(),
                    std::string() // avatarUrl
                });
            }
            return all;
        });
}


dispatch::future<CrowdinClient::ProjectDetails> CrowdinClient::GetProjectDetails(const CrowdinClient::ProjectInfo& project)
{
    auto project_id = std::get<int>(project.internalID);

    auto url = "projects/" + std::to_string(project_id);
    auto prj = std::make_shared<ProjectDetails>();
    static const int NO_ID = -1;

    return m_api->get(url)
    .then([this, url, prj](json r)
    {
        // Handle project info
        const json& d = r["data"];

        if (get_value(d, "publicDownloads", false) == false)
            throw Exception(_("Downloading translations is disabled in this project."));

        d.at("name").get_to(prj->name);
        d.at("id").get_to(prj->id);
        for (const auto& langCode: d.at("targetLanguageIds"))
            prj->languages.push_back(Language::FromLanguageTag(std::string(langCode)));

        //TODO: get more until all files gotten (if more than 500)
        return m_api->get(url + "/files?limit=500");
    })
    .then([this, url, prj](json r)
    {
        // Handle project files
        for (auto& i : r["data"])
        {
            const json& d = i["data"];
            if (d["type"] != "assets")
            {
                FileInfo f;
                d.at("id").get_to(f.id);
                f.fullPath = '/' + d.at("name").get<std::string>();
                f.dirId = get_value(d, "directoryId", NO_ID);
                f.branchId = get_value(d, "branchId", NO_ID);
                d.at("name").get_to(f.fileName);
                f.title = get_value(d, "title", f.fileName);
                prj->files.push_back(std::move(f));
            }
        }
        //TODO: get more until all dirs gotten (if more than 500)
        return m_api->get(url + "/directories?limit=500");
    })
    .then([this, url, prj](json r)
    {
        // Handle directories
        struct dir_info
        {
            std::string name, title;
            int parentId;
        };
        std::map<int, dir_info> dirs;

        for (const auto& i : r["data"])
        {
            const json& d = i["data"];
            const json& parent = d["directoryId"];
            std::string name;
            d.at("name").get_to(name);
            dirs.insert(
            {
                d.at("id").get<int>(),
                {
                    name,
                    get_value(d, "title", name),
                    parent.is_null() ? NO_ID : parent.get<int>()
                }
            });
        }

        for (auto& i : prj->files)
        {
            std::list<std::string> path;
            int dirId = i.dirId;
            while (dirId != NO_ID)
            {
                const auto& dir = dirs[dirId];
                path.push_front(dir.title);
                i.fullPath.insert(0, '/' + dir.name);
                dirId = dir.parentId;
            }
            i.dirName = boost::join(path, "/");
        }
    })
    .then([this, url]()
    {
        //TODO: get more until all branches gotten (if more than 500)
        return m_api->get(url + "/branches?limit=500");
    })
    .then([this, url, prj](json r)
    {
        // Handle branches
        struct branch_info
        {
            std::string name, title;
        };
        std::map<int, branch_info> branches;

        for (const auto& i : r.at("data"))
        {
            const json& d = i.at("data");
            const auto name = d.at("name").get<std::string>();
            const std::string title = get_value(d, "title", name);
            branches.insert({d.at("id").get<int>(), {name, title}});
        }

        for (auto& i : prj->files)
        {
            if (i.branchId != NO_ID)
            {
                i.branchName = branches[i.branchId].title;
                i.fullPath.insert(0, '/' + branches[i.branchId].name);
            }
        }

        return *prj;
    });
}


static void PostprocessDownloadedXLIFF(const wxString& filename)
{
    // Crowdin XLIFF files have translations pre-filled with the source text if
    // not yet translated. Undo this as it is undesirable to translators.
    try
    {
        auto cat = Catalog::Create(filename);

        bool modified = false;
        for (auto& item: cat->items())
        {
            if (item->IsFuzzy() && !item->HasPlural() && item->GetString() == item->GetTranslation())
            {
                item->ClearTranslation();
                modified = true;
            }
        }

        if (modified)
        {
            Catalog::ValidationResults dummy1;
            Catalog::CompilationStatus dummy2;
            cat->Save(filename, false, dummy1, dummy2);
        }
    }
    catch (...)
    {
        return;
    }
}


dispatch::future<void> CrowdinClient::DownloadFile(int project_id,
                                                   const Language& lang,
                                                   int file_id,
                                                   const std::string& file_extension,
                                                   bool forceExportAsXliff,
                                                   const std::wstring& output_file)
{
    wxLogTrace("poedit.crowdin", "DownloadFile(project_id=%d, lang=%s, file_id=%d, file_extension=%s, output_file=%S)", project_id, lang.LanguageTag(), file_id, file_extension.c_str(), output_file.c_str());

    wxString ext(file_extension);
    ext.MakeLower();
    const bool isXLIFFNative = (ext == "xliff" || ext == "xlf") && !forceExportAsXliff;
    const bool isXLIFFConverted = (!isXLIFFNative && ext != "po" && ext != "pot") || forceExportAsXliff;

    json options({
        { "targetLanguageId", lang.LanguageTag() },
        // for XLIFF and PO files should be exported "as is" so set to `false`
        { "exportAsXliff", isXLIFFConverted },
        // ensure that XLIFF files contain not-yet-translated entries, see https://github.com/vslavik/poedit/pull/648
        { "skipUntranslatedStrings", false }
    });

    return m_api->post(
        "projects/" + std::to_string(project_id) + "/translations/builds/files/" + std::to_string(file_id),
        json_data(options)
        )
        .then([](json r)
        {
            wxLogTrace("poedit.crowdin", "Got file URL: %s", r.dump().c_str());
            auto url = r.at("data").at("url").get<std::string>();
            return http_client::download_from_anywhere(url);
        })
        .then([=](downloaded_file file)
        {
            wxString outfile(output_file);
            file.move_to(outfile);

            if (isXLIFFNative || isXLIFFConverted)
                PostprocessDownloadedXLIFF(outfile);
        });
}


dispatch::future<void> CrowdinClient::UploadFile(int project_id,
                                                 const Language& lang,
                                                 int file_id,
                                                 const std::string& file_extension,
                                                 const std::string& file_content)
{
    wxLogTrace("poedit.crowdin", "UploadFile(project_id=%d, lang=%s, file_id=%d, file_extension=%s)", project_id, lang.LanguageTag().c_str(), file_id, file_extension.c_str());
    return m_api->post(
            "storages",
            octet_stream_data(file_content),
            { { "Crowdin-API-FileName", "crowdin." + file_extension } }
        )
        .then([this, project_id, file_id, lang] (json r) {
            wxLogTrace("poedit.crowdin", "File uploaded to temporary storage: %s", r.dump().c_str());
            const auto storageId = r["data"]["id"];
            return m_api->post(
                "projects/" + std::to_string(project_id) + "/translations/" + lang.LanguageTag(),
                json_data({
                    { "storageId", storageId },
                    { "fileId", file_id }
                }))
                .then([](json r) {
                    wxLogTrace("poedit.crowdin", "File uploaded: %s", r.dump().c_str());
                });
        });
}


bool CrowdinClient::InitWithAuthToken(const crowdin_token& token)
{
    wxLogTrace("poedit.crowdin", "Authorization: %s", token.encoded.c_str());

    if (!token.is_valid())
        return false;

    m_api = std::make_unique<crowdin_http_client>(*this, "https://" + token.domain + "crowdin.com/api/v2/");
    m_api->set_authorization("Bearer " + token.encoded);
    return true;
}


bool CrowdinClient::IsSignedIn() const
{
    return m_api || GetValidToken().is_valid();
}


void CrowdinClient::SignInIfAuthorized()
{
    auto token = GetValidToken();
    if (!token.is_valid())
        return;

    if (InitWithAuthToken(token))
    {
        wxLogTrace("poedit.crowdin", "Token: %s", token.encoded.c_str());
    }
    else
    {
        wxLogTrace("poedit.crowdin", "Token was invalid/expired");
    }
}


CrowdinClient::crowdin_token CrowdinClient::GetValidToken() const
{
    if (m_cachedAuthToken)
        return *m_cachedAuthToken;

    // Our tokens stored in keychain have the form of <version>:<token>, so not
    // only do we have to check for token's existence but also that its version
    // is current:
    std::string token;
    if (keytar::GetPassword("Crowdin", "", &token) && token.substr(0, 2) == "2:")
    {
        token = token.substr(2);
    }
    else
    {
        // 'token' is empty or invalid, make sure it is empty
        token.clear();
    }

    crowdin_token ct(token);
    m_cachedAuthToken = std::make_unique<crowdin_token>(ct);
    return ct;
}


void CrowdinClient::SaveAndSetToken(const std::string& token)
{
    crowdin_token ct(token);
    if (!ct.is_valid())
        return;

    m_cachedAuthToken = std::make_unique<crowdin_token>(ct);
    if (InitWithAuthToken(ct))
        keytar::AddPassword("Crowdin", "", "2:" + ct.encoded);
}


void CrowdinClient::SignOut()
{
    m_api.reset();
    m_cachedAuthToken.reset();
    keytar::DeletePassword("Crowdin", "");
}


// ----------------------------------------------------------------
// Singleton management
// ----------------------------------------------------------------

static std::once_flag initializationFlag;
CrowdinClient* CrowdinClient::ms_instance = nullptr;

CrowdinClient& CrowdinClient::Get()
{
    std::call_once(initializationFlag, []() {
        ms_instance = new CrowdinClient;
    });
    return *ms_instance;
}

void CrowdinClient::CleanUp()
{
    if (ms_instance)
    {
        delete ms_instance;
        ms_instance = nullptr;
    }
}

