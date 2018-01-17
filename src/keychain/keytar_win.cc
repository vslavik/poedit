/*
 * Copyright (c) 2013 GitHub Inc.
 * Copyright (c) 2015-2018 Vaclav Slavik
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

#include "str_helpers.h"

#include <windows.h>
#include <wincred.h>

#define SERVICE_PREFIX L"Poedit:"

namespace
{

inline std::wstring make_name(const std::string& service, const std::string& user)
{
    auto s = SERVICE_PREFIX + str::to_wstring(service);
    if (!user.empty())
        s += L":" + str::to_wstring(user);
    return s;
}

}

namespace keytar
{

bool AddPassword(const std::string& service, const std::string& user, const std::string& password)
{
  std::wstring target_name = make_name(service, user);

  CREDENTIAL cred = { 0 };
  cred.Type = CRED_TYPE_GENERIC;
  cred.TargetName = const_cast<wchar_t*>(target_name.c_str());
  cred.CredentialBlobSize = password.size();
  cred.CredentialBlob = (LPBYTE)(password.data());
  cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

  return ::CredWrite(&cred, 0) == TRUE;
}

bool GetPassword(const std::string& service, const std::string& user, std::string* password)
{
  std::wstring target_name = make_name(service, user);

  CREDENTIAL* cred;
  if (::CredRead(target_name.c_str(), CRED_TYPE_GENERIC, 0, &cred) == FALSE)
    return false;

  *password = std::string(reinterpret_cast<char*>(cred->CredentialBlob),
                          cred->CredentialBlobSize);
  ::CredFree(cred);
  return true;
}

bool DeletePassword(const std::string& service, const std::string& user)
{
  std::wstring target_name = make_name(service, user);

  return ::CredDelete(target_name.c_str(), CRED_TYPE_GENERIC, 0) == TRUE;
}

}  // namespace keytar
