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
#include "str_helpers.h"

#include <boost/algorithm/string/predicate.hpp>

#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/filestream.h>

#ifdef _WIN32
    #include <windows.h>
    #include <wininet.h>
#endif

using namespace web;

namespace
{

using utility::string_t;
using utility::conversions::to_string_t;

#ifndef _UTF16_STRINGS
inline string_t to_string_t(const std::wstring& s) { return str::to_utf8(s); }
#endif

class gzip_compression_support : public http::http_pipeline_stage
{
public:
    pplx::task<http::http_response> propagate(http::http_request request) override
    {
        request.headers().add(http::header_names::accept_encoding, L"gzip");

        return next_stage()->propagate(request).then([](http::http_response response) -> pplx::task<http::http_response>
        {
            std::wstring encoding;
            if (response.headers().match(http::header_names::content_encoding, encoding) && encoding == L"gzip")
            {
                return response.extract_vector().then([response](std::vector<unsigned char> compressed) mutable -> http::http_response
                {
                    namespace io = boost::iostreams;

                    io::array_source source(reinterpret_cast<char*>(compressed.data()), compressed.size());
                    io::filtering_istream in;
                    in.push(io::gzip_decompressor());
                    in.push(source);

                    std::vector<char> decompressed;
                    io::back_insert_device<std::vector<char>> sink(decompressed);
                    io::copy(in, sink);

                    response.set_body(concurrency::streams::bytestream::open_istream(std::move(decompressed)), decompressed.size());
                    return response;
                });
            }
            else
            {
                return pplx::task_from_result(response);
            }
        });
    }
};

} // anonymous namespace

struct json_dict::native
{
    native(const web::json::value& x) : val(x) {}

    web::json::value val;

    const web::json::value& get(const std::string& key)
    {
        return val.at(to_string_t(key));
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
    return str::to_utf8(m_native->get(name).as_string());
}

std::wstring json_dict::wstring(const char *name) const
{
    return str::to_wstring(m_native->get(name).as_string());
}

int json_dict::number(const char *name) const
{
    return m_native->get(name).as_integer();
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
    impl(http_client& owner, const std::string& url_prefix, int flags)
        : m_owner(owner), m_native(sanitize_url(url_prefix, flags))
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

        std::shared_ptr<http::http_pipeline_stage> gzip_stage = std::make_shared<gzip_compression_support>();
        m_native.add_handler(gzip_stage);
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
        req.set_request_uri(to_string_t(url));

        m_native.request(req)
        .then([=](http::http_response response)
        {
            handle_error(response);
            handler(make_json_dict(response.extract_json().get()));
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

        fstream::open_ostream(to_string_t(output_file)).then([=](ostream outFile)
        {
            *fileStream = outFile;
            http::http_request req(http::methods::GET);
            req.headers().add(http::header_names::user_agent, m_userAgent);
            if (!m_auth.empty())
                req.headers().add(http::header_names::authorization, m_auth);
            req.set_request_uri(to_string_t(url));
            return m_native.request(req);
        })
        .then([=](http::http_response response)
        {
            handle_error(response);
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
                handler(json_dict());
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
        req.set_request_uri(to_string_t(url));

        auto body = data.body();
        req.set_body(body, data.content_type());
        req.headers().set_content_length(body.size());

        m_native.request(req)
        .then([=](http::http_response response)
        {
            handle_error(response);
            handler(json_dict());
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
    // handle non-OK responses:
    void handle_error(http::http_response r)
    {
        if (r.status_code() == http::status_codes::OK)
            return; // not an error

        int status_code = r.status_code();
        std::string msg;
        if (r.headers().content_type() == L"application/json")
        {
            try
            {
                auto json = make_json_dict(r.extract_json().get());
                msg = m_owner.parse_json_error(json);
            }
            catch (...) {} // report original error if parsing broken
        }
        if (msg.empty())
            msg = str::to_utf8(r.reason_phrase());
        m_owner.on_error_response(status_code, msg);
        throw http::http_exception(status_code, msg);
    }

    // convert to wstring and make WinXP ready
    static string_t sanitize_url(const std::string& url, int flags)
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
                    return U("http://") + string_t(url.begin() + 8, url.end());
            }
        }
    #endif
        return to_string_t(url);
    }

private:
    http_client& m_owner;
    http::client::http_client m_native;
    std::wstring m_userAgent;
    std::wstring m_auth;
};



http_client::http_client(const std::string& url_prefix, int flags)
    : m_impl(new impl(*this, url_prefix, flags))
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
