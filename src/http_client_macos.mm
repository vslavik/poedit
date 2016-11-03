/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2014-2016 Vaclav Slavik
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

#import "AFHTTPClient.h"
#import "AFHTTPRequestOperation.h"


class http_exception : public std::runtime_error
{
public:
    http_exception(const std::string& what) : std::runtime_error(what) {}
};


class http_client::impl
{
public:
    impl(http_client& owner, const std::string& url_prefix, int /*flags*/) : m_owner(owner)
    {
        NSString *str = str::to_NS(url_prefix);
        m_native = [[AFHTTPClient alloc] initWithBaseURL:[NSURL URLWithString:str]];

        [m_native setDefaultHeader:@"Accept" value:@"application/json"];

        // AFNetworking operations aren't CPU-bound, so shouldn't use the default queue
        // size limits. This avoids request stalling at the cost of sending more requests
        // to the server.
        [m_native.operationQueue setMaxConcurrentOperationCount:NSIntegerMax];

        int majorVersion, minorVersion, patchVersion;
        NSOperatingSystemVersion macos = [[NSProcessInfo processInfo] operatingSystemVersion];
        majorVersion = (int)macos.majorVersion;
        minorVersion = (int)macos.minorVersion;
        patchVersion = (int)macos.patchVersion;
        NSString *macos_str = (patchVersion == 0)
                            ? [NSString stringWithFormat:@"%d.%d", majorVersion, minorVersion]
                            : [NSString stringWithFormat:@"%d.%d.%d", majorVersion, minorVersion, patchVersion];
        [m_native setDefaultHeader:@"User-Agent" value:[NSString stringWithFormat:@"Poedit/%s (Mac OS X %@)", POEDIT_VERSION, macos_str]];
    }

    ~impl()
    {
        m_native = nil;
    }

    bool is_reachable() const
    {
        return m_native.networkReachabilityStatus != AFNetworkReachabilityStatusNotReachable;
    }

    void set_authorization(const std::string& auth)
    {
        if (!auth.empty())
            [m_native setDefaultHeader:@"Authorization" value:str::to_NS(auth)];
        else
            [m_native clearAuthorizationHeader];
    }

    dispatch::future<json> get(const std::string& url)
    {
        auto promise = std::make_shared<dispatch::promise<json>>();

        [m_native getPath:str::to_NS(url)
               parameters:nil
                  success:^(AFHTTPRequestOperation *op, NSData *responseData)
        {
            #pragma unused(op)
            try
            {
                promise->set_value(extract_json(responseData));
            }
            catch (...)
            {
                dispatch::set_current_exception(promise);
            }
        }
        failure:^(AFHTTPRequestOperation *op, NSError *e)
        {
            handle_error(op, e, *promise);
        }];

        return promise->get_future();
    }

    dispatch::future<void> download(const std::string& url, const std::wstring& output_file)
    {
        auto promise = std::make_shared<dispatch::promise<void>>();

        NSString *outfile = str::to_NS(output_file);
        NSURLRequest *request = [m_native requestWithMethod:@"GET"
                                                       path:str::to_NS(url)
                                                 parameters:nil];
        // Read the entire file into memory, then save to file. This is done instead of
        // setting operation.outputStream to stream directly to the file because it that
        // case the failure handler wouldn't receive JSON data with the error.
        //
        // This doesn't matter much, because files downloaded by Poedit are very small.
        AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
        [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *op, NSData *data)
        {
            #pragma unused(op)
            [data writeToFile:outfile atomically:YES];
            promise->set_value();
        }
        failure:^(AFHTTPRequestOperation *op, NSError *e)
        {
            handle_error(op, e, *promise);
        }];

        [m_native enqueueHTTPRequestOperation:operation];

        return promise->get_future();
    }

    dispatch::future<json> post(const std::string& url, const http_body_data& data)
    {
        auto promise = std::make_shared<dispatch::promise<json>>();

        NSMutableURLRequest *request = [m_native requestWithMethod:@"POST"
                                                              path:str::to_NS(url)
                                                        parameters:nil];

        auto body = data.body();
        [request setValue:str::to_NS(data.content_type()) forHTTPHeaderField:@"Content-Type"];
        [request setValue:[NSString stringWithFormat:@"%lu", body.size()] forHTTPHeaderField:@"Content-Length"];
        [request setHTTPBody:[NSData dataWithBytes:body.data() length:body.size()]];

        AFHTTPRequestOperation *operation = [m_native HTTPRequestOperationWithRequest:request success:^(AFHTTPRequestOperation *op, NSData *responseData)
        {
            #pragma unused(op)
            try
            {
                promise->set_value(extract_json(responseData));
            }
            catch (...)
            {
                dispatch::set_current_exception(promise);
            }
        }
        failure:^(AFHTTPRequestOperation *op, NSError *e)
        {
            handle_error(op, e, *promise);
        }];

        [m_native enqueueHTTPRequestOperation:operation];

        return promise->get_future();
    }

private:
    template<typename T>
    void handle_error(AFHTTPRequestOperation *op, NSError *e, dispatch::promise<T>& promise)
    {
        int status_code = (int)op.response.statusCode;
        std::string desc;
        if (op.responseData && [op.response.MIMEType isEqualToString:@"application/json"])
        {
            NSData *reply = op.responseData;
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
            desc = str::to_utf8([e localizedDescription]);
            // Fixup some common cases to be more readable
            if (status_code == 503 && desc == "Expected status code in (200-299), got 503")
                desc = "Service Unavailable";
        }

        m_owner.on_error_response(status_code, desc);
        promise.set_exception(http_exception(desc));
    }

    json extract_json(NSData *data)
    {
        return json::parse(std::string((char*)data.bytes, data.length));
    }

    http_client& m_owner;
    AFHTTPClient *m_native;
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
