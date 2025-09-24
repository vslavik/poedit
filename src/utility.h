/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2010-2025 Vaclav Slavik
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

#ifndef Poedit_utility_h
#define Poedit_utility_h

#include <map>
#include <vector>

#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/string.h>

#if wxUSE_GUI
    #include <wx/toplevel.h>
#endif


// ----------------------------------------------------------------------
// Misc platform differences
// ----------------------------------------------------------------------

#ifdef __WXMSW__
    #define MSW_OR_OTHER(msw, other) msw
#else
    #define MSW_OR_OTHER(msw, other) other
#endif

#ifdef __WXOSX__
    #define MACOS_OR_OTHER(mac, other) mac
#else
    #define MACOS_OR_OTHER(mac, other) other
#endif

#ifdef __WXOSX__
    #define BORDER_WIN(dir, n) Border(dir, 0)
    #define BORDER_MACOS(dir, n) Border(dir, n)
#else
    #define BORDER_WIN(dir, n) Border(dir, n)
    #define BORDER_MACOS(dir, n) Border(dir, 0)
#endif

#ifdef __WXOSX__
    inline int AboveChoicePadding()
    {
        if (__builtin_available(macOS 11.0, *))
            return 2;
        else
            return 0;
    }
#else
    #define AboveChoicePadding()  0
#endif


// ----------------------------------------------------------------------
// Misc helpers
// ----------------------------------------------------------------------

wxString EscapeMarkup(const wxString& str);

// Encoding and decoding a string with C escape sequences:

template<typename T>
inline void EscapeCStringInplace(T& str)
{
    for (typename T::iterator i = str.begin(); i != str.end(); ++i)
    {
        switch ((wchar_t)*i)
        {
            case '"' :           i = ++str.insert(i, '\\'); break;
            case '\a': *i = 'a'; i = ++str.insert(i, '\\'); break;
            case '\b': *i = 'b'; i = ++str.insert(i, '\\'); break;
            case '\f': *i = 'f'; i = ++str.insert(i, '\\'); break;
            case '\n': *i = 'n'; i = ++str.insert(i, '\\'); break;
            case '\r': *i = 'r'; i = ++str.insert(i, '\\'); break;
            case '\t': *i = 't'; i = ++str.insert(i, '\\'); break;
            case '\v': *i = 'v'; i = ++str.insert(i, '\\'); break;
            case '\\':           i = ++str.insert(i, '\\'); break;
            default:
                break;
        }
    }
}

template<typename T>
inline T EscapeCString(const T& str)
{
    T out(str);
    EscapeCStringInplace(out);
    return out;
}

template<typename T>
inline T UnescapeCString(const T& str)
{
    if (str.find('\\') == T::npos)
        return str;

    T out;
    out.reserve(str.length());
    for (auto i = str.begin(); i != str.end(); ++i)
    {
        wchar_t c = *i;
        if (c == '\\')
        {
            if (++i != str.end())
            {
                switch ((wchar_t)*i)
                {
                    case 'a': out += '\a'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'v': out += '\v'; break;
                    case '\\':
                    case '"':
                    case '\'':
                    case '?':
                        out += *i;
                        break;
                    default:
                        out += c;
                        out += *i;
                        break;
                }
            }
            else
            {
                out += c;
                break;
            }
        }
        else
        {
            out += c;
        }
    }
    return out;
}


wxFileName MakeFileName(const wxString& path);

inline wxFileName MakeFileName(wxFileName fn)
{
    fn.MakeAbsolute();
    return fn;
}

wxFileName CommonDirectory(const wxFileName& a, const wxFileName& b);

template<typename T>
inline wxFileName CommonDirectory(const T& a)
{
    wxFileName root;
    for (auto& i: a)
        root = CommonDirectory(root, MakeFileName(i));
    return root;
}


inline wxString MaskForType(const char *extensions, const wxString& description, bool showExt = true)
{
    (void)showExt;
#ifdef __WXMSW__
    if (showExt)
        return wxString::Format("%s (%s)|%s", description, extensions, extensions);
    else
#endif
        return wxString::Format("%s|%s", description, extensions);
}

/// A helper class to calculate a display diff of strings
class Diff
{
public:
    /// Constructs a Diff object with a edit sequence from a string
    /// @arg from to the string @arg to
    Diff(const wxString& from, const wxString& to);

    /// A type of element in the shortest edit sequence
    enum class Action {
        Common,  ///< symbols are the same
        Add,     ///< symbols were added
        Delete   ///< symbols were removed
    };

    /// An element from the shortest edit sequence
    typedef std::pair<Action, wxString> sequenceElement;
    /// A type to represent a shortest edit sequence i.e. the sequence of
    /// substrings with an action (add remove, don't change) attached to them
    typedef std::vector<sequenceElement> sequence;

    /// @returns the shortest edit sequence in a form suitable for interpretation
    const sequence& getSes() { return ses; };

    /// @returns the diff ready to be displayed as a markup
    /// @arg addColor    a background color for a string being added
    /// @arg deleteColor a background color for a string being removed
    wxString getMarkup(const wxString& addColor=wxString("lightgreen"),
                       const wxString& deleteColor=wxString("pink"));

private:
    sequence ses;
};

#if wxUSE_GUI && defined(__WXMSW__)
bool IsRunningUnderScreenReader();
#endif


// ----------------------------------------------------------------------
// TempDirectory
// ----------------------------------------------------------------------

// Helper class for managing temporary directories.
// Cleans the directory when destroyed.
class TempDirectory
{
public:
    // creates randomly-named temp directory with "poedit" name prefix
    TempDirectory();
    ~TempDirectory();

    TempDirectory(const TempDirectory&) = delete;

    bool IsOk() const { return !m_dir.empty(); }

    const wxString& DirName() const { return m_dir; }

    // creates new file name in that directory
    wxString CreateFileName(const wxString& suffix);

    /// Clears the temp directory (only safe if none of the files are open). Called by dtor.
    void Clear();

    // whether to keep temporary files
    static void KeepFiles(bool keep = true) { ms_keepFiles = keep; }

private:
    std::map<wxString, int> m_counters;
    wxString m_dir;

    static bool ms_keepFiles;
};

/// Holder of temporary file for creating the output.
/// Use Commit() to move the written file to its final location.
/// Destructor deletes the temp file if it still exists.
class TempOutputFileFor
{
public:
    explicit TempOutputFileFor(const wxString& filename);
    explicit TempOutputFileFor(const wxFileName& filename) : TempOutputFileFor(filename.GetFullPath()) {}

    TempOutputFileFor(const TempOutputFileFor&) = delete;

    ~TempOutputFileFor();

    /// Name of the temporary placeholder
    const wxString& FileName() const { return m_filenameTmp; }

    /// Renames temp file to the final one (passed to ctor).
    bool Commit();

    /// Rename file to replace another *while preserving destination
    /// file's permissions*.
    /// Make this helper publicly accessible for code that can't
    /// use TempOutputFileFor directly.
    static bool ReplaceFile(const wxString& temp, const wxString& dest);

#ifdef __WXOSX__
    wxString m_tempDir;
#endif
    wxString m_filenameTmp;
    wxString m_filenameFinal;
};

#ifdef __WXMSW__
/// Return filename safe for passing to CLI tools (gettext).
/// Uses 8.3 short names to avoid Unicode and codepage issues.
wxString CliSafeFileName(const wxString& fn);
#else
inline wxString CliSafeFileName(const wxString& fn) { return fn; }
#endif


// ----------------------------------------------------------------------
// Helpers for persisting windows' state
// ----------------------------------------------------------------------

#if wxUSE_GUI

enum WinStateFlags
{
    WinState_Pos  = 1,
    WinState_Size = 2,
    WinState_All  = WinState_Pos | WinState_Size
};

void SaveWindowState(const wxTopLevelWindow *win, int flags = WinState_All);
void RestoreWindowState(wxTopLevelWindow *win, const wxSize& defaultSize,
                        int flags = WinState_All);

inline wxString WindowStatePath(const wxWindow *win)
{
    return wxString::Format("/windows/%s/", win->GetName().c_str());
}

#endif // wxUSE_GUI

#endif // Poedit_utility_h
