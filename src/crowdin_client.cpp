﻿/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2015 Vaclav Slavik
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

#define OAUTH_CLIENT_ID     "poedit-y1aBMmDbm3s164dtJ4Ur150e2"
#define OAUTH_AUTHORIZE_URL "/oauth2/authorize?response_type=token&client_id=" OAUTH_CLIENT_ID
#define OAUTH_URI_PREFIX    "poedit://auth/crowdin/"

// Recursive extract files from /api/project/*/info response
void ExtractFilesFromInfo(std::vector<std::wstring>& out, const json_dict& r, const std::wstring& prefix)
{
    r.iterate_array("files",[&out,prefix](const json_dict& i){
        auto name = prefix + i.wstring("name");
        auto node_type = i.utf8_string("node_type");
        if (node_type == "file")
        {
            out.push_back(name);
        }
        else if (node_type == "directory")
        {
            ExtractFilesFromInfo(out, i, name + L"/");
        }
    });
}

} // anonymous namespace


std::string CrowdinClient::WrapLink(const std::string& page)
{
    std::string url("https://poedit.net/crowdin");
    if (!page.empty() && page != "/")
        url += "?u=" + http_client::url_encode(page);
    return url;
}


class CrowdinClient::crowdin_http_client : public http_client
{
public:
    crowdin_http_client(CrowdinClient& owner)
        : http_client("https://api.crowdin.com"), m_owner(owner)
    {}

protected:
    std::string parse_json_error(const json_dict& response) const override
    {
        auto msg = response.subdict("error").utf8_string("message");

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
    }

    CrowdinClient& m_owner;
};


CrowdinClient::CrowdinClient() : m_api(new crowdin_http_client(*this))
{
    SignInIfAuthorized();
}

CrowdinClient::~CrowdinClient() {}


void CrowdinClient::Authenticate(std::function<void()> callback)
{
    auto url = WrapLink(OAUTH_AUTHORIZE_URL);
    m_authCallback = callback;
    wxLaunchDefaultBrowser(url);
}


void CrowdinClient::HandleOAuthCallback(const std::string& uri)
{
    if (!m_authCallback)
        return;

    const regex re("access_token=([^&]+)&");
    smatch m;
    if (!regex_search(uri, m, re))
        return;

    SaveAndSetToken(m.str(1));

    m_authCallback();
    m_authCallback = nullptr;
}


bool CrowdinClient::IsOAuthCallback(const std::string& uri)
{
    return boost::starts_with(uri, OAUTH_URI_PREFIX);
}


void CrowdinClient::GetUserInfo(std::function<void(UserInfo)> callback)
{
    m_api->get("/api/account/profile?json=",
            // OK:
            [callback](const json_dict& r){
                auto profile = r.subdict("profile");
                UserInfo u;
                u.name = profile.wstring("name");
                u.login = profile.wstring("login");
                callback(u);
            },
            // error:
            [](std::exception_ptr) {}
    );
}


void CrowdinClient::GetUserProjects(std::function<void(std::vector<ProjectListing>)> onResult,
                                    error_func_t onError)
{
    m_api->get("/api/account/get-projects?json=&role=all",
            // OK:
            [onResult](const json_dict& r){
                std::vector<ProjectListing> all;
                r.iterate_array("projects",[&all](const json_dict& i){
                    all.push_back({i.wstring("name"),
                                   i.utf8_string("identifier"),
                                   (bool)i.number("downloadable")});
                });
                onResult(all);
            },
            onError
    );
}


void CrowdinClient::GetProjectInfo(const std::string& project_id,
                                   std::function<void(ProjectInfo)> onResult,
                                   error_func_t onError)
{
    auto url = "/api/project/" + project_id + "/info?json=&project-identifier=" + project_id;
    m_api->get(url,
            // OK:
            [onResult](const json_dict& r){
                ProjectInfo prj;
                auto details = r.subdict("details");
                prj.name = details.wstring("name");
                prj.identifier = details.utf8_string("identifier");
                r.iterate_array("languages",[&prj](const json_dict& i){
                    if (i.number("can_translate") != 0)
                        prj.languages.push_back(Language::TryParse(i.wstring("code")));
                });
                ExtractFilesFromInfo(prj.files, r, L"/");
                onResult(prj);
            },
            onError
    );
}


void CrowdinClient::DownloadFile(const std::string& project_id, const std::wstring& file, const Language& lang,
                                 const std::wstring& output_file,
                                 std::function<void()> onSuccess,
                                 error_func_t onError)
{
    // NB: "export_translated_only" means the translation is not filled with the source string
    //     if there's no translation, i.e. what Poedit wants.
    auto url = "/api/project/" + project_id + "/export-file"
                   "?json="
                   "&export_translated_only=1"
                   "&language=" + lang.RFC3066() +
                   "&file=" + http_client::url_encode(file);
    m_api->download(url, output_file, onSuccess, onError);
}


void CrowdinClient::UploadFile(const std::string& project_id, const std::wstring& file, const Language& lang,
                                 const std::string& file_content,
                                 std::function<void()> onSuccess,
                                 error_func_t onError)
{
    auto url = "/api/project/" + project_id + "/upload-translation";

    multipart_form_data data;
    data.add_value("json", "");
    data.add_value("language", lang.RFC3066());
    data.add_value("import_duplicates", "0");
    data.add_value("import_eq_suggestions", "0");
    data.add_file("files[" + str::to_utf8(file) + "]", "upload.po", file_content);

    m_api->post(url, data, onSuccess, onError);
}


bool CrowdinClient::IsSignedIn() const
{
    std::string token;
    return keytar::GetPassword("Crowdin", &token);
}


void CrowdinClient::SignInIfAuthorized()
{
    std::string token;
    if (keytar::GetPassword("Crowdin", &token))
        SetToken(token);
}


void CrowdinClient::SetToken(const std::string& token)
{
    m_api->set_authorization(!token.empty() ? "Bearer " + token : "");
}


void CrowdinClient::SaveAndSetToken(const std::string& token)
{
    SetToken(token);
    keytar::AddPassword("Crowdin", token);
}


void CrowdinClient::SignOut()
{
    m_api->set_authorization("");
    keytar::DeletePassword("Crowdin");
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

