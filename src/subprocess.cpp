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

#include "subprocess.h"

#include "errors.h"

#include <sstream>
#include <boost/algorithm/string.hpp>

#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/mstream.h>
#include <wx/timer.h>
#include <wx/txtstrm.h>


namespace
{


class Process : public wxProcess
{
public:
    Process() : m_timer(this)
    {
    }

    void watch_pipes()
    {
        Bind(wxEVT_TIMER, &Process::on_timer, this);
        m_timer.Start(100/*ms*/);
    }

    subprocess::Output make_output(int status)
    {
        read_available_output();
        return {status, m_stdout.str(), m_stderr.str()};
    }

protected:
    void read_available_output(std::ostringstream& oss, wxInputStream *s)
    {
        if (!s || !s->CanRead())
            return;

        char buffer[4096];
        while (true)
        {
            if (!s->CanRead())
                break;
            s->Read(buffer, sizeof(buffer));
            size_t bytesRead = s->LastRead();
            if (!bytesRead)
                break; // EOF or temporarily no more data
            oss.write(buffer, bytesRead);
        }
    }

    void read_available_output()
    {
        read_available_output(m_stdout, GetInputStream());
        read_available_output(m_stderr, GetErrorStream());
    }

    void on_timer(wxTimerEvent&)
    {
        read_available_output();
    }

    wxTimer m_timer;
    std::ostringstream m_stdout, m_stderr;
};


class PromiseNotifyingProcess : public Process
{
public:
    PromiseNotifyingProcess() = default;

    auto& get_promise() { return m_promise; }
    auto get_future() { return m_promise.get_future(); }

protected:
    void OnTerminate([[maybe_unused]]int pid, int status) override
    {
        if (status != 0)
            wxLogTrace("poedit.execute", "  execution failed with exit code %d", status);
        m_promise.set_value(make_output(status));
        delete this;
    }

    dispatch::promise<subprocess::Output> m_promise;
};


} // anonymous namespace


namespace subprocess
{


wxString try_find_program(const wxString& program, const wxString& primary_path)
{
    wxFileName fn(program);

    if (fn.IsAbsolute())
        return program;

    fn.SetPath(primary_path);
    fn.SetName(program);
#ifdef __WXMSW__
    fn.SetExt("exe");
#endif
    if (fn.IsFileExecutable())
        return fn.GetFullPath();

    return program;
}


std::vector<wxString> Output::extract_lines(const std::string& output)
{
    std::vector<wxString> lines;
    wxMemoryInputStream s(output.data(), output.size());
    wxTextInputStream tis(s);
    while (true)
    {
        auto line = tis.ReadLine();
        if (!line.empty())
            lines.push_back(line);

        if (s.Eof())
            break;
        if (!s)
            break;
    }

    return lines;
}


Arguments::Arguments(const wxString& cmdline)
    : Arguments(wxCmdLineParser::ConvertStringToArgs(cmdline, wxCMD_LINE_SPLIT_UNIX))
{
}

wxString Arguments::pretty_print() const
{
    return "[" + boost::join(m_args, ", ") + "]";
}


wxExecuteEnv& Runner::wxenv()
{
    if (!m_env)
    {
        m_env = std::make_shared<wxExecuteEnv>();
        wxGetEnvMap(&m_env->env);
    }
    return *m_env;
}


void Runner::preprocess_args(Arguments& args) const
{
    if (m_primaryPath.empty())
        return;

    auto program = args.args()[0];
    args.replace(0, try_find_program(program, m_primaryPath));
}


#if wxUSE_GUI
dispatch::future<Output> Runner::do_run_async(Arguments&& argv)
{
    preprocess_args(argv);

    auto env = m_env;
    auto process = new PromiseNotifyingProcess();
    auto future = process->get_future();

    dispatch::on_main([env, process, argv=std::move(argv)]()
    {
        try
        {
            process->Redirect();

            wxLogTrace("poedit.execute", "executing process (async): %s", argv.pretty_print());
            auto retval = wxExecute(argv, wxEXEC_ASYNC, process, env.get());
            if (retval == 0)
            {
                wxLogTrace("poedit.execute", "  failed to launch child process(%d): %s", (int)retval, argv.pretty_print());
                BOOST_THROW_EXCEPTION(Exception(wxString::Format(_("Cannot execute program: %s"), argv.pretty_print())));
            }

            process->watch_pipes();
        }
        catch (...)
        {
            dispatch::set_current_exception(process->get_promise());
        }
    });

    return future;
}
#endif


Output Runner::do_run_sync(Arguments&& argv)
{
#if wxUSE_GUI
    if (!wxThread::IsMain())
        return do_run_async(std::move(argv)).get();
#endif

    preprocess_args(argv);

    Process process;
    process.Redirect();
    wxLogTrace("poedit.execute", "executing process (sync): %s", argv.pretty_print());
    auto retval = wxExecute(argv, wxEXEC_BLOCK | wxEXEC_NODISABLE | wxEXEC_NOEVENTS, &process, m_env.get());
    if (retval == -1)
    {
        wxLogTrace("poedit.execute", "  failed to launch child process(%d): %s", (int)retval, argv.pretty_print());
        BOOST_THROW_EXCEPTION(Exception(wxString::Format(_("Cannot execute program: %s"), argv.pretty_print())));
    }
    else if (retval != 0)
    {
        wxLogTrace("poedit.execute", "  execution failed with exit code %d", (int)retval);
    }

    return process.make_output((int)retval);
}


} // namespace subprocess
