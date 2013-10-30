/*
 *  This file is part of Poedit (http://www.poedit.net)
 *
 *  Copyright (C) 2013 Vaclav Slavik
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

#include "language.h"

#include <mutex>
#include <unordered_map>

static std::mutex gs_mutexGetPluralFormForLanguage;

std::string GetPluralFormForLanguage(std::string lang)
{
    if ( lang.empty() )
        return std::string();

    std::lock_guard<std::mutex> lock(gs_mutexGetPluralFormForLanguage);

    static const std::unordered_map<std::string, std::string> forms = {
        #include "language_impl_plurals.h"
    };

    auto i = forms.find(lang);
    if ( i != forms.end() )
        return i->second;

    size_t pos = lang.rfind('@');
    if ( pos != std::string::npos )
    {
        lang = lang.substr(0, pos);
        i = forms.find(lang);
        if ( i != forms.end() )
            return i->second;
    }

    pos = lang.rfind('_');
    if ( pos != std::string::npos )
    {
        lang = lang.substr(0, pos);
        i = forms.find(lang);
        if ( i != forms.end() )
            return i->second;
    }

    return std::string();
}
