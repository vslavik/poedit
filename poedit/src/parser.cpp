
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      parser.cpp
    
      Database of available parsers
    
      (c) Vaclav Slavik, 1999

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/wfstream.h>
#include <wx/config.h>
#include <wx/tokenzr.h>

#include "parser.h"


#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(ParserArray);

void ParsersDB::Read(wxConfigBase *cfg)
{
    Clear();
    
    cfg->SetExpandEnvVars(false);

    Parser info;
    wxString key, oldpath = cfg->GetPath();
    wxStringTokenizer tkn(cfg->Read("Parsers/List", ""), ";");

    while (tkn.HasMoreTokens())
    {
        info.Name = tkn.GetNextToken();
        key = info.Name; key.Replace("/", "_");
        cfg->SetPath("Parsers/" + key);
        info.Extensions = cfg->Read("Extensions", "");
        info.Command = cfg->Read("Command", "");
        info.KeywordItem = cfg->Read("KeywordItem", "");
        info.FileItem = cfg->Read("FileItem", "");
        Add(info);
        cfg->SetPath(oldpath);
    }
}



void ParsersDB::Write(wxConfigBase *cfg)
{
    if (cfg->HasGroup("Parsers"))
        cfg->DeleteGroup("Parsers");

    cfg->SetExpandEnvVars(false);
    
    if (GetCount() == 0) return;
    
    size_t i;
    wxString list;
    list << Item(0).Name;
    for (i = 1; i < GetCount(); i++)
        list << ";" << Item(i).Name;
    cfg->Write("Parsers/List", list);
    
    wxString oldpath = cfg->GetPath();
    wxString key;
    for (i = 0; i < GetCount(); i++)
    {
        key = Item(i).Name; key.Replace("/", "_");
        cfg->SetPath("Parsers/" + key);
        cfg->Write("Extensions", Item(i).Extensions);
        cfg->Write("Command", Item(i).Command);
        cfg->Write("KeywordItem", Item(i).KeywordItem);
        cfg->Write("FileItem", Item(i).FileItem);
        cfg->SetPath(oldpath);
    }
}



wxArrayString Parser::SelectParsable(const wxArrayString& files)
{
    wxStringTokenizer tkn(Extensions, ";");
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



wxString Parser::GetCommand(const wxArrayString& files, 
                            const wxArrayString& keywords, 
                            const wxString& output)
{
    wxString cmdline, kline, fline;
    
    cmdline = Command;
    cmdline.Replace("%o", output);
    
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
        dummy = FileItem;
        dummy.Replace("%f", files[i]);
        fline << " " << dummy;
    }
    
    cmdline.Replace("%K", kline);
    cmdline.Replace("%F", fline);
    
    return cmdline;
}

