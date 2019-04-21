/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016-2017 Vaclav Slavik
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

#ifndef Poedit_json_h
#define Poedit_json_h

#include "str_helpers.h"

#ifdef HAVE_NLOHMANN_JSON_HPP
 #include <nlohmann/json.hpp>
#else
 #include "../deps/json/single_include/nlohmann/json.hpp"
#endif

using json = nlohmann::json;


namespace str
{

inline std::wstring to_wstring(const json& x)
{
    return to_wstring(x.get<std::string>());
}

inline std::wstring to_wstring(json&& x)
{
    return to_wstring(x.get<std::string>());
}

}
#endif // Poedit_json_h
