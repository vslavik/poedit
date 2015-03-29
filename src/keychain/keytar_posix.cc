#include "keytar.h"

#include <libsecret/secret.h>
#include <stdio.h>
#include <unistd.h>

namespace keytar {

namespace {

const SecretSchema kSchema = {
  "io.github.atom.node-keytar", SECRET_SCHEMA_DONT_MATCH_NAME, {
    { "service", SECRET_SCHEMA_ATTRIBUTE_STRING },
    { "account", SECRET_SCHEMA_ATTRIBUTE_STRING },
    { NULL }
  }
};

}  // namespace

bool AddPassword(const std::string& service,
                 const std::string& account,
                 const std::string& password) {
  GError *error = NULL;
  gboolean result = secret_password_store_sync(
      &kSchema,
      SECRET_COLLECTION_DEFAULT,
      (service + "/" + account).c_str(),  // label
      password.c_str(),
      NULL,  // not cancellable
      &error,
      "service", service.c_str(),
      "account", account.c_str(),
      NULL);
  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
  }
  return result;
}

bool GetPassword(const std::string& service,
                 const std::string& account,
                 std::string* password) {
  GError *error = NULL;
  gchar* raw_passwords;
  raw_passwords = secret_password_lookup_sync(
      &kSchema,
      NULL,  // not cancellable
      &error,
      "service", service.c_str(),
      "account", account.c_str(),
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

bool DeletePassword(const std::string& service, const std::string& account) {
  GError *error = NULL;
  gboolean result = secret_password_clear_sync(
      &kSchema,
      NULL,  // not cancellable
      &error,
      "service", service.c_str(),
      "account", account.c_str(),
      NULL);
  if (error != NULL) {
    fprintf(stderr, "%s\n", error->message);
    g_error_free(error);
  }
  return result;
}

bool FindPassword(const std::string& service, std::string* password) {
  GError *error = NULL;
  gchar* raw_passwords;
  raw_passwords = secret_password_lookup_sync(
      &kSchema,
      NULL,  // not cancellable
      &error,
      "service", service.c_str(),
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

}  // namespace keytar
