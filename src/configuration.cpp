/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2016 Vaclav Slavik
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

#include "configuration.h"

#include <wx/config.h>

#include <mutex>

namespace
{

// wxConfig isn't thread safe because it's implementing using SetPath.
// Make sure it's only accessed from a single location at a time.
std::mutex g_wxconfig;

} // anonymous namespace


bool Config::Read(const std::string& key, std::string *out)
{
    std::lock_guard<std::mutex> guard(g_wxconfig);

    wxString s;
    if (!wxConfig::Get()->Read(key, &s))
        return false;
    *out = s.ToStdString();
    return true;
}

void Config::Write(const std::string& key, const std::string& value)
{
    std::lock_guard<std::mutex> guard(g_wxconfig);

    wxConfig::Get()->Write(key, wxString(value));
}

bool Config::Read(const std::string& key, std::wstring *out)
{
    std::lock_guard<std::mutex> guard(g_wxconfig);

    wxString s;
    if (!wxConfig::Get()->Read(key, &s))
        return false;
    *out = s.ToStdWstring();
    return true;
}

void Config::Write(const std::string& key, const std::wstring& value)
{
    std::lock_guard<std::mutex> guard(g_wxconfig);

    wxConfig::Get()->Write(key, wxString(value));
}

bool Config::Read(const std::string& key, bool *out)
{
    std::lock_guard<std::mutex> guard(g_wxconfig);

    return wxConfig::Get()->Read(key, out);
}

void Config::Write(const std::string& key, bool value)
{
    std::lock_guard<std::mutex> guard(g_wxconfig);

    wxConfig::Get()->Write(key, value);
}


MergeBehavior Config::MergeBehavior()
{
    ::MergeBehavior value = Merge_FuzzyMatch;

    std::string stored;
    if (Read("/merge_behavior", &stored))
    {
        if (stored == "fuzzy_match")
            value = Merge_FuzzyMatch;
        else if (stored == "use_tm")
            value = Merge_UseTM;
        else
            value = Merge_None;
    }
    else
    {
        bool use_tm;
        if (Read("/use_tm_when_updating", &use_tm))
            value = use_tm ? Merge_UseTM : Merge_None;
    }

    return value;
}

void Config::MergeBehavior(::MergeBehavior b)
{
    std::string value;
    switch (b)
    {
        case Merge_None:
            value = "none";
            break;
        case Merge_FuzzyMatch:
            value = "fuzzy_match";
            break;
        case Merge_UseTM:
            value = "use_tm";
            break;
    }

    Write("/merge_behavior", value);
}
