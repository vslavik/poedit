#ifndef SRC_KEYTAR_H_
#define SRC_KEYTAR_H_

#include <string>

namespace keytar {

bool AddPassword(const std::string& service,
                 const std::string& account,
                 const std::string& password);

bool GetPassword(const std::string& service,
                 const std::string& account,
                 std::string* password);

bool DeletePassword(const std::string& service, const std::string& account);

bool FindPassword(const std::string& service, std::string* password);

}  // namespace keytar

#endif  // SRC_KEYTAR_H_
