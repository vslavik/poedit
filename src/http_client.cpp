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

#include "http_client.h"

#include "utility.h"
#include "str_helpers.h"

#include <iomanip>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <wx/filename.h>
#include <wx/uri.h>


class downloaded_file::impl
{
public:
    impl(std::string filename)
    {
        // filter out invalid characters in filenames
        std::replace_if(filename.begin(), filename.end(), boost::is_any_of("\\/:\"<>|?*"), '_');
        m_fn = m_tmpdir.CreateFileName(!filename.empty() ? str::to_wx(filename) : "data");
    }

    wxFileName filename() const { return m_fn; }

    void move_to(const wxFileName& target)
    {
        TempOutputFileFor::ReplaceFile(m_fn.GetFullPath(), target.GetFullPath());
    }

private:
    TempDirectory m_tmpdir;
    wxFileName m_fn;
};

downloaded_file::downloaded_file(const std::string& filename) : m_impl(new impl(filename)) {}
wxFileName downloaded_file::filename() const { return m_impl->filename(); }
void downloaded_file::move_to(const wxFileName& target) { return m_impl->move_to(target); }
downloaded_file::~downloaded_file() {}


multipart_form_data::multipart_form_data()
{
    boost::uuids::random_generator gen;
    m_boundary = boost::uuids::to_string(gen());
}

void multipart_form_data::add_value(const std::string& name, const std::string& value)
{
    m_body += "--" + m_boundary + "\r\n";
    m_body += "Content-Disposition: form-data; name=\"" + name + "\"\r\n\r\n";
    m_body += value;
    m_body += "\r\n";
}

void multipart_form_data::add_file(const std::string& name, const std::string& filename, const std::string& file_content)
{
    m_body += "--" + m_boundary + "\r\n";
    m_body += "Content-Disposition: form-data; name=\"" + name + "\"; filename=\"" + filename + "\"\r\n";
    m_body += "Content-Type: application/octet-stream\r\n";
    m_body += "Content-Transfer-Encoding: binary\r\n";
    m_body += "\r\n";
    m_body += file_content;
    m_body += "\r\n";
}

std::string multipart_form_data::content_type() const
{
    return "multipart/form-data; boundary=" + m_boundary;
}

std::string multipart_form_data::body() const
{
    return m_body + "--" + m_boundary + "--\r\n\r\n";
}


void urlencoded_data::add_value(const std::string& name, const std::string& value)
{
    if (!m_body.empty())
        m_body += '&';
    m_body += name;
    m_body += '=';
    m_body += http_client::url_encode(value);
}


json_data::json_data(const json& data)
{
    m_body = data.dump();
}


dispatch::future<downloaded_file> http_client::download_from_anywhere(const std::string& url, const headers& hdrs)
{
    // http_client requires that all requests are relative to the provided prefix
    // (this is a C++REST SDK limitation enforced on some platforms), so we need
    // to determine the URL's prefix, create a transient http_client for it and
    // use it to perform the request.

    wxURI uri(url);
    const std::string prefix = str::to_utf8(uri.GetScheme() + "://" + uri.GetServer());

    auto transient = std::make_shared<http_client>(prefix);
    return transient->download(url, hdrs)
           .then([transient](downloaded_file file)
           {
               // The entire purpose of this otherwise-useless closure is to
               // capture the `transient` http_client instance and ensure it won't
               // be destroyed too early.
               //
               // It will only be released at this point.
               return file;
           });
}


std::string http_client::url_encode(const std::string& s, int flags)
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
        else if (c == ' ' && !(flags & encode_no_plus))
        {
            escaped << '+';
        }
        else if (c == '/' && (flags & encode_keep_slash))
        {
            escaped << '/';
        }
        else
        {
            escaped << '%' << std::setw(2) << int((unsigned char)c);
        }
    }

    return escaped.str();
}

std::string http_client::url_encode(const std::wstring& s, int flags)
{
    return url_encode(str::to_utf8(s), flags);
}
