
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edapp.cpp
    
      Application class
    
      (c) Vaclav Slavik, 1999-2004

*/


#include <wx/wxprec.h>

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/cmdline.h>
#include <wx/log.h>
#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_all.h>

#include "edapp.h"
#include "edframe.h"
#include "manager.h"
#include "prefsdlg.h"
#include "parser.h"
#include "chooselang.h"


IMPLEMENT_APP(poEditApp);

wxString poEditApp::GetAppPath() const
{
#if defined(__UNIX__)
    wxString home;
    if (!wxGetEnv(_T("POEDIT_PREFIX"), &home))
        home = wxString::FromAscii(POEDIT_PREFIX);
    return home;
#elif defined(__WXMSW__)
    wxString regkey;
    regkey.Printf(_T("%s/application_path"), GetAppVersion().c_str());
    wxString path = wxConfigBase::Get()->Read(regkey, wxEmptyString);
    if (!path)
    {
        wxLogError(_("poEdit installation is broken, cannot find application's home directory."));
        path = _T(".");
    }
    return path;
#else
#error "Unsupported platform!"
#endif
}

wxString poEditApp::GetAppVersion() const
{
    return _T("1.2.5");
}


static wxArrayString gs_filesToOpen;

#ifdef __UNIX__
#define CFG_FILE (home + _T(".poedit/config"))
#else
#define CFG_FILE wxEmptyString
#endif

bool poEditApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

#ifdef __UNIX__
    wxString home = wxGetHomeDir() + _T("/");

    // create poEdit cfg dir, move ~/.poedit to ~/.poedit/config
    // (upgrade from older versions of poEdit which used ~/.poedit file)
    if (!wxDirExists(home + _T(".poedit")))
    {
        if (wxFileExists(home + _T(".poedit")))
            wxRenameFile(home + _T(".poedit"), home + _T(".poedit2"));
        wxMkdir(home + _T(".poedit"));
        if (wxFileExists(home + _T(".poedit2")))
            wxRenameFile(home + _T(".poedit2"), home + _T(".poedit/config"));
    }
#endif

    SetVendorName(_T("Vaclav Slavik"));
    SetAppName(_T("poedit"));
    wxConfigBase::Set(
        new wxConfig(wxEmptyString, wxEmptyString, CFG_FILE, wxEmptyString, 
                     wxCONFIG_USE_GLOBAL_FILE | wxCONFIG_USE_LOCAL_FILE));
    wxConfigBase::Get()->SetExpandEnvVars(false);

    wxLocale::AddCatalogLookupPathPrefix(GetAppPath() + _T("/share/locale"));

    m_locale.Init(GetUILanguage());
    
    m_locale.AddCatalog(_T("poedit"));
    m_locale.AddCatalog(_T("poedit-wxstd")); // for semistatic builds

    wxImage::AddHandler(new wxGIFHandler);
    wxFileSystem::AddHandler(new wxZipFSHandler);
    
    wxString resPath = GetAppPath() + _T("/share/poedit/resources.zip");
    if (!wxFileExists(resPath))
    {
        wxString msg;
        
#ifdef __UNIX__
        msg.Printf(_("Cannot find resources file '%s'!\n"
                     "poEdit was configured to be installed in '%s'.\n"
                     "You may try to set POEDIT_PREFIX environment variable to point\n"
                     "to the location where you installed poEdit."), 
                     resPath.c_str(), GetAppPath().c_str());
#else
        msg.Printf(_("Cannot find resources file '%s'!\nPlease reinstall poEdit."), 
                     resPath.c_str());
#endif
        wxMessageBox(msg, _("poEdit Error"), wxOK | wxICON_ERROR);
        return false;
    }
    else
    {
        wxXmlResource::Get()->InitAllHandlers();
        wxXmlResource::Get()->Load(resPath);
    }
    
    SetDefaultCfg(wxConfig::Get());

    if (wxConfig::Get()->Read(_T("translator_name"), _T("nothing")) == _T("nothing"))
    {
        wxMessageBox(_("This is first time you run poEdit.\nPlease fill in your name and e-mail address.\n(This information is used only in catalogs headers)"), _("Setup"),
                       wxOK | wxICON_INFORMATION);
                       
        PreferencesDialog dlg;
        dlg.TransferTo(wxConfig::Get());
        if (dlg.ShowModal() == wxID_OK)
            dlg.TransferFrom(wxConfig::Get());
    }
      
    if (gs_filesToOpen.GetCount() == 0)
     
    {
        if (wxConfig::Get()->Read(_T("manager_startup"), (long)false))
            ManagerFrame::Create()->Show(true);
        else
            poEditFrame::Create(wxEmptyString);
    }
    else
    {
        for (size_t i = 0; i < gs_filesToOpen.GetCount(); i++)
            poEditFrame::Create(gs_filesToOpen[i]);
    }

    return true;
}
    
void poEditApp::SetDefaultParsers(wxConfigBase *cfg)
{
    ParsersDB pdb;
    bool changed = false;
    wxString defaultsVersion = cfg->Read(_T("Parsers/DefaultsVersion"),
                                         _T("1.2.x"));
    pdb.Read(cfg);

    // Add C/C++ parser (only if there's isn't any parser already):
    if (pdb.GetCount() == 0)
    {
        Parser p;
        p.Name = _T("C/C++");
        p.Extensions = _T("*.c;*.cpp;*.h;*.hpp;*.cc;*.C;*.cxx;*.hxx");
        p.Command = _T("xgettext --force-po -o %o %C %K %F");
        p.KeywordItem = _T("-k%k");
        p.FileItem = _T("%f");
        p.CharsetItem = _T("--from-code=%c");
        pdb.Add(p);
        changed = true;
    }

    // If upgrading poEdit to 1.2.4, add dxgettext parser for Delphi:
#ifdef __WINDOWS__
    if (defaultsVersion == _T("1.2.x"))
    {
        Parser p;
        p.Name = _T("Delphi (dxgettext)");
        p.Extensions = _T("*.pas;*.inc;*.dpr;*.xfm;*.dfm");
        p.Command = _T("dxgettext --so %o %F");
        p.KeywordItem = wxEmptyString;
        p.FileItem = _T("%f");
        pdb.Add(p);
        changed = true;
    }
#endif

    // If upgrading poEdit to 1.2.5, update C++ parser to handle --from-code:
    if (defaultsVersion == _T("1.2.x") || defaultsVersion == _T("1.2.4"))
    {
        for (unsigned i = 0; i < pdb.GetCount(); i++)
        {
            if (pdb[i].Name == _T("C/C++"))
            {
                if (pdb[i].Command == _T("xgettext --force-po -o %o %K %F"))
                {
                    pdb[i].Command = _T("xgettext --force-po -o %o %C %K %F");
                    pdb[i].CharsetItem = _T("--from-code=%c");
                    changed = true;
                }
                break;
            }
        }
    }

    if (changed)
    {
        pdb.Write(cfg);
        cfg->Write(_T("Parsers/DefaultsVersion"), GetAppVersion());
    }
}

void poEditApp::SetDefaultCfg(wxConfigBase *cfg)
{
    SetDefaultParsers(cfg);

    if (cfg->Read(_T("version"), wxEmptyString) == GetAppVersion()) return;

    if (cfg->Read(_T("TM/database_path"), wxEmptyString).IsEmpty())
    {
        wxString dbpath;
#if defined(__UNIX__)
        dbpath = wxGetHomeDir() + _T("/.poedit/tm");
#elif defined(__WXMSW__)
        // VS: this distinguishes between NT and Win9X systems -- the former
        //     has users' home directories while on the latter wxGetHomeDir
        //     will return path of the executable
        if (wxGetHomeDir().IsSameAs(GetAppPath() + _T("\\bin"), false))      
            dbpath = GetAppPath() + _T("\\share\\poedit\\tm");
        else
            dbpath = wxGetHomeDir() + _T("\\poedit_tm");
#endif
        cfg->Write(_T("TM/database_path"), dbpath);
    }

    if (cfg->Read(_T("TM/search_paths"), wxEmptyString).IsEmpty())
    {
        wxString paths;
#if defined(__UNIX__)
        paths = wxGetHomeDir() + _T(":/usr/share/locale:/usr/local/share/locale");
#elif defined(__WXMSW__)
        paths = _T("C:");
#endif
        cfg->Write(_T("TM/search_paths"), paths);
    }

    cfg->Write(_T("version"), GetAppVersion());
}

void poEditApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    wxApp::OnInitCmdLine(parser);
    parser.AddParam(_T("catalog.po"), wxCMD_LINE_VAL_STRING, 
                    wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
}

bool poEditApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    if (!wxApp::OnCmdLineParsed(parser))
        return FALSE;

    for (size_t i = 0; i < parser.GetParamCount(); i++)
        gs_filesToOpen.Add(parser.GetParam(i));
    return TRUE;
}
