/*
 * Copyright (c) 2013 GitHub Inc.
 * Copyright (c) 2015 Vaclav Slavik
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

#include <windows.h>
#include <wincred.h>

#define SERVICE_NAME "Poedit/"

namespace keytar
{

bool AddPassword(const std::string& account, const std::string& password)
{
  std::string target_name = SERVICE_PREFIX + account;

  CREDENTIAL cred = { 0 };
  cred.Type = CRED_TYPE_GENERIC;
  cred.TargetName = const_cast<char*>(target_name.c_str());
  cred.CredentialBlobSize = password.size();
  cred.CredentialBlob = (LPBYTE)(password.data());
  cred.Persist = CRED_PERSIST_LOCAL_MACHINE;

  return ::CredWrite(&cred, 0) == TRUE;
}

bool GetPassword(const std::string& account, std::string* password)
{
  std::string target_name = SERVICE_PREFIX + account;

  CREDENTIAL* cred;
  if (::CredRead(target_name.c_str(), CRED_TYPE_GENERIC, 0, &cred) == FALSE)
    return false;

  *password = std::string(reinterpret_cast<char*>(cred->CredentialBlob),
                          cred->CredentialBlobSize);
  ::CredFree(cred);
  return true;
}

bool DeletePassword(const std::string& account)
{
  std::string target_name = SERVICE_PREFIX + account;

  return ::CredDelete(target_name.c_str(), CRED_TYPE_GENERIC, 0) == TRUE;
}

}  // namespace keytar
