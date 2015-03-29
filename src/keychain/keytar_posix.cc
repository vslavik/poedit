#include "keytar.h"

#include <gnome-keyring.h>
#include <stdio.h>

namespace keytar {

namespace {

const GnomeKeyringPasswordSchema kGnomeSchema = {
  GNOME_KEYRING_ITEM_GENERIC_SECRET, {
    { "service", GNOME_KEYRING_ATTRIBUTE_TYPE_STRING },
    { "account", GNOME_KEYRING_ATTRIBUTE_TYPE_STRING },
    { NULL }
  }
};

}  // namespace

bool AddPassword(const std::string& service,
                 const std::string& account,
                 const std::string& password) {
  GnomeKeyringResult result = gnome_keyring_store_password_sync(
      &kGnomeSchema,
      NULL,  // Default keyring.
      (service + "/" + account).c_str(),  // Display name.
      password.c_str(),
      "service", service.c_str(),
      "account", account.c_str(),
      NULL);
  return result == GNOME_KEYRING_RESULT_OK;
}

bool GetPassword(const std::string& service,
                 const std::string& account,
                 std::string* password) {
  gchar* raw_passwords;
  GnomeKeyringResult result = gnome_keyring_find_password_sync(
      &kGnomeSchema,
      &raw_passwords,
      "service", service.c_str(),
      "account", account.c_str(),
      NULL);
  if (result != GNOME_KEYRING_RESULT_OK)
    return false;

  if (raw_passwords != NULL)
    *password = raw_passwords;
  gnome_keyring_free_password(raw_passwords);
  return true;
}

bool DeletePassword(const std::string& service, const std::string& account) {
  return gnome_keyring_delete_password_sync(
      &kGnomeSchema,
      "service", service.c_str(),
      "account", account.c_str(),
      NULL) == GNOME_KEYRING_RESULT_OK;
}

bool FindPassword(const std::string& service, std::string* password) {
  gchar* raw_passwords;
  GnomeKeyringResult result = gnome_keyring_find_password_sync(
      &kGnomeSchema,
      &raw_passwords,
      "service", service.c_str(),
      NULL);
  if (result != GNOME_KEYRING_RESULT_OK)
    return false;

  if (raw_passwords != NULL)
    *password = raw_passwords;
  gnome_keyring_free_password(raw_passwords);
  return true;
}

}  // namespace keytar
