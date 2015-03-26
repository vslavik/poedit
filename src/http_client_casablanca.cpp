/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2014-2015 Vaclav Slavik
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

#include "http_client.h"

#include "version.h"

#include <boost/algorithm/string/predicate.hpp>
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/filestream.h>

#ifdef _WIN32
    #include <windows.h>
    #include <wininet.h>
#endif


using namespace web;

struct json_dict::native
{
    native(const web::json::value& x) : val(x) {}

    web::json::value val;

    const web::json::value& get(const std::string& key)
    {
        return val.at(std::wstring(key.begin(), key.end()));
    }
};

static inline json_dict make_json_dict(const web::json::value& x)
{
    return std::make_shared<json_dict::native>(x);
}

bool json_dict::is_null(const char *name) const
{
    return m_native->get(name).is_null();
}

json_dict json_dict::subdict(const char *name) const
{
    return make_json_dict(m_native->get(name));
}

std::string json_dict::utf8_string(const char *name) const
{
    return utility::conversions::to_utf8string(m_native->get(name).as_string());
}

std::wstring json_dict::wstring(const char *name) const
{
    return m_native->get(name).as_string();
}

int json_dict::number(const char *name) const
{
    auto& val = m_native->get(name);
    if (val.is_integer())
    {
        return val.as_integer();
    }
    else
    {
        // Some broken APIs may return strings instead of numbers, so lets try
        // that too as a fallback
        return std::stoi(val.as_string());
    }
}

double json_dict::double_number(const char *name) const
{
    return m_native->get(name).as_double();
}

void json_dict::iterate_array(const char *name, std::function<void(const json_dict&)> on_item) const
{
    auto& val = m_native->get(name);
    if (!val.is_array())
        return;
    auto size = val.size();
    for (size_t i = 0; i < size; ++i)
    {
        on_item(make_json_dict(val.at(i)));
    }
}



class http_client::impl
{
public:
    impl(const std::string& url_prefix, int flags) : m_native(sanitize_url(url_prefix, flags))
    {
        #define make_wide_str(x) make_wide_str_(x)
        #define make_wide_str_(x) L ## x

        #if defined(_WIN32)
            #define USER_AGENT_PLATFORM L" (Windows)"
        #elif defined(__unix__)
            #define USER_AGENT_PLATFORM L" (Unix)"
        #else
            #define USER_AGENT_PLATFORM
        #endif
        m_userAgent = L"Poedit/" make_wide_str(POEDIT_VERSION) USER_AGENT_PLATFORM;
    }

    ~impl()
    {
    }

    bool is_reachable() const
    {
    #ifdef _WIN32
        DWORD flags;
        return ::InternetGetConnectedState(&flags, 0);
    #else
        return true; // TODO
    #endif
    }

    void set_authorization(const std::string& auth)
    {
        m_auth = std::wstring(auth.begin(), auth.end());
    }

    void get(const std::string& url,
             std::function<void(const http_response&)> handler)
    {
        http::http_request req(http::methods::GET);
        req.headers().add(http::header_names::accept,     L"application/json");
        req.headers().add(http::header_names::user_agent, m_userAgent);
        if (!m_auth.empty())
            req.headers().add(http::header_names::authorization, m_auth);
        req.set_request_uri(std::wstring(url.begin(), url.end()));

        m_native.request(req)
        .then([=](http::http_response response)
        {
            try
            {
                http_response r;
                r.m_ok = (response.status_code() == http::status_codes::OK);
                r.m_data = make_json_dict(response.extract_json().get());
                handler(r);
            }
            catch (...)
            {
                handler(std::current_exception());
            }
        })
        .then([=](pplx::task<void> t)
        {
            try { t.get(); }
            catch (...)
            {
                handler(std::current_exception());
            }
        });
    }

    void download(const std::string& url, const std::wstring& output_file, response_func_t handler)
    {
        using namespace concurrency::streams;
        auto fileStream = std::make_shared<ostream>();

        fstream::open_ostream(output_file).then([=](ostream outFile)
        {
            *fileStream = outFile;
            http::http_request req(http::methods::GET);
            req.headers().add(http::header_names::user_agent, m_userAgent);
            if (!m_auth.empty())
                req.headers().add(http::header_names::authorization, m_auth);
            req.set_request_uri(std::wstring(url.begin(), url.end()));
            return m_native.request(req);
        })
        .then([=](http::http_response response)
        {
            if (response.status_code() != http::status_codes::OK)
                throw http::http_exception(response.status_code(), response.reason_phrase());
            return response.body().read_to_end(fileStream->streambuf());
        })
        .then([=](size_t)
        {
            return fileStream->close();
        })
        .then([=](pplx::task<void> t)
        {
            try
            {
                t.get();
                handler(http_response());
            }
            catch (...)
            {
                handler(std::current_exception());
            }
        });
    }

    void post(const std::string& url, const multipart_form_data& data, response_func_t handler)
    {
        http::http_request req(http::methods::POST);
        req.headers().add(http::header_names::accept,     L"application/json");
        req.headers().add(http::header_names::user_agent, m_userAgent);
        if (!m_auth.empty())
            req.headers().add(http::header_names::authorization, m_auth);
        req.set_request_uri(std::wstring(url.begin(), url.end()));

        auto body = data.body();
        req.set_body(body, data.content_type());
        req.headers().set_content_length(body.size());

        m_native.request(req)
        .then([=](http::http_response response)
        {
            try
            {
                http_response r;
                r.m_ok = (response.status_code() == http::status_codes::OK);
                handler(r);
            }
            catch (...)
            {
                handler(std::current_exception());
            }
        })
        .then([=](pplx::task<void> t)
        {
            try { t.get(); }
            catch (...)
            {
                handler(std::current_exception());
            }
        });
    }

private:
    // convert to wstring and make WinXP ready
    static std::wstring sanitize_url(const std::string& url, int flags)
    {
        (void)flags;
    #ifdef _WIN32
        if (flags & http_client::uses_sni)
        {
            // Windows XP doesn't support SNI and so can't connect over SSL to
            // hosts that use it. The use of SNI is increasingly common and some
            // APIs Poedit needs to connect to use it. To keep things simple, just
            // disable SSL on Windows XP.
            OSVERSIONINFOEX info;
            ZeroMemory(&info, sizeof(info));
            info.dwOSVersionInfoSize = sizeof(info);
            GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&info));
            if (info.dwMajorVersion < 6) // XP
            {
                if (boost::starts_with(url, "https://"))
                    return L"http://" + std::wstring(url.begin() + 8, url.end());
            }
        }
    #endif
        return std::wstring(url.begin(), url.end());
    }

private:
    http::client::http_client m_native;
    std::wstring m_userAgent;
    std::wstring m_auth;
};



http_client::http_client(const std::string& url_prefix, int flags)
    : m_impl(new impl(url_prefix, flags))
{
}

http_client::~http_client()
{
}

bool http_client::is_reachable() const
{
    return m_impl->is_reachable();
}

void http_client::set_authorization(const std::string& auth)
{
    m_impl->set_authorization(auth);
}

void http_client::get(const std::string& url,
                      std::function<void(const http_response&)> handler)
{
    m_impl->get(url, handler);
}

void http_client::download(const std::string& url, const std::wstring& output_file, response_func_t handler)
{
    m_impl->download(url, output_file, handler);
}

void http_client::post(const std::string& url, const multipart_form_data& data, response_func_t handler)
{
    m_impl->post(url, data, handler);
}
