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

#ifndef Poedit_http_client_h
#define Poedit_http_client_h

#include <codecvt>
#include <exception>
#include <functional>
#include <iomanip>
#include <locale>
#include <memory>
#include <sstream>
#include <string>

class http_response;


class json_dict
{
public:
    struct native;
    json_dict() {}
    json_dict(std::shared_ptr<native> value) : m_native(value) {}

    bool is_null(const char *name) const;

    json_dict subdict(const char *name) const;

    std::string utf8_string(const char *name) const;
    std::wstring wstring(const char *name) const;
#ifdef __APPLE__
    std::string native_string(const char *name) const { return utf8_string(name); }
#else
    std::wstring native_string(const char *name) const { return wstring(name); }
#endif

    int number(const char *name) const;
    double double_number(const char *name) const;
    void iterate_array(const char *name, std::function<void(const json_dict&)> on_item);

private:
    std::shared_ptr<native> m_native;
};


/**
    Client for accessing HTTP REST APIs.
 */
class http_client
{
public:
    /// Connection flags for the client.
    enum flags
    {
        /// The host uses SNI for SSL. Will use http instead of https on Windows XP.
        uses_sni = 1
    };

    /**
        Creates an instance of the client object.
        
        The client is good for accessing URLs with the provided prefix
        (which may be any prefix, not just the hostname).
        
        @param flags OR-combination of http_client::flags values.
     */
    http_client(const std::string& url_prefix, int flags = 0);
    ~http_client();

    /// Return true if the server is reachable, i.e. client is online
    bool is_reachable() const;

    /// Perform a request at the given URL and call function to handle the result
    void request(const std::string& url,
                 std::function<void(const http_response&)> handler);

    /// Perform a request at the given URL and ignore the response.
    void request(const std::string& url)
    {
        request(url, [](const http_response&){});
    }

    // Helper for encoding text as URL-encoded UTF-8
    static std::string url_encode(const std::string& s)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (auto c: s)
        {
            if (c == '-' || c == '_' || c == '.' || c == '~' ||
                (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9'))
            {
                escaped << c;
            }
            else
            {
                escaped << '%' << std::setw(2) << int((unsigned char)c);
            }
        }

        return escaped.str();
    }

    static std::string url_encode(const std::wstring& s)
    {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> convert;
        return url_encode(convert.to_bytes(s));
    }

private:
    class impl;
    friend class http_response;

    std::unique_ptr<impl> m_impl;
};


/// Response to a HTTP request
class http_response
{
public:
    http_response() : m_ok(true) {}
    http_response(std::exception_ptr e) : m_ok(false), m_error(e) {}

    /// Is the response acceptable?
    bool ok() const { return m_ok; }

    /// Returns json data from the response. May throw if there was error.
    const json_dict& json() const
    {
        if (m_error)
            std::rethrow_exception(m_error);
        return m_data;
    }

private:
    bool m_ok;
    std::exception_ptr m_error;
    json_dict m_data;

    friend class http_client::impl;
};

#endif // Poedit_http_client_h
