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

#include "str_helpers.h"

#import "AFHTTPClient.h"
#import "AFJSONRequestOperation.h"


class http_exception : public std::runtime_error
{
public:
    http_exception(const std::string& what) : std::runtime_error(what) {}
};


struct json_dict::native
{
    native(NSDictionary *d) : dict(d) {}
    NSDictionary *dict;

    bool is_null(const char *key) const
    {
        id obj = dict[[NSString stringWithUTF8String:key]];
        return obj == nil || obj == [NSNull null];
    }

    id get(const char *key, Class klass)
    {
        id obj = dict[[NSString stringWithUTF8String:key]];
        if (obj == nil)
            throw http_exception(std::string("JSON key error: ") + key);
        if (![obj isKindOfClass:klass])
            throw http_exception(std::string("JSON type error: ") + key);
        return obj;
    }
};

static inline json_dict make_json_dict(NSDictionary *d)
{
    return std::make_shared<json_dict::native>(d);
}

bool json_dict::is_null(const char *name) const
{
    return m_native->is_null(name);
}

json_dict json_dict::subdict(const char *name) const
{
    NSDictionary *d = m_native->get(name, [NSDictionary class]);
    return make_json_dict(d);
}

std::string json_dict::utf8_string(const char *name) const
{
    NSString *s = m_native->get(name, [NSString class]);
    return str::to_utf8(s);
}

std::wstring json_dict::wstring(const char *name) const
{
    NSString *s = m_native->get(name, [NSString class]);
    return str::to_wstring(s);
}

int json_dict::number(const char *name) const
{
    try
    {
        NSNumber *n = m_native->get(name, [NSNumber class]);
        return [n intValue];
    }
    catch (std::exception&)
    {
        // Some broken APIs may return strings instead of numbers, so lets try
        // that too as a fallback
        return std::stoi(native_string(name));
    }
}

double json_dict::double_number(const char *name) const
{
    NSNumber *n = m_native->get(name, [NSNumber class]);
    return [n doubleValue];
}

void json_dict::iterate_array(const char *name, std::function<void(const json_dict&)> on_item) const
{
    NSArray *array = m_native->get(name, [NSArray class]);
    for (NSDictionary *item in array)
    {
        on_item(make_json_dict(item));
    }
}



class http_client::impl
{
public:
    typedef http_client::response_func_t response_funct_t;

    impl(const std::string& url_prefix, int /*flags*/)
    {
        NSString *str = str::to_NS(url_prefix);
        m_native = [[AFHTTPClient alloc] initWithBaseURL:[NSURL URLWithString:str]];

        [m_native registerHTTPOperationClass:[AFJSONRequestOperation class]];
        [m_native setDefaultHeader:@"Accept" value:@"application/json"];

        // Some APIs return text/plain content with JSON data
        [AFJSONRequestOperation addAcceptableContentTypes:[NSSet setWithObject:@"text/plain"]];
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

    void get(const std::string& url, response_func_t handler)
    {
        [m_native getPath:str::to_NS(url)
               parameters:nil
                  success:^(AFHTTPRequestOperation *op, NSDictionary *responseObject)
        {
            http_response r;
            r.m_ok = [op hasAcceptableStatusCode] && [op hasAcceptableContentType];
            r.m_data = make_json_dict(responseObject);
            handler(r);
        }
        failure:^(AFHTTPRequestOperation*, NSError *e)
        {
            NSString *desc = [e localizedDescription];
            handler(std::make_exception_ptr(http_exception(str::to_utf8(desc))));
        }];
    }

    void download(const std::string& url, const std::wstring& output_file, response_func_t handler)
    {
    	NSURLRequest *request = [m_native requestWithMethod:@"GET"
                                                       path:str::to_NS(url)
                                                 parameters:nil];
        AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
        operation.outputStream = [NSOutputStream outputStreamToFileAtPath:str::to_NS(output_file) append:NO];

        [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *op, id responseObject)
        {
            #pragma unused(responseObject)
            http_response r;
            r.m_ok = [op hasAcceptableStatusCode] && [op hasAcceptableContentType];
            handler(r);
        }
        failure:^(AFHTTPRequestOperation*, NSError *e)
        {
            NSString *desc = [e localizedDescription];
            handler(std::make_exception_ptr(http_exception(str::to_utf8(desc))));
        }];

        [m_native enqueueHTTPRequestOperation:operation];
    }

    void post(const std::string& url, const multipart_form_data& data, response_func_t handler)
    {
        NSMutableURLRequest *request = [m_native requestWithMethod:@"POST"
                                                              path:str::to_NS(url)
                                                        parameters:nil];

        auto body = data.body();
        [request setValue:str::to_NS(data.content_type()) forHTTPHeaderField:@"Content-Type"];
        [request setValue:[NSString stringWithFormat:@"%lu", body.size()] forHTTPHeaderField:@"Content-Length"];
        [request setHTTPBody:[NSData dataWithBytes:body.data() length:body.size()]];

        AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];

        [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *op, id responseObject)
        {
            #pragma unused(responseObject)
            http_response r;
            r.m_ok = [op hasAcceptableStatusCode] && [op hasAcceptableContentType];
            handler(r);
        }
        failure:^(AFHTTPRequestOperation*, NSError *e)
        {
            NSString *desc = [e localizedDescription];
            handler(std::make_exception_ptr(http_exception(str::to_utf8(desc))));
        }];

        [m_native enqueueHTTPRequestOperation:operation];
    }

private:
    AFHTTPClient *m_native;
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

void http_client::get(const std::string& url, response_func_t handler)
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