//  file_mapping.hpp  --------------------------------------------------------------//

//  Copyright 2010 Vicente J. Botet Escriba
//  Copyright 2015 Andrey Semashev

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt


#ifndef BOOST_DETAIL_WINAPI_FILE_MAPPING_HPP
#define BOOST_DETAIL_WINAPI_FILE_MAPPING_HPP

#include <boost/detail/winapi/basic_types.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

#if !defined( BOOST_USE_WINDOWS_H )
extern "C" {
#if !defined( BOOST_NO_ANSI_APIS )
BOOST_SYMBOL_IMPORT boost::detail::winapi::HANDLE_ WINAPI
CreateFileMappingA(
    boost::detail::winapi::HANDLE_ hFile,
    ::_SECURITY_ATTRIBUTES* lpFileMappingAttributes,
    boost::detail::winapi::DWORD_ flProtect,
    boost::detail::winapi::DWORD_ dwMaximumSizeHigh,
    boost::detail::winapi::DWORD_ dwMaximumSizeLow,
    boost::detail::winapi::LPCSTR_ lpName);

BOOST_SYMBOL_IMPORT boost::detail::winapi::HANDLE_ WINAPI
OpenFileMappingA(
    boost::detail::winapi::DWORD_ dwDesiredAccess,
    boost::detail::winapi::BOOL_ bInheritHandle,
    boost::detail::winapi::LPCSTR_ lpName);
#endif

BOOST_SYMBOL_IMPORT boost::detail::winapi::HANDLE_ WINAPI
CreateFileMappingW(
    boost::detail::winapi::HANDLE_ hFile,
    ::_SECURITY_ATTRIBUTES* lpFileMappingAttributes,
    boost::detail::winapi::DWORD_ flProtect,
    boost::detail::winapi::DWORD_ dwMaximumSizeHigh,
    boost::detail::winapi::DWORD_ dwMaximumSizeLow,
    boost::detail::winapi::LPCWSTR_ lpName);

BOOST_SYMBOL_IMPORT boost::detail::winapi::HANDLE_ WINAPI
OpenFileMappingW(
    boost::detail::winapi::DWORD_ dwDesiredAccess,
    boost::detail::winapi::BOOL_ bInheritHandle,
    boost::detail::winapi::LPCWSTR_ lpName);

BOOST_SYMBOL_IMPORT boost::detail::winapi::LPVOID_ WINAPI
MapViewOfFileEx(
    boost::detail::winapi::HANDLE_ hFileMappingObject,
    boost::detail::winapi::DWORD_ dwDesiredAccess,
    boost::detail::winapi::DWORD_ dwFileOffsetHigh,
    boost::detail::winapi::DWORD_ dwFileOffsetLow,
    boost::detail::winapi::SIZE_T_ dwNumberOfBytesToMap,
    boost::detail::winapi::LPVOID_ lpBaseAddress);

BOOST_SYMBOL_IMPORT boost::detail::winapi::BOOL_ WINAPI
FlushViewOfFile(
    boost::detail::winapi::LPCVOID_ lpBaseAddress,
    boost::detail::winapi::SIZE_T_ dwNumberOfBytesToFlush);

BOOST_SYMBOL_IMPORT boost::detail::winapi::BOOL_ WINAPI
UnmapViewOfFile(boost::detail::winapi::LPCVOID_ lpBaseAddress);
}
#endif

namespace boost {
namespace detail {
namespace winapi {

#if !defined( BOOST_NO_ANSI_APIS )
using ::OpenFileMappingA;
#endif
using ::OpenFileMappingW;
using ::MapViewOfFileEx;
using ::FlushViewOfFile;
using ::UnmapViewOfFile;

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE HANDLE_ CreateFileMappingA(
    HANDLE_ hFile,
    SECURITY_ATTRIBUTES_* lpFileMappingAttributes,
    DWORD_ flProtect,
    DWORD_ dwMaximumSizeHigh,
    DWORD_ dwMaximumSizeLow,
    LPCSTR_ lpName)
{
    return ::CreateFileMappingA(
        hFile,
        reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpFileMappingAttributes),
        flProtect,
        dwMaximumSizeHigh,
        dwMaximumSizeLow,
        lpName);
}
#endif

BOOST_FORCEINLINE HANDLE_ CreateFileMappingW(
    HANDLE_ hFile,
    ::_SECURITY_ATTRIBUTES* lpFileMappingAttributes,
    DWORD_ flProtect,
    DWORD_ dwMaximumSizeHigh,
    DWORD_ dwMaximumSizeLow,
    LPCWSTR_ lpName)
{
    return ::CreateFileMappingW(
        hFile,
        reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpFileMappingAttributes),
        flProtect,
        dwMaximumSizeHigh,
        dwMaximumSizeLow,
        lpName);
}

#if !defined( BOOST_NO_ANSI_APIS )
BOOST_FORCEINLINE HANDLE_ create_file_mapping(
    HANDLE_ hFile,
    SECURITY_ATTRIBUTES_* lpFileMappingAttributes,
    DWORD_ flProtect,
    DWORD_ dwMaximumSizeHigh,
    DWORD_ dwMaximumSizeLow,
    LPCSTR_ lpName)
{
    return ::CreateFileMappingA(
        hFile,
        reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpFileMappingAttributes),
        flProtect,
        dwMaximumSizeHigh,
        dwMaximumSizeLow,
        lpName);
}

BOOST_FORCEINLINE HANDLE_ open_file_mapping(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCSTR_ lpName)
{
    return ::OpenFileMappingA(dwDesiredAccess, bInheritHandle, lpName);
}
#endif

BOOST_FORCEINLINE HANDLE_ create_file_mapping(
    HANDLE_ hFile,
    ::_SECURITY_ATTRIBUTES* lpFileMappingAttributes,
    DWORD_ flProtect,
    DWORD_ dwMaximumSizeHigh,
    DWORD_ dwMaximumSizeLow,
    LPCWSTR_ lpName)
{
    return ::CreateFileMappingW(
        hFile,
        reinterpret_cast< ::_SECURITY_ATTRIBUTES* >(lpFileMappingAttributes),
        flProtect,
        dwMaximumSizeHigh,
        dwMaximumSizeLow,
        lpName);
}

BOOST_FORCEINLINE HANDLE_ open_file_mapping(DWORD_ dwDesiredAccess, BOOL_ bInheritHandle, LPCWSTR_ lpName)
{
    return ::OpenFileMappingW(dwDesiredAccess, bInheritHandle, lpName);
}

}
}
}

#endif // BOOST_DETAIL_WINAPI_FILE_MAPPING_HPP
