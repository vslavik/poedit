/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2000-2025 Vaclav Slavik
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

#ifndef Poedit_gexecute_h
#define Poedit_gexecute_h

#include "subprocess.h"

#include <wx/string.h>

#include <vector>


/// Parsed gettext errors output.
struct ParsedGettextErrors
{
    enum Level
    {
        Warning,
        Error
    };

    struct Item
    {
        wxString text;
        Level level = Error;
        wxString file;
        int line = -1;

        bool has_location() const { return line != -1; }

        wxString pretty_print() const;
    };

    std::vector<Item> items;

    explicit operator bool() const { return !items.empty(); }

    /// Output errors only to wxLogError.
    void log_errors();

    /// Output all issues to wxLogError.
    void log_all();

    ParsedGettextErrors& operator+=(const ParsedGettextErrors& other)
    {
        items.insert(items.end(), other.items.begin(), other.items.end());
        return *this;
    }
};


#if defined(__WXOSX__) || defined(__WXMSW__)

extern wxString GetGettextPackagePath();
extern wxString GetGettextBinariesPath();

inline wxString GetGettextBinaryPath(const wxString& program)
{
    return subprocess::try_find_program(program, GetGettextBinariesPath());
}

#else

inline wxString GetGettextBinaryPath(const wxString& program) { return program; }

#endif

/// Checks if installed gettext tools are recent enough.
extern bool check_gettext_version(int major, int minor, int patch = 0);

/**
    Extract gettext-formatted errors from stderr output.

    Providing @a program enables filtering out of gettext's message prefixes.

    @see GettextRunner::parse_stderr() for better API.
 */
extern ParsedGettextErrors parse_gettext_stderr(const subprocess::Output& output, const std::wstring& program = {});


// Specialized runner for executing gettext tools
class GettextRunner : public subprocess::Runner
{
public:
    GettextRunner();

    /// Extract gettext-formatted errors from stderr output.
    ParsedGettextErrors parse_stderr(const subprocess::Output& output) const
    {
        return parse_gettext_stderr(output, m_program);
    }

protected:
    void preprocess_args(subprocess::Arguments& args) const override;

    // last program executed
    mutable std::wstring m_program;
};

#endif // Poedit_gexecute_h
