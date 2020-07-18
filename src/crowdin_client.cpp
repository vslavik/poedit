/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2020 Vaclav Slavik
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

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <wx/translation.h>
#include <wx/utils.h>

#include <iostream>

// GCC's libstdc++ didn't have functional std::regex implementation until 4.9
#if (defined(__GNUC__) && !defined(__clang__) && !wxCHECK_GCC_VERSION(4,9))
    #include <boost/regex.hpp>
    using boost::regex;
    using boost::regex_search;
    using boost::smatch;
#else
    #include <regex>
    using std::regex;
    using std::regex_search;
    using std::smatch;
#endif

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
            return response.at("errors").at(0).at("error").at("errors").at(0).at("message");
        }
        catch (...)
        {
            try
            {
                // Like e.g. on 401 here https://support.crowdin.com/api/v2/#operation/api.user.get
                // as well as in most other requests on above error code
                return response.at("error").at("message");
            }
            catch (...)
            {
                std::string msg;
                msg = _("JSON request error").utf8_str();
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
            m_owner.SignOut();
        }
        wxLogTrace("poedit.crowdin", "JSON error: %s", message.c_str());
    }

    CrowdinClient& m_owner;
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

    smatch m;

    if (!(regex_search(uri, m, regex("state=([^&]+)"))
            && m.size() > 1
            && m.str(1) == m_authCallbackExpectedState))
        return;

    if (!(regex_search(uri, m, regex("access_token=([^&]+)"))
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

dispatch::future<CrowdinClient::UserInfo> CrowdinClient::GetUserInfo()
{
    return m_api->get("user")
        .then([](json r)
        {
            wxLogTrace("poedit.crowdin", "Got user info: %s", r.dump().c_str());
            const json& d = r["data"];
            UserInfo u;
            u.login = str::to_wstring(d["username"]);
            u.avatar = d.value("avatarUrl", "");
            std::string fullName;
            try
            {
                fullName = d.at("fullName");
                // if indvidual (not enterprise) account
            }
            catch (...) // or enterpise otherwise
            {
                try
                {
                    fullName = d.at("firstName");
                }
                catch (...) {}
                try
                {
                    std::string lastName = d.at("lastName");
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
                u.name = u.login;
            else
                u.name = str::to_wstring(fullName);
            return u;
        });
}


dispatch::future<std::vector<CrowdinClient::ProjectListing>> CrowdinClient::GetUserProjects()
{
    // TODO: handle pagination if projects more than 500
    //       (what is quite rare case I guess)
    return m_api->get("projects?limit=500")
        .then([](json r)
        {
            wxLogTrace("poedit.crowdin", "Got projects: %s", r.dump().c_str());
            std::vector<ProjectListing> all;
            for (const auto& d : r["data"])
            {
                const json& i = d["data"];
                all.push_back(
                {
                    str::to_wstring(i["name"]),
                    i["identifier"],
                    i["id"]
                });
            }
            return all;
        });
}


dispatch::future<CrowdinClient::ProjectInfo> CrowdinClient::GetProjectInfo(const int project_id)
{
    auto url = "projects/" + std::to_string(project_id);
    auto prj = std::make_shared<ProjectInfo>();
    static const int NO_ID = -1;

    return m_api->get(url)
    .then([this, url, prj](json r)
    {
        // Handle project info
        const json& d = r["data"];

        if (get_value(d, "publicDownloads", false) == false)
            throw Exception(_("Downloading translations is disabled in this project."));

        prj->name = str::to_wstring(d["name"]);
        prj->id = d["id"];
        for (const auto& langCode : d["targetLanguageIds"])
            prj->languages.push_back(Language::TryParse(str::to_wstring(langCode)));

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
                f.id = d["id"];
                f.fullPath = '/' + std::string(d.at("name"));
                f.dirId = get_value(d, "directoryId", NO_ID);
                f.branchId = get_value(d, "branchId", NO_ID);
                f.fileName = d.at("name");
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
            std::string name = d.at("name");
            dirs.insert(
            {
                d.at("id"),
                {
                    name,
                    get_value(d, "title", name),
                    parent.is_null() ? NO_ID : int(parent)
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

        for (const auto& i : r["data"])
        {
            const json& d = i["data"];
            const std::string name = d.at("name");
            const std::string title = get_value(d, "title", name);
            branches.insert({d.at("id"), {name, title}});
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
    auto cat = Catalog::Create(filename);
    if (!cat->IsOk())
        return;

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
        { "exportAsXliff", isXLIFFConverted }
    });
    // ensure that XLIFF files contain not-yet-translated entries, see https://github.com/vslavik/poedit/pull/648
    if (isXLIFFNative)
        options["skipUntranslatedStrings"] = false;

    return m_api->post(
        "projects/" + std::to_string(project_id) + "/translations/builds/files/" + std::to_string(file_id),
        json_data(options)
        )
        .then([](json r)
        {
            wxLogTrace("poedit.crowdin", "Got file URL: %s", r.dump().c_str());
            std::string url = r["data"]["url"];
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


static std::string base64_decode_json_part(const std::string &in)
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


void CrowdinClient::InitWithAuthToken(const std::string& token)
{
    wxLogTrace("poedit.crowdin", "Authorization: %s", token.c_str());

    if (token.empty())
        return;

    try
    {
        auto token_json = json::parse(
            base64_decode_json_part(std::string(
                wxString(token).AfterFirst('.').BeforeFirst('.').utf8_str()
        )));
        std::string domain = get_value(token_json, "domain", "");
        if (!domain.empty())
            domain += '.';

        m_api = std::make_unique<crowdin_http_client>(*this, "https://" + domain + "crowdin.com/api/v2/");
        m_api->set_authorization("Bearer " + token);
    }
    catch (...)
    {
        wxLogTrace("poedit.crowdin", "Failed to decode token. Most probable invalid/corrupted or unknown/unsupported type", token.c_str());
    }

}


bool CrowdinClient::IsSignedIn() const
{
    return m_api || !GetValidToken().empty();
}


void CrowdinClient::SignInIfAuthorized()
{
    auto token = GetValidToken();
    if (token.empty())
        return;

    InitWithAuthToken(token);
    wxLogTrace("poedit.crowdin", "Token: %s", token.c_str());
}


std::string CrowdinClient::GetValidToken() const
{
    // Our tokens stored in keychain have the form of <version>:<token>, so not
    // only do we have to check for token's existence but also that its version
    // is current:
    std::string token;
    if (keytar::GetPassword("Crowdin", "", &token) && token.substr(0, 2) == "2:")
        return token.substr(2);

    return std::string();
}


void CrowdinClient::SaveAndSetToken(const std::string& token)
{
    keytar::AddPassword("Crowdin", "", "2:" + token);
    InitWithAuthToken(token);
}


void CrowdinClient::SignOut()
{
    m_api.reset();
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

