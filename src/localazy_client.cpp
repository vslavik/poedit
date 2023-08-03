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

#include <iostream>

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
            return response.at("error").get<std::string>();
        }
        catch (...)
        {
            return std::string();
        }
    }

    void on_error_response(int& statusCode, std::string& message) override
    {
        if (statusCode == 401/*Unauthorized*/)
        {
            // message is e.g. "The access token provided is invalid"
            message = _("Not authorized, please sign in again.").utf8_str();
        }
        wxLogTrace("poedit.localazy", "JSON error: %s", message.c_str());
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


void LocalazyClient::HandleAuthCallback(const std::string& uri)
{
    wxLogTrace("poedit.localazy", "Callback URI %s", uri.c_str());

    if (!m_authCallback)
        return;

    std::smatch m;
    if (!(std::regex_search(uri, m, std::regex("//localazy/oauth/([^&]+)")) && m.size() > 1))
        return;

    ExchangeTemporaryToken(m.str(1))
    .then_on_main([=](){
        m_authCallback->set_value();
        m_authCallback.reset();
    });
}


dispatch::future<void> LocalazyClient::ExchangeTemporaryToken(const std::string& token)
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

               m_metadata->add(projectId, project, user);
               m_tokens->add(projectId, token);
               SaveMetadataAndTokens();
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


void LocalazyClient::SaveMetadataAndTokens()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    Config::LocalazyMetadata(m_metadata->to_string());

    auto encoded_tokens = m_tokens->to_string();
    if (encoded_tokens.empty())
        keytar::DeletePassword("Localazy", "");
    else
        keytar::AddPassword("Localazy", "", "1:" + encoded_tokens);
}


dispatch::future<CloudAccountClient::UserInfo> LocalazyClient::GetUserInfo()
{
    std::lock_guard<std::mutex> guard(m_mutex);

    auto user = m_metadata->user();
    UserInfo info;
    info.service = SERVICE_ID;
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
            SERVICE_ID,
            p.at("id").get<std::string>(),
            p.at("name").get<std::wstring>(),
            p.at("slug").get<std::string>(),
            p.at("image").get<std::string>()
        });
    }

    return dispatch::make_ready_future(std::move(all));
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
    SaveMetadataAndTokens();
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
