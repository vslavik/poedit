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

#include "configuration.h"

#include <wx/config.h>
#include <wx/thread.h>

#include <mutex>


// This class ensures that wxConfig is only accessed MT-safely
class MTSafeConfig : public wxConfig
{
public:
    typedef wxConfig Base;

    MTSafeConfig(const std::wstring& configFile)
        : wxConfig("", "", configFile, "", wxCONFIG_USE_GLOBAL_FILE | wxCONFIG_USE_LOCAL_FILE)
    {
    }

    struct Lock
    {
        Lock(const MTSafeConfig *self_) : self(self_) { self->m_cs.Enter(); }
        ~Lock() { self->m_cs.Leave(); }

        const MTSafeConfig *self;
    };

    const wxString& GetPath() const override
    {
        Lock l(this);
        return Base::GetPath();
    }

    bool GetFirstGroup(wxString& str, long& index) const override
    {
        Lock l(this);
        return Base::GetFirstGroup(str, index);
    }

    bool GetNextGroup(wxString& str, long& index) const override
    {
        Lock l(this);
        return Base::GetNextGroup(str, index);
    }

    bool GetFirstEntry(wxString& str, long& index) const override
    {
        Lock l(this);
        return Base::GetFirstEntry(str, index);
    }

    bool GetNextEntry(wxString& str, long& index) const override
    {
        Lock l(this);
        return Base::GetNextEntry(str, index);
    }

    size_t GetNumberOfEntries(bool recursive = false) const override
    {
        Lock l(this);
        return Base::GetNumberOfEntries(recursive);
    }

    size_t GetNumberOfGroups(bool recursive = false) const override
    {
        Lock l(this);
        return Base::GetNumberOfGroups(recursive);
    }

    bool HasGroup(const wxString& name) const override
    {
        Lock l(this);
        return Base::HasGroup(name);
    }

    bool HasEntry(const wxString& name) const override
    {
        Lock l(this);
        return Base::HasEntry(name);
    }

    bool Flush(bool currentOnly = false) override
    {
        Lock l(this);
        return Base::Flush(currentOnly);
    }

    bool RenameEntry(const wxString& oldName, const wxString& newName) override
    {
        Lock l(this);
        return Base::RenameEntry(oldName, newName);
    }

    bool RenameGroup(const wxString& oldName, const wxString& newName) override
    {
        Lock l(this);
        return Base::RenameGroup(oldName, newName);
    }

    bool DeleteEntry(const wxString& key, bool bDeleteGroupIfEmpty = true) override
    {
        Lock l(this);
        return Base::DeleteEntry(key, bDeleteGroupIfEmpty);
    }

    bool DeleteGroup(const wxString& key) override
    {
        Lock l(this);
        return Base::DeleteGroup(key);
    }

    bool DeleteAll() override
    {
        Lock l(this);
        return Base::DeleteAll();
    }

    bool DoReadString(const wxString& key, wxString *pStr) const override
    {
        Lock l(this);
        return Base::DoReadString(key, pStr);
    }

    bool DoReadLong(const wxString& key, long *pl) const override
    {
        Lock l(this);
        return Base::DoReadLong(key, pl);
    }

#if wxUSE_BASE64
    bool DoReadBinary(const wxString& key, wxMemoryBuffer* buf) const override
    {
        Lock l(this);
        return Base::DoReadBinary(key, buf);
    }
#endif // wxUSE_BASE64

    bool DoWriteString(const wxString& key, const wxString& value) override
    {
        Lock l(this);
        return Base::DoWriteString(key, value);
    }

    bool DoWriteLong(const wxString& key, long value) override
    {
        Lock l(this);
        return Base::DoWriteLong(key, value);
    }

#if wxUSE_BASE64
    bool DoWriteBinary(const wxString& key, const wxMemoryBuffer& buf) override
    {
        Lock l(this);
        return Base::DoWriteBinary(key, buf);
    }
#endif // wxUSE_BASE64

private:
    wxConfigBase *m_real;
    mutable wxCriticalSection m_cs;
};


namespace
{

// wxConfig isn't thread safe because it's implemented using SetPath.
// Make sure it's only accessed from a single location at a time.
std::mutex g_configAccess;

struct CfgLock
{
    CfgLock() : m_guard(g_configAccess), m_wxLock(dynamic_cast<MTSafeConfig*>(wxConfigBase::Get())) {}

    std::lock_guard<std::mutex> m_guard;
    MTSafeConfig::Lock m_wxLock;
};

} // anonymous namespace


void Config::Initialize(const std::wstring& configFile)
{
    // Because wxConfig is accessed directly elsewhere, both in Poedit (still)
    // and in wx itself, we must use a n MT-safe implementation.
    wxConfigBase::Set(new MTSafeConfig(configFile));
    wxConfigBase::Get()->SetExpandEnvVars(false);
}


bool Config::Read(const std::string& key, std::string *out)
{
    CfgLock lock;

    wxString s;
    if (!wxConfig::Get()->Read(key, &s))
        return false;
    *out = s.ToStdString();
    return true;
}

void Config::Write(const std::string& key, const std::string& value)
{
    CfgLock lock;
    wxConfig::Get()->Write(key, wxString(value));
}

bool Config::Read(const std::string& key, std::wstring *out)
{
    CfgLock lock;

    wxString s;
    if (!wxConfig::Get()->Read(key, &s))
        return false;
    *out = s.ToStdWstring();
    return true;
}

void Config::Write(const std::string& key, const std::wstring& value)
{
    CfgLock lock;
    wxConfig::Get()->Write(key, wxString(value));
}

bool Config::Read(const std::string& key, bool *out)
{
    CfgLock lock;
    return wxConfig::Get()->Read(key, out);
}

void Config::Write(const std::string& key, bool value)
{
    CfgLock lock;
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
