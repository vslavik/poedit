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


#include "localazy_client.h"

#include "catalog.h"
#include "configuration.h"
#include "errors.h"
#include "http_client.h"
#include "keychain/keytar.h"
#include "str_helpers.h"

#include <chrono>
#include <functional>
#include <mutex>
#include <stack>
#include <iostream>
#include <ctime>
#include <regex>
#include <thread>

#include <boost/algorithm/string.hpp>

#include <iostream>

using namespace std::chrono_literals;


// ----------------------------------------------------------------
// Implementation
// ----------------------------------------------------------------

std::string LocalazyClient::AttributeLink(std::string page)
{
    static const char *base_url = "https://localazy.com";
    static const char *ref = "ref=a9PjgZZmxYvt-12r";

    if (!boost::starts_with(page, "http"))
        page = base_url + page;

    page += page.find('?') == std::string::npos ? '?' : '&';
    page += ref;

    return page;
}


class LocalazyClient::localazy_http_client : public http_client
{
public:
    localazy_http_client() : http_client("https://api.localazy.com") {}

protected:
    std::string parse_json_error(const json& response) const override
    {
        wxLogTrace("poedit.localazy", "JSON error: %s", response.dump().c_str());

        try
        {
            return response.at("message").get<std::string>();
        }
        catch (...)
        {
            return std::string();
        }
    }
};


class LocalazyClient::metadata
{
public:
    explicit metadata(const std::string& serialized)
    {
        if (serialized.empty())
            clear();
        else
            m_data = json::parse(serialized);
    }

    void clear()
    {
        m_data = json();
    }

    std::string to_string() const
    {
        if (m_data.is_null())
            return std::string();
        else
            return m_data.dump();
    }

    void add(const std::string& id, const json& project, const json& user)
    {
        m_data["user"] = user;
        m_data["projects"][id] = project;
    }

    bool is_valid() const { return !m_data.is_null(); }

    const json& user() const { return m_data.at("user"); }
    const json& projects() const { return m_data.at("projects"); }

private:
    json m_data;
};


class LocalazyClient::project_tokens
{
public:
    explicit project_tokens(const std::string& encoded_tokens)
    {
        if (encoded_tokens.empty())
            return;

        std::vector<std::string> parts;
        boost::split(parts, encoded_tokens, boost::is_any_of("/"));
        if (parts.empty())
            return;

        for (auto& part: parts)
        {
            std::vector<std::string> kv;
            boost::split(kv, part, boost::is_any_of("="));
            if (kv.size() != 2)
            {
                m_tokens.clear();
                return;
            }
            m_tokens[kv[0]] = kv[1];
        }
    }

    void clear()
    {
        m_tokens.clear();
    }

    std::string to_string() const
    {
        std::string result;
        for (auto& kv: m_tokens)
        {
            if (!result.empty())
                result += "/";
            result += kv.first + "=" + kv.second;
        }
        return result;

    }

    void add(const std::string& project, const std::string& token)
    {
        m_tokens[project] = token;
    }

    std::string get(const std::string& project) const
    {
        return m_tokens.at(project);
    }

    bool is_valid() const { return !m_tokens.empty(); }

private:
    std::map<std::string, std::string> m_tokens;
};


LocalazyClient::LocalazyClient() : m_api(new localazy_http_client())
{
    InitMetadataAndTokens();
}

LocalazyClient::~LocalazyClient() {}


dispatch::future<void> LocalazyClient::Authenticate()
{
    m_authCallback.reset(new dispatch::promise<void>);

    auto url = "https://localazy.com/extauth/oauth/poedit?ref=a9PjgZZmxYvt-12r";

    wxLaunchDefaultBrowser(AttributeLink(url));
    return m_authCallback->get_future();
}


bool LocalazyClient::IsAuthCallback(const std::string& uri)
{
    return boost::starts_with(uri, "poedit://localazy/");
}


dispatch::future<std::shared_ptr<LocalazyClient::ProjectInfo>> LocalazyClient::HandleAuthCallback(const std::string& uri)
{
    wxLogTrace("poedit.localazy", "Callback URI %s", uri.c_str());

    std::smatch m;
    if (!(std::regex_search(uri, m, std::regex("//localazy/(open|oauth)/([^&]+)")) && m.size() > 2))
        return dispatch::make_ready_future(std::shared_ptr<ProjectInfo>());

    std::string verb(m.str(1));
    std::string tempToken(m.str(2));

    // direct opening needs to work even when unexpected, but in-app auth shouldn't:
    if (!m_authCallback && verb != "open")
        return dispatch::make_ready_future(std::shared_ptr<ProjectInfo>());

    return
    ExchangeTemporaryToken(tempToken)
    .then_on_main([=](ProjectInfo prjInfo){
        if (m_authCallback)
        {
            m_authCallback->set_value();
            m_authCallback.reset();
        }
        if (verb == "open")
            return std::make_shared<ProjectInfo>(prjInfo);
        else
            return std::shared_ptr<ProjectInfo>();
    });
}


dispatch::future<LocalazyClient::ProjectInfo> LocalazyClient::ExchangeTemporaryToken(const std::string& token)
{
    // http_client requires that all requests are relative to the provided prefix
    // (this is a C++REST SDK limitation enforced on some platforms), so we need
    // to create a transient http_client for it and use it to perform the request.
    auto transient = std::make_shared<http_client>("https://localazy.com");

    json data({
        { "token", token }
    });
    return transient->post("/extauth/exchange", json_data(data))
           .then_on_main([transient,this](json r)
           {
               // notice that we must capture the `transient` http_client instance so that it won't
               // be destroyed before the request is done processing
               auto token = r.at("accessToken").get<std::string>();
               auto project = r.at("project");
               auto user = r.at("user");
               auto projectId = project.at("id").get<std::string>();

               ProjectInfo prjInfo {
                   SERVICE_NAME,
                   projectId,
                   project.at("name").get<std::wstring>(),
                   project.at("slug").get<std::string>(),
                   project.at("image").get<std::string>()
               };

               std::lock_guard<std::mutex> guard(m_mutex);
               m_metadata->add(projectId, project, user);
               m_tokens->add(projectId, token);
               SaveMetadataAndTokens(guard);

               return prjInfo;
           });
}


void LocalazyClient::InitMetadataAndTokens()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    m_metadata.reset(new metadata(Config::LocalazyMetadata()));

    // Our tokens stored in keychain have the form of <version>:<token>, so not
    // only do we have to check for token's existence but also that its version is current:
    std::string encoded_tokens;
    if (keytar::GetPassword("Localazy", "", &encoded_tokens) && encoded_tokens.substr(0, 2) == "1:")
    {
        encoded_tokens = encoded_tokens.substr(2);
    }
    else
    {
        // 'encoded' is empty or invalid, make sure it is empty
        encoded_tokens.clear();
    }

    m_tokens.reset(new project_tokens(encoded_tokens));
}


void LocalazyClient::SaveMetadataAndTokens(std::lock_guard<std::mutex>& /*acquiredLock - just to make sure caller holds it*/)
{
    Config::LocalazyMetadata(m_metadata->to_string());

    auto encoded_tokens = m_tokens->to_string();
    if (encoded_tokens.empty())
        keytar::DeletePassword("Localazy", "");
    else
        keytar::AddPassword("Localazy", "", "1:" + encoded_tokens);
}


std::string LocalazyClient::GetAuthorization(const std::string& project_id) const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return "Bearer " + m_tokens->get(project_id);
}


dispatch::future<CloudAccountClient::UserInfo> LocalazyClient::GetUserInfo()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto user = m_metadata->user();
    UserInfo info;
    info.service = SERVICE_NAME;
    user.at("slug").get_to(info.login);
    user.at("image").get_to(info.avatarUrl);
    info.name = str::to_wstring(user.at("name").get<std::string>());
    return dispatch::make_ready_future(std::move(info));
}


dispatch::future<std::vector<CloudAccountClient::ProjectInfo>> LocalazyClient::GetUserProjects()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    std::vector<CloudAccountClient::ProjectInfo> all;

    for (auto p : m_metadata->projects())
    {
        all.push_back(
        {
            SERVICE_NAME,
            p.at("id").get<std::string>(),
            p.at("name").get<std::wstring>(),
            p.at("slug").get<std::string>(),
            p.at("image").get<std::string>()
        });
    }

    return dispatch::make_ready_future(std::move(all));
}


dispatch::future<CloudAccountClient::ProjectDetails> LocalazyClient::GetProjectDetails(const ProjectInfo& project)
{
    auto project_id = std::get<std::string>(project.internalID);
    http_client::headers headers {{"Authorization", GetAuthorization(project_id)}};

    return m_api->get("/projects?languages=true", headers)
           .then([project_id](json r)
           {
               for (auto prj: r)
               {
                   auto id = prj.at("id");
                   if (id == project_id)
                   {
                       ProjectDetails details;

                       // there's only one "file" in Localazy projects:
                       ProjectFile f;
                       auto internal = std::make_shared<FileInternal>();
                       f.internal = internal;
                       f.title = _("All strings");
                       prj.at("url").get_to(f.description);
                       details.files.push_back(f);

                       for (auto lang: prj.at("languages"))
                       {
                           auto code = lang.at("code").get<std::string>();
                           auto tag = code;

                           // Localazy uses non-standard codes such as zh_CN#Hans, we need to convert it to
                           // a language tag (zh-Hans-CN). We assume that the format is lang_region#script.
                           static const std::regex re_locale("([a-z]+)(_([A-Z0-9]+))?(#([A-Z][a-z]*))?");
                           std::smatch m;
                           if (std::regex_search(code, m, re_locale))
                           {
                               tag = m[1].str();
                               if (m[5].matched)
                                   tag += "-" + m[5].str();
                               if (m[3].matched)
                                   tag += "-" + m[3].str();
                           }

                           internal->tagToLocale[tag] = code;

                           auto l = Language::FromLanguageTag(tag);
                           if (l.IsValid())
                               details.languages.push_back(l);
                       }

                       return details;
                   }
               }

               throw Exception(_(L"Couldn’t download Localazy project details."));
           });
}


std::wstring LocalazyClient::CreateLocalFilename(const ProjectInfo& project, const ProjectFile& /*file*/, const Language& lang) const
{
    auto project_name = project.name;
    // sanitize to be safe filenames:
    std::replace_if(project_name.begin(), project_name.end(), boost::is_any_of("\\/:\"<>|?*"), '_');

    return project_name + L"." + str::to_wstring(lang.LanguageTag()) + L".json";
}


std::shared_ptr<LocalazyClient::FileSyncMetadata> LocalazyClient::ExtractSyncMetadata(Catalog& catalog)
{
    if (catalog.Header().GetHeader("X-Generator") != "Localazy")
        return nullptr;

    auto meta = std::make_shared<LocalazySyncMetadata>();
    meta->service = SERVICE_NAME;
    meta->langCode = catalog.GetLanguage().LanguageTag();

    // extract project information from filename in the form of ProjectName.lang.json:
    auto name = wxFileName(catalog.GetFileName()).GetName().BeforeLast('.').ToStdWstring();
    std::lock_guard<std::mutex> guard(m_mutex);
    for (auto p : m_metadata->projects())
    {
        auto pname = p.at("name").get<std::wstring>();
        if (name == pname)
        {
            p.at("id").get_to(meta->projectId);
            return meta;
        }
    }

    // no matching project found in the loop above
    return nullptr;
}


dispatch::future<void> LocalazyClient::DownloadFile(const std::wstring& output_file, std::shared_ptr<FileSyncMetadata> meta_)
{
    auto meta = std::dynamic_pointer_cast<LocalazySyncMetadata>(meta_);

    auto locale = meta->langCode;
    boost::replace_all(meta->langCode, "-", "_");
    boost::replace_all(locale, "#", "%23"); // urlencode

    http_client::headers headers {{"Authorization", GetAuthorization(meta->projectId)}};

    return m_api->download("/projects/" + meta->projectId + "/exchange/export/" + locale, headers)
            .then([=](downloaded_file file)
            {
                wxString outfile(output_file);
                file.move_to(outfile);
            });
}


dispatch::future<void> LocalazyClient::DownloadFile(const std::wstring& output_file, const ProjectInfo& project, const ProjectFile& file, const Language& lang)
{
    auto internal = std::static_pointer_cast<FileInternal>(file.internal);

    auto meta = std::make_shared<LocalazySyncMetadata>();
    meta->projectId = std::get<std::string>(project.internalID);
    meta->langCode = internal->tagToLocale.at(lang.LanguageTag());

    return DownloadFile(output_file, meta);
}


dispatch::future<void> LocalazyClient::UploadFile(const std::string& file_buffer, std::shared_ptr<FileSyncMetadata> meta_)
{
    class upload_json_data : public octet_stream_data
    {
    public:
        using octet_stream_data::octet_stream_data;
        std::string content_type() const override { return "application/json"; };
    };

    auto meta = std::dynamic_pointer_cast<LocalazySyncMetadata>(meta_);

    std::string prefix("/projects/" + meta->projectId + "/exchange");
    http_client::headers headers {{"Authorization", GetAuthorization(meta->projectId)}};

    return m_api->post(prefix + "/import", upload_json_data(file_buffer), headers)
        .then([this,prefix,headers] (json r) {
            auto ok = r.at("result").get<bool>();
            if (!ok)
            {
                throw Exception(_(L"There was an error when uploading translations to Localazy."));
            }

            auto status_url = prefix + "/status/" + r.at("statusId").get<std::string>();
            // wait until processing finishes, by polling status:
            while (true)
            {
                std::this_thread::sleep_for(500ms);
                auto status = m_api->get(status_url, headers).get();
                auto text = status.at("status").get<std::string>();
                if (text == "done")
                    return;
                if (text != "in_progress" && text != "scheduled")
                {
                    throw Exception(_(L"There was an error when uploading translations to Localazy."));
                }
            };
        });
}


bool LocalazyClient::IsSignedIn() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_tokens->is_valid() && m_metadata->is_valid();
}


void LocalazyClient::SignOut()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    m_metadata->clear();
    m_tokens->clear();
    SaveMetadataAndTokens(guard);
}


// ----------------------------------------------------------------
// Singleton management
// ----------------------------------------------------------------

static std::once_flag initializationFlag;
LocalazyClient* LocalazyClient::ms_instance = nullptr;

LocalazyClient& LocalazyClient::Get()
{
    std::call_once(initializationFlag, []() {
        ms_instance = new LocalazyClient;
    });
    return *ms_instance;
}

void LocalazyClient::CleanUp()
{
    if (ms_instance)
    {
        delete ms_instance;
        ms_instance = nullptr;
    }
}
