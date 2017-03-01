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

#include <libsecret/secret.h>
#include <stdio.h>
#include <unistd.h>

namespace keytar
{

namespace
{

#ifdef __GNUC__
   #pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

const SecretSchema kSchema = {
  "net.poedit.Credentials", SECRET_SCHEMA_NONE,
  {
    { "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
    { "user", SECRET_SCHEMA_ATTRIBUTE_STRING },
    { NULL, SecretSchemaAttributeType(0) },
  }
};

}  // namespace

bool AddPassword(const std::string& service, const std::string& user, const std::string& password)
{
  std::string label = "Poedit: " + service;
  if (!user.empty())
   label += " (" + user + ")";

  GError *error = NULL;
  gboolean result = secret_password_store_sync(
      &kSchema,
      SECRET_COLLECTION_DEFAULT,
      label.c_str(),
      password.c_str(),
      NULL,  // not cancellable
      &error,
      "service", service.c_str(),
      "user", user.c_str(),
      NULL);
  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
  }
  return result;
}

bool GetPassword(const std::string& service, const std::string& user, std::string* password)
{
  GError *error = NULL;
  gchar* raw_passwords;
  raw_passwords = secret_password_lookup_sync(
      &kSchema,
      NULL,  // not cancellable
      &error,
      "service", service.c_str(),
      "user", user.c_str(),
      NULL);

  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
    return false;
  } else if (raw_passwords == NULL) {
    return false;
  } else {
    *password = raw_passwords;
  }
  secret_password_free(raw_passwords);
  return true;
}

bool DeletePassword(const std::string& service, const std::string& user)
{
  GError *error = NULL;
  gboolean result = secret_password_clear_sync(
      &kSchema,
      NULL,  // not cancellable
      &error,
      "service", service.c_str(),
      "user", user.c_str(),
      NULL);
  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
  }
  return result;
}

}  // namespace keytar
