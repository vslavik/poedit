
/*

    poedit, a wxWindows i18n catalogs editor

    ---------------
      edapp.cpp
    
      Application class
    
      (c) Vaclav Slavik, 1999

*/


#ifdef __GNUG__
#pragma implementation
#endif

#include <wx/wxprec.h>

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/fs_zip.h>
#include <wx/image.h>
#include <wx/log.h>
#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_all.h>

#include "edapp.h"
#include "edframe.h"
#include "manager.h"
#include "prefsdlg.h"


IMPLEMENT_APP(poEditApp);

wxString poEditApp::GetAppPath() const
{
#if defined(__UNIX__)
    wxString home;
    if (!wxGetEnv(_T("POEDIT_PREFIX"), &home))
        home = POEDIT_PREFIX;
    return home;
#elif defined(__WXMSW__)
    wxString path = wxConfigBase::Get()->Read(_T("application_path"), wxEmptyString);
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
    return _T("1.1.3");
}


#ifdef __UNIX__
#define CFG_FILE (home + _T(".poedit/config"))
#else
#define CFG_FILE wxEmptyString
#endif

bool poEditApp::OnInit()
{
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

    wxImage::AddHandler(new wxGIFHandler);
    wxFileSystem::AddHandler(new wxZipFSHandler);

    wxTheXmlResource->InitAllHandlers();
    wxTheXmlResource->Load(GetAppPath() + _T("/share/poedit/resources.zip"));
    
    SetDefaultCfg(wxConfig::Get());

    if (wxConfig::Get()->Read(_T("translator_name"), _T("nothing")) == _T("nothing"))
    {
        wxMessageBox(_("This is first time you run poEdit.\n"
                       "Please fill in your name and e-mail address.\n"
                       "(This information is used only in catalogs headers)"), _("Setup"),
                       wxOK | wxICON_INFORMATION);
                       
        PreferencesDialog dlg;
        dlg.TransferTo(wxConfig::Get());
        if (dlg.ShowModal() == wxID_OK)
            dlg.TransferFrom(wxConfig::Get());
    }
      
    if (argc <= 1 && wxConfig::Get()->Read(_T("manager_startup"), (long)false))
        ManagerFrame::Create()->Show(true);
    else
        poEditFrame::Create(argc > 1 ? argv[1] : wxEmptyString);

    return true;
}

void poEditApp::SetDefaultCfg(wxConfigBase *cfg)
{
    if (cfg->Read(_T("version"), wxEmptyString) == GetAppVersion()) return;

    if (cfg->Read(_T("Parsers/List"), wxEmptyString).IsEmpty())
    {
        cfg->Write(_T("Parsers/List"), _T("C/C++"));

        cfg->Write(_T("Parsers/C_C++/Extensions"), 
                   _T("*.c;*.cpp;*.h;*.hpp;*.cc;*.C;*.cxx;*.hxx"));
        cfg->Write(_T("Parsers/C_C++/Command"), 
                   _T("xgettext --force-po -C -o %o %K %F"));
        cfg->Write(_T("Parsers/C_C++/KeywordItem"), 
                   _T("-k%k"));
        cfg->Write(_T("Parsers/C_C++/FileItem"), 
                   _T("%f"));
    }

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
