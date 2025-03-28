/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2025 Vaclav Slavik
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

#ifdef HAVE_HTTP_CLIENT

#include "concurrency.h"
#include "json.h"

#include <wx/filename.h>

#include <exception>
#include <map>
#include <memory>
#include <string>

class http_client;


/**
    File downloaded using http_client::download().

    The file is stored on disk in a unique temporary directory using filename
    corresponding to the URL.

    The temporary directory and the file exist only for the lifetime of the
    downloaded_file object and are deleted after destructions. You must read the
    file or move it elsewhere before that.
 */
class downloaded_file
{
public:
    downloaded_file(const std::string& filename = "", const std::string& etag = "");
    ~downloaded_file();

    /// Return location of the temporary file
    wxFileName filename() const;

    /// Return downloaded file's ETag if present or empty string otherwise
    std::string etag() const;

    /// Move the file to a different location
    void move_to(const wxFileName& target);

private:
    class impl;
    // would prefer unique_ptr and movable-only downloaded_file class,
    // but pplx concurrency runtime in VC++ can't hold movable-only results
    // (https://github.com/microsoft/cpprestsdk/pull/47)
    std::shared_ptr<impl> m_impl;
};


/// Abstract base class for encoded body data
class http_body_data
{
public:
    http_body_data() {}
    virtual ~http_body_data() {}

    /// Content-Type header to use with the data.
    virtual std::string content_type() const = 0;

    /// Returns generated body of the request.
    virtual std::string body() const = 0;
};

/// Stores unspecified binary data
class octet_stream_data : public http_body_data
{
public:
    octet_stream_data(const std::string& body) : m_body(body) {};

    /// Content-Type header to use with the data.
    std::string content_type() const override { return "application/octet-stream"; };

    /// Returns generated body of the request.
    std::string body() const override { return m_body; };
private:
    std::string m_body;
};

/// Stores POSTed data (RFC 1867)
class multipart_form_data : public http_body_data
{
public:
    multipart_form_data();

    /// Add a form value.
    void add_value(const std::string& name, const std::string& value);

    /// Add file upload.
    void add_file(const std::string& name, const std::string& filename, const std::string& file_content);

    std::string content_type() const override;
    std::string body() const override;

private:
    std::string m_boundary;
    std::string m_body;
};

/// Stores application/x-www-form-urlencoded data
class urlencoded_data : public http_body_data
{
public:
    /// Add a form value.
    void add_value(const std::string& name, const std::string& value);

    std::string content_type() const override { return "application/x-www-form-urlencoded"; }
    std::string body() const override { return m_body; }

private:
    std::string m_body;
};

/// Stores application/json data
class json_data : public http_body_data
{
public:
    json_data(const json& data);

    std::string content_type() const override { return "application/json"; }
    std::string body() const override { return m_body; }

private:
    std::string m_body;
};


// Exception thrown when HTTP request fails with error status code
class http_response_error : public std::runtime_error
{
public:
    http_response_error(int status, const std::string& what) : std::runtime_error(what), m_status(status) {}

    int status_code() const { return m_status; }

private:
    int m_status;
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
        // currently no flags are used
        default_flags = 0
    };

    using headers = std::vector<std::pair<std::string, std::string>>;

    /**
        Creates an instance of the client object.
        
        The client is good for accessing URLs with the provided prefix
        (which may be any prefix, not just the hostname).
        
        @param flags OR-combination of http_client::flags values.
     */
    http_client(const std::string& url_prefix, int flags = default_flags);
    virtual ~http_client();

#ifndef __DARWIN__
    /// Sets Accept-Language to use (language tag; managed automatically on macOS)
    static void set_ui_language(const std::string& lang);
#endif

    /// Sets Authorization header to be used in all requests
    void set_authorization(const std::string& auth);

    /// Perform a GET request at the given URL
    dispatch::future<json> get(const std::string& url, const headers& hdrs = headers());

    /**
        Perform a GET request and store the body in a file.

        This method supports ETag handling. If the headers include If-None-Match value
        and the server returns 304 Not Modified, downloaded_file is not returned and
        an exception is thrown instead.
     */
    dispatch::future<downloaded_file> download(const std::string& url, const headers& hdrs = headers());

    /**
        Convenience variant of download() for downloading without having full http_client.

        This is useful e.g. when downloading from unknown host. @a url is absolute URL.
     */
    static dispatch::future<downloaded_file> download_from_anywhere(const std::string& url, const headers& hdrs = headers());

    /**
        Perform a POST request with multipart/form-data formatted @a params.
     */
    dispatch::future<json> post(const std::string& url, const http_body_data& data, const headers& hdrs = headers());


    // Helper for encoding text as URL-encoded UTF-8

    enum encode_flags
    {
        encode_no_plus = 1,     // don't encode spaces as +
        encode_keep_slash = 2,  // don't encode / as %2f
    };

    static std::string url_encode(const std::string& s, int flags = 0);
    static std::string url_encode(const std::wstring& s, int flags = 0);

protected:
    /**
        Extract more detailed, client specific error response from the
        JSON body of error response, if available.
        
        Does nothing by default, but can be overridden in derived class.
     */
    virtual std::string parse_json_error(const json& /*response*/) const
        { return std::string(); }

    /**
         Called when an error response is returned, before calling error handler.

         Can be used to react to specific errors, e.g. invalidate expired OAuth tokens,
         or to modify the response.
     */
    virtual void on_error_response(int& /*statusCode*/, std::string& /*message*/) {};

private:
    class impl;
    std::unique_ptr<impl> m_impl;
};

/// Monitor if networking is available
class http_reachability
{
public:
    /// Creates an instance of the monitor object.
    http_reachability(const std::string& url);
    ~http_reachability();

    /// Return true if the host is reachable, i.e. client is online
    bool is_reachable() const;

private:
    class impl;
    std::unique_ptr<impl> m_impl;
};

#endif // HAVE_HTTP_CLIENT

#endif // Poedit_http_client_h
