/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2020 Vaclav Slavik
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
#include "version.h"


class http_exception : public std::runtime_error
{
public:
    http_exception(const std::string& what) : std::runtime_error(what) {}
};


class http_client::impl
{
public:
    impl(http_client& owner, const std::string& url_prefix, int /*flags*/)
        : m_owner(owner), m_authHeader(nil)
    {
        int majorVersion, minorVersion, patchVersion;
        NSOperatingSystemVersion macos = [[NSProcessInfo processInfo] operatingSystemVersion];
        majorVersion = (int)macos.majorVersion;
        minorVersion = (int)macos.minorVersion;
        patchVersion = (int)macos.patchVersion;
        NSString *macos_str = (patchVersion == 0)
                            ? [NSString stringWithFormat:@"%d.%d", majorVersion, minorVersion]
                            : [NSString stringWithFormat:@"%d.%d.%d", majorVersion, minorVersion, patchVersion];
        NSString *user_agent = [NSString stringWithFormat:@"Poedit/%s (Mac OS X %@)", POEDIT_VERSION, macos_str];

        auto config = [NSURLSessionConfiguration defaultSessionConfiguration];
        config.HTTPAdditionalHeaders = @{
            @"User-Agent": user_agent,
            @"Accept": @"application/json"
        };

        NSString *str = str::to_NS(url_prefix);

        m_baseURL = [NSURL URLWithString:str];
        m_session = [NSURLSession sessionWithConfiguration:config];
    }

    ~impl()
    {
        m_session = nil;
    }

    void set_authorization(const std::string& auth)
    {
        m_authHeader = auth.empty() ? nil : str::to_NS(auth);
    }

    dispatch::future<json> get(const std::string& url)
    {
        auto promise = std::make_shared<dispatch::promise<json>>();

        auto request = build_request(@"GET", url);
        auto task = [m_session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            try
            {
                if (handle_error(data, response, error, *promise))
                    return;
                promise->set_value(extract_json(data));
            }
            catch (...)
            {
                dispatch::set_current_exception(promise);
            }
        }];
        [task resume];

        return promise->get_future();
    }

    dispatch::future<void> download(const std::string& url, const std::wstring& output_file)
    {
        auto promise = std::make_shared<dispatch::promise<void>>();

        NSString *outputPath = str::to_NS(output_file);
        auto request = build_request(@"GET", url);
        auto task = [m_session downloadTaskWithRequest:request completionHandler:^(NSURL *location, NSURLResponse *response, NSError *error) {
            try
            {
                if (handle_error(nil, response, error, *promise))
                    return;

                NSError *err = nil;
                if (![[NSFileManager defaultManager] moveItemAtPath:[location path] toPath:outputPath error:&err])
                    throw std::runtime_error(str::to_utf8([err localizedDescription]));
                promise->set_value();
            }
            catch (...)
            {
                dispatch::set_current_exception(promise);
            }
        }];
        [task resume];

        return promise->get_future();
    }

    dispatch::future<json> post(const std::string& url, const http_body_data& body_data)
    {
        auto promise = std::make_shared<dispatch::promise<json>>();

        auto request = build_request(@"POST", url);
        auto body = body_data.body();
        [request setValue:str::to_NS(body_data.content_type()) forHTTPHeaderField:@"Content-Type"];
        [request setValue:[NSString stringWithFormat:@"%lu", body.size()] forHTTPHeaderField:@"Content-Length"];
        [request setHTTPBody:[NSData dataWithBytes:body.data() length:body.size()]];

        auto task = [m_session dataTaskWithRequest:request completionHandler:^(NSData *data, NSURLResponse *response, NSError *error) {
            try
            {
                if (handle_error(data, response, error, *promise))
                    return;
                promise->set_value(extract_json(data));
            }
            catch (...)
            {
                dispatch::set_current_exception(promise);
            }
        }];
        [task resume];

        return promise->get_future();
    }

private:
    NSMutableURLRequest *build_request(NSString *method, const std::string& relative_url)
    {
        auto url = [NSURL URLWithString:str::to_NS(relative_url) relativeToURL:m_baseURL];
        auto request = [NSMutableURLRequest requestWithURL:url];
        request.HTTPMethod = method;
        if (m_authHeader)
            [request setValue:m_authHeader forHTTPHeaderField:@"Authorization"];
        return request;
    }

    template<typename T>
    bool handle_error(NSData *data, NSURLResponse *response_, NSError *error, dispatch::promise<T>& promise)
    {
        NSHTTPURLResponse *response = (NSHTTPURLResponse*)response_;
        int status_code = response ? (int)response.statusCode : 200;

        if (error == nil && status_code >= 200 && status_code < 300)
            return false;  // no error

        std::string desc;

        // try to parse error description if present:
        if (data && [response.MIMEType isEqualToString:@"application/json"])
        {
            NSData *reply = data;
            if (reply && reply.length > 0)
            {
                try
                {
                    desc = m_owner.parse_json_error(extract_json(reply));
                }
                catch (...) {} // report original error if parsing broken
            }
        }

        if (desc.empty())
        {
            if (error)
                desc = str::to_utf8([error localizedDescription]);
            else
                desc = str::to_utf8([NSHTTPURLResponse localizedStringForStatusCode:status_code]);
        }

        try
        {
            m_owner.on_error_response(status_code, desc);
            BOOST_THROW_EXCEPTION(http_exception(desc));
        }
        catch (...)
        {
            dispatch::set_current_exception(promise);
        }

        return true;
    }

    json extract_json(NSData *data)
    {
        return json::parse(std::string((char*)data.bytes, data.length));
    }

private:
    http_client& m_owner;

    NSURLSession *m_session;
    NSURL *m_baseURL;
    NSString *m_authHeader;
};


http_client::http_client(const std::string& url_prefix, int flags)
    : m_impl(new impl(*this, url_prefix, flags))
{
}

http_client::~http_client()
{
}

void http_client::set_authorization(const std::string& auth)
{
    m_impl->set_authorization(auth);
}

dispatch::future<json> http_client::get(const std::string& url)
{
    return m_impl->get(url);
}

dispatch::future<void> http_client::download(const std::string& url, const std::wstring& output_file)
{
    return m_impl->download(url, output_file);
}

dispatch::future<json> http_client::post(const std::string& url, const http_body_data& data)
{
    return m_impl->post(url, data);
}


class http_reachability::impl
{
public:
    impl(const std::string& url)
    {
        NSString *host = [[NSURL URLWithString:str::to_NS(url)] host];
        m_nr = SCNetworkReachabilityCreateWithName(kCFAllocatorDefault, [host UTF8String]);
    }

    ~impl()
    {
        if (m_nr)
        {
            SCNetworkReachabilityUnscheduleFromRunLoop(m_nr, CFRunLoopGetMain(), kCFRunLoopCommonModes);
            CFRelease(m_nr);
        }
    }

    bool is_reachable() const
    {
        SCNetworkReachabilityFlags flags;
        if (m_nr && SCNetworkReachabilityGetFlags(m_nr, &flags))
            return (flags & kSCNetworkReachabilityFlagsReachable) != 0;

        return true;  // fallback assumption
    }

private:
    SCNetworkReachabilityRef m_nr;
};


http_reachability::http_reachability(const std::string& url)
    : m_impl(new impl(url))
{
}

http_reachability::~http_reachability()
{
}

bool http_reachability::is_reachable() const
{
    return m_impl->is_reachable();
}
