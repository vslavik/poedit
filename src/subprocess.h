/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 2025 Vaclav Slavik
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

#ifndef Poedit_subprocess_h
#define Poedit_subprocess_h

#include "concurrency.h"
#include "str_helpers.h"

#include <wx/string.h>
#include <wx/utils.h>

#include <memory>


namespace subprocess
{

/**
   Helper that returns full path to @a program if it exists in the @a primary_path directory
   or unmodified @a program value if it does not. Used to run external binaries from known
   locations.
 */
wxString try_find_program(const wxString& program, const wxString& primary_path);


/// Collected result of running a command.
struct Output
{
    int exit_code = -1;
    std::string std_out;
    std::string std_err;

    std::vector<wxString> std_out_lines() const
        { return extract_lines(std_out); }

    std::vector<wxString> std_err_lines() const
        { return extract_lines(std_err); }

    explicit operator bool() const { return exit_code == 0; }

private:
    static std::vector<wxString> extract_lines(const std::string& output);
};

typedef std::shared_ptr<wxExecuteEnv> environment_ptr;


// Holder for execution command arguments
class Arguments
{
public:
    Arguments(const Arguments&) = delete;
    Arguments(Arguments&&) = default;

    template<typename T>
    Arguments(const T& list) : Arguments(list.begin(), list.end()) {}

    Arguments(std::initializer_list<wxString> list) : Arguments(list.begin(), list.end()) {}

    template<typename Iter>
    Arguments(Iter begin, Iter end)
    {
        m_args.reserve(end - begin);
        for (auto i = begin; i != end; ++i)
            m_args.push_back(str::to_wstring(*i));
    }

    Arguments(const wxString& cmdline);

    const std::vector<std::wstring>& args() const{ return m_args; }

    void replace(size_t index, const wxString& value)
    {
        if (m_args[index] != value)
            m_args[index] = str::to_wstring(value);
    }

    void insert(size_t index, const wxString& value)
    {
        m_args.insert(m_args.begin() + index, str::to_wstring(value));
    }

    operator const wchar_t* const*() const
    {
        m_cargs.clear();
        m_cargs.reserve(m_args.size() + 1);
        for (auto& arg: m_args)
            m_cargs.push_back(arg.c_str());
        m_cargs.push_back(nullptr);
        return m_cargs.data();
    }

    wxString pretty_print() const;

private:
    std::vector<std::wstring> m_args;
    mutable std::vector<const wchar_t*> m_cargs;
};


/**
    Interface for running a subprocess.

    This has both sync and async variants and is safe to call from
    non-main threads, unlike wxExecute.

    The instance can be safely destroyed after calling a run_xxx() function,
    even if async execution didn't finish yet.
 */
class Runner
{
public:
    Runner() = default;
    virtual ~Runner() {}

    /// Returns environment variables map used for execution
    wxEnvVariableHashMap& env();

    /// Add environment variable to the environment used for execution
    void env(const wxString& var, const wxString& value)
    {
        env()[var] = value;
    }

    /// Sets the path where to look for programs.
    void set_primary_path(const wxString& path) { m_primaryPath = path; }

    /// Runs command asynchronously and returns a future for its output.
    dispatch::future<Output> run_async(const std::vector<wxString>& argv)
    {
        return do_run_async(argv);
    }

    /// Runs command asynchronously and returns a future for its output.
    template<typename... Args>
    dispatch::future<Output> run_async(const Args&... argv)
    {
        return do_run_async({argv...});
    }

    /**
        Runs command asynchronously and returns a future for its output.

        The argument is parsed using UNIX quotation rules.
        It is recommend to use the argv variant instead.
     */
    dispatch::future<Output> run_command_async(const wxString& cmdline)
    {
        return do_run_async(cmdline);
    }


    /// Runs command synchronously and returns its output.
    Output run_sync(const std::vector<wxString>& argv)
    {
        return do_run_sync(argv);
    }

    /// Runs command synchronously and returns its output.
    template<typename... Args>
    Output run_sync(const Args&... argv)
    {
        return do_run_sync({argv...});
    }

    /**
        Runs command synchronously and returns its output.

        The argument is parsed using UNIX quotation rules.
        It is recommend to use the argv variant instead.
     */
    Output run_command_sync(const wxString& cmdline)
    {
        return do_run_sync(cmdline);
    }

protected:
    /**
        Preproceses arguments before execution.

        Can be overriden to e.g. add system-depending flags or change program name.
        Overriden version much call base class one, because it implements
        primary path lookup.
     */
    virtual void preprocess_args(Arguments& args) const;

    // run_* implementation:
    dispatch::future<Output> do_run_async(Arguments&& argv);
    Output do_run_sync(Arguments&& argv);

protected:
    std::shared_ptr<wxExecuteEnv> m_env;
    wxString m_primaryPath;
};


#ifdef MACOS_BUILD_WITHOUT_APPKIT

// Stub implementations to reduce dependencies in Quicklook extensions on macOS.
//
// wxExecute() uses NSWorkspace, which is unavailable in extensions; it's the only AppKit
// dependency in wxBase and we can avoid linking that in.

inline Arguments::Arguments(const wxString&) {}
inline std::vector<wxString> Output::extract_lines(const std::string&) { return {}; }
inline void Runner::preprocess_args(Arguments&) const {}
inline dispatch::future<Output> Runner::do_run_async(Arguments&&) { wxASSERT(false); throw std::logic_error("not implemented"); }
inline Output Runner::do_run_sync(Arguments&&) { wxASSERT(false); throw std::logic_error("not implemented"); }
inline wxEnvVariableHashMap& Runner::env() { wxASSERT(false); throw std::logic_error("not implemented"); }

#endif // MACOS_BUILD_WITHOUT_APPKIT


} // namespace subprocess


#endif // Poedit_subprocess_h
