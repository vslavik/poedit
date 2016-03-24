/*
 *  This file is part of Poedit (https://poedit.net)
 *
 *  Copyright (C) 1999-2016 Vaclav Slavik
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


#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

#include "extractor.h"
#include "gexecute.h"



void ExtractorsDB::Read(wxConfigBase *cfg)
{
    Data.clear();
    
    cfg->SetExpandEnvVars(false);

    Extractor info;
    wxString key, oldpath = cfg->GetPath();
    wxStringTokenizer tkn(cfg->Read("Parsers/List", wxEmptyString), ";");

    while (tkn.HasMoreTokens())
    {
        info.Name = tkn.GetNextToken();
        key = info.Name; key.Replace("/", "_");
        cfg->SetPath("Parsers/" + key);
        info.Enabled = cfg->ReadBool("Enabled", true);
        info.Extensions = cfg->Read("Extensions", wxEmptyString);
        info.Command = cfg->Read("Command", wxEmptyString);
        info.KeywordItem = cfg->Read("KeywordItem", wxEmptyString);
        info.FileItem = cfg->Read("FileItem", wxEmptyString);
        info.CharsetItem = cfg->Read("CharsetItem", wxEmptyString);
        Data.push_back(info);
        cfg->SetPath(oldpath);
    }
}



void ExtractorsDB::Write(wxConfigBase *cfg)
{
#if 0 // asserts on wxGTK, some bug in wx
    if (cfg->HasGroup("Parsers"))
        cfg->DeleteGroup("Parsers");
#endif

    cfg->SetExpandEnvVars(false);
    
    if (Data.empty())
        return;
    
    size_t i;
    wxString list;
    list << Data[0].Name;
    for (i = 1; i < Data.size(); i++)
        list << ";" << Data[i].Name;
    cfg->Write("Parsers/List", list);
    
    wxString oldpath = cfg->GetPath();
    wxString key;
    for (const auto& item: Data)
    {
        key = item.Name; key.Replace("/", "_");
        cfg->SetPath("Parsers/" + key);
        cfg->Write("Enabled", item.Enabled);
        cfg->Write("Extensions", item.Extensions);
        cfg->Write("Command", item.Command);
        cfg->Write("KeywordItem", item.KeywordItem);
        cfg->Write("FileItem", item.FileItem);
        cfg->Write("CharsetItem", item.CharsetItem);
        cfg->SetPath(oldpath);
    }
}
    
int ExtractorsDB::FindExtractor(const wxString& name)
{
    for (size_t i = 0; i < Data.size(); i++)
    {
        if (Data[i].Name == name)
            return int(i);
    }
    return -1;
}


wxArrayString Extractor::SelectParsable(const wxArrayString& files)
{
    wxStringTokenizer tkn(Extensions, ";, \t", wxTOKEN_STRTOK);
    wxString wildcard;
    wxArrayString result;
    size_t i;

    while (tkn.HasMoreTokens())
    {
        wildcard = tkn.GetNextToken();
#ifdef __WXMSW__
        wildcard.MakeLower();
#endif
        for (i = 0; i < files.GetCount(); i++)
#ifdef __WXMSW__
            if (files[i].Lower().Matches(wildcard))
#else
            if (files[i].Matches(wildcard))
#endif
            {
                result.Add(files[i]);
            }
    }

    return result;
}



wxString Extractor::GetCommand(const wxArrayString& files,
                               const wxArrayString& keywords,
                               const wxString& output,
                               const wxString& charset)
{
    wxString cmdline, kline, fline;
    
    cmdline = Command;
    cmdline.Replace("%o", QuoteCmdlineArg(output));
    
    wxString dummy;
    size_t i;

    for (i = 0; i < keywords.GetCount(); i++)
    {
        dummy = KeywordItem;
        dummy.Replace("%k", keywords[i]);
        kline << " " << dummy;
    }

    for (i = 0; i < files.GetCount(); i++)
    {
        wxString fn = files[i];
#ifdef __WXMSW__
        // Gettext tools can't handle Unicode filenames well (due to using
        // char* arguments), so work around this by using the short names.
        if (!fn.IsAscii())
        {
            fn = wxFileName(fn).GetShortPath();
            fn.Replace("\\", "/");
        }
#endif

        dummy = FileItem;
        dummy.Replace("%f", QuoteCmdlineArg(fn));
        fline << " " << dummy;
    }

    wxString charsetline;
    if (!charset.empty())
    {
        charsetline = CharsetItem;
        charsetline.Replace("%c", charset);
    }

    cmdline.Replace("%C", charsetline);
    cmdline.Replace("%K", kline);
    cmdline.Replace("%F", fline);

    return cmdline;
}
