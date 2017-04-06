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

#ifndef Poedit_configuration_h
#define Poedit_configuration_h

#include <string>

// What to do during msgmerge
enum MergeBehavior
{
    Merge_None,
    Merge_FuzzyMatch,
    Merge_UseTM
};


/**
    High-level interface to configuration storage.

    Unlike wxConfig, this is thread-safe.
 */
class Config
{
public:
    static void Initialize(const std::wstring& configFile);

    static bool UseTM() { return Read("/use_tm", true); }
    static void UseTM(bool use) { Write("/use_tm", use); }

    // What to do during merge
    static ::MergeBehavior MergeBehavior();
    static void MergeBehavior(::MergeBehavior b);

private:
    template<typename T>
    static T Read(const std::string& key, T defval)
    {
        T val = T();
        if (Read(key, &val))
            return val;
        else
            return defval;
    }

    static bool Read(const std::string& key, std::string *out);
    static bool Read(const std::string& key, std::wstring *out);
    static bool Read(const std::string& key, bool *out);

    static void Write(const std::string& key, const std::string& value);
    static void Write(const std::string& key, const std::wstring& value);
    static void Write(const std::string& key, bool value);
};

#endif // Poedit_configuration_h
