/*
 * Copyright (c) 2013 GitHub Inc.
 * Copyright (c) 2015-2017 Vaclav Slavik
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "keytar.h"

#include <Security/Security.h>

namespace
{

const std::string SERVICE_NAME("net.poedit.Poedit");

inline std::string make_name(const std::string& service)
{
    return SERVICE_NAME + "." + service;
}

}

namespace keytar
{

bool AddPassword(const std::string& service, const std::string& user, const std::string& password)
{
  auto serv = make_name(service);
  OSStatus status = SecKeychainAddGenericPassword(NULL,
                                                  (UInt32)serv.length(),
                                                  serv.data(),
                                                  (UInt32)user.length(),
                                                  user.data(),
                                                  (UInt32)password.length(),
                                                  password.data(),
                                                  NULL);
  return status == errSecSuccess;
}

bool GetPassword(const std::string& service, const std::string& user, std::string* password)
{
  auto serv = make_name(service);
  void *data;
  UInt32 length;
  OSStatus status = SecKeychainFindGenericPassword(NULL,
                                                  (UInt32)serv.length(),
                                                  serv.data(),
                                                  (UInt32)user.length(),
                                                  user.data(),
                                                  &length,
                                                  &data,
                                                  NULL);
  if (status != errSecSuccess)
    return false;

  *password = std::string(reinterpret_cast<const char*>(data), length);
  SecKeychainItemFreeContent(NULL, data);
  return true;
}

bool DeletePassword(const std::string& service, const std::string& user)
{
  auto serv = make_name(service);
  SecKeychainItemRef item;
  OSStatus status = SecKeychainFindGenericPassword(NULL,
                                                   (UInt32)serv.length(),
                                                   serv.data(),
                                                   (UInt32)user.length(),
                                                   user.data(),
                                                   NULL,
                                                   NULL,
                                                   &item);
  if (status != errSecSuccess)
    return false;

  status = SecKeychainItemDelete(item);
  CFRelease(item);
  return status == errSecSuccess;
}

}  // namespace keytar
