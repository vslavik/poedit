/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2015-2017 Vaclav Slavik
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

#include "str_helpers.h"

#include <iomanip>
#include <sstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>


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
