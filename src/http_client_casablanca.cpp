/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2017 Vaclav Slavik
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

#include <boost/throw_exception.hpp>

#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_client.h>
#include <cpprest/http_msg.h>
#include <cpprest/filestream.h>

#ifdef _WIN32
    #include <windows.h>
    #include <winhttp.h>
    #include <netlistmgr.h>
    #pragma comment(lib, "ole32.lib")

    // can't include both winhttp.h and wininet.h, so put a declaration here
    //#include <wininet.h>
    EXTERN_C DECLSPEC_IMPORT BOOL STDAPICALLTYPE InternetGetConnectedState(__out LPDWORD lpdwFlags, __reserved DWORD dwReserved);
#endif

namespace
{

using namespace web;
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
            string_t encoding;
            if (response.headers().match(http::header_names::content_encoding, encoding) && encoding == _XPLATSTR("gzip"))
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


class http_client::impl
{
public:
    impl(http_client& owner, const std::string& url_prefix, int flags)
        : m_owner(owner),
          m_native
          (
              sanitize_url(url_prefix, flags)
            #ifdef _WIN32
              , get_client_config()
            #endif
          )
    {
        #define make_wide_str(x) make_wide_str_(x)
        #define make_wide_str_(x) L ## x

        #if defined(_WIN32)
            #define USER_AGENT_PLATFORM L" (Windows NT " + windows_version() + L")"
        #elif defined(__unix__)
            #define USER_AGENT_PLATFORM L" (Unix)"
        #else
            #define USER_AGENT_PLATFORM
        #endif
        m_userAgent = L"Poedit/" make_wide_str(POEDIT_VERSION) USER_AGENT_PLATFORM;

        std::shared_ptr<http::http_pipeline_stage> gzip_stage = std::make_shared<gzip_compression_support>();
        m_native.add_handler(gzip_stage);

    #ifdef _WIN32
        m_networkListManager = nullptr;
        CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, IID_INetworkListManager, (LPVOID *)&m_networkListManager);
    #endif
    }

    ~impl()
    {
    #ifdef _WIN32
        if (m_networkListManager)
            m_networkListManager->Release();
    #endif
    }

    static string_t ui_language;

    bool is_reachable() const
    {
    #ifdef _WIN32
        if (m_networkListManager)
        {
            NLM_CONNECTIVITY result;
            HRESULT hr = m_networkListManager->GetConnectivity(&result);
            if (SUCCEEDED(hr))
                return result & (NLM_CONNECTIVITY_IPV4_INTERNET|NLM_CONNECTIVITY_IPV6_INTERNET);
        }
        // manager fallback (IPv6 ignorant):
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

    dispatch::future<::json> get(const std::string& url)
    {
        http::http_request req(http::methods::GET);
        req.headers().add(http::header_names::accept,     L"application/json");
        req.headers().add(http::header_names::user_agent, m_userAgent);
        req.headers().add(http::header_names::accept_language, ui_language);
        if (!m_auth.empty())
            req.headers().add(http::header_names::authorization, m_auth);
        req.set_request_uri(to_string_t(url));

        return
        m_native.request(req)
        .then([=](http::http_response response)
        {
            handle_error(response);
            return ::json::parse(response.extract_utf8string().get());
        });
    }

    dispatch::future<void> download(const std::string& url, const std::wstring& output_file)
    {
        using namespace concurrency::streams;
        auto fileStream = std::make_shared<ostream>();

        return
        fstream::open_ostream(to_string_t(output_file)).then([=](ostream outFile)
        {
            *fileStream = outFile;
            http::http_request req(http::methods::GET);
            req.headers().add(http::header_names::user_agent, m_userAgent);
            req.headers().add(http::header_names::accept_language, ui_language);
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
        });
    }

    dispatch::future<::json> post(const std::string& url, const http_body_data& data)
    {
        http::http_request req(http::methods::POST);
        req.headers().add(http::header_names::accept,     L"application/json");
        req.headers().add(http::header_names::user_agent, m_userAgent);
        req.headers().add(http::header_names::accept_language, ui_language);
        if (!m_auth.empty())
            req.headers().add(http::header_names::authorization, m_auth);
        req.set_request_uri(to_string_t(url));

        auto body = data.body();
        req.set_body(body, data.content_type());
        req.headers().set_content_length(body.size());

        return
        m_native.request(req)
        .then([=](http::http_response response)
        {
            handle_error(response);
            return ::json::parse(response.extract_utf8string().get());
        });
    }

private:
    // handle non-OK responses:
    void handle_error(http::http_response r)
    {
        if (r.status_code() >= 200 && r.status_code() < 300)
            return; // not an error

        int status_code = r.status_code();
        std::string msg;
        if (r.headers().content_type() == L"application/json")
        {
            try
            {
                auto json = ::json::parse(r.extract_utf8string().get());
                msg = m_owner.parse_json_error(json);
            }
            catch (...) {} // report original error if parsing broken
        }
        if (msg.empty())
            msg = str::to_utf8(r.reason_phrase());
        m_owner.on_error_response(status_code, msg);
        BOOST_THROW_EXCEPTION(http::http_exception(status_code, msg));
    }

    // convert to wstring
    static string_t sanitize_url(const std::string& url, int /*flags*/)
    {
        return to_string_t(url);
    }

#ifdef _WIN32
    // prepare WinHttp configuration
    static http::client::http_client_config get_client_config()
    {
        http::client::http_client_config c;

        // WinHttp doesn't share WinInet/MSIE's proxy settings and has its own,
        // but many users don't have properly configured both. Adopting proxy
        // settings like this in desktop software is recommended behavior, see
        // https ://blogs.msdn.microsoft.com/ieinternals/2013/10/11/understanding-web-proxy-configuration/
        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ieConfig = { 0 };
        if (WinHttpGetIEProxyConfigForCurrentUser(&ieConfig))
        {
            if (ieConfig.fAutoDetect)
            {
                c.set_proxy(web_proxy::use_auto_discovery);
            }
            if (ieConfig.lpszProxy)
            {
                // Explicitly add // to the URL to work around a bug in C++ REST SDK's
                // parsing of proxies with port number in their address
                // (see https://github.com/Microsoft/cpprestsdk/issues/57)
                c.set_proxy(uri(L"//" + std::wstring(ieConfig.lpszProxy)));
            }
        }
        return c;
    }

    static std::wstring windows_version()
    {
        OSVERSIONINFOEX info = { 0 };
        info.dwOSVersionInfoSize = sizeof(info);

        NTSTATUS(WINAPI *fRtlGetVersion)(LPOSVERSIONINFOEXW);
        fRtlGetVersion = reinterpret_cast<decltype(fRtlGetVersion)>(GetProcAddress(GetModuleHandleA("ntdll"), "RtlGetVersion"));
        if (fRtlGetVersion)
            fRtlGetVersion(&info);

        return std::to_wstring(info.dwMajorVersion) + L"." + std::to_wstring(info.dwMinorVersion);
    }

    INetworkListManager *m_networkListManager;
#endif

    http_client& m_owner;
    http::client::http_client m_native;
    std::wstring m_userAgent;
    std::wstring m_auth;
};


string_t http_client::impl::ui_language;


http_client::http_client(const std::string& url_prefix, int flags)
    : m_impl(new impl(*this, url_prefix, flags))
{
}

http_client::~http_client()
{
}

void http_client::set_ui_language(const std::string& lang)
{
    impl::ui_language = to_string_t(lang);
}

bool http_client::is_reachable() const
{
    return m_impl->is_reachable();
}

void http_client::set_authorization(const std::string& auth)
{
    m_impl->set_authorization(auth);
}

dispatch::future<::json> http_client::get(const std::string& url)
{
    return m_impl->get(url);
}

dispatch::future<void> http_client::download(const std::string& url, const std::wstring& output_file)
{
    return m_impl->download(url, output_file);
}

dispatch::future<::json> http_client::post(const std::string& url, const http_body_data& data)
{
    return m_impl->post(url, data);
}
