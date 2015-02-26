/*
 *  This file is part of Poedit (http://poedit.net)
 *
 *  Copyright (C) 2010-2015 Vaclav Slavik
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

#ifndef _UTILITY_H_
#define _UTILITY_H_

#ifndef HAVE_MKDTEMP
    #ifdef __WXOSX__
        #define HAVE_MKDTEMP
    #endif
#endif

#include <wx/app.h>
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/toplevel.h>

#include <functional>

// ----------------------------------------------------------------------
// Multithreading helpers
// ----------------------------------------------------------------------

#if defined(__WXOSX__) && defined(__clang__)
    #define HAVE_DISPATCH
    extern void call_on_main_thread_impl(std::function<void()> func);
#endif

/**
    Simply calls the callable @a func on the main thread, asynchronously.
 */
template<typename F>
void call_on_main_thread(F&& func)
{
#ifdef HAVE_DISPATCH
    call_on_main_thread_impl(func);
#else
    wxTheApp->CallAfter(func);
#endif
}

#ifndef _MSC_VER

template<typename... Args>
auto on_main_thread_impl(std::function<void(Args...)> func) -> std::function<void(Args...)>
{
    return [func](Args... args){
        call_on_main_thread([func,args...]{
            func(args...);
        });
    };
}

#else

// Visual Studio 2013 is broken and won't parse the above; 2015 fixes it.
inline auto on_main_thread_impl(std::function<void()> func) -> std::function<void()>
{
    return [func](){ call_on_main_thread([=]{ func(); }); };
}
template<typename A1>
auto on_main_thread_impl(std::function<void(A1)> func) -> std::function<void(A1)>
{
    return [func](A1 a1){ call_on_main_thread([=]{ func(a1); }); };
}
template<typename A1, typename A2>
auto on_main_thread_impl(std::function<void(A1,A2)> func) -> std::function<void(A1,A2)>
{
    return [func](A1 a1, A2 a2){ call_on_main_thread([=]{ func(a1,a2); }); };
}
template<typename A1, typename A2, typename A3>
auto on_main_thread_impl(std::function<void(A1, A2, A3)> func) -> std::function<void(A1, A2, A3)>
{
    return [func](A1 a1, A2 a2, A3 a3){ call_on_main_thread([=]{ func(a1, a2, a3); }); };
}

#endif

/**
    Wraps a callable into std::function called on the main thread.
    
    Returned function takes the same arguments as @a func and is called on the
    main thread. I.e. the returned object may be called from any thread, but
    @a func is guaranteed to execute on the main one.
    
    Notice that it is necessary to specify template parameters because they
    cannot be deduced.
    
    Example usage:
    
        on_main_thread<int,std::string>(this, [=](int i, std::string s){
            ...
        })
 */
template<typename... Args, typename F>
auto on_main_thread(F&& func) -> std::function<void(Args...)>
{
    return on_main_thread_impl(std::function<void(Args...)>(func));
}

/**
    Like on_main_thread<> but is only called if @a window is still valid
    (using a wxWeakRef<> to check).
 */
template<typename... Args, typename F, typename Class>
auto on_main_thread_for_window(Class *self, F&& func) -> std::function<void(Args...)>
{
    wxWeakRef<Class> weak(self);
    return on_main_thread<Args...>([=](Args... args){
        if (weak)
            func(args...);
    });
}


/**
    Wraps a method into function called on the main thread.
    
    Returned function takes the same arguments as the provided method @a func
    and is called on the main thread. I.e. the returned object may be called
    from any thread, but @a func is guaranteed to execute on the main one.
    
    Example usage:
    
        on_main_thread(this, &CrowdinOpenDialog::OnFetchedProjects)
 */
template<typename Class, typename... Args>
auto on_main_thread(Class *self, void (Class::*func)(Args...)) -> std::function<void(Args...)>
{
    wxWeakRef<Class> weak(self);
    return on_main_thread<Args...>([=](Args... args){
        if (weak)
            ((*weak.get()).*func)(args...);
    });
}


// ----------------------------------------------------------------------
// Misc platform differences
// ----------------------------------------------------------------------

#ifdef __WXMSW__
    #define MSW_OR_OTHER(msw, other) msw
#else
    #define MSW_OR_OTHER(msw, other) other
#endif

#ifdef __WXOSX__
    #define BORDER_WIN(dir, n) Border(dir, 0)
    #define BORDER_OSX(dir, n) Border(dir, n)
#else
    #define BORDER_WIN(dir, n) Border(dir, n)
    #define BORDER_OSX(dir, n) Border(dir, 0)
#endif

// ----------------------------------------------------------------------
// Misc helpers
// ----------------------------------------------------------------------

wxString EscapeMarkup(const wxString& str);


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

    bool IsOk() const { return !m_dir.empty(); }

    // creates new file name in that directory
    wxString CreateFileName(const wxString& suffix);

    // whether to keep temporary files
    static void KeepFiles(bool keep = true) { ms_keepFiles = keep; }

private:
    int m_counter;
    wxString m_dir;
    wxArrayString m_files;

    static bool ms_keepFiles;
};

/// Holder of temporary file for creating the output.
/// Use Commit() to move the written file to its final location.
/// Destructor deletes the temp file if it still exists.
class TempOutputFileFor
{
public:
    explicit TempOutputFileFor(const wxString& filename);
    ~TempOutputFileFor();

    /// Name of the temporary placeholder
    const wxString& FileName() const { return m_filenameTmp; }

    /// Renames temp file to the final one (passed to ctor).
    bool Commit();

#ifdef __WXOSX__
    wxString m_tempDir;
#endif
    wxString m_filenameTmp;
    wxString m_filenameFinal;
};

// Helper for writing files


// ----------------------------------------------------------------------
// Helpers for persisting windows' state
// ----------------------------------------------------------------------

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

#endif // _UTILITY_H_
