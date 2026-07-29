
#ifndef Poedit_compat_cpprestsdk_h
#define Poedit_compat_cpprestsdk_h

// Workaround problems with C++REST SDK not compiling with VS2026 which
// removed non-standard std:: extensions.
//
// This header is force-included when compiling cpprestsdk and also used
// by wrap_cpprestsdk.h

#if defined(_MSC_VER) && _MSC_VER >= 1950

#include <cstddef>

namespace stdext
{
    template<typename Ptr>
    constexpr Ptr checked_array_iterator(
        Ptr ptr, std::size_t /*size*/, std::size_t index = 0) noexcept
    {
        return ptr + index;
    }
}

#endif

#endif // Poedit_compat_cpprestsdk_h
