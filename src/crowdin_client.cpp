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

#include "http_client.h"
#include "keychain/keytar.h"
#include "str_helpers.h"

#include <functional>
#include <mutex>
#include <boost/algorithm/string.hpp>

#include <wx/translation.h>
#include <wx/utils.h>

#include <iostream>

using namespace std;

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

#define OAUTH_SCOPE         "project+tm"
#define OAUTH_CLIENT_ID     "k0uFz5HYQh0VzWgZmOpA"
#define OAUTH_CLIENT_SECRET "p6Ko83cd4l4SD2TYeY8Fheffw8VuN3bML5jx7BeQ"
//      any arbitrary unique unguessable string (e.g. UUID in hex)
#define OAUTH_STATE         "948cf13ffffb47119d6cfa2b68898f67"
//      The value of below macro should be set exactly as is (without quotes)
//      to "Authorization Callback URL" of Crowdin application
//      https://support.crowdin.com/enterprise/creating-oauth-app/
#define OAUTH_URI_PREFIX    "poedit://auth/crowdin/"
#define OAUTH_AUTHORIZE_URL "/oauth/authorize" \
	"?" "response_type" "=" "code" \
	"&" "scope"         "=" OAUTH_SCOPE \
	"&" "client_id"     "=" OAUTH_CLIENT_ID \
	"&" "state"         "=" OAUTH_STATE \
        "&" "redirect_uri"  "=" OAUTH_URI_PREFIX

} // anonymous namespace


std::string CrowdinClient::WrapLink(const std::string& page)
{
    std::string url("https://accounts.crowdin.com");
    if (!page.empty() && page != "/")
        url += page;
    return url;
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
        cout << "\n\nJSON error: " << response << "\n\n";
        std::string msg = response["error"]["message"];

        // Translate commonly encountered messages:
        if (msg == "Translations download is forbidden by project owner")
            msg = _("Downloading translations is disabled in this project.").utf8_str();
        return msg;
    }

    void on_error_response(int& statusCode, std::string& message) override
    {
        if (statusCode == 401/*Unauthorized*/)
        {
            // message is e.g. "The access token provided is invalid"
            message = _("Not authorized, please sign in again.").utf8_str();
            m_owner.SignOut();
        }
        printf("\n\nJSON error: %s\n\n", message.c_str());
    }

    CrowdinClient& m_owner;
};

CrowdinClient::CrowdinClient() :
    m_api(new crowdin_http_client(*this, "https://serhiy.crowdin.com/api/v2")),
    m_oauth(new crowdin_http_client(*this, "https://accounts.crowdin.com")),
    m_downloader(new crowdin_http_client(*this, "https://production-enterprise-tmp.downloads.crowdin.com"/*"https://crowdin-importer.downloads.crowdin.com"*/))
{
    SignInIfAuthorized();
}

CrowdinClient::~CrowdinClient() {}


dispatch::future<void> CrowdinClient::Authenticate()
{
    auto url = WrapLink(OAUTH_AUTHORIZE_URL);
    m_authCallback.reset(new dispatch::promise<void>);
    wxLaunchDefaultBrowser(url);
    return m_authCallback->get_future();
}


void CrowdinClient::HandleOAuthCallback(const std::string& uri)
{
    if (!m_authCallback)
        return;

    const regex re("code=([^&]+)&state=([^&]+)");
    smatch m;
    if (!regex_search(uri, m, re)
        || m.size() < 3
        || m.str(2) != OAUTH_STATE) {
        return;
    }

    m_oauth->post("/oauth/token", json_data(json({
        { "grant_type", "authorization_code" },
        { "client_id", OAUTH_CLIENT_ID },
        { "client_secret", OAUTH_CLIENT_SECRET },
        { "redirect_uri", OAUTH_URI_PREFIX },
        { "code", m.str(1) }
    }))).then([this](json r) {
        SaveAndSetToken(r["access_token"]);

        m_authCallback->set_value();
        m_authCallback.reset();
    });
}


bool CrowdinClient::IsOAuthCallback(const std::string& uri)
{
    return boost::starts_with(uri, OAUTH_URI_PREFIX);
}

//TODO: validate JSON schema in all API responses
//      and handle errors since now Poedit just crashes
//      in case of some of expected keys missing

dispatch::future<CrowdinClient::UserInfo> CrowdinClient::GetUserInfo()
{
    return m_api->get("/user")
        .then([](json r)
        {
            cout << "\n\nGot user info: " << r << "\n\n";
            const json& d = r["data"];
            UserInfo u;
            u.login = str::to_wstring(d["username"]);
            u.name = str::to_wstring(d.value("fullName", ""));
            if(u.name.empty()) {
                u.name = str::to_wstring(d["firstName"]) + " " + str::to_wstring(d["lastName"]);
            }
            if(u.name.empty()) {
                u.name = u.login;
            }
            return u;
        });
}


dispatch::future<std::vector<CrowdinClient::ProjectListing>> CrowdinClient::GetUserProjects()
{
    // TODO: handle pagination if projects more than 500
    //       (what is quite rare case I guess)
    return m_api->get("/projects?limit=500")
        .then([](json r)
        {
            cout << "\n\nGot projects: " << r << endl<<endl;
            std::vector<ProjectListing> all;
            for (auto&& d : r["data"])
            {
                const json& i = d["data"];
                all.push_back({
                    str::to_wstring(i["name"]),
                    i["id"].get<int>(),
                    //TODO: should be tested with project not owning by
                    //      currently authorized user as well (not only
                    //      for owning) after it will be implemented in
                    //      in API v2 (unimplemented yet)
                    /*(bool)i["publicDownloads"].get<int>()*/true
                });
            }
            return all;
        });
}


dispatch::future<CrowdinClient::ProjectInfo> CrowdinClient::GetProjectInfo(const int project_id)
{
    auto url = "/projects/" + std::to_string(project_id);
    return m_api->get(url)
        .then([this, url](json r)
        {
            ProjectInfo prj;
            const json& d = r["data"];
            prj.name = str::to_wstring(d["name"]);
            prj.id = d["id"];
            for (auto&& langCode : d["targetLanguageIds"]) {
                prj.languages.push_back(Language::TryParse(str::to_wstring(langCode)));
            }
            return m_api->get(url + "/files?limit=500")
                .then([prj_ = prj](json r)
                {
                    auto prj = prj_;
                    //TODO: files should be with full path
                    for(auto&& i : r["data"]) {
                        const json& d = i["data"];
                        prj.files.push_back({L"/" + str::to_wstring(d["name"]), d["id"]});
                    }
                    //TODO: get more until all files gotten (if more than 500)
                    return prj;
                });
        });
}


dispatch::future<void> CrowdinClient::DownloadFile(const long project_id,
                                                   const long file_id,
                                                   const std::wstring& file_name,
                                                   const std::string& lang_tag,
                                                   const std::wstring& output_file)
{
    cout << "\n\nGetting file URL: " << "/projects/" + std::to_string(project_id) + "/files/" + std::to_string(file_id) + "/download" << "\n\n";
    return m_api->post(
        "/projects/" + std::to_string(project_id) + "/translations/builds/files/" + std::to_string(file_id),
        json_data(json({
            { "targetLanguageId", lang_tag },
            // for XLIFF files should be exported "as is" so set to `false`
            { "exportAsXliff", !wxString(file_name).Lower().EndsWith(".xliff.xliff") }
        }))
        .then([this, output_file] (json r) {
            cout << "\n\nGotten file URL: "<< r << "\n\n";
            return m_downloader->download(r["data"]["url"], output_file);
        });
}


dispatch::future<void> CrowdinClient::UploadFile(const long project_id,
                                                 const long file_id,
                                                 const std::string& lang_tag,
                                                 const std::string& file_content)
{
    return m_api->post(
            "/storages",
            octet_stream_data(file_content),
            { { "Crowdin-API-FileName", L"poedit.xliff"} }
        )
        .then([this, project_id, file_id, lang_tag] (json r) {
            cout << "File uploaded to temporary storage: " << r << "\n\n";
            const auto storageId = r["data"]["id"].get<int>();
            return m_api->post(
                "/projects/" + std::to_string(project_id) + "/translations/" + lang_tag,
                json_data(json({
                    { "storageId", storageId },
                    { "fileId", file_id },
                    { "importDuplicates", true },
                    { "autoApproveImported", true }
                })))
                .then([](json r) {
                    cout << "File uploaded: " << r << "\n\n";
                });
        });
}


bool CrowdinClient::IsSignedIn() const
{
    std::string token;
    return keytar::GetPassword("Crowdin", "", &token);
}


void CrowdinClient::SignInIfAuthorized()
{
    std::string token;
    if (keytar::GetPassword("Crowdin", "", &token))
        SetToken(token);
    cout << "\n\nToken: "<<token<<"\n\n";
}


void CrowdinClient::SetToken(const std::string& token)
{
    m_api->set_authorization(!token.empty() ? "Bearer " + token : "");
}


void CrowdinClient::SaveAndSetToken(const std::string& token)
{
    SetToken(token);
    keytar::AddPassword("Crowdin", "", token);
}


void CrowdinClient::SignOut()
{
    m_api->set_authorization("");
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

